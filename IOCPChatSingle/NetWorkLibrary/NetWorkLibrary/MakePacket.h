#pragma once

#include <Windows.h>
#include "PacketBuffer.h"

void mpCreateCharacterTest(PacketBuffer* packet, unsigned long long id, int shX, int shY);

void mpMoveStopOk(PacketBuffer* packet);

void mpChatComplete(PacketBuffer* packet);

void mpLocalChat(PacketBuffer* packet, unsigned long long id, BYTE len, const char* message);


///////////////


void mpMonitorLogin(PacketBuffer* packet, int ServerNo);

void mpMonitorDataUpdate(PacketBuffer* packet, BYTE DataType, int DataValue, int TimeStamp);