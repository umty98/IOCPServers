#pragma once

#include <Windows.h>
#include "PacketBuffer.h"

void mpMonitorToolReqLogin(PacketBuffer* packet, BYTE Status);

void mpMonitorDataUpdate(PacketBuffer* packet, BYTE ServerNo, BYTE DataType, int DataValue, int timeStamp);
