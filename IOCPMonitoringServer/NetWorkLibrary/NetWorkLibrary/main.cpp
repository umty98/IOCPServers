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

createDump::CCrashDump crashDump;

bool GetMonitoringWanServerSetting(string getserverIP, int& getport, int& getworkerThreadCount, int& getconCurrentThreadCount, int& getmaxConnections, int& getmaxPlayers, bool& nagle);
bool GetMonitoringLanServerSetting(string getserverIP, int& getport, int& getworkerThreadCount, int& getconCurrentThreadCount, int& getmaxConnections, int& getmaxPlayers, bool& nagle);
unsigned __stdcall MonitoringThreadProc(void* p);
unsigned __stdcall DBWriterProc(void* p);

DWORD keyTime;
DWORD maxWanPlayer;
DWORD maxLanPlayer;

DWORD sendFrame;
HANDLE g_StopEvent = nullptr;

DWORD WanPort;
DWORD LanPort;

DWORD sendCnt = 0;
DWORD dbConnected = 0;
DWORD dbSave = 0;

#define SENDTIME 300

int main()
{
    MakeAllDataSet();

    g_StopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);  // 수동 리셋

    unsigned monitorThreadID;
    HANDLE hMonitorThread = (HANDLE)_beginthreadex(
        nullptr,                   // 보안 속성
        0,                         // 기본 스택 크기
        MonitoringThreadProc,      // 스레드 함수
        g_StopEvent,       // 파라미터로 이벤트 핸들 전달
        0,                         // 생성 즉시 실행
        &monitorThreadID
    );
    if (!hMonitorThread)
    {
        printf("모니터링 스레드 생성 실패\n");
        CloseHandle(g_StopEvent);
        return 1;
    }

    unsigned dbThreadID;
    HANDLE hDBWriterThread = (HANDLE)_beginthreadex(
        nullptr,
        0,
        DBWriterProc,
        g_StopEvent,
        0,
        &dbThreadID
    );
    if (!hDBWriterThread)
    {
        printf("DB저장 스레드 생성 실패\n");
        CloseHandle(g_StopEvent);
        return 1;
    }

    //////////WAN////////////////
    string serverIP;
    int port;
    int workerThreadCount;
    int conCurrentThreadCount;
    int maxConnections;
    int maxPlayers;
    bool useNagle = true;

    if (GetMonitoringWanServerSetting(serverIP, port, workerThreadCount, conCurrentThreadCount, maxConnections, maxPlayers, useNagle))
    {
        printf("Monitoring Server Wan Parser Clear!\n");
    }
    else
    {
        printf("Monitoring Server Wan Parser something wrong\n");
        return 1;
    }

    maxWanPlayer = maxConnections;
    WanPort = port;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int workerCount = sysInfo.dwNumberOfProcessors;
    // 동시 실행 스레드 수를 (코어 수 - 3)으로 설정 (최소 1 보장)
    int concurrency = workerCount - 3;
    if (concurrency < 1) concurrency = 1;

    workerCount = 6;
    concurrency = 4;

    if (!wanServer.Start(serverIP.c_str(), port, workerThreadCount, conCurrentThreadCount, maxConnections, maxPlayers, useNagle))
    {
        printf("Wan 서버 시작 실패\n");
        return 1;
    }
    //////////LAN////////////////
    string LanserverIP;
    int Lanport;
    int LanworkerThreadCount;
    int LanconCurrentThreadCount;
    int LanmaxConnections;
    int LanmaxPlayers;
    bool LanuseNagle = true;

    if (GetMonitoringLanServerSetting(LanserverIP, Lanport, LanworkerThreadCount, LanconCurrentThreadCount, LanmaxConnections, LanmaxPlayers, LanuseNagle))
    {
        printf("Monitoring Server Lan Parser Clear!\n");
    }
    else
    {
        printf("Monitoring Server Lan Parser something wrong\n");
        return 1;
    }

    maxLanPlayer = LanmaxConnections;
    LanPort = Lanport;

    if (!lanServer.Start(LanserverIP.c_str(), Lanport, LanworkerThreadCount, LanconCurrentThreadCount, LanmaxConnections, LanmaxPlayers, LanuseNagle))
    {
        printf("Lan 서버 시작 실패\n");
        return 1;
    }

    ////////////////////////////////////////////////

    SetCharacter();
    SetLanCharacter();
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

            SendUpdataDataAll();
            keyTime += 1000;
            sendCnt++;
            //비정상 세션 차단
        }

        if (currentTime >= sendFrame)
        {
            //lanserver에 보낼게 없는듯 받기만 하는데??
            //lanServer.SendAllPacket();
            // 
            wanServer.SendAllPacket();
            sendFrame += SENDTIME;
        }

        DWORD sleepTime = (sendFrame > currentTime)
            ? sendFrame - currentTime
            : 0;
        if (sleepTime > 0)
            Sleep(sleepTime);
    }


    SetEvent(g_StopEvent);

    // 4) 모니터링 스레드가 종료될 때까지 대기
    WaitForSingleObject(hMonitorThread, INFINITE);
    WaitForSingleObject(hDBWriterThread, INFINITE);
    // 5) 핸들 정리
    CloseHandle(hMonitorThread);
    CloseHandle(hDBWriterThread);


    wanServer.Stop();
    lanServer.Stop();
    printf("서버가 종료됩니다.\n");

	return 0;
}


bool GetMonitoringWanServerSetting(string getserverIP, int &getport, int &getworkerThreadCount, int &getconCurrentThreadCount, int &getmaxConnections, int& getmaxPlayers, bool& nagle)
{
    Parser parser;
    parser.LoadFile("WanServerSetting.txt");

    parser.GetValue("IP", getserverIP);
    parser.GetValue("Port", getport);
    parser.GetValue("workerThreadCount", getworkerThreadCount);
    parser.GetValue("conCurrentThreadCount", getconCurrentThreadCount);
    parser.GetValue("maxConnections", getmaxConnections);
    parser.GetValue("maxPlayers", getmaxPlayers);
    int nagleOption;
    parser.GetValue("useNagle", nagleOption);
    nagle = nagleOption == 1 ? 1 : 0;

    cout << "Wan IP : " << getserverIP << '\n';
    cout << "Wan Port : " << getport << '\n';
    cout << "Wan workerThreadCount : " << getworkerThreadCount << '\n';
    cout << "Wan conCurrentThreadCount : " << getconCurrentThreadCount << '\n';
    cout << "Wan maxConnections : " << getmaxConnections << '\n';
    cout << "Wan maxPlayers : " << getmaxPlayers << '\n';
    cout << "Wan nagleOption : " << nagle << '\n';

    return parser.CheckLeak();
}

bool GetMonitoringLanServerSetting(string getserverIP, int& getport, int& getworkerThreadCount, int& getconCurrentThreadCount, int& getmaxConnections, int& getmaxPlayers, bool& nagle)
{
    Parser parser;
    parser.LoadFile("LanServerSetting.txt");

    parser.GetValue("IP", getserverIP);
    parser.GetValue("Port", getport);
    parser.GetValue("workerThreadCount", getworkerThreadCount);
    parser.GetValue("conCurrentThreadCount", getconCurrentThreadCount);
    parser.GetValue("maxConnections", getmaxConnections);
    parser.GetValue("maxPlayers", getmaxPlayers);
    int nagleOption;
    parser.GetValue("useNagle", nagleOption);
    nagle = nagleOption == 1 ? 1 : 0;

    cout << "Lan IP : " << getserverIP << '\n';
    cout << "Lan Port : " << getport << '\n';
    cout << "Lan workerThreadCount : " << getworkerThreadCount << '\n';
    cout << "Lan conCurrentThreadCount : " << getconCurrentThreadCount << '\n';
    cout << "Lan maxConnections : " << getmaxConnections << '\n';
    cout << "Lan maxPlayers : " << getmaxPlayers << '\n';
    cout << "Lan nagleOption : " << nagle << '\n';

    return parser.CheckLeak();
}

unsigned __stdcall MonitoringThreadProc(void* p)
{
    HANDLE hStopEvent = (HANDLE)p;

    MonitoringData* chatActive = MonitoringDataMap[dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN];
    MonitoringData* loginActive = MonitoringDataMap[dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN];
    MonitoringData* gameActive = MonitoringDataMap[dfMONITOR_DATA_TYPE_GAME_SERVER_RUN];

    DWORD lastTime = timeGetTime();

    while (WaitForSingleObject(hStopEvent, 1000) == WAIT_TIMEOUT)
    {     
        DWORD waitResult = WaitForSingleObject(hStopEvent, 0);
        if (waitResult == WAIT_OBJECT_0)
            break;

        DWORD currentTime = timeGetTime();

        if (currentTime - lastTime > 1000)
        {
            printf("MonitoringServer\n");
            printf("WanPort : %d | LanPort : %d\n", WanPort, LanPort);
            //printf("sendCnt : %lu\n", sendCnt);
            printf("lanServer Status\n");
            printf("ChatConnected : %c | loginConnected : %c | gameActive : %c\n",
                ((chatActive->data == 1) ? 'O' : 'X'), ((loginActive->data == 1) ? 'O' : 'X'), ((gameActive->data == 1) ? 'O' : 'X'));
            printf("wanServer Status\n");
            printf("monitoringClients : %d\n", wanServer.currentPlayerCnt);
            printf("DBConnected : %c | cnt : %ld\n", (dbConnected == 1) ? 'O' : 'X', dbSave);
            printf("------------PDH--------------\n");
            ProcessorTime.Update();
            int processTotal = ProcessorTime.GetTotal();
            int processKernal = ProcessorTime.GetKernel();
            int processUser = ProcessorTime.GetUser();
            printf("Processor Total : %d | Kernal : %d | User : %d\n", processTotal, processKernal, processUser);
            pdhMonitor.Update();
            int nonpaged = pdhMonitor.GetNonPagedMemory();
            printf("nonpaged: %d Mbytes\n", nonpaged);
            int recvbytes = pdhMonitor.GetRecvBytes();
            printf("recvKbytes: %d\n", recvbytes);
            int sendbytes = pdhMonitor.GetSendBytes();
            printf("sendKbytes: %d\n", sendbytes);
            int useAble = pdhMonitor.GetUseableMemory();
            printf("useAble: %d Mbytes\n", useAble);
            int now = static_cast<int>(time(nullptr));

            //여기서 pdhMonitor수집

            MonitoringData* data40 = MonitoringDataMap[dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL];
            data40->data = processTotal;
            data40->timeStamp = now;
            MonitoringStatsMap[dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL]->Add(processTotal);

            MonitoringData* data41 = MonitoringDataMap[dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY];
            data41->data = nonpaged;
            data41->timeStamp = now;
            MonitoringStatsMap[dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY]->Add(nonpaged);

            MonitoringData* data42 = MonitoringDataMap[dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV];
            data42->data = recvbytes;
            data42->timeStamp = now;
            MonitoringStatsMap[dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV]->Add(recvbytes);

            MonitoringData* data43 = MonitoringDataMap[dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND];
            data43->data = sendbytes;
            data43->timeStamp = now;
            MonitoringStatsMap[dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND]->Add(sendbytes);

            MonitoringData* data44 = MonitoringDataMap[dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY];
            data44->data = useAble;
            data44->timeStamp = now;
            MonitoringStatsMap[dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY]->Add(useAble);

            lastTime += 1000;
        }

        DWORD sleepTime = (lastTime > currentTime)
            ? lastTime - currentTime
            : 0;
        if (sleepTime > 0)
            Sleep(sleepTime);

    }
    printf("MonitoringThread: 종료 신호 감지, 스레드 종료\n");
    return 0;
}


unsigned __stdcall DBWriterProc(void* p)
{
    HANDLE hStopEvent = (HANDLE)p;

    constexpr DWORD DbSaveInterval = 1000 * 60 * 10;  // 10분
   // constexpr DWORD DbSaveInterval = 1000 * 60;  // 10분

    //DWORD lastSaveTime = timeGetTime();
    DataBase db;
    if (db.Connect("127.0.0.1", "root", "game123", "logdb", 3306))
    {
        dbConnected = 1;
    }

     //여기서 DB저장하는거 연결하고 아래에서 하면 될듯
    MonitoringData* chatActive   = MonitoringDataMap[dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN];
    MonitoringData* loginActive  = MonitoringDataMap[dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN];
    MonitoringData* gameActive   = MonitoringDataMap[dfMONITOR_DATA_TYPE_GAME_SERVER_RUN];
    while (1)
    {
        DWORD waitResult = WaitForSingleObject(hStopEvent, DbSaveInterval);
        if (waitResult == WAIT_OBJECT_0)
            break;

        if (dbConnected == 1)
        {
            if (!db.BeginTransaction())
                DebugBreak();
            if (chatActive->data == 1)
            {
                for (int i = ChatServerDataStart + 1; i <= ChatServerDataEnd; i++)
                {
                    int outMin, outMax, outAvg;
                    MonitoringStatsMap[i]->GetSnapshot(outMin, outMax, outAvg);
                    if (!db.InsertMonitorLog(ChatServerNum, i, outMin, outMax, outAvg, MonitoringNameMap[i]))
                        DebugBreak();
                }
            }

            if (loginActive->data == 1)
            {
                for (int i = LoginServerDataStart + 1; i <= LoginServerDataEnd; i++)
                {
                    int outMin, outMax, outAvg;
                    MonitoringStatsMap[i]->GetSnapshot(outMin, outMax, outAvg);
                    if(!db.InsertMonitorLog(LoginServerNum, i, outMin, outMax, outAvg, MonitoringNameMap[i]))
						DebugBreak();
                }
            }

            if (gameActive->data == 1)
            {
                for (int i = GameServerDataStart + 1; i <= GameServerDataEnd; i++)
                {
                    int outMin, outMax, outAvg;
                    MonitoringStatsMap[i]->GetSnapshot(outMin, outMax, outAvg);
                    if(!db.InsertMonitorLog(GameServerNum, i, outMin, outMax, outAvg, MonitoringNameMap[i]))
						DebugBreak();
                }
            }

            for (int i = ServerComDataStart; i <= ServerComDataEnd; i++)
            {
                int outMin, outMax, outAvg;
                MonitoringStatsMap[i]->GetSnapshot(outMin, outMax, outAvg);
                if(!db.InsertMonitorLog(4, i, outMin, outMax, outAvg, MonitoringNameMap[i]))
					DebugBreak();
            }
            if (!db.Commit())
                DebugBreak();
            dbSave++;
        }
        //데이터 초기화
        if (chatActive->data == 1)
        {
            for (int i = ChatServerDataStart + 1; i <= ChatServerDataEnd; i++)
            {
                int outMin, outMax, outAvg;
                MonitoringStatsMap[i]->Reset();
            }
        }
        if (loginActive->data == 1)
        {
            for (int i = LoginServerDataStart + 1; i <= LoginServerDataEnd; i++)
            {
                int outMin, outMax, outAvg;
                MonitoringStatsMap[i]->Reset();
            }
        }
        if (gameActive->data == 1)
        {
            for (int i = GameServerDataStart + 1; i <= GameServerDataEnd; i++)
            {
                int outMin, outMax, outAvg;
                MonitoringStatsMap[i]->Reset();
            }
        }
        for (int i = ServerComDataStart; i <= ServerComDataEnd; i++)
        {
            int outMin, outMax, outAvg;
            MonitoringStatsMap[i]->Reset();
        }
    }
    printf("DBWriterThread: 종료 신호 감지, 스레드 종료\n");
    if(dbConnected == 1)
    {
        db.Disconnect();
	}
    return 0;
}


