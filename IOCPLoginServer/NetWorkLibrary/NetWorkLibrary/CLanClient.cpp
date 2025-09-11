#include "pch.h"

CLanClient::CLanClient()
    :m_uniqueIdCounter(1),
    m_maxConnections(0),
    m_iocpHandle(NULL),
    m_workerThreads(nullptr),
    m_workerThreadCount(0),
    m_maxPlayers(0),
    currentPlayerCnt(0)
{
    InitializeCriticalSection(&m_stackCS);
    m_startIP[0] = '\0';
    m_startPort = 0;
}

void CLanClient::SendAllPacket()
{
    PostQueuedCompletionStatus(m_iocpHandle, 0, (ULONG_PTR)(-1), NULL);
}

void CLanClient::PostSendAll()
{
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
}

CLanClient::~CLanClient()
{
    DeleteCriticalSection(&m_stackCS);

    for (int i = 0; i < m_maxConnections; i++)
    {
        delete m_sessions[i];
    }
}


bool CLanClient::Connect(const char* ip, int port, int workerThreadCount, int conCurrentThreadCount, int maxConnections, int maxPlayers, bool useNagle)
{

    strncpy_s(m_startIP, sizeof(m_startIP), ip, _TRUNCATE);
    m_startIP[sizeof(m_startIP) - 1] = '\0';
    m_startPort = port;

    timeBeginPeriod(1);

    m_maxConnections = maxConnections;
    m_maxPlayers = maxPlayers;
    m_uniqueIdCounter = 1;

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
        param->client = this;
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

    m_iocpHandle = CreateIoCompletionPort
    (
        INVALID_HANDLE_VALUE,
        nullptr,
        0,
        conCurrentThreadCount
    );
    if (!m_iocpHandle)
    {
        printf("IOCP 핸들 생성 실패\n");
        return false;
    }

    for (int i = 0; i < m_maxConnections; ++i)
    {
        uint16_t idx = m_freeIndices.top();
        m_freeIndices.pop();

        SESSION* session = m_sessions[idx];
        session->sessionID = MakeSessionID(idx);
        session->Init();

        SOCKET s = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);

        if (s == INVALID_SOCKET)
        {
            printf("클라이언트 소켓 생성 실패\n");
            return false;
        }

        struct linger lng = { 1, 0 };
        setsockopt(s, SOL_SOCKET, SO_LINGER, (char*)&lng, sizeof(lng));
        int sndBuf = 0;
        setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&sndBuf, sizeof(sndBuf));
        if (!useNagle)
        {
            int flag = 1;
            setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
        }

        sockaddr_in srv = {};
        srv.sin_family = AF_INET;
        srv.sin_port = htons((uint16_t)port);
        inet_pton(AF_INET, ip, &srv.sin_addr);

        if (connect(s, (sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS)
            {
                printf("서버 연결 실패: %d\n", err);
                return false;
            }
        }

        CreateIoCompletionPort(
            (HANDLE)s,
            m_iocpHandle,
            (ULONG_PTR)session,
            0
        );
        session->sock = s;

        OnEnterJoinServer(session->sessionID);

        if (!PostRecv(session))
        {
            if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                ReleaseSession(session);
        }

        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            ReleaseSession(session);
    }

    m_workerThreads = new HANDLE[workerThreadCount];
    m_workerThreadCount = workerThreadCount;
    for (int i = 0; i < workerThreadCount; ++i) {
        unsigned tid;
        m_workerThreads[i] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, m_workerParams[i], 0, &tid);
        if (!m_workerThreads[i]) {
            printf("워커 스레드 생성 실패\n");
            return false;
        }
    }

    return true;
}

void CLanClient::Stop()
{
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

    printf("All threads returned from client\n");

    if (m_iocpHandle)
    {
        CloseHandle(m_iocpHandle);
        m_iocpHandle = NULL;
    }

    WSACleanup();
}

bool CLanClient::Disconnect(uint64_t sessionID)
{
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

bool CLanClient::SendPacket(uint64_t sessionID, PacketBuffer* packet)
{
    uint16_t index = GetSessionIndex(sessionID);
    if (index >= m_sessions.size() || m_sessions[index] == nullptr ||
        m_sessions[index]->sessionID != sessionID)
    {
        return false;
    }

    //if (!m_sessions[index]->isActive)
    //{
    //    printf("send에서 isactive false\n");
    //}

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
        netHeader.code = Lan_Protocol_Code;
        netHeader.len = packet->GetDataSize() - sizeof(NetWorkHeader);
        netHeader.randkey = Random_Key;
        netHeader.checkSum = -1;
        packet->EnqueueHeader((char*)&netHeader, sizeof(NetWorkHeader));

        //packet->SetCheckSum(packet);

        //여기서 encoding
        //packet->Encode(packet);
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

void CLanClient::ReleaseSession(SESSION* session)
{
    uint16_t index = GetSessionIndex(session->sessionID);

    //ioCnt랑 ioFlag가 0이면 CAS로 ioFlag1로 바꿈
    if (InterlockedCompareExchange64(&session->ioState.whole, 1, 0) != 0)
        return;


    PacketBuffer* buffer;
    while (1)
    {
        int ret = session->sendQueue.Dequeue(buffer);
        if (ret == -1)
            break;
        buffer->Release();
    }

    OnLeaveServer(session->sessionID);

    // freeIndices(stack)에 인덱스 반환 (락으로 보호)
    EnterCriticalSection(&m_stackCS);
    //소켓 닫고 해당 인덱스 세션 클리어해서 바로 재사용될 수 있게
    closesocket(session->sock);
    m_sessions[index]->Clear();
    m_freeIndices.push(index);
    LeaveCriticalSection(&m_stackCS);
}

bool CLanClient::PostRecv(SESSION* session)
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

bool CLanClient::PostSend(SESSION* session)
{
    if (session->stopIO == 1)
        return true;

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

        if (bufCount > 100) // 최대 버퍼 개수 제한 (예: 100개로 설정, 필요에 따라 조정 가능)
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

void CLanClient::DisconnectSession(SESSION* session)
{
    InterlockedExchange(&session->stopIO, 1);

    CancelIoEx((HANDLE)session->sock, nullptr);
}

unsigned __stdcall CLanClient::WorkerThread(LPVOID param)
{
    WorkerParam* wParam = static_cast<WorkerParam*>(param);
    CLanClient* client = wParam->client;

    printf("client workerThread 생성\n");

    DWORD bytesTransferred;
    SESSION* session = nullptr;
    OVERLAPPED* overlapped = nullptr;

    while (true)
    {
        BOOL res = GetQueuedCompletionStatus(client->m_iocpHandle, &bytesTransferred, (PULONG_PTR)&session, &overlapped, INFINITE);
        // 종료 신호: session==NULL, overlapped==NULL이면 종료
      /*  if (session == nullptr && overlapped == nullptr)
            break;*/

        if (overlapped == nullptr)
        {
            if ((ULONG_PTR)session == (ULONG_PTR)(-1))
            {
                client->PostSendAll();
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
                    client->ReleaseSession(session);
                continue;
            }

            // 오류 처리 (필요 시 로그)
        }
        if (bytesTransferred == 0)
        {
            //ioCnt--
            if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                client->ReleaseSession(session);
            continue;
        }

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

                if (netHeader.code != Lan_Protocol_Code)
                {
                    //printf("FIXED_CODE ERROR\n");
                    //DebugBreak();
                    client->DisconnectSession(session);
                    //InterlockedIncrement(&client->disconnectCntProtocol);
                    break;
                }

                if (session->curReaderBuf->GetDataSize() < sizeof(NetWorkHeader) + netHeader.len)
                    break;

                //여기 수정해야됨 
                PacketBuffer* packet = PacketBuffer::Alloc();
                session->curReaderBuf->DequeueData(packet->GetBufferPtr(), netHeader.len + sizeof(NetWorkHeader));
                packet->MoveWritePos(netHeader.len + sizeof(NetWorkHeader));
                //packet->Decode(packet);

                packet->DequeueData((char*)&netHeader, sizeof(netHeader));

                //if (netHeader.checkSum != packet->GetPayLoadCheckSum(packet))
                //{
                //    //printf("CHECKSUM_ERROR");
                //    //DebugBreak();
                //    client->DisconnectSession(session);
                //    //InterlockedIncrement(&server->disconnectCntProtocol);
                //    break;
                //}

                //OnRecv
                //server->m_recvTps[TPSIndex]++;
                client->OnRecv(session->sessionID, packet);
                packet->Release();
            }

            PacketBufferReader* newReaderBuf = PacketBufferReader::Alloc();
            PacketBufferReader::copyRest(session->curReaderBuf, newReaderBuf);
            session->curReaderBuf->Release();
            session->curReaderBuf = newReaderBuf;


            if (!client->PostRecv(session))
            {
                //ioCnt--
                if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                    client->ReleaseSession(session);
            }
        }
        else if (overlapped == &session->sendOverlapped)
        {
            //server->m_sendBytes[TPSIndex] += (bytesTransferred + ((bytesTransferred /1460)+1)*40);

            for (int i = 0; i < session->sendCnt; i++)
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
            //sendflag 다시 0으로 
            InterlockedExchange(&session->sendState.parts.Flag, 0);

            if (!client->PostSend(session))
            {
                //ioCnt--
                if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
                    client->ReleaseSession(session);
            }
            //
        }
        //ioCnt--
        if (InterlockedDecrement(&session->ioState.parts.Count) == 0)
            client->ReleaseSession(session);
    }
    printf("WorkerThread 종료\n");
    return 0;
}

/////////////////////////////

std::vector<st_Client*> clientMap;
MyLanClient client;
DWORD isClientConnected = 0;

void MyLanClient::OnEnterJoinServer(uint64_t sessionID)
{
    isClientConnected = 1;
    st_Client* newClient = clientMap[GetClientIndex(sessionID)];
    newClient->sessionID = sessionID;
    newClient->isActive = true;
}

void MyLanClient::OnLeaveServer(uint64_t sessionID)
{
    isClientConnected = 0;
    st_Client* newClient = clientMap[GetClientIndex(sessionID)];
    newClient->sessionID = 0;
    newClient->isActive = false;
}

void MyLanClient::OnRecv(uint64_t sessionID, PacketBuffer* packet)
{
    ContentHeader conHeader;
    packet->DequeueData((char*)&conHeader, sizeof(ContentHeader));

    switch (conHeader.type)
    {
    default:
        client.Disconnect(sessionID);
        break;
    }
}

void SetClient()
{
    clientMap.resize(maxClient, nullptr);
    for (int i = 0; i < maxClient; i++)
    {
        st_Client* client = new st_Client();
        clientMap[i] = client;
    }
}

void Lan_SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet)
{
    client.SendPacket(sessionID, packet);
}
