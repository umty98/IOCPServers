#include "pch.h"

std::vector<st_LanClient*> lanClientMap;

MyLanServer lanServer;

bool MyLanServer::OnClientJoin(uint64_t sessionID)
{
	st_LanClient* newClient = lanClientMap[GetCharacterIndex(sessionID)];
	newClient->isActive = true;
	newClient->sessionID = sessionID;

	if (InterlockedIncrement(&currentPlayerCnt) > m_maxPlayers)
	{
		return false;
	}

	return true;
}

void MyLanServer::OnClientLeave(uint64_t sessionID)
{
	InterlockedDecrement(&currentPlayerCnt);

	st_LanClient* deleteClient = lanClientMap[GetCharacterIndex(sessionID)];

	deleteClient->isActive = false;
	deleteClient->sessionID = 0;
	if (deleteClient->ServerNo == ChatServerNum)
	{
		MonitoringData* data = MonitoringDataMap[dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN];
		data->data = 0;
	}
	else if (deleteClient->ServerNo == GameServerNum)
	{
		MonitoringData* data = MonitoringDataMap[dfMONITOR_DATA_TYPE_GAME_SERVER_RUN];
		data->data = 0;
	}
	else if(deleteClient->ServerNo == LoginServerNum)
	{
		MonitoringData* data = MonitoringDataMap[dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN];
		data->data = 0;
	}

	deleteClient->ServerNo = -1;
}

void MyLanServer::OnRecv(uint64_t sessionID, PacketBuffer* packet)
{
	ContentHeader conHeader;
	packet->DequeueData((char*)&conHeader, sizeof(ContentHeader));

	switch (conHeader.type)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		Lan_netPacketProc_Login(sessionID, packet);
		break;
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		Lan_netPacketProc_UpdataData(sessionID, packet);
		break;
	default:
		lanServer.Disconnect(sessionID);
		break;
	}
}

void SetLanCharacter()
{
	lanClientMap.resize(maxLanPlayer, nullptr);
	for (int i = 0; i < maxLanPlayer; i++)
	{
		st_LanClient* client = new st_LanClient();
		lanClientMap[i] = client;
	}
}

void Lan_netPacketProc_Login(uint64_t sessionID, PacketBuffer* packet)
{
	if (packet->GetDataSize() != 4)
	{
		lanServer.Disconnect(sessionID);
		return;
	}

	int ServerNumber;

	st_LanClient* client = lanClientMap[GetCharacterIndex(sessionID)];

	*packet >> ServerNumber;

	//if (ServerNumber == 2)
	//{
	//	MonitoringData* data = MonitoringDataMap[dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN];
	//	data->data = 1;
	//}

	client->ServerNo = ServerNumber;
}

void Lan_netPacketProc_UpdataData(uint64_t sessionID, PacketBuffer* packet)
{
	if (packet->GetDataSize() != 9)
	{
		lanServer.Disconnect(sessionID);
		return;
	}

	BYTE DataType;
	int DataValue;
	int TimeStamp;

	*packet >> DataType >> DataValue >> TimeStamp;

	MonitoringData* data = MonitoringDataMap[DataType];
	data->data = DataValue;
	data->timeStamp = TimeStamp;

	MonitoringStatsMap[DataType]->Add(DataValue);
}

void Lan_SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet)
{
	lanServer.SendPacket(sessionID, packet);
}

