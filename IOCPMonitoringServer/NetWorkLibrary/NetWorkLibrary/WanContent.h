#pragma once

extern DWORD maxWanPlayer;
//static TLSMemoryPool<JobMessage> jobMessagePool;

struct st_WanClient
{
	uint64_t sessionID;

    DWORD dwLastRecvTime;

    bool isActive;
    //int ServerNo;
    st_WanClient()
        : sessionID(0), dwLastRecvTime(0), isActive(false)
    {
		
	}
};

extern std::vector<st_WanClient*> wanClientMap;

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

class MyWanServer : public CWanServer
{
public:
    MyWanServer() {}
    virtual ~MyWanServer() {}

public:
    virtual bool OnConnectionRequest(const char* ip, int port) override
    {
        return true;
    }

    virtual bool OnClientJoin(uint64_t sessionID) override;


    virtual void OnClientLeave(uint64_t sessionID) override;


    virtual void OnRecv(uint64_t sessionID, PacketBuffer* packet) override;

};

extern MyWanServer wanServer;

void Wan_netPacketProc_Login(uint64_t sessionID, PacketBuffer* packet);

//SendPacket
void Wan_SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet);

void SendUpdataDataAll();


////
void SetChatDataAll();

void SetGameDataAll();

void SetLoginDataAll();

void SetMonitorDataAll();