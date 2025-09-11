//#include "MakePacket.h"
//#include "Protocol.h"
#include "pch.h"



void mpResultLogin(PacketBuffer* packet, INT64 AccountNo, BYTE Status, WCHAR ID[20], WCHAR Nickname[20], WCHAR GameServerIP[16], USHORT GameServerPort, WCHAR ChatServerIP[16], USHORT ChatServerPort)
{
	ContentHeader conHeader;
	conHeader.type = en_PACKET_CS_CHAT_RES_LOGIN;

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
	*packet << AccountNo;
	*packet << Status;

	packet->EnqueueData((char*)ID, sizeof(WCHAR) * 20);
	packet->EnqueueData((char*)Nickname, sizeof(WCHAR) * 20);
	packet->EnqueueData((char*)GameServerIP, sizeof(WCHAR) * 16);
	*packet << GameServerPort;
	packet->EnqueueData((char*)ChatServerIP, sizeof(WCHAR) * 16);
	*packet << ChatServerPort;
}

void mpMonitorLogin(PacketBuffer* packet, int ServerNo)
{
	ContentHeader conHeader;
	conHeader.type = en_PACKET_SS_MONITOR_LOGIN;

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
	*packet << ServerNo;
}

void mpMonitorDataUpdate(PacketBuffer* packet, BYTE DataType, int DataValue, int TimeStamp)
{
	ContentHeader conHeader;
	conHeader.type = en_PACKET_SS_MONITOR_DATA_UPDATE;

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
	*packet << DataType << DataValue << TimeStamp;
}

