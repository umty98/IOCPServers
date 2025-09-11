#pragma once

extern DWORD maxLanPlayer;

struct st_LanClient
{
    uint64_t sessionID;

    bool isActive;
    int ServerNo;
    st_LanClient()
        :sessionID(0), isActive(false), ServerNo(-1)
    {

    }
};

extern std::vector<st_LanClient*> lanClientMap;

void SetLanCharacter();

class MyLanServer : public CLanServer
{
public:
	MyLanServer() {}
	~MyLanServer() {}

public:
    virtual bool OnConnectionRequest(const char* ip, int port) override
    {
        if (strcmp(ip, "127.0.0.1") != 0)
        {
            return false;
        }

        return true;
    }

    virtual bool OnClientJoin(uint64_t sessionID) override;


    virtual void OnClientLeave(uint64_t sessionID) override;


    virtual void OnRecv(uint64_t sessionID, PacketBuffer* packet) override;
};

extern MyLanServer lanServer;

void Lan_netPacketProc_Login(uint64_t sessionID, PacketBuffer* packet);

void Lan_netPacketProc_UpdataData(uint64_t sessionID, PacketBuffer* packet);

//SendPacket
void Lan_SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet);

