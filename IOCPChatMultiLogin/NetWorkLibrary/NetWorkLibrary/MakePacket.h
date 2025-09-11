#pragma once

#include <Windows.h>
#include "PacketBuffer.h"

void mpResultLogin(PacketBuffer* packet, BYTE Status, INT64 AccountNo);

void mpResultSectorMove(PacketBuffer* packet, INT64 AccountNo, WORD SectorX, WORD SectorY);

void mpResultMessage(PacketBuffer* packet, INT64 AccountNo, WCHAR ID[20], WCHAR Nickname[20], WORD MessageLen, WCHAR* Message);


//////////
 
void mpMonitorLogin(PacketBuffer* packet, int ServerNo);

void mpMonitorDataUpdate(PacketBuffer* packet, BYTE DataType, int DataValue, int TimeStamp);