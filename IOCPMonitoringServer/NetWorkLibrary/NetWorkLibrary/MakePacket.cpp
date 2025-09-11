//#include "MakePacket.h"
//#include "Protocol.h"
#include "pch.h"

void mpMonitorToolReqLogin(PacketBuffer* packet, BYTE Status)
{
	ContentHeader conHeader;
	conHeader.type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
	*packet << Status;
}

void mpMonitorDataUpdate(PacketBuffer* packet, BYTE ServerNo, BYTE DataType, int DataValue, int timeStamp)
{
	ContentHeader conHeader;
	conHeader.type = en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
	*packet << ServerNo << DataType << DataValue << timeStamp;
}
