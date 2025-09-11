#pragma once
#include <Windows.h>

#define Protocol_Code 0x77
#define Fixed_Code 0x32
#define Random_Key 0x35

#define Lan_Protocol_Code 0x88
#define Lan_Fixed_Code 0x88

#pragma pack(push, 1)
struct ContentHeader
{
	WORD type;
};
#pragma pack(pop)

#define en_PACKET_CS_CHAT_SERVER 100

#define en_PACKET_CS_CHAT_REQ_LOGIN 101
// 채팅서버 로그인 요청
//	{
//		WORD	Type
//
//		INT64	AccountNo
//		char	SessionKey[64];		// 인증토큰
//	}

#define en_PACKET_CS_CHAT_RES_LOGIN 102
// 채팅서버 로그인 응답
//
//	{
//		WORD	Type
//
//		INT64	AccountNo
// 		BYTE Status;
// 
//		WCHAR ID[20];
//		WCHAR NickName[20];
// 
//		WCHAR	GameServerIP[16]	// 접속대상 게임,채팅 서버 정보
//		USHORT	GameServerPort
//		WCHAR	ChatServerIP[16]
//		USHORT	ChatServerPort
// 
//	}
//


#define dfNETWORK_PACKET_RECV_TIMEOUT	3000


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


#define dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN  1		// 로그인서버 실행여부 ON / OFF
#define dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU  2		// 로그인서버 CPU 사용률
#define dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM  3		// 로그인서버 메모리 사용 MByte
#define dfMONITOR_DATA_TYPE_LOGIN_SESSION  4		// 로그인서버 세션 수 (컨넥션 수)
#define dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS  5		// 로그인서버 인증 처리 초당 횟수
#define dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL  6		// 로그인서버 패킷풀 사용량

#define LoginServerNum 1