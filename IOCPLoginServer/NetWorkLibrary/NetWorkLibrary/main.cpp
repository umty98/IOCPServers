#include <iostream>
#include <conio.h>
//#include "CLanServer.h"
//#include "Content.h"
#include "pch.h"
//#include "PacketBuffer.h"
//#include "PacketBufferReader.h"
//#include "TLSObjectPool.h"
 
//#include "Parser.h"
#include "MiniDump.h"
using namespace std;

//#define SERVERPORT 6000
#define SENDTIME 100
createDump::CCrashDump crashDump;

bool GetServerSetting(string getserverIP, int& getport, int& getworkerThreadCount, int& getconCurrentThreadCount, int& getmaxConnections, int& getmaxPlayers, bool& nagle);
bool GetClientSetting(string& clientIP, int& clientPort, int& clientWorkerThread, int& clientConCurrentThread, int& clientmaxConnections, int& clientmaxPlayers, bool& clientuseNagle);
unsigned __stdcall MonitoringThreadProc(void* p);
DWORD startTime;
DWORD keyTime;
DWORD maxPlayer;
DWORD maxClient;
DWORD sendFrame;
DWORD sendCnt = 0;
DWORD sendFrameFlag = 0;

HANDLE g_hMonitorStopEvent = nullptr;


int main()
{
    timeBeginPeriod(1);
    g_hMonitorStopEvent = CreateEvent
    (
        nullptr,   // 보안 속성
        TRUE,      // 수동 리셋
        FALSE,     // 초기 상태 = 비시그널
        nullptr    // 이름 없음
    );
    if (!g_hMonitorStopEvent)
    {
        printf("모니터링 이벤트 생성 실패 (%u)\n", GetLastError());
        return 1;
    }

    //server
    string serverIP;
    int port;
    int workerThreadCount;
    int conCurrentThreadCount;
    int maxConnections;
    int maxPlayers;
    bool useNagle = true;

    if (GetServerSetting(serverIP, port, workerThreadCount, conCurrentThreadCount, maxConnections, maxPlayers, useNagle))
    {
        printf("Parser Clear!\n");
    }
    else
    {
        printf("Parser something wrong\n");
        return 1;
    }

    maxPlayer = maxConnections;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int workerCount = sysInfo.dwNumberOfProcessors;
    // 동시 실행 스레드 수를 (코어 수 - 3)으로 설정 (최소 1 보장)
    int concurrency = workerCount - 3;
    if (concurrency < 1) concurrency = 1;

    workerCount = 6;
    concurrency = 4;

    if (!server.Start(serverIP.c_str(), port, workerThreadCount, conCurrentThreadCount, maxConnections, maxPlayers, useNagle))
    {
        printf("서버 시작 실패\n");
        return 1;
    }
    
    //client
    string clientIP;
    int clientPort;
    int clientWorkerThread;
    int clientConCurrentThread;
    int clientmaxConnections;
    int clientmaxPlayers;
    bool clientuseNagle = true;

    if (GetClientSetting(clientIP, clientPort, clientWorkerThread, clientConCurrentThread, clientmaxConnections, clientmaxPlayers, clientuseNagle))
    {
        printf("Client Setting Parser Clear!\n");
    }
    else
    {
        printf("Client Setting Parser something wrong\n");
        return 1;
    }
    maxClient = clientmaxConnections;
    SetClient();

    if (!client.Connect(clientIP.c_str(), clientPort, clientWorkerThread, clientConCurrentThread, clientmaxConnections, clientmaxPlayers, clientuseNagle))
    {
        printf("모니터링 연결 실패\n");
        // return 1;
    }

    ////모니터링 스레드 생성
    unsigned monitorThreadID;
    HANDLE hMonitorThread = (HANDLE)_beginthreadex
    (
        nullptr,                   // 보안 속성
        0,                         // 기본 스택 크기
        MonitoringThreadProc,      // 스레드 함수
        g_hMonitorStopEvent,       // 파라미터로 이벤트 핸들 전달
        0,                         // 생성 즉시 실행
        &monitorThreadID
    );
    if (!hMonitorThread)
    {
        printf("모니터링 스레드 생성 실패\n");
        CloseHandle(g_hMonitorStopEvent);
        return 1;
    }

    SetCharacter();
    startTime = timeGetTime();
    keyTime = timeGetTime();
    sendFrame = timeGetTime();

    while (1)
    {
        DWORD currentTime = timeGetTime();

        if (currentTime - keyTime > 1000)
        {
            if (_kbhit())  // 키 입력이 있는지 확인
            {
                int ch = _getch();
                if (ch == 's' || ch == 'S')
                {
                    break;
                }
            }
            keyTime += 1000;

            //CheckWrongConnection();
        }

        if (currentTime >= sendFrame)
        {
            if (InterlockedExchange(&sendFrameFlag, 1) == 0)
            {
                server.SendAllPacket();
                sendCnt++;
            }
            sendFrame += SENDTIME;
        }

        DWORD sleepTime = (sendFrame > currentTime)
            ? sendFrame - currentTime
            : 0;
        if (sleepTime > 0)
            Sleep(sleepTime);
    }

    server.Stop();
    printf("서버가 종료됩니다.\n");

	return 0;
}


bool GetServerSetting(string getserverIP, int &getport, int &getworkerThreadCount, int &getconCurrentThreadCount, int &getmaxConnections, int& getmaxPlayers, bool& nagle)
{
    Parser parser;
    parser.LoadFile("ServerSetting.txt");

    parser.GetValue("IP", getserverIP);
    parser.GetValue("Port", getport);
    parser.GetValue("workerThreadCount", getworkerThreadCount);
    parser.GetValue("conCurrentThreadCount", getconCurrentThreadCount);
    parser.GetValue("maxConnections", getmaxConnections);
    parser.GetValue("maxPlayers", getmaxPlayers);
    int nagleOption;
    parser.GetValue("useNagle", nagleOption);
    nagle = nagleOption == 1 ? 1 : 0;

    cout << "IP : " << getserverIP << '\n';
    cout << "Port : " << getport << '\n';
    cout << "workerThreadCount : " << getworkerThreadCount << '\n';
    cout << "conCurrentThreadCount : " << getconCurrentThreadCount << '\n';
    cout << "maxConnections : " << getmaxConnections << '\n';
    cout << "maxPlayers : " << getmaxPlayers << '\n';
    cout << "nagleOption : " << nagle << '\n';

    return parser.CheckLeak();
}


bool GetClientSetting(string& clientIP, int& clientPort, int& clientWorkerThread, int& clientConCurrentThread, int& clientmaxConnections, int& clientmaxPlayers, bool& clientuseNagle)
{
    Parser parser;
    parser.LoadFile("ClientSetting.txt");

    parser.GetValue("IP", clientIP);
    parser.GetValue("Port", clientPort);
    parser.GetValue("workerThreadCount", clientWorkerThread);
    parser.GetValue("conCurrentThreadCount", clientConCurrentThread);
    parser.GetValue("maxConnections", clientmaxConnections);
    parser.GetValue("maxPlayers", clientmaxPlayers);
    int nagleOption;
    parser.GetValue("useNagle", nagleOption);
    clientuseNagle = nagleOption == 1 ? 1 : 0;

    cout << "client setting" << '\n';
    cout << "IP : " << clientIP << '\n';
    cout << "Port : " << clientPort << '\n';
    cout << "workerThreadCount : " << clientWorkerThread << '\n';
    cout << "conCurrentThreadCount : " << clientConCurrentThread << '\n';
    cout << "maxConnections : " << clientmaxConnections << '\n';
    cout << "maxPlayers : " << clientmaxPlayers << '\n';
    cout << "nagleOption : " << clientuseNagle << '\n';

    return parser.CheckLeak();
}



unsigned __stdcall MonitoringThreadProc(void* p)
{
    DWORD nextPrint = timeGetTime();

    DWORD clientID = clientMap[0]->sessionID;

    PacketBuffer* packet = client.CreatePacketBuffer();
    mpMonitorLogin(packet, LoginServerNum);
    Lan_SendPacket_Unicast(clientID, packet);
    packet->Release();

    client.SendAllPacket();

    while (true)
    {
        DWORD dw = WaitForSingleObject(g_hMonitorStopEvent, 0);
        if (dw == WAIT_OBJECT_0)
            break;

        DWORD now = timeGetTime();
        int nowDB = static_cast<int>(time(nullptr));

        if (now >= nextPrint)
        {
            nextPrint += 1000;
            int cnt = server.GetSessionCount();

            uint64_t totalRBytes = 0, totalSBytes = 0, totalRTPS = 0, totalSTPS = 0;

            //system("cls");
            printf("-----------Login server status-----------\n");
            for (int i = 0; i < server.m_workerThreadCount; i++)
            {
                uint64_t rBytes = server.m_recvBytes[i];
                uint64_t sBytes = server.m_sendBytes[i];

                uint64_t rTPS = server.m_recvTps[i];
                uint64_t sTPS = server.m_sendTps[i];

                server.m_recvBytes[i] = 0;
                server.m_sendBytes[i] = 0;

                server.m_recvTps[i] = 0;
                server.m_sendTps[i] = 0;

                /*   printf("Worker %2d | Recv=%10llu B/s | Send=%10llu B/s | Recv=%10llu TPS/s | Send=%10llu TPS/s\n",
                       i, (unsigned long long)rBytes, (unsigned long long)sBytes, (unsigned long long)rTPS, (unsigned long long)sTPS);*/
                totalRBytes += rBytes;
                totalSBytes += sBytes;
                totalRTPS += rTPS;
                totalSTPS += sTPS;
            }

            printf(">> TOTAL  Recv=%10llu B/s | Send=%10llu B/s | Recv=%10llu TPS/s | Send=%10llu TPS/s\n\n",
                (unsigned long long)totalRBytes,
                (unsigned long long)totalSBytes,
                (unsigned long long)totalRTPS,
                (unsigned long long)totalSTPS);

            printf("Monitoring Server : %s\n", isClientConnected == 1 ? "connected" : "disconnected");
            printf("dbConnectCnt : %lu | redisConnectCnt : %lu\n", totalDBConnect, totalRedisConnect);
            printf("Total Accepted: (고유 카운터: %llu), Current Session Count: %d\n", server.m_uniqueIdCounter, cnt);
            printf("CharacterCnt : %lu\n", server.currentPlayerCnt);
            printf("packetbufferCnt2 : %lu\n", TLSMemoryPool<PacketBuffer>::GetCount());
            printf("acceptTPS : %lu | packetTPS : %lu | loginTPS : %lu\n", server.acceptTPS, packetTPS, loginTPS);
            printf("sendFrame : %lu\n", sendCnt);
            sendCnt = 0;

            printf("Disconnect  protocol : %lu | checksum : %lu | sameID : %lu\n", server.disconnectCntProtocol, server.disconnectCntCheckSum, sameIDDisconnectCnt);
			printf("packetTypeError : %lu | packetLenError : %lu\n", server.disconnectPacketType, LPLongCnt);
            /////PDHMonitoring and CPUusage
            printf("------------PDH--------------\n");

            PacketBuffer* buffer0 = client.CreatePacketBuffer();
            mpMonitorDataUpdate(buffer0, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, isClientConnected, nowDB);
            Lan_SendPacket_Unicast(clientID, buffer0);
            buffer0->Release();

            ProcessorTime.Update();
            ProcessTime.Update();
            printf("Processor:%d / Process:%d \n", ProcessorTime.GetTotal(), ProcessTime.GetTotal());
            PacketBuffer* buffer1 = client.CreatePacketBuffer();
            mpMonitorDataUpdate(buffer1, dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, ProcessTime.GetTotal(), nowDB);
            Lan_SendPacket_Unicast(clientID, buffer1);
            buffer1->Release();

            printf("ProcessorKernel:%d / ProcessKernel:%d \n", ProcessorTime.GetKernel(), ProcessTime.GetKernel());
            printf("ProcessorUser:%d / ProcessUser:%d \n", ProcessorTime.GetUser(), ProcessTime.GetUser());
            pdhMonitor.Update();
            printf("ProcessUser: %d Mbytes\n", pdhMonitor.GetProcessUserMemory());
            PacketBuffer* buffer2 = client.CreatePacketBuffer();
            mpMonitorDataUpdate(buffer2, dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, pdhMonitor.GetProcessUserMemory(), nowDB);
            Lan_SendPacket_Unicast(clientID, buffer2);
            buffer2->Release();

            printf("ProcessNonpaged: %d bytes\n", pdhMonitor.GetProcessNonpagedMemory());
            printf("useAble: %d Mbytes\n", pdhMonitor.GetUseableMemory());
            printf("nonpaged: %d Mbytes\n", pdhMonitor.GetNonPagedMemory());
            printf("recvKbytes: %d\n", pdhMonitor.GetRecvBytes());
            printf("sendKbytes: %d\n", pdhMonitor.GetSendBytes());

            PacketBuffer* buffer3 = client.CreatePacketBuffer();
            mpMonitorDataUpdate(buffer3, dfMONITOR_DATA_TYPE_LOGIN_SESSION, cnt, nowDB);
            Lan_SendPacket_Unicast(clientID, buffer3);
            buffer3->Release();

            PacketBuffer* buffer5 = client.CreatePacketBuffer();
            mpMonitorDataUpdate(buffer5, dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, loginTPS, nowDB);
            Lan_SendPacket_Unicast(clientID, buffer5);
            buffer5->Release();

            PacketBuffer* buffer6 = client.CreatePacketBuffer();
            mpMonitorDataUpdate(buffer6, dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, TLSMemoryPool<PacketBuffer>::GetCount(), nowDB);
            Lan_SendPacket_Unicast(clientID, buffer6);
            buffer6->Release();

            InterlockedExchange(&server.acceptTPS, 0);
            InterlockedExchange(&packetTPS, 0);
            InterlockedExchange(&loginTPS, 0);

            //여기서 send를 할 생각
            client.SendAllPacket();
        }

        now = timeGetTime();
        DWORD sleepTime = (nextPrint > now) ? (nextPrint - now) : 0;
        if (sleepTime > 0)
            Sleep(sleepTime);
    }

    printf("MonitoringThread: 종료 신호 감지, 스레드 종료\n");
    return 0;
}