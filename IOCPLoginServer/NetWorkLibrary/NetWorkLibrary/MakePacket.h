#pragma once

#include <Windows.h>
#include "PacketBuffer.h"


void mpResultLogin(PacketBuffer* packet, INT64 AccountNo, BYTE Status, WCHAR ID[20], WCHAR Nickname[20], WCHAR GameServerIP[16],
	USHORT GameServerPort, WCHAR ChatServerIP[16], USHORT ChatServerPort);

///////////

void mpMonitorLogin(PacketBuffer* packet, int ServerNo);

void mpMonitorDataUpdate(PacketBuffer* packet, BYTE DataType, int DataValue, int TimeStamp);