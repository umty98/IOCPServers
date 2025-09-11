#pragma once

//extern LockFreeQueue<PacketBuffer*> jobQueue;
extern int frameCnt;

//
extern DWORD jobQueueSize;
extern DWORD successSize;
extern DWORD sendCnt;
extern DWORD sendFrameFlag;

class CLanServer
{
public:
    // 패킷 헤더 구조체
#pragma pack(push, 1)
    struct NetWorkHeader
    {
        BYTE code;
        WORD len;
        BYTE randkey;
        BYTE checkSum;
    };
#pragma pack(pop)

    typedef union
    {
        LONG64 whole;
        struct
        {
            LONG Flag;
            LONG Count;
        }parts;
    } U;

    struct alignas(64) SESSION
    {
        alignas(64) U ioState;
        U sendState;
        LONG stopIO;
        LONG sendCnt;
        bool isActive;
        OVERLAPPED recvOverlapped;
        OVERLAPPED sendOverlapped;
        PacketBufferReader* curReaderBuf;
        LockFreeQueue<PacketBuffer*> sendQueue;
        //LONG ioCount;
        alignas(64) SOCKET sock;
        DWORD port;
        uint64_t sessionID; // 64비트: 상위 16비트는 vector 인덱스, 하위 48비트는 고유 unique session id
        std::string ip;
   

        SESSION()
            : sock(INVALID_SOCKET),
            sessionID(0), port(0), ip(""), sendCnt(0), isActive(false), stopIO(0)
        {
            ZeroMemory(&recvOverlapped, sizeof(OVERLAPPED));
            ZeroMemory(&sendOverlapped, sizeof(OVERLAPPED));
            sendState.whole = 0;
           // ioState.whole = 0;
           // ioState.parts.Flag = 1;
            //시작시 ioFlag 켜져있고
            ioState.parts.Flag = 1;
            ioState.parts.Count = 0;
            curReaderBuf = PacketBufferReader::Alloc();
            // printf("session 생성자호출\n");
        }
        ~SESSION()
        {
            //printf("session 소멸자호출\n");
        }

        void Init()
        {
            //활성화시 ioFlag 0으로 세팅
            isActive = true;
            InterlockedIncrement(&ioState.parts.Count);
            ioState.parts.Flag = 0;
            InterlockedExchange(&stopIO, 0);
        }

        void Clear()
        {
            ZeroMemory(&recvOverlapped, sizeof(OVERLAPPED));
            ZeroMemory(&sendOverlapped, sizeof(OVERLAPPED));
            sock = INVALID_SOCKET;
            //recvQ.ClearBuffer();
            sessionID = 0;
            port = 0;
            ip = "";
            sendCnt = 0;
            isActive = false;
            sendState.whole = 0;
            //ioState.whole = 0;
            //ioState.parts.Flag = 0;
            //printf("clear ioState %d : %d\n", ioState.parts.Count, ioState.parts.Flag);

            //기존 recvBuf 새걸로 교체
            curReaderBuf->Clear();
            //PacketBuffer* newRecvBuf = PacketBuffer::Alloc();
            //curRecvBuf->Release();
            //curRecvBuf = newRecvBuf;
        }
    };
public:
    // Start: ip, port, workerThread 수, 동시 실행 스레드 수, 최대 접속자 수, Nagle 옵션을 인자로 받음
    bool Start(const char* ip, int port, int workerThreadCount, int conCurrentThreadCount, int maxConnections, int maxPlayers, bool useNagle);
    void Stop();
    int GetSessionCount();
    bool Disconnect(uint64_t sessionID);
    bool SendPacket(uint64_t sessionID, PacketBuffer* packet);
    bool SendPacket(SESSION* session, PacketBuffer* packet);

    void SendAllPacket();

    void PostSendAll();

    //virtual int GetHeaderSize() const
    //{
    //    return sizeof(stHeader);
    //}

    virtual ~CLanServer();

    inline PacketBuffer* CreatePacketBuffer()
    {
        //int hdr = GetHeaderSize();
        return PacketBuffer::Alloc(sizeof(NetWorkHeader));
    }
    //debuging용
    alignas(64) DWORD disconnectCntProtocol =0;
    DWORD disconnectCntCheckSum =0;
    DWORD disconnectPacketType  =0;
    DWORD disconnectHeartBeat   =0;
    DWORD wrongIP = 0;
    DWORD acceptFail = 0;
    std::vector<std::string> ips;
    std::vector<DWORD> errorCode;

protected:
    CLanServer();

    // 순수가상 함수 (상속 받아 구현)
    virtual bool OnConnectionRequest(const char* ip, int port) = 0;
    virtual bool OnClientJoin(uint64_t sessionID) = 0;      //accept에서 jobqueue
    virtual void OnClientLeave(uint64_t sessionID) = 0;     //release에서 jobqueue
    virtual void OnRecv(uint64_t sessionID, PacketBuffer* packet) = 0;
   // virtual void OnRecv(uint64_t sessionID, PacketBufferReader* packet) = 0;

public:
    int m_maxPlayers;
    DWORD currentPlayerCnt;
private:
    struct WorkerParam
    {
        CLanServer* server;
        int idx;
    };

    // sessionID 생성 및 인덱스 추출 관련 inline 함수
    inline uint64_t MakeSessionID(uint16_t index)
    {
        // vector 인덱스(index)를 상위 16비트에, m_uniqueIdCounter의 하위 48비트 값을 하위 48비트에 결합합니다.
        // m_uniqueIdCounter++로 현재 값을 사용한 후 증가시키며, 0xFFFFFFFFFFFFULL 마스크를 통해 하위 48비트만 사용합니다.
        return (((uint64_t)index << 48) | (m_uniqueIdCounter++ & 0xFFFFFFFFFFFFULL));
    }
    inline uint16_t GetSessionIndex(uint64_t sessionID)
    {
        // sessionID의 상위 16비트가 vector 인덱스이므로, 오른쪽 48비트 시프트 후 16비트로 변환합니다.
        return static_cast<uint16_t>(sessionID >> 48);
    }

    void ReleaseSession(SESSION* session);
    bool PostRecv(SESSION* session);
    bool PostSend(SESSION* session);
    void DisconnectSession(SESSION* session);


    static unsigned __stdcall WorkerThread(LPVOID param);
    static unsigned __stdcall AcceptThread(LPVOID param);
    //static unsigned __stdcall MonitorThread(LPVOID param);

public:

    alignas(8) uint64_t* m_recvTps;  // 1초 동안 recv된 패킷
    alignas(8) uint64_t* m_sendTps;  // 1초 동안 send된 패킷

    alignas(8) uint64_t* m_recvBytes; // 1초 동안 recv된 바이트
    alignas(8) uint64_t* m_sendBytes; // 1초 동안 send된 바이트

    DWORD acceptTPS;
 
public:
    // freeIndices(stack)를 보호하기 위한 크리티컬 섹션 (push/pop 시 사용)
    CRITICAL_SECTION m_stackCS;
    // 사용 가능한 vector 인덱스 관리 (push/pop 시 m_stackCS 사용)
    std::stack<uint16_t> m_freeIndices;
    
   // LockFreeStack<int> m_freeIndices; // Lock-free stack for free indices

    // 고정 크기 vector: 각 인덱스에 SESSION 포인터를 저장 (읽기는 락 없이 수행)
    std::vector<SESSION*> m_sessions;
    //Param 보관용
    std::vector<WorkerParam*> m_workerParams;
    //Tps용

    uint64_t m_uniqueIdCounter;  // 하위 48비트용 고유 session id 카운터
    int m_maxConnections;

    HANDLE m_iocpHandle;
    HANDLE* m_workerThreads;
    int m_workerThreadCount;
    HANDLE m_acceptThread;
    //HANDLE m_monitorThread;
    //HANDLE m_monitorEvent;  // Monitor thread 종료 이벤트

    SOCKET m_listenSock;

    // 시작 IP/port (AcceptThread에서 사용)
    char m_startIP[64];
    int m_startPort;
};

