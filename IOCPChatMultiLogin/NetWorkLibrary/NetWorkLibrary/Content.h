#pragma once

extern DWORD jobQueueTPS;
extern DWORD maxPlayer;
//static TLSMemoryPool<JobMessage> jobMessagePool;

extern DWORD packetTPS;
extern DWORD loginTPS;
extern DWORD sectorTPS;
extern DWORD chatTPS;
extern DWORD sameIDDisconnectCnt;
extern DWORD deleteIDErrorCnt;
extern DWORD redisCnt;

extern DWORD redisDisconnectCnt;
extern DWORD loginTwiceDisconnectCnt;
extern DWORD loginNotreceiveSectorCnt;
extern DWORD loginNotReceiveChatCnt;

extern DWORD typeErrorCnt;
extern DWORD contentErrorCnt;
extern DWORD contentLenErrorCnt;

struct st_SECTOR_POS
{
    int iX; 
    int iY;
};

struct alignas(64) st_Character
{
	uint64_t sessionID;

    st_SECTOR_POS CurSector;

    DWORD dwLastRecvTime;

    INT64 AccountNo;
    WCHAR ID[20];
    WCHAR Nickname[20];

    bool LoginReceived;
    bool isActive;
    bool sectorMade;

    st_Character()
        : sessionID(0), dwLastRecvTime(0), AccountNo(0), LoginReceived(false), isActive(false), sectorMade(false)
    {
		memset(ID, 0, sizeof(ID));
		memset(Nickname, 0, sizeof(Nickname));
        CurSector.iX = -1;
        CurSector.iY = -1;
	}
};

extern std::vector<st_Character*> characterMap;
//extern DWORD startTime;

void SetCharacter();

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

    virtual bool OnClientJoin(uint64_t sessionID) override;


    virtual void OnClientLeave(uint64_t sessionID) override;


    virtual void OnRecv(uint64_t sessionID, PacketBuffer* packet) override;

};

extern MyLanServer server;

void netPacketProc_Login(uint64_t sessionID, PacketBuffer* packet);

void netPacketProc_SectorMove(uint64_t sessionID, PacketBuffer* packet);

void netPacketProc_Message(uint64_t sessionID, PacketBuffer* packet);

void netPacketProc_Heartbeat(uint64_t sessionID, PacketBuffer* packet);


//SendPacket
void SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet);

void SendPacket_SectorOne(int iSectorX, int iSectorY, PacketBuffer* packet, uint64_t exceptSessionID);

void SendPacket_Around(uint64_t sessionID, PacketBuffer* packet, bool bSendMe = false);

int GetCharacterCnt();


