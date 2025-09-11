#pragma once

extern DWORD jobQueueTPS;
extern DWORD maxPlayer;
//static TLSMemoryPool<JobMessage> jobMessagePool;

extern DWORD packetTPS;
extern DWORD sameIDDisconnectCnt;
extern DWORD loginTPS;

//Attack
extern DWORD LoginTwiceCnt;
extern DWORD LPLongCnt;
extern DWORD LPShortCnt;




struct st_Character
{
	uint64_t sessionID;

    DWORD dwLastRecvTime;

    INT64 AccountNo;
    WCHAR ID[20];
    WCHAR Nickname[20];
    char  SessionKey[64]; // 인증 토큰

    bool LoginReceived;
    bool isActive;

    st_Character()
        : sessionID(0), dwLastRecvTime(0), AccountNo(0), LoginReceived(false), isActive(false)
    {
		memset(ID, 0, sizeof(ID));
		memset(Nickname, 0, sizeof(Nickname));
        memset(SessionKey, 0, sizeof(SessionKey));
	}
};

extern std::vector<st_Character*> characterMap;
extern DWORD startTime;

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

//SendPacket
void SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet);

void CheckWrongConnection();

int GetCharacterCnt();


