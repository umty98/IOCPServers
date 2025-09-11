#pragma once

// Speed up Windows headers
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX             // prevent min/max macros

// Must include WinSock2 *before* windows.h
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#include <Pdh.h>

#pragma comment(lib,"Pdh.lib")
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

// Standard and STL headers you use everywhere
#include <cstdint>
#include <vector>
#include <stack>
#include <string>
#include <iostream>
#include <conio.h>
#include <stdio.h>
#include <strsafe.h>
#include <unordered_set>
#include <unordered_map>
#include <stdint.h>
#include <mmsystem.h>  
#include <process.h>

#include "CPUUsage.h"
#include "PDHMonitor.h"

// Your library headers
#include "TLSObjectPool.h"
#include "RingBuffer.h"
#include "PacketBuffer.h"
#include "PacketBufferReader.h"
#include "Protocol.h"
#include "LockFreeQueue.h"
#include "CLanServer.h"
#include "CLanClient.h"
#include "Sector.h"
#include "Content.h"
#include "MakePacket.h"

#include "Parser.h"

/*
수정할 사항들
tps측정할때 falsesharing이 발생할수 있음.

session의 경우 멀티스레딩환경에서 접근하는데 session구조체 자체가 falsesharing을 유발할수 있음.
session포인터는 vector로 되어 있는데 그럼 이게 처음에 접근을 포인터로 하니까 falsesharing이 발생하지 않을까?
근데 session포인터의 경우 session자체를 new delete하지 않고 재사용하니까 falsesharing이 발생안할것 같음.
그러고 wsasend하기 전에 vector로 할당하는데 vector 자체는 내부에서 락이 있으니까 결국에는 이 부분에서 동기화가 생길수 있음.
그렇다면 고정배열 array로 최대값의 크기만큼만 할당하고 크기를 정해서 하는 방식으로 하면 어떨까 생각해봄....

커널 사용량 많이 나오는거 kbhit while문을 무한 반복 돌고 있어서 kbhit를 무한 반복할때 커널 사용량 10퍼
time넣어서 1초마다 돌게 하니까 유저모드 사용량 20퍼로 증가함
*/