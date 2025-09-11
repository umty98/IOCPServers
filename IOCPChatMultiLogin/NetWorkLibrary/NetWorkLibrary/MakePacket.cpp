//#include "MakePacket.h"
//#include "Protocol.h"
#include "pch.h"

void mpResultLogin(PacketBuffer* packet, BYTE Status, INT64 AccountNo)
{
	ContentHeader conHeader;
	conHeader.type = en_PACKET_CS_CHAT_RES_LOGIN;

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
	*packet << Status;
	*packet << AccountNo;
}

void mpResultSectorMove(PacketBuffer* packet, INT64 AccountNo, WORD SectorX, WORD SectorY)
{
	ContentHeader conHeader;
	conHeader.type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
	*packet << AccountNo;
	*packet << SectorX;
	*packet << SectorY;
}

void mpResultMessage(PacketBuffer* packet, INT64 AccountNo, WCHAR ID[20], WCHAR Nickname[20], WORD MessageLen, WCHAR* Message)
{
	ContentHeader conHeader;
	conHeader.type = en_PACKET_CS_CHAT_RES_MESSAGE;

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
	*packet << AccountNo;
	packet->EnqueueData((char*)ID, sizeof(WCHAR) * 20);
	packet->EnqueueData((char*)Nickname, sizeof(WCHAR) * 20);
	*packet << MessageLen;
	packet->EnqueueData((char*)Message, MessageLen);
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

