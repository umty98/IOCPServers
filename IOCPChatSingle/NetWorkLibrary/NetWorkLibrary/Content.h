#pragma once

extern DWORD jobQueueTPS;
extern DWORD maxPlayer;
extern LockFreeQueue<JobMessage*> jobQueue;
extern DWORD sendFrameFlag;
extern DWORD sendCnt;
extern DWORD MoveStartTps;
extern DWORD MoveStopTps;
extern DWORD ChatStartTps;
extern DWORD ChatEndTps;
extern DWORD heartBeatTps;


static TLSMemoryPool<JobMessage> jobMessagePool;

struct st_SECTOR_POS
{
    int iX;
    int iY;
};

struct st_Character
{
	uint64_t sessionID;

    BYTE dwAction;
    BYTE byDirection;
    BYTE byMoveDirection;

    int shX;
    int shY;

    st_SECTOR_POS CurSector;
    st_SECTOR_POS OldSector;

    int chatCnt;

    char chHP;
    bool isMoving;

    DWORD dwLastRecvTime;

    bool isActive;

    st_Character()
        : sessionID(0), dwAction(0), byDirection(0),
        byMoveDirection(0), shX(0), shY(0), CurSector({ 0, 0 }),
        OldSector({ 0, 0 }), chHP(100), isMoving(false), chatCnt(0), dwLastRecvTime(0), isActive(false)
    {}
};

void SetCharacter();

extern DWORD startTime;

inline uint16_t GetCharacterIndex(uint64_t sessionID)
{
    // sessionID의 상위 16비트가 vector 인덱스이므로, 오른쪽 48비트 시프트 후 16비트로 변환합니다.
    return static_cast<uint16_t>(sessionID >> 48);
}

static constexpr uint64_t CHARACTER_ID_MASK = (1ULL << 48) - 1;

inline uint64_t GetCharacterID(uint64_t sessionID)
{
    return sessionID & CHARACTER_ID_MASK;
}

class MyLanServer : public CLanServer
{
public:
    MyLanServer() {}
    virtual ~MyLanServer() {}

public:
    virtual bool OnConnectionRequest(const char* ip, int port) override
    {
        return true;
    }

    virtual bool OnClientJoin(uint64_t sessionID) override
    {
        JobMessage* jobMessage = jobMessagePool.allocate();
        jobMessage->sessionID = sessionID;
        jobMessage->jobType = CreateCharacter;
        jobMessage->packetBuffer = nullptr;
                  
        jobQueue.Enqueue(jobMessage);

        if (InterlockedIncrement(&currentPlayerCnt) > m_maxPlayers)
        {
            //InterlockedDecrement(&currentPlayerCnt);
            return false;
        }

        return true;
    }

    virtual void OnClientLeave(uint64_t sessionID) override
    {
        InterlockedDecrement(&currentPlayerCnt);

        //PacketBuffer* deleteBuffer = PacketBuffer::Alloc();
        //ContentHeader conHeader;
        ////conHeader.len = 0;
        //conHeader.type = DeleteCharacter;
        //deleteBuffer->EnqueueData((char*)&conHeader, sizeof(conHeader));

        //JobMessage* jobMessage = new JobMessage;
        JobMessage* jobMessage = jobMessagePool.allocate();
        jobMessage->sessionID = sessionID;
        jobMessage->jobType = DeleteCharacter;
        jobMessage->packetBuffer = nullptr;

        //jobMessage->packetBuffer->AddRef();
        jobQueue.Enqueue(jobMessage);
        //jobMessage->packetBuffer->Release();
    }

    virtual void OnRecv(uint64_t sessionID, PacketBuffer* packet) override
    {
        JobMessage* jobMessage = jobMessagePool.allocate();
        jobMessage->sessionID = sessionID;
        jobMessage->jobType = clientServer;
        jobMessage->packetBuffer = packet;

        jobMessage->packetBuffer->AddRef();
        jobQueue.Enqueue(jobMessage);

    }


    //virtual void OnRecv(uint64_t sessionID, PacketBufferReader* packet) override
    //{
    //  //  netPacketProc_Echo(sessionID, packet);
    //}
};

extern MyLanServer server;

void Update();

bool CheckXY(int x, int y);

bool jobProcess(uint64_t sessionID, BYTE type, PacketBuffer* packet);

bool CharacterMoveCheck(short x, short y);

bool Sector_UpdateCharacter(st_Character* pCharacter);

bool netPacketProc_CreateCharacter(uint64_t sessionID, PacketBuffer* packet);

bool netPacketProc_MoveStart(uint64_t sessionID, PacketBuffer* packet);

bool netPacketProc_MoveStop(uint64_t sessionID, PacketBuffer* packet);

bool netPacketProc_LocalChat(uint64_t sessionID, PacketBuffer* packet);

bool netPacketProc_ChatEnd(uint64_t sessionID, PacketBuffer* packet);

bool netPacketProc_HeartBeat(uint64_t sessionID, PacketBuffer* packet);

bool netPacketProc_DeleteCharacter(uint64_t sessionID, PacketBuffer* packet);

//SendPacket
void SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet);

void SendPacket_SectorOne(int iSectorX, int iSectorY, PacketBuffer* packet, uint64_t exceptSessionID);

void SendPacket_Around(uint64_t sessionID, PacketBuffer* packet, bool bSendMe = false);

int GetCharacterCnt();


