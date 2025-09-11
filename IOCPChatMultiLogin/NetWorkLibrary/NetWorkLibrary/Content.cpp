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
//LockFreeQueue<PacketBuffer*> jobQueue;

std::unordered_set<UINT64> g_LoginIDSet;
std::unordered_map<UINT64, st_Character*> g_LoginIDMap;
CRITICAL_SECTION g_LoginIDSetCS;

//UINT32 nextId = 1;
MyLanServer server;

alignas(64) DWORD jobQueueSize;
DWORD successSize;
DWORD jobQueueTPS;
DWORD packetTPS;
DWORD loginTPS;
DWORD sectorTPS;
DWORD chatTPS;
DWORD sameIDDisconnectCnt;
DWORD deleteIDErrorCnt;
DWORD redisCnt;


DWORD redisDisconnectCnt;
DWORD loginTwiceDisconnectCnt;
DWORD loginNotreceiveSectorCnt;
DWORD loginNotReceiveChatCnt;

DWORD typeErrorCnt;
DWORD contentErrorCnt;
DWORD contentLenErrorCnt;



void SetCharacter()
{
	characterMap.resize(maxPlayer, nullptr);
	for (int i = 0; i < maxPlayer; i++)
	{
		st_Character* character = new st_Character();
		characterMap[i] = character;
	}
	InitializeCriticalSection(&g_LoginIDSetCS);
}

void netPacketProc_Login(uint64_t sessionID, PacketBuffer* packet)
{
	InterlockedIncrement(&loginTPS);

	if (packet->GetDataSize() != 152)
	{
		InterlockedIncrement(&contentLenErrorCnt);
		server.Disconnect(sessionID);
		return;
	}


	st_Character* character = characterMap[GetCharacterIndex(sessionID)];
	character->dwLastRecvTime = timeGetTime();

	if (!character->LoginReceived)
	{
		character->LoginReceived = true;
	}
	else
	{
		server.Disconnect(sessionID);
		InterlockedIncrement(&loginTwiceDisconnectCnt);
		return;
	}

	INT64 accountID;
	*packet >> accountID;

	packet->DequeueData((char*)character->ID, sizeof(WCHAR) * 20);
	packet->DequeueData((char*)character->Nickname, sizeof(WCHAR) * 20);
	

	char SessionKey[64];
	packet->DequeueData((char*)&SessionKey, sizeof(char) * 64);

	//중복 ID검사
	EnterCriticalSection(&g_LoginIDSetCS);
	if (g_LoginIDSet.find(accountID) != g_LoginIDSet.end())
	{
		st_Character* debugcharacter = g_LoginIDMap[accountID];
		LeaveCriticalSection(&g_LoginIDSetCS);
		server.Disconnect(debugcharacter->sessionID);
		InterlockedIncrement(&sameIDDisconnectCnt);
		//return;
	}
	else
	{
		g_LoginIDSet.insert(accountID);
		g_LoginIDMap[accountID] = character;
		LeaveCriticalSection(&g_LoginIDSetCS);
	}

	character->AccountNo = accountID;
	std::string sessionKeyStr(SessionKey, 64);
	//레디스 확인
	if (GetRedisInited())
	{
		bool redisOk = false;
		std::string key = std::to_string(accountID);
		GetRedisClient().get(key,
			[&](cpp_redis::reply& reply)
			{
				if (reply.is_string())
				{
					const std::string& val = reply.as_string();
					if (val == sessionKeyStr) redisOk = true;
				}
			});
		GetRedisClient().sync_commit();
		if (!redisOk)
		{
			server.Disconnect(sessionID);
			InterlockedIncrement(&redisDisconnectCnt);
			return;
		}
		GetRedisClient().del({ key });
		GetRedisClient().sync_commit();
		InterlockedIncrement(&redisCnt);
	}

	PacketBuffer* sendPacket = server.CreatePacketBuffer();
	mpResultLogin(sendPacket, 1, character->AccountNo);
	SendPacket_Unicast(sessionID, sendPacket);
	sendPacket->Release();
}

void netPacketProc_SectorMove(uint64_t sessionID, PacketBuffer* packet)
{
	InterlockedIncrement(&sectorTPS);

	if (packet->GetDataSize() != 12)
	{
		InterlockedIncrement(&contentLenErrorCnt);
		server.Disconnect(sessionID);
		return;
	}

	st_Character* character = characterMap[GetCharacterIndex(sessionID)];
	character->dwLastRecvTime = timeGetTime();

	INT64 accountID;
	WORD SectorX, SectorY;

	*packet >> accountID;
	*packet >> SectorX;
	*packet >> SectorY;

	if (accountID != character->AccountNo)
	{
		server.Disconnect(sessionID);
		return;
	}

	if (SectorX < 0 || SectorX >= dfSECTOR_MAX_X || SectorY < 0 || SectorY >= dfSECTOR_MAX_Y)
	{
		InterlockedIncrement(&contentErrorCnt);
		server.Disconnect(sessionID);
		return;
	}

	if (!character->LoginReceived)
	{
		server.Disconnect(sessionID);
		InterlockedIncrement(&loginNotreceiveSectorCnt);
		return;
	}

	if (!character->sectorMade)
	{
		character->sectorMade = true;
		character->CurSector.iX = SectorX;
		character->CurSector.iY = SectorY;

		AddSector(character->CurSector, character);
	}
	else
	{
		RemoveSector(character->CurSector, character);
		character->CurSector.iX = SectorX;
		character->CurSector.iY = SectorY;
		AddSector(character->CurSector, character);
	}

	PacketBuffer* sendPacket = server.CreatePacketBuffer();
	mpResultSectorMove(sendPacket, character->AccountNo, character->CurSector.iX, character->CurSector.iY);
	SendPacket_Unicast(sessionID, sendPacket);
	sendPacket->Release();
}

void netPacketProc_Message(uint64_t sessionID, PacketBuffer* packet)
{
	InterlockedIncrement(&chatTPS);

	st_Character* character = characterMap[GetCharacterIndex(sessionID)];
	character->dwLastRecvTime = timeGetTime();

	if (packet->GetDataSize() < 10)
	{
		InterlockedIncrement(&contentLenErrorCnt);
		server.Disconnect(sessionID);
		return;
	}

	INT64 accountID;
	WORD MessageLen;

	*packet >> accountID;
	*packet >> MessageLen;

	if (accountID != character->AccountNo)
	{
		server.Disconnect(sessionID);
		return;
	}

	if (!character->LoginReceived)
	{
		server.Disconnect(sessionID);
		InterlockedIncrement(&loginNotReceiveChatCnt);
		return;
	}

	DWORD dataSize = packet->GetDataSize();

	if (dataSize > 300 || dataSize <= 0)
	{
		InterlockedIncrement(&contentLenErrorCnt);
		server.Disconnect(sessionID);
		return;
	}

	if (dataSize != MessageLen)
	{
		InterlockedIncrement(&contentLenErrorCnt);
		server.Disconnect(sessionID);
		return;
	}
	WCHAR charMessage[500];
	packet->DequeueData((char*)&charMessage, MessageLen);

	PacketBuffer* sendPacket = server.CreatePacketBuffer();
	mpResultMessage(sendPacket, character->AccountNo, character->ID, character->Nickname, MessageLen, charMessage);
	//SendPacket_Unicast(sessionID, sendPacket);
	SendPacket_Around(sessionID, sendPacket, true);
	sendPacket->Release();
}

void netPacketProc_Heartbeat(uint64_t sessionID, PacketBuffer* packet)
{
	if (packet->GetDataSize() != 0)
	{
		server.Disconnect(sessionID);
		return;
	}
	st_Character* character = characterMap[GetCharacterIndex(sessionID)];

	character->dwLastRecvTime = timeGetTime();
}


void SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet)
{
	server.SendPacket(sessionID, packet);
}

void SendPacket_SectorOne(int iSectorX, int iSectorY, PacketBuffer* packet, uint64_t exceptSessionID)
{
	auto& lock = g_SectorLock[iSectorY][iSectorX];
	AcquireSRWLockShared(&lock);	
	//EnterCriticalSection(&lock);
	for (auto* character : g_Sector[iSectorY][iSectorX])
	{
		if (character->sessionID != exceptSessionID)
		{
			SendPacket_Unicast(character->sessionID, packet);
		}
	}
	//LeaveCriticalSection(&lock);
	ReleaseSRWLockShared(&lock);
}

void SendPacket_Around(uint64_t sessionID, PacketBuffer* packet, bool bSendMe)
{
	st_Character* character = characterMap[GetCharacterIndex(sessionID)];
	st_SECTOR_AROUND sectorsAround;
	GetSectorAround(character->CurSector.iX, character->CurSector.iY, &sectorsAround);

	std::unordered_set<uint64_t> targetSessions;

	for (int i = 0; i < sectorsAround.iCount; i++)
	{
		int x = sectorsAround.Around[i].iX;
		int y = sectorsAround.Around[i].iY;

		auto& lock = g_SectorLock[y][x];
		//EnterCriticalSection(&lock);
		AcquireSRWLockShared(&lock);
		for (auto* ch : g_Sector[y][x])
		{
			targetSessions.insert(ch->sessionID);
		}
		ReleaseSRWLockShared(&lock);
		//LeaveCriticalSection(&lock);
	}

	for (uint64_t targetID : targetSessions)
	{
		SendPacket_Unicast(targetID, packet);
	}
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
	st_Character* newCharacter = characterMap[GetCharacterIndex(sessionID)];
	newCharacter->isActive = true;
	newCharacter->sessionID = sessionID;
	newCharacter->dwLastRecvTime = timeGetTime();

	if (InterlockedIncrement(&currentPlayerCnt) > m_maxPlayers)
	{
		return false;
	}

	return true;
}

void MyLanServer::OnClientLeave(uint64_t sessionID)
{
	InterlockedDecrement(&currentPlayerCnt);

	st_Character* character = characterMap[GetCharacterIndex(sessionID)];

	if (character->LoginReceived)
	{
		EnterCriticalSection(&g_LoginIDSetCS);
		//g_LoginIDSet.erase(character->AccountNo);
		auto it = g_LoginIDSet.find(character->AccountNo);
		if (it != g_LoginIDSet.end())
		{
			g_LoginIDSet.erase(it);

		}
		else
		{
			if (character->AccountNo != 0)
			{
				InterlockedIncrement(&deleteIDErrorCnt);
			}
		}
		g_LoginIDMap.erase(character->AccountNo);
		LeaveCriticalSection(&g_LoginIDSetCS);
	}

	if (character->sectorMade)
	{
		RemoveSector(character->CurSector, character);
		character->sectorMade = false;
	}
	character->isActive = false;
	character->CurSector.iX = -1;
	character->CurSector.iY = -1;
	character->LoginReceived = false;
	character->sessionID = 0;
	character->AccountNo = 0;
}

void MyLanServer::OnRecv(uint64_t sessionID, PacketBuffer* packet)
{
	InterlockedIncrement(&packetTPS);


	if (packet->GetDataSize() < 2)
	{
		server.Disconnect(sessionID);
		InterlockedIncrement(&typeErrorCnt);
		return;
	}

	ContentHeader conHeader;
	packet->DequeueData((char*)&conHeader, sizeof(ContentHeader));

	switch (conHeader.type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		netPacketProc_Login(sessionID, packet);
		break;
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		netPacketProc_SectorMove(sessionID, packet);
		break;
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		netPacketProc_Message(sessionID, packet);
		break;
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		netPacketProc_Heartbeat(sessionID, packet);
		break;
	default:
		server.Disconnect(sessionID);
		InterlockedIncrement(&typeErrorCnt);
		break;
	}
}
