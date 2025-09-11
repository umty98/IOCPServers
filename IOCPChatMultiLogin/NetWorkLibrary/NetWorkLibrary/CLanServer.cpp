//#include "CLanServer.h"
//#include <stdio.h>
//#include <process.h>
//#include <mmsystem.h>  // timeBeginPeriod 포함
//#include <vector>
//#include "Content.h"
//#include "Protocol.h"
//#include "Sector.h"

#include "pch.h"

// 생성자: 초기화 및 크리티컬 섹션 생성, 시작 IP/port 초기화
CLanServer::CLanServer()
    : m_uniqueIdCounter(1), // 0을 피하기 위해 1부터 시작
    m_maxConnections(0),
    m_iocpHandle(NULL),
    m_workerThreads(nullptr),
    m_workerThreadCount(0),
    m_acceptThread(NULL),
    m_recvTps(NULL),
    m_sendTps(NULL),
    m_recvBytes(NULL),
    m_sendBytes(NULL),
    m_listenSock(INVALID_SOCKET),
    acceptTPS(0),
    m_maxPlayers(0),
    currentPlayerCnt(0)
{
    InitializeCriticalSection(&m_stackCS);
    m_startIP[0] = '\0';
    m_startPort = 0;
}

CLanServer::~CLanServer()
{
    //Stop();
    DeleteCriticalSection(&m_stackCS);

    for (int i = 0; i < m_maxConnections; i++)
    {
        delete m_sessions[i];
    }
}

void CLanServer::ReleaseSession(SESSION* session)
{
    //session 삭제 예정 플래그 검사
   /* if (!session->toDelete)
        return;*/

    uint16_t index = GetSessionIndex(session->sessionID);

    //ioCnt랑 ioFlag가 0이면 CAS로 ioFlag1로 바꿈
    if (InterlockedCompareExchange64(&session->ioState.whole, 1, 0) != 0)
        return;

    //printf("releasesession 호출 : %d\n", index);

    //int remainCnt = session->sendState.parts.Count;
    //printf("release remainCnt : %d\n", remainCnt);

    //for (int i = 0; i < remainCnt; i++)
    //{
    //    PacketBuffer* buffer;
    //    session->sendQueue.Dequeue(buffer);
    //    buffer->Release();
    //    //printf("release에서 남은거 반환\n");
    //}

    PacketBuffer* buffer;
    while (1)
    {
        int ret = session->sendQueue.Dequeue(buffer);
        if (ret == -1)
            break;
        buffer->Release();
    }

    OnClientLeave(session->sessionID);

    // freeIndices(stack)에 인덱스 반환 (락으로 보호)
    EnterCriticalSection(&m_stackCS);
    //소켓 닫고 해당 인덱스 세션 클리어해서 바로 재사용될 수 있게
    closesocket(session->sock);
    m_sessions[index]->Clear();
    m_freeIndices.push(index);
    LeaveCriticalSection(&m_stackCS);
}

int CLanServer::GetSessionCount()
{
    int cnt = 0;
   
    for (int i = 0; i < m_maxConnections; i++)
    {
        if (m_sessions[i]->isActive)
            cnt++;
    }

    return cnt;
}

bool CLanServer::Disconnect(uint64_t sessionID)
{
    //여기 나중에 바꿔야됨
    uint16_t index = GetSessionIndex(sessionID);

    if (index >= m_sessions.size() || m_sessions[index] == nullptr ||
        m_sessions[index]->sessionID != sessionID)
    {
        return true;
    }

    SESSION* session = m_sessions[index];

    //ioCnt++
    InterlockedIncrement(&session->ioState.parts.Count);

    //ioFlag가 true면 리턴
    if (InterlockedCompareExchange(&session->ioState.parts.Flag, 1, 1) == 1)
    {
        //ioCnt--
        /*if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            ReleaseSession(session);*/
        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            ReleaseSession(session);

        return true;
    }

    if (session->sessionID != sessionID)
    {
        // 세션이 바뀌었으면 //아래와 같은 이유로 무조건 여기서도 release가 들어가야 된다고 생각됨.
        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
        {
            ReleaseSession(session);
        }
        return false;
    }

    //stopIO Flag 바꾸기
    InterlockedExchange(&session->stopIO, 1);
    //InterlockedIncrement(&disconnectHeartBeat);
    //CancelIOEx로 걸려있는 모든 작업 취소
    CancelIoEx((HANDLE)session->sock, nullptr);

    //ioCnt--
    if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
    {
        //session->toDelete = true;
        ReleaseSession(session);
    }
    return true;
}

bool CLanServer::SendPacket(uint64_t sessionID, PacketBuffer* packet)
{
    uint16_t index = GetSessionIndex(sessionID);
    if (index >= m_sessions.size() || m_sessions[index] == nullptr ||
        m_sessions[index]->sessionID != sessionID)
    {
        return false;
    }

    SESSION* session = m_sessions[index];

    //ioCnt++
    InterlockedIncrement(&session->ioState.parts.Count);

    //ioFlag가 true면 리턴
    if (InterlockedCompareExchange(&session->ioState.parts.Flag, 1, 1) == 1)
    {
        //ioCnt--
        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            ReleaseSession(session);
        //InterlockedDecrement(&session->ioState.parts.Count);
        return false;
    }

    if (session->sessionID != sessionID)
    {
        // 세션이 바뀌었으면 //아래와 같은 이유로 무조건 여기서도 release가 들어가야 된다고 생각됨.
        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
        {
            ReleaseSession(session);
        }
        return false;
    }

    //헤더 생성
    if (!packet->GetBufEncoded())
    {
        NetWorkHeader netHeader;
        netHeader.code = Wan_Protocol_Code;
        netHeader.len = packet->GetDataSize() - sizeof(NetWorkHeader);
        netHeader.randkey = Random_Key;
        netHeader.checkSum = -1;
        packet->EnqueueHeader((char*)&netHeader, sizeof(NetWorkHeader));

        packet->SetCheckSum(packet);

        //여기서 encoding
        packet->Encode(packet);
    }
   
    packet->AddRef();
    session->sendQueue.Enqueue(packet);
    //packet->Release();

    //sendCnt++
   // InterlockedIncrement(&session->sendState.parts.Count);

    //ioCnt--
    if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
    {
        //session->toDelete = true;
        ReleaseSession(session);
    }

    return true;
}

bool CLanServer::SendPacket(SESSION* session, PacketBuffer* packet)
{
    // 필요에 따라 구현 (현재 단순히 true 반환)
    return true;
}

void CLanServer::SendAllPacket()
{
    //일괄 send를 위한 postQueue
    PostQueuedCompletionStatus(m_iocpHandle, 0, (ULONG_PTR)(-1), NULL);
}

void CLanServer::PostSendAll()
{
    //printf("PostSendAll\n");
    //DWORD startTime = timeGetTime();
    
    for (auto* session : m_sessions)
    {
        if (session->isActive == false)
            continue;

        //ioCnt++
        InterlockedIncrement(&session->ioState.parts.Count);

        //releaseCheck
        if (InterlockedCompareExchange(&session->ioState.parts.Flag, 1, 1) == 1)
        {
            if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                ReleaseSession(session);
            //InterlockedDecrement(&session->ioState.parts.Count);
            continue;
        }

        if (!PostSend(session))
        {
            //ioCnt--
            if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                ReleaseSession(session);
        }
        //ioCnt--
        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            ReleaseSession(session);
    }
   // DWORD endTime = timeGetTime();
    //printf("sendTime : %lu\n", endTime - startTime);
    InterlockedExchange(&sendFrameFlag, 0);
}

bool CLanServer::PostRecv(SESSION* session)
{
    if (session->stopIO == 1)
        return true;

    DWORD freeTotal = session->curReaderBuf->GetBufferSize() - session->curReaderBuf->GetDataSize();
    if (freeTotal == 0)
    {
        printf("recvQ 공간 부족.\n");

        return true;
    }
    
    WSABUF wsabuf;
    wsabuf.buf = session->curReaderBuf->GetWriteBufferPtr();
    wsabuf.len = freeTotal;
    int bufCount = 1;

    DWORD flags = 0, bytesRecv = 0;
    //ioCnt++
    InterlockedIncrement(&session->ioState.parts.Count);
    int ret = WSARecv(session->sock, &wsabuf, bufCount, &bytesRecv, &flags, &session->recvOverlapped, NULL);
    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
            return false;
    }

    if (InterlockedCompareExchange(&session->stopIO, 1, 1) == 1)
    {
        CancelIoEx((HANDLE)session->sock, nullptr);
    }

    return true;
}

bool CLanServer::PostSend(SESSION* session)
{ 
    if (session->stopIO == 1)
        return true;

    if (session->sendQueue.Size() > 500)
    {
		server.DisconnectSession(session);
        return true;
    }

    if (session->sendQueue.Size() == 0)
        return true;

    if (InterlockedExchange(&session->sendState.parts.Flag, 1) == 1)
        return true;

    int cnt = 0;
    while (cnt <= 1)
    {
        //int bufCount = session->sendState.parts.Count;
        int bufCount = session->sendQueue.Size();

        if (bufCount == 0)
        {
            if (cnt == 0)
            {
                cnt++;
                continue;
            }

            InterlockedExchange(&session->sendState.parts.Flag, 0);
            return true;
        }

        if(bufCount >100) // 최대 버퍼 개수 제한 (예: 100개로 설정, 필요에 따라 조정 가능)
			bufCount = 100;

        session->sendCnt = bufCount;
       
       /* std::vector<PacketBuffer*> packets;
        session->sendQueue.PeekData(packets, bufCount);
        std::vector<WSABUF> wsabufs(bufCount);*/

        PacketBuffer* packets[100];
        session->sendQueue.PeekData(packets, bufCount);
        WSABUF wsabufs[100];

        for (int i = 0; i < bufCount; i++)
        {
            wsabufs[i].buf = packets[i]->GetBufferPtr();
            wsabufs[i].len = packets[i]->GetDataSize();
        }

        DWORD bytesSent = 0;
        //ioCnt++
        InterlockedIncrement(&session->ioState.parts.Count);
        //int ret = WSASend(session->sock, wsabufs.data(), bufCount, &bytesSent, 0, &session->sendOverlapped, NULL);
        int ret = WSASend(session->sock, wsabufs, bufCount, &bytesSent, 0, &session->sendOverlapped, NULL);
        if (ret == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err != WSA_IO_PENDING)
            {
                //std::cout << "Send error 코드 : " << err << '\n';
                //sendFlag 0으로
                InterlockedExchange(&session->sendState.parts.Flag, 0);
                return false;
            }

        }
        if (InterlockedCompareExchange(&session->stopIO, 1, 1) == 1)
        {
            CancelIoEx((HANDLE)session->sock, nullptr);
        }

        return true;
    }   
}

void CLanServer::DisconnectSession(SESSION* session)
{
    InterlockedExchange(&session->stopIO, 1);

    CancelIoEx((HANDLE)session->sock, nullptr);
}

//////////////////////////////////////////////////////////////////////////
// Worker Thread: IOCP 종료 신호를 받으면 종료
//////////////////////////////////////////////////////////////////////////
unsigned __stdcall CLanServer::WorkerThread(LPVOID param)
{
    //CLanServer* server = reinterpret_cast<CLanServer*>(param);
    WorkerParam* wParam = static_cast<WorkerParam*>(param);
    CLanServer* server = wParam->server;
    int TPSIndex = wParam->idx;
    printf("TPSIndex : %d\n", TPSIndex);

    DWORD bytesTransferred;
    SESSION* session = nullptr;
    OVERLAPPED* overlapped = nullptr;

    //레디스 연결
    //if (g_redisInited == false)
    //{
    //    bool redisOk = false;
    //    g_redisClient.connect(
    //        "127.0.0.1", 6379,
    //        // 이 람다는 연결 성공(ok)/실패(failed) 시점에 호출됩니다
    //        [&](const std::string& host,
    //            std::size_t       port,
    //            cpp_redis::connect_state status)
    //        {
    //            if (status == cpp_redis::connect_state::ok)
    //            {
    //                redisOk = true;    // TCP 레벨 연결 성공
    //            }
    //        });
    //    g_redisClient.sync_commit();
    //    if (redisOk)
    //    {
    //        g_redisInited = true;
    //        InterlockedIncrement(&totalRedisConnect);
    //    }
    //}

    if (GetRedisInited() == false)
    {
        bool redisOk = false;
        GetRedisClient().connect(
            "127.0.0.1", 6379,
            [&](const std::string& host,
                std::size_t port,
                cpp_redis::connect_state status)
            {
                if (status == cpp_redis::connect_state::ok)
                {
                    redisOk = true;    // TCP 레벨 연결 성공
                }
            });
        GetRedisClient().sync_commit();
        if (redisOk)
        {
            GetRedisInited() = true;
            InterlockedIncrement(&totalRedisConnect);
        }
    }

    while (true)
    {
        BOOL res = GetQueuedCompletionStatus(server->m_iocpHandle, &bytesTransferred, (PULONG_PTR)&session, &overlapped, INFINITE);
        // 종료 신호: session==NULL, overlapped==NULL이면 종료
      /*  if (session == nullptr && overlapped == nullptr)
            break;*/
        if (overlapped == nullptr)
        {
            if ((ULONG_PTR)session == (ULONG_PTR)(-1))
            {
                server->PostSendAll();
                continue;
            }
            if (session == nullptr)
                break;
        }

        if (!res)
        {
            int err = GetLastError();
            if (err == ERROR_OPERATION_ABORTED)
            {
                if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                    server->ReleaseSession(session);
                continue;
            }

            // 오류 처리 (필요 시 로그)
        }
        if (bytesTransferred == 0)
        {
            //ioCnt--
            if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                server->ReleaseSession(session);
            continue;
        }
        //if (session == nullptr || overlapped == nullptr)
        //{
        //    printf("session 또는 overlapped가 nullptr\n");
        //    continue;
        //}

        if (overlapped == &session->recvOverlapped)
        {
            //server->m_recvBytes[TPSIndex] += bytesTransferred;

            session->curReaderBuf->MoveWritePos(bytesTransferred);

            while (true)
            {
                if (session->curReaderBuf->GetDataSize() < sizeof(NetWorkHeader))
                    break;

                NetWorkHeader netHeader;
                int netHeaderPeek = session->curReaderBuf->PeekData((char*)&netHeader, sizeof(NetWorkHeader));

                if (netHeader.code != Wan_Protocol_Code)
                {
                    //printf("FIXED_CODE ERROR\n");
                    //DebugBreak();
                    server->DisconnectSession(session);
                    InterlockedIncrement(&server->disconnectCntProtocol);
                    break;
                }

                if (netHeader.len > 1000)
                {
                    server->DisconnectSession(session);
                    break;
                }

                if (session->curReaderBuf->GetDataSize() < sizeof(NetWorkHeader) + netHeader.len)
                    break;

                //여기 수정해야됨 
                PacketBuffer* packet = PacketBuffer::Alloc();
                session->curReaderBuf->DequeueData(packet->GetBufferPtr(), netHeader.len + sizeof(NetWorkHeader));
                packet->MoveWritePos(netHeader.len + sizeof(NetWorkHeader));
                packet->Decode(packet);

                packet->DequeueData((char*)&netHeader, sizeof(netHeader));

                if (netHeader.checkSum != packet->GetPayLoadCheckSum(packet))
                {
                    //printf("CHECKSUM_ERROR");
                    //DebugBreak();
                    server->DisconnectSession(session);
           		    InterlockedIncrement(&server->disconnectCntProtocol);  
                    break;
                }
 
                //OnRecv
                server->m_recvTps[TPSIndex]++;
                server->OnRecv(session->sessionID, packet);
                packet->Release();
            }

            PacketBufferReader* newReaderBuf = PacketBufferReader::Alloc();
            PacketBufferReader::copyRest(session->curReaderBuf, newReaderBuf);
            session->curReaderBuf->Release();
            session->curReaderBuf = newReaderBuf;


            if (!server->PostRecv(session))
            {
                //ioCnt--
                if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                    server->ReleaseSession(session);
            }
        }
        else if (overlapped == &session->sendOverlapped)
        {
            //server->m_sendBytes[TPSIndex] += (bytesTransferred + ((bytesTransferred /1460)+1)*40);

            for (int i = 0; i < session->sendCnt;i++)
            {
                PacketBuffer* toDeleteBuffer;
                session->sendQueue.Dequeue(toDeleteBuffer);
                toDeleteBuffer->Release();

                ////sendCnt--
                //InterlockedDecrement(&session->sendState.parts.Count);
                //server->m_sendTps[TPSIndex]++;
            }
            //sendCnt--
            //InterlockedAdd(&session->sendState.parts.Count, -(session->sendCnt));
            server->m_sendTps[TPSIndex] += session->sendCnt;
            //sendflag 다시 0으로 
            InterlockedExchange(&session->sendState.parts.Flag, 0);

            //if (!server->PostSend(session))
            //{
            //    //ioCnt--
            //    if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            //        server->ReleaseSession(session);
            //}
            //
        }
        //ioCnt--
        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            server->ReleaseSession(session);
    }
    printf("WorkerThread 종료\n");
  /*  if (g_redisInited)
    {
        g_redisClient.disconnect();
        g_redisInited = false;
    }*/
    if (GetRedisInited())
    {
        GetRedisClient().disconnect();
        GetRedisInited() = false;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// Accept Thread: listen 소켓(m_listenSock)을 사용, 소켓이 닫히면 종료
//////////////////////////////////////////////////////////////////////////
unsigned __stdcall CLanServer::AcceptThread(LPVOID param)
{
    CLanServer* server = reinterpret_cast<CLanServer*>(param);
    SOCKET listenSock = server->m_listenSock;
    printf("Server listening on %s:%d\n", (strlen(server->m_startIP) > 0 ? server->m_startIP : "0.0.0.0"), server->m_startPort);

    while (true)
    {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrLen);
        if (clientSock == INVALID_SOCKET)
        {
            printf("Accept 종료\n");
            break;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        if (!server->OnConnectionRequest(clientIP, ntohs(clientAddr.sin_port)))
        {
            closesocket(clientSock);
            continue;
        }


        // freeIndices(stack)에서 인덱스 추출 (락 보호)
        uint16_t index = 0;
        EnterCriticalSection(&server->m_stackCS);
        if (server->m_freeIndices.empty())
        {
            LeaveCriticalSection(&server->m_stackCS);
            closesocket(clientSock);
            continue;
        }

        index = server->m_freeIndices.top();
        server->m_freeIndices.pop();
        LeaveCriticalSection(&server->m_stackCS);
        //printf("accept 에서 index : %d\n", index);
        //SESSION* session = new SESSION();
        SESSION* session = server->m_sessions[index];
       // session->Clear();
        session->Init();
        session->sock = clientSock;
        session->ip = clientIP;
        session->port = ntohs(clientAddr.sin_port);
        session->sessionID = server->MakeSessionID(index);
        //session->isActive = true;

        CreateIoCompletionPort((HANDLE)clientSock, server->m_iocpHandle, (ULONG_PTR)session, 0);

        InterlockedIncrement(&server->acceptTPS);

        if (!server->OnClientJoin(session->sessionID))
        {
            if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                server->ReleaseSession(session);

            continue;
        }

        if (!server->PostRecv(session))
        {
            //printf("초기 WSARecv 등록 실패: session %llu\n", session->sessionID);
            //if (InterlockedDecrement(&session->ioCount) == 0)
                //server->ReleaseSession(session);
            //ioCnt--
            if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                server->ReleaseSession(session);
        }

        // Init에서 올린 ioCnt--
        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            server->ReleaseSession(session);
    }
    printf("AcceptThread 종료\n");
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// Start / Stop
//////////////////////////////////////////////////////////////////////////
bool CLanServer::Start(const char* ip, int port, int workerThreadCount, int conCurrentThreadCount, int maxConnections, int maxPlayers, bool useNagle)
{
    strncpy_s(m_startIP, sizeof(m_startIP), ip, _TRUNCATE);
    m_startIP[sizeof(m_startIP) - 1] = '\0';
    m_startPort = port;

    timeBeginPeriod(1);

    m_maxConnections = maxConnections;
    m_maxPlayers = maxPlayers;
    m_uniqueIdCounter = 1;

    // TPS 카운터 배열 생성
    m_recvTps = new uint64_t[workerThreadCount];
    m_sendTps = new uint64_t[workerThreadCount];
    for (int i = 0; i < workerThreadCount; ++i)
    {
        m_recvTps[i] = 0;
        m_sendTps[i] = 0;
    }

    //TPBytes 카운터 배열 생성
    m_recvBytes = new uint64_t[workerThreadCount];
    m_sendBytes = new uint64_t[workerThreadCount];
    for (int i = 0;i < workerThreadCount; i++)
    {
        m_recvBytes[i] = 0;
        m_sendBytes[i] = 0;
    }

    m_sessions.resize(m_maxConnections, nullptr);
    for (int i = 0; i < m_maxConnections; i++)
    {
        SESSION* session = new SESSION();
        m_sessions[i] = session;
    }
    //workerthread에 넘길 파람 정의
    m_workerParams.resize(workerThreadCount, nullptr);
    for (int i = 0; i < workerThreadCount; i++)
    {
        WorkerParam* param = new WorkerParam;
        param->server = this;
        param->idx = i;
        m_workerParams[i] = param;
    }

    // m_sessions 벡터를 최대 접속자 수 크기로 초기화하고, 모든 인덱스를 m_freeIndices에 넣음.
    {
        EnterCriticalSection(&m_stackCS);
        for (uint16_t i = 0; i < m_maxConnections; ++i)
        {
            m_freeIndices.push(i);
        }
        LeaveCriticalSection(&m_stackCS);
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패\n");
        return false;
    }
    g_logManager.inputLog("WSAStartup 성공\n");
    m_listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenSock == INVALID_SOCKET)
    {
        printf("listen 소켓 생성 실패\n");
        return false;
    }
    g_logManager.inputLog("listen 소켓 생성 성공\n");
    struct linger lingerOption = { 1, 0 };
    setsockopt(m_listenSock, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(lingerOption));

    int sendBufferSize = 0;
    setsockopt(m_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufferSize, sizeof(sendBufferSize));
    g_logManager.inputLog("송신버퍼 0 링거 활성화 성공\n");
    if (!useNagle)
    {
        int nagleFlag = 1;
        setsockopt(m_listenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&nagleFlag, sizeof(nagleFlag));
        printf("Nagle Off\n");
        g_logManager.inputLog("Nagle Off\n");
    }
    else
    {
        printf("Nagle On\n");
        g_logManager.inputLog("Nagle On\n");
    }


    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(m_startPort);

    if (::bind(m_listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        printf("bind 실패\n");
        closesocket(m_listenSock);
        return false;
    }
    g_logManager.inputLog("bind 성공\n");
    if (listen(m_listenSock, SOMAXCONN_HINT(65535)) == SOCKET_ERROR)
    {
        printf("listen 실패\n");
        closesocket(m_listenSock);
        return false;
    }
    m_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, conCurrentThreadCount);
    if (m_iocpHandle == NULL)
    {
        printf("IOCP 생성 실패\n");
        return false;
    }

    m_workerThreadCount = workerThreadCount;
    m_workerThreads = new HANDLE[m_workerThreadCount];
    //std::vector<WorkerParam> m_workerParams(workerThreadCount);
    for (int i = 0; i < m_workerThreadCount; i++)
    {
        unsigned threadID;

        m_workerThreads[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, m_workerParams[i], 0, &threadID);
        if (m_workerThreads[i] == NULL)
        {
            printf("Worker 스레드 생성 실패\n");
            return false;
        }
    }

    /*m_monitorEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_monitorEvent == NULL)
    {
        printf("Monitor 이벤트 생성 실패\n");
        return false;
    }*/

    unsigned acceptThreadID;
    m_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, &acceptThreadID);
    if (m_acceptThread == NULL)
    {
        printf("Accept 스레드 생성 실패\n");
        return false;
    }

 /*   unsigned monitorThreadID;
    m_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, &monitorThreadID);*/

    return true;
}

void CLanServer::Stop()
{
    if (m_listenSock != INVALID_SOCKET)
    {
        closesocket(m_listenSock);
        m_listenSock = INVALID_SOCKET;
    }
    if (m_acceptThread)
    {
        WaitForSingleObject(m_acceptThread, INFINITE);
        CloseHandle(m_acceptThread);
        m_acceptThread = NULL;
    }
   /* if (m_monitorEvent)
        SetEvent(m_monitorEvent);
    if (m_monitorThread)
    {
        WaitForSingleObject(m_monitorThread, INFINITE);
        CloseHandle(m_monitorThread);
        m_monitorThread = NULL;
    }*/
    for (int i = 0; i < m_workerThreadCount; i++)
    {
        PostQueuedCompletionStatus(m_iocpHandle, 0, (ULONG_PTR)NULL, NULL);
    }
    if (m_workerThreads)
    {
        WaitForMultipleObjects(m_workerThreadCount, m_workerThreads, TRUE, INFINITE);
        for (int i = 0; i < m_workerThreadCount; i++)
        {
            CloseHandle(m_workerThreads[i]);
        }
        delete[] m_workerThreads;
        m_workerThreads = nullptr;
    }

    g_logManager.stop();

    printf("All threads returned\n");

    if (m_iocpHandle)
    {
        CloseHandle(m_iocpHandle);
        m_iocpHandle = NULL;
    }
  /*  if (m_monitorEvent)
    {
        CloseHandle(m_monitorEvent);
        m_monitorEvent = NULL;
    }*/
    WSACleanup();
}
