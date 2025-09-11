//#include "MakePacket.h"
//#include "Protocol.h"
#include "pch.h"

void mpCreateCharacterTest(PacketBuffer* packet, unsigned long long id, int shX, int shY)
{
	ContentHeader conHeader;
	//conHeader.len = 16;
	conHeader.type = stPacket_Chat_Client_CreateCharacter;
	//packet->Clear();

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));

	*packet << id;
	*packet << shX;
	*packet << shY;
}

void mpMoveStopOk(PacketBuffer* packet)
{
	ContentHeader conHeader;
	//conHeader.len = 0;
	conHeader.type = stPacket_Chat_Client_MoveStopOk;
	//packet->Clear();

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));

}

void mpChatComplete(PacketBuffer* packet)
{

	ContentHeader conHeader;
	//conHeader.len = 0;
	conHeader.type = stPacket_Chat_Client_ChatComplete;
	//packet->Clear();

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));
}

void mpLocalChat(PacketBuffer* packet, unsigned long long id, BYTE len, const char* message)
{
	ContentHeader conHeader;
	//conHeader.len = len + 9;
	conHeader.type = stPacket_Chat_Client_LocalChat;
	//packet->Clear();

	packet->EnqueueData((char*)&conHeader, sizeof(conHeader));

	*packet << id;
	*packet << len;
	packet->EnqueueData(message, len);

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




