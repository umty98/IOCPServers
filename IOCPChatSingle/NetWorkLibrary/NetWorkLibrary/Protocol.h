#pragma once
#include <Windows.h>

#define Protocol_Code 0x89
#define Fixed_Code 0xa9
#define Random_Key 0x35

#define Lan_Protocol_Code 0x88
#define Lan_Fixed_Code 0x88

#pragma pack(push, 1)
struct ContentHeader
{
	WORD type;
};
#pragma pack(pop)

#pragma pack(push,1)
struct JobMessage
{
	uint64_t sessionID;
	BYTE jobType;
	PacketBuffer* packetBuffer;
};
#pragma pack(pop)

#define clientServer 0
#define contentWorker 10
#define CreateCharacter 11
#define DeleteCharacter 12


#define stPacket_Chat_Client_CreateCharacter	0
//
//	8	-	CharacterKey
//	4	-	x 좌표
//	4	-	y 좌표
//

#define stPacket_Client_Chat_MoveStart		1
//
//	1	-	Direction
//	4	-	x 좌표
//	4	-	y 좌표
//

#define stPacket_Client_Chat_MoveStop		2
//
//	1	-	Direction
//	4	-	x 좌표
//	4	-	y 좌표
//

#define stPacket_Client_Chat_LocalChat		3
//
//	1	-	ChatMessageLen	
//	ChatMessageLen - ChatMessage
//

#define stPacket_Chat_Client_LocalChat		4
//
//	8	-	CharacterKey
//	1	-	NickNameLen	// 없음
//	NickNameLen - NickName	//없음
//	1	-	ChatMessageLen
//	ChatMessageLen - ChatMessage
//

#define stPacket_Client_Chat_HeartBeat		5
//
//	데이터 없음
//

#define stPacket_Chat_Client_MoveStopOk		6


#define stPacket_Client_Chat_ChatEnd	7

#define stPacket_Chat_Client_ChatComplete	8


//-----------------------------------------------------------------
// 화면 이동 범위.
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	0
#define dfRANGE_MOVE_LEFT	0
#define dfRANGE_MOVE_RIGHT	6400
#define dfRANGE_MOVE_BOTTOM	6400

#define dfNETWORK_PACKET_RECV_TIMEOUT	30000

//-----------------------------------------------------------------
// 캐릭터 이동 속도   // 25fps 기준 이동속도
//-----------------------------------------------------------------
#define dfSPEED_PLAYER_X	6	
#define dfSPEED_PLAYER_Y	4	

//-----------------------------------------------------------------
// 이동 오류체크 범위
//-----------------------------------------------------------------
#define dfERROR_RANGE		50


#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7


#define en_PACKET_SS_MONITOR 20000

#define en_PACKET_SS_MONITOR_LOGIN 20001
//
//		WORD Type
//		int ServerNo
//

#define en_PACKET_SS_MONITOR_DATA_UPDATE 20002
//
//		WORD Type
//
//		BYTE DataType
//		int  DataValue
//		int  TimeStamp
//

#define Chatserver 3

#define dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN  30		// 채팅서버 ChatServer 실행 여부 ON / OFF
#define dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU  31		// 채팅서버 ChatServer CPU 사용률
#define dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM  32		// 채팅서버 ChatServer 메모리 사용 MByte
#define dfMONITOR_DATA_TYPE_CHAT_SESSION  33		// 채팅서버 세션 수 (컨넥션 수)
#define dfMONITOR_DATA_TYPE_CHAT_PLAYER  34		// 채팅서버 인증성공 사용자 수 (실제 접속자)
#define dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS  35		// 채팅서버 UPDATE 스레드 초당 초리 횟수
#define dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL  36		// 채팅서버 패킷풀 사용량
#define dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL  37		// 채팅서버 UPDATE MSG 풀 사용량