//#include <cstdint> 
//#include <vector>
//#include "CLanServer.h"
//#include "PacketBuffer.h"
//#include "Content.h"
//#include "Protocol.h"
//#include "Sector.h"
//#include "MakePacket.h"
//#include "LockFreeQueue.h"
#include "pch.h"

std::vector<st_WanClient*> wanClientMap;

//UINT32 nextId = 1;
MyWanServer wanServer;

void SetCharacter()
{
	wanClientMap.resize(maxWanPlayer, nullptr);
	for (int i = 0; i < maxWanPlayer; i++)
	{
		st_WanClient* client = new st_WanClient();
		wanClientMap[i] = client;
	}
}

void Wan_netPacketProc_Login(uint64_t sessionID, PacketBuffer* packet)
{
	if (packet->GetDataSize() != 32)
	{
		wanServer.Disconnect(sessionID);
		return;
	}

	char LoginSessionKey[32];

	packet->DequeueData(LoginSessionKey, 32);


	PacketBuffer* sendPacket = wanServer.CreatePacketBuffer();
	if (memcmp(LoginSessionKey, LOGIN_KEY, sizeof(Lan_Protocol_Code)) == 0)
	{
		mpMonitorToolReqLogin(sendPacket, dfMONITOR_TOOL_LOGIN_OK);
	}
	else
	{
		mpMonitorToolReqLogin(sendPacket, dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY);
	}
	Wan_SendPacket_Unicast(sessionID, sendPacket);
	sendPacket->Release();

}

void Wan_SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet)
{
	wanServer.SendPacket(sessionID, packet);
}

void SendUpdataDataAll()
{
	//int time = MonitoringDataMap[ChatServerDataStart]->timeStamp;
	//PacketBuffer* testPacket = wanServer.CreatePacketBuffer();
	//mpMonitorDataUpdate(testPacket, 3, ChatServerDataStart - 1, true, time);
	//for (auto* client : wanClientMap)
	//{
	//	Wan_SendPacket_Unicast(client->sessionID, testPacket);
	//}
	//testPacket->Release();

	//for (int i = ChatServerDataStart; i <= ChatServerDataEnd; i++)
	//{
	//	PacketBuffer* packet = wanServer.CreatePacketBuffer();
	//	MonitoringData* data = MonitoringDataMap[i];
	//	mpMonitorDataUpdate(packet, ChatServerNum, i, data->data, data->timeStamp);
	//	
	//	for (auto* client : wanClientMap)
	//	{
	//		Wan_SendPacket_Unicast(client->sessionID, packet);
	//	}
	//	packet->Release();
	//}

	SetLoginDataAll();
	SetChatDataAll();
	SetGameDataAll();
	SetMonitorDataAll();
}

void SetChatDataAll()
{
	if (MonitoringDataMap[ChatServerDataStart] == 0)
		return;

	for (int i = ChatServerDataStart; i <= ChatServerDataEnd; i++)
	{
		PacketBuffer* packet = wanServer.CreatePacketBuffer();
		MonitoringData* data = MonitoringDataMap[i];
		mpMonitorDataUpdate(packet, ChatServerNum, i, data->data, data->timeStamp);

		for (auto* client : wanClientMap)
		{
			Wan_SendPacket_Unicast(client->sessionID, packet);
		}
		packet->Release();
	}
}

void SetGameDataAll()
{
	if( MonitoringDataMap[GameServerDataStart] == 0)
		return;

	for(int i= GameServerDataStart; i <= GameServerDataEnd; i++)
	{
		PacketBuffer* packet = wanServer.CreatePacketBuffer();
		MonitoringData* data = MonitoringDataMap[i];
		mpMonitorDataUpdate(packet, GameServerNum, i, data->data, data->timeStamp);
		for (auto* client : wanClientMap)
		{
			Wan_SendPacket_Unicast(client->sessionID, packet);
		}
		packet->Release();
	}
}

void SetLoginDataAll()
{
	if (MonitoringDataMap[LoginServerDataStart] == 0)
		return;

	for (int i = LoginServerDataStart; i <= LoginServerDataEnd; i++)
	{
		PacketBuffer* packet = wanServer.CreatePacketBuffer();
		MonitoringData* data = MonitoringDataMap[i];
		mpMonitorDataUpdate(packet, LoginServerNum, i, data->data, data->timeStamp);

		for (auto* client : wanClientMap)
		{
			Wan_SendPacket_Unicast(client->sessionID, packet);
		}
		packet->Release();
	}
}

void SetMonitorDataAll()
{
	for(int i= ServerComDataStart;i<= ServerComDataEnd; i++)
	{
		PacketBuffer* packet = wanServer.CreatePacketBuffer();
		MonitoringData* data = MonitoringDataMap[i];
		mpMonitorDataUpdate(packet, 0, i, data->data, data->timeStamp);
		for (auto* client : wanClientMap)
		{
			Wan_SendPacket_Unicast(client->sessionID, packet);
		}
		packet->Release();
	}
}


bool MyWanServer::OnClientJoin(uint64_t sessionID)
{
	st_WanClient* newClient = wanClientMap[GetCharacterIndex(sessionID)];
	newClient->isActive = true;
	newClient->sessionID = sessionID;

	if (InterlockedIncrement(&currentPlayerCnt) > m_maxPlayers)
	{
		return false;
	}
	
	return true;
}

void MyWanServer::OnClientLeave(uint64_t sessionID)
{
	InterlockedDecrement(&currentPlayerCnt);

	st_WanClient* deleteClient = wanClientMap[GetCharacterIndex(sessionID)];

	deleteClient->isActive = false;
	deleteClient->sessionID = 0;
}

void MyWanServer::OnRecv(uint64_t sessionID, PacketBuffer* packet)
{
	ContentHeader conHeader;
	packet->DequeueData((char*)&conHeader, sizeof(ContentHeader));
	switch (conHeader.type)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		Wan_netPacketProc_Login(sessionID, packet);
		break;
	default:
		wanServer.Disconnect(sessionID);
		InterlockedIncrement(&disconnectPacketType);
		break;
	}
}
