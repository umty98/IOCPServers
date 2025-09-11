//#include <cstdint> 
//#include <vector>
//#include "CLanServer.h"
//#include "PacketBuffer.h"
//#include "Content.h"
//#include "Protocol.h"
//#include "Sector.h"
//#include "MakePacket.h"
//#include "LockFreeQueue.h"
#include "pch.h"

std::vector<st_Character*> characterMap;
SRWLOCK g_characterMapLock = SRWLOCK_INIT;

std::unordered_set<std::wstring> g_LoginIDSet;
SRWLOCK g_LoginIDSetLock = SRWLOCK_INIT;

//UINT32 nextId = 1;
MyLanServer server;

DWORD jobQueueSize;
DWORD successSize;
DWORD jobQueueTPS;

DWORD packetTPS;
DWORD loginTPS;

DWORD sameIDDisconnectCnt;
//Attack
DWORD LoginTwiceCnt;
DWORD LPLongCnt;
DWORD LPShortCnt;


void SetCharacter()
{
	characterMap.resize(maxPlayer, nullptr);
	for (int i = 0; i < maxPlayer; i++)
	{
		st_Character* character = new st_Character();
		characterMap[i] = character;
	}
}

void netPacketProc_Login(uint64_t sessionID, PacketBuffer* packet)
{
	InterlockedIncrement(&loginTPS);

	if (packet->GetDataSize() != 72)
	{
		InterlockedIncrement(&LPLongCnt);
		server.Disconnect(sessionID);
		return;
	}

	AcquireSRWLockExclusive(&g_characterMapLock);
	st_Character* character = characterMap[GetCharacterIndex(sessionID)];
	character->dwLastRecvTime = timeGetTime();

	if (!character->LoginReceived)
	{
		character->LoginReceived = true;
	}
	else
	{
		InterlockedIncrement(&LoginTwiceCnt);
		server.Disconnect(sessionID);
		return;
	}

	INT64 accountID;
	*packet >> accountID;

	//char SessionKey[64];
	//packet->DequeueData((char*)&SessionKey, sizeof(char) * 64);
	packet->DequeueData((char*)character->SessionKey, sizeof(char) * 64);
	character->AccountNo = accountID;
	ReleaseSRWLockExclusive(&g_characterMapLock);

	// 모니터링 서버랑도 연결하고 main에서 그러고 

	if (g_dbInited == true)
	{
		if (!g_db.GetAccountInfo(accountID, character->ID, character->Nickname))
		{
			DebugBreak();
			server.Disconnect(sessionID);
			return;
		}
	}

	if (GetRedisInited())
	{
		std::string redisKey = std::to_string(accountID);
		std::string redisValue(character->SessionKey, 64);
		GetRedisClient().set(redisKey, redisValue);
		GetRedisClient().expire(redisKey, 30);
		GetRedisClient().sync_commit();
	}


	WCHAR gameserverIP[16] = L"0.0.0.0";
	//WCHAR chatServerIP[16] = L"10.0.1.1";
	//WCHAR chatServerIP[16] = L"127.0.0.1";
	WCHAR chatServerIP[16] = L"10.0.2.1";

	PacketBuffer* sendPacket = server.CreatePacketBuffer();
	mpResultLogin(sendPacket, character->AccountNo, 1, character->ID, character->Nickname, gameserverIP, 0, chatServerIP, 12001);
	SendPacket_Unicast(sessionID, sendPacket);
	sendPacket->Release();
}

void SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet)
{
	server.SendPacket(sessionID, packet);
}

void CheckWrongConnection()
{
	AcquireSRWLockShared(&g_characterMapLock);
	for (auto& ch : characterMap)
	{
		if (!ch->isActive)
			continue;

		if (timeGetTime() - ch->dwLastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT) // 3초 이상 패킷을 받지 못한 경우
			server.Disconnect(ch->sessionID);
	}
	ReleaseSRWLockShared(&g_characterMapLock);
}

int GetCharacterCnt()
{
	int cnt = 0;
	for (auto iter = characterMap.begin(); iter != characterMap.end();)
	{
		st_Character* pCharacter = *iter;

		if (pCharacter->isActive)
			cnt++;

		iter++;
	}

	return cnt;
}

bool MyLanServer::OnClientJoin(uint64_t sessionID)
{
	AcquireSRWLockExclusive(&g_characterMapLock);

	st_Character* newCharacter = characterMap[GetCharacterIndex(sessionID)];
	newCharacter->dwLastRecvTime = timeGetTime();
	newCharacter->isActive = true;
	newCharacter->sessionID = sessionID;

	if (InterlockedIncrement(&currentPlayerCnt) > m_maxPlayers)
	{
		ReleaseSRWLockExclusive(&g_characterMapLock);
		return false;
	}
	ReleaseSRWLockExclusive(&g_characterMapLock);
	return true;
}

void MyLanServer::OnClientLeave(uint64_t sessionID)
{
	AcquireSRWLockExclusive(&g_characterMapLock);

	InterlockedDecrement(&currentPlayerCnt);

	st_Character* character = characterMap[GetCharacterIndex(sessionID)];

	character->isActive = false;
	character->LoginReceived = false;
	character->sessionID = 0;

	ReleaseSRWLockExclusive(&g_characterMapLock);
}

void MyLanServer::OnRecv(uint64_t sessionID, PacketBuffer* packet)
{
	InterlockedIncrement(&packetTPS);

	if (packet->GetDataSize() < 2)
	{
		server.Disconnect(sessionID);
		InterlockedIncrement(&disconnectPacketType);
		return;
	}

	ContentHeader conHeader;
	packet->DequeueData((char*)&conHeader, sizeof(ContentHeader));

	switch (conHeader.type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		netPacketProc_Login(sessionID, packet);
		break;
	default:
		server.Disconnect(sessionID);
		InterlockedIncrement(&disconnectPacketType);
		break;
	}
}
