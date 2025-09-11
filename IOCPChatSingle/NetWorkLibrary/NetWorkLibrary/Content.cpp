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
LockFreeQueue<JobMessage*> jobQueue;

//UINT32 nextId = 1;
MyLanServer server;

int frameCnt;

DWORD jobQueueSize;
DWORD successSize;
DWORD jobQueueTPS;
DWORD MoveStartTps;
DWORD MoveStopTps;
DWORD ChatStartTps;
DWORD ChatEndTps;
DWORD heartBeatTps;

void SetCharacter()
{
	characterMap.resize(maxPlayer, nullptr);
	for (int i = 0; i < maxPlayer; i++)
	{
		st_Character* character = new st_Character();
		characterMap[i] = character;
	}
}

void Update()
{
	//jobQueue처리
	while (1)
	{
		JobMessage* jobMessage;
		int ret = jobQueue.Dequeue(jobMessage);
		if (ret != 0)
			break;

		if (!jobProcess(jobMessage->sessionID, jobMessage->jobType, jobMessage->packetBuffer))
			server.Disconnect(jobMessage->sessionID);
		if (jobMessage->packetBuffer != nullptr)
			jobMessage->packetBuffer->Release();
		//jobMessage->packetBuffer->Release();
		jobMessagePool.free(jobMessage);
		//delete jobMessage;

		jobQueueTPS++;
		//InterlockedDecrement(&jobQueueSize);
	}


	int plusLogic = ((timeGetTime() - startTime) / 40) + 1;
	startTime += 40 * plusLogic;

	frameCnt++;
	for (int i = 0; i < plusLogic; i++)
	{
		for (auto iter = characterMap.begin(); iter != characterMap.end();)
		{
			st_Character* pCharacter = *iter;
			if (!pCharacter->isActive)
			{
				++iter;
				continue;
			}

			//if (timeGetTime() - pCharacter->dwLastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT)
			//{
			//	//여기서 정리해야됨
			//	//DebugBreak();
			//	server.Disconnect(pCharacter->sessionID);
			//	//InterlockedIncrement(&server.disconnectHeartBeat);
			//	++iter;
			//	continue;
			//}

			if (pCharacter->isMoving)
			{
				switch (pCharacter->dwAction)
				{
				case dfPACKET_MOVE_DIR_LL:
					if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X, pCharacter->shY))
					{
						pCharacter->shX -= dfSPEED_PLAYER_X;
					}
					break;
				case dfPACKET_MOVE_DIR_LU:
					if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X, pCharacter->shY - dfSPEED_PLAYER_Y))
					{
						pCharacter->shX -= dfSPEED_PLAYER_X;
						pCharacter->shY -= dfSPEED_PLAYER_Y;
					}
					break;
				case dfPACKET_MOVE_DIR_UU:
					if (CharacterMoveCheck(pCharacter->shX, pCharacter->shY - dfSPEED_PLAYER_Y))
						pCharacter->shY -= dfSPEED_PLAYER_Y;
					break;
				case dfPACKET_MOVE_DIR_RU:
					if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X, pCharacter->shY - dfSPEED_PLAYER_Y))
					{
						pCharacter->shX += dfSPEED_PLAYER_X;
						pCharacter->shY -= dfSPEED_PLAYER_Y;
					}
					break;
				case dfPACKET_MOVE_DIR_RR:
					if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X, pCharacter->shY))
						pCharacter->shX += dfSPEED_PLAYER_X;
					break;
				case dfPACKET_MOVE_DIR_RD:
					if (CharacterMoveCheck(pCharacter->shX + dfSPEED_PLAYER_X, pCharacter->shY + dfSPEED_PLAYER_Y))
					{
						pCharacter->shX += dfSPEED_PLAYER_X;
						pCharacter->shY += dfSPEED_PLAYER_Y;
					}
					break;
				case dfPACKET_MOVE_DIR_DD:
					if (CharacterMoveCheck(pCharacter->shX, pCharacter->shY + dfSPEED_PLAYER_Y))
						pCharacter->shY += dfSPEED_PLAYER_Y;
					break;
				case dfPACKET_MOVE_DIR_LD:
					if (CharacterMoveCheck(pCharacter->shX - dfSPEED_PLAYER_X, pCharacter->shY + dfSPEED_PLAYER_Y))
					{
						pCharacter->shX -= dfSPEED_PLAYER_X;
						pCharacter->shY += dfSPEED_PLAYER_Y;
					}
					break;
				}
				Sector_UpdateCharacter(pCharacter);
			}
			++iter;
		}
	}

	//if (InterlockedExchange(&sendFrameFlag, 1) == 0)
	//{
	//	server.SendAllPacket();
	//	sendCnt++;
	//}

	DWORD elapsed = startTime - timeGetTime();
	if (elapsed <= 40)
	{
		Sleep(elapsed);
	}

}

bool CheckXY(int x, int y)
{
	if (x < dfRANGE_MOVE_LEFT || x >= dfRANGE_MOVE_RIGHT || y < dfRANGE_MOVE_TOP || y >= dfRANGE_MOVE_BOTTOM)
		return false;

	return true;
}

bool jobProcess(uint64_t sessionID, BYTE type, PacketBuffer* packet)
{
	if (type == clientServer)
	{
		ContentHeader conHeader;
		packet->DequeueData((char*)&conHeader, sizeof(ContentHeader));

		switch (conHeader.type)
		{
		case stPacket_Client_Chat_MoveStart:
			return netPacketProc_MoveStart(sessionID, packet);
			break;
		case stPacket_Client_Chat_MoveStop:
			return netPacketProc_MoveStop(sessionID, packet);
			break;
		case stPacket_Client_Chat_LocalChat:
			return netPacketProc_LocalChat(sessionID, packet);
			break;
		case stPacket_Client_Chat_ChatEnd:
			return netPacketProc_ChatEnd(sessionID, packet);
			break;
		case stPacket_Client_Chat_HeartBeat:
			return netPacketProc_HeartBeat(sessionID, packet);
			break;
		default:
			return false;
			break;
		}
	}
	else if (type == DeleteCharacter)
	{
		return netPacketProc_DeleteCharacter(sessionID, packet);
	}
	else if (type == CreateCharacter)
	{
		return netPacketProc_CreateCharacter(sessionID, packet);
	}
	else
	{
		return false;
	}
	
	return true;
}

bool CharacterMoveCheck(short x, short y)
{
	if (x < dfRANGE_MOVE_LEFT || x >= dfRANGE_MOVE_RIGHT || y < dfRANGE_MOVE_TOP || y >= dfRANGE_MOVE_BOTTOM)
		return false;

	return true;
}

bool Sector_UpdateCharacter(st_Character* pCharacter)
{
	int newSectorX = pCharacter->shX / dfSECTOR_SIZE_X;
	int newSectorY = pCharacter->shY / dfSECTOR_SIZE_Y;

	if (newSectorX == pCharacter->CurSector.iX && newSectorY == pCharacter->CurSector.iY)
	{
		return false;
	}

	RemoveSector(pCharacter->CurSector, pCharacter);

	pCharacter->OldSector = pCharacter->CurSector;
	pCharacter->CurSector.iX = newSectorX;
	pCharacter->CurSector.iY = newSectorY;

	AddSector(pCharacter->CurSector, pCharacter);

	return true;
}

bool netPacketProc_CreateCharacter(uint64_t sessionID, PacketBuffer* packet)
{
	st_Character* newCharacter = characterMap[GetCharacterIndex(sessionID)];

	newCharacter->sessionID = sessionID;
	newCharacter->byDirection = (rand() % 2 == 0) ? dfPACKET_MOVE_DIR_LL : dfPACKET_MOVE_DIR_RR;
	newCharacter->dwAction = newCharacter->byDirection;
	newCharacter->isMoving = false;
	newCharacter->dwLastRecvTime = timeGetTime();
	//newCharacter->shX = 3500;
	//newCharacter->shY = 3500;
	newCharacter->shX = rand() % 6390 + 1;
	newCharacter->shY = rand() % 6390 + 1;
	newCharacter->CurSector.iX = newCharacter->shX / dfSECTOR_SIZE_X;
	newCharacter->CurSector.iY = newCharacter->shY / dfSECTOR_SIZE_Y;
	newCharacter->OldSector.iX = newCharacter->shX / dfSECTOR_SIZE_X;
	newCharacter->OldSector.iY = newCharacter->shY / dfSECTOR_SIZE_Y;
	newCharacter->isActive = true;
	newCharacter->chHP = 100;

	AddSector(newCharacter->CurSector, newCharacter);

	PacketBuffer* sendPacket = server.CreatePacketBuffer();
	
	mpCreateCharacterTest(sendPacket, GetCharacterID(sessionID), newCharacter->shX, newCharacter->shY);
	
	//if (server.SendPacket(sessionID, sendPacket))
	//      return true;

	SendPacket_Unicast(sessionID, sendPacket);
	sendPacket->Release();
	return true;
}

bool netPacketProc_MoveStart(uint64_t sessionID, PacketBuffer* packet)
{
	MoveStartTps++;

	if (packet->GetDataSize() != 9)
		return false;

	BYTE byDirection;
	int shX, shY;

	*packet >> byDirection;
	*packet >> shX;
	*packet >> shY;

	if (!CheckXY(shX, shY))
		return false;

	st_Character* pCharacter = characterMap[GetCharacterIndex(sessionID)];
	pCharacter->dwLastRecvTime = timeGetTime();

	pCharacter->dwAction = byDirection;
	switch (byDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	default:
		break;
	}
	pCharacter->isMoving = true;
	//session->dwLastRecvTime = timeGetTime();
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	Sector_UpdateCharacter(pCharacter);
	return true;
}

bool netPacketProc_MoveStop(uint64_t sessionID, PacketBuffer* packet)
{
	MoveStopTps++;

	if (packet->GetDataSize() != 9)
		return false;

	BYTE byDirection;
	int shX, shY;

	*packet >> byDirection;
	*packet >> shX;
	*packet >> shY;

	if (!CheckXY(shX, shY))
		return false;

	st_Character* pCharacter = characterMap[GetCharacterIndex(sessionID)];
	pCharacter->dwLastRecvTime = timeGetTime();
	pCharacter->byDirection = byDirection;
	pCharacter->isMoving = false;
	//session->dwLastRecvTime = timeGetTime();

	pCharacter->shX = shX;
	pCharacter->shY = shY;

	Sector_UpdateCharacter(pCharacter);

	PacketBuffer* sendPacket = server.CreatePacketBuffer();
	
	mpMoveStopOk(sendPacket);

	SendPacket_Unicast(sessionID, sendPacket);
	sendPacket->Release();
	return true;
}

bool netPacketProc_LocalChat(uint64_t sessionID, PacketBuffer* packet)
{
	ChatStartTps++;

	if (packet->GetDataSize() < 1)
		return false;

	BYTE chatLen;
	*packet >> chatLen;

	if (chatLen > 150 || chatLen <= 0)
	{
		return false;
	}

	if (packet->GetDataSize() != chatLen)
		return false;

	std::string chatMessage;
	chatMessage.resize(chatLen);

	packet->DequeueData(&chatMessage[0], chatLen);

	st_Character* pCharacter = characterMap[GetCharacterIndex(sessionID)];
	pCharacter->dwLastRecvTime = timeGetTime();

	PacketBuffer* sendPacket = server.CreatePacketBuffer();
	
	mpLocalChat(sendPacket, GetCharacterID(sessionID), chatLen, chatMessage.c_str());
	SendPacket_Around(sessionID, sendPacket);

	sendPacket->Release();
	return true;
}

bool netPacketProc_ChatEnd(uint64_t sessionID, PacketBuffer* packet)
{
	ChatEndTps++;

	if (packet->GetDataSize() != 0)
		return false;

	PacketBuffer* sendPacket = server.CreatePacketBuffer();
	
	mpChatComplete(sendPacket);

	/*if (server.SendPacket(sessionID, sendPacket))
		return true;*/

	st_Character* pCharacter = characterMap[GetCharacterIndex(sessionID)];
	pCharacter->dwLastRecvTime = timeGetTime();

	SendPacket_Unicast(sessionID, sendPacket);
	sendPacket->Release();
	return true;
}

bool netPacketProc_HeartBeat(uint64_t sessionID, PacketBuffer* packet)
{
	heartBeatTps++;

	if (packet->GetDataSize() != 0)
		return false;

	st_Character* pCharacter = characterMap[GetCharacterIndex(sessionID)];

	pCharacter->dwLastRecvTime = timeGetTime();

	return true;
}

bool netPacketProc_DeleteCharacter(uint64_t sessionID, PacketBuffer* packet)
{
	st_Character* deleteCharacter = characterMap[GetCharacterIndex(sessionID)];

	deleteCharacter->isActive = false;

	RemoveSector(deleteCharacter->CurSector, deleteCharacter);

	return true;
}

void SendPacket_Unicast(uint64_t sessionID, PacketBuffer* packet)
{
	server.SendPacket(sessionID, packet);
}

void SendPacket_SectorOne(int iSectorX, int iSectorY, PacketBuffer* packet, uint64_t exceptSessionID)
{
	for (auto* character : g_Sector[iSectorY][iSectorX])
	{
		if (character->sessionID != exceptSessionID)
		{
			SendPacket_Unicast(character->sessionID, packet);
		}
	}
}

void SendPacket_Around(uint64_t sessionID, PacketBuffer* packet, bool bSendMe)
{
	st_Character* character = characterMap[GetCharacterIndex(sessionID)];
	st_SECTOR_AROUND sectorsAround;
	GetSectorAround(character->CurSector.iX, character->CurSector.iY, &sectorsAround);

	for (int i = 0; i < sectorsAround.iCount; i++)
	{
		SendPacket_SectorOne(sectorsAround.Around[i].iX, sectorsAround.Around[i].iY, packet, bSendMe ? NULL : sessionID);
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




