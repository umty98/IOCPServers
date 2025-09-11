#pragma once
#include <Windows.h>

#define Wan_Protocol_Code 0x77
#define Wan_Fixed_Code 0x32

#define Lan_Protocol_Code 0x88
#define Lan_Fixed_Code 0x88
#define Random_Key 0x35

#pragma pack(push, 1)
struct ContentHeader
{
	WORD type;
};
#pragma pack(pop)

#define en_PACKET_CS_CHAT_SERVER 0

#define en_PACKET_CS_CHAT_REQ_LOGIN 1
// 채팅서버 로그인 요청
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WCHAR	ID[20]				// null 포함
//		WCHAR	Nickname[20]		// null 포함
//		char	SessionKey[64];		// 인증토큰
//	}

#define en_PACKET_CS_CHAT_RES_LOGIN 2
// 채팅서버 로그인 응답
//
//	{
//		WORD	Type
//
//		BYTE	Status				// 0:실패	1:성공
//		INT64	AccountNo
//	}
//

#define en_PACKET_CS_CHAT_REQ_SECTOR_MOVE 3
// 채팅서버 섹터 이동 결과
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WORD	SectorX
//		WORD	SectorY
//	}

#define en_PACKET_CS_CHAT_RES_SECTOR_MOVE 4
// 채팅서버 섹터 이동 결과
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WORD	SectorX
//		WORD	SectorY
//	}

#define en_PACKET_CS_CHAT_REQ_MESSAGE 5
// 채팅서버 채팅보내기 요청
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WORD	MessageLen
//		WCHAR	Message[MessageLen / 2]		// null 미포함
//	}
//

#define en_PACKET_CS_CHAT_RES_MESSAGE 6
// 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		WCHAR	ID[20]						// null 포함
//		WCHAR	Nickname[20]				// null 포함
//		
//		WORD	MessageLen
//		WCHAR	Message[MessageLen / 2]		// null 미포함
//	}
//

#define en_PACKET_CS_CHAT_REQ_HEARTBEAT 7
// 하트비트
//
//	{
//		WORD		Type
//	}
//

//-----------------------------------------------------------------
// 화면 이동 범위.
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	0
#define dfRANGE_MOVE_LEFT	0
#define dfRANGE_MOVE_RIGHT	6400
#define dfRANGE_MOVE_BOTTOM	6400

#define dfNETWORK_PACKET_RECV_TIMEOUT	30000


///////////////////////////////////////////////////////////////////////

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


#define dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN  30		// 채팅서버 ChatServer 실행 여부 ON / OFF
#define dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU  31		// 채팅서버 ChatServer CPU 사용률
#define dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM  32		// 채팅서버 ChatServer 메모리 사용 MByte
#define dfMONITOR_DATA_TYPE_CHAT_SESSION  33		// 채팅서버 세션 수 (컨넥션 수)
#define dfMONITOR_DATA_TYPE_CHAT_PLAYER  34		// 채팅서버 인증성공 사용자 수 (실제 접속자)
#define dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS  35		// 채팅서버 UPDATE 스레드 초당 초리 횟수
#define dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL  36		// 채팅서버 패킷풀 사용량
#define dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL  37		// 채팅서버 UPDATE MSG 풀 사용량