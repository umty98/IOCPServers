#pragma once
#include <Windows.h>

#define Wan_Protocol_Code 109
#define Wan_Fixed_Code 30
#define Random_Key 0x35
#define LOGIN_KEY "ajfw@!cv980dSZ[fje#@fdj123948djf"

#define Lan_Protocol_Code 0x88
#define Lan_Fixed_Code 0x88

#pragma pack(push, 1)
struct ContentHeader
{
	WORD type;
};
#pragma pack(pop)

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
//		int DataValue
//		int TimeStamp
//

#define en_PACKET_CS_MONITOR 25000

#define en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN 25001
//
//		WORD Type
// 
//		char LoginSessionKey[32]
//

#define en_PACKET_CS_MONITOR_TOOL_RES_LOGIN 25002
//
//		WROD Type
//
//		BYTE Status
//

#define en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE 25003
//
//		WORD Type
//
//		BYTE ServerNo
//		BYTE DataType
//		int DataValue
//		int TimeStamp
//
//



#define dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN  1		// 로그인서버 실행여부 ON / OFF
#define dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU  2		// 로그인서버 CPU 사용률
#define dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM  3		// 로그인서버 메모리 사용 MByte
#define dfMONITOR_DATA_TYPE_LOGIN_SESSION  4		// 로그인서버 세션 수 (컨넥션 수)
#define dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS  5		// 로그인서버 인증 처리 초당 횟수
#define dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL  6		// 로그인서버 패킷풀 사용량

#define dfMONITOR_DATA_TYPE_GAME_SERVER_RUN  10		// GameServer 실행 여부 ON / OFF
#define dfMONITOR_DATA_TYPE_GAME_SERVER_CPU  11		// GameServer CPU 사용률
#define dfMONITOR_DATA_TYPE_GAME_SERVER_MEM  12		// GameServer 메모리 사용 MByte
#define dfMONITOR_DATA_TYPE_GAME_SESSION  13		// 게임서버 세션 수 (컨넥션 수)
#define dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER  14		// 게임서버 AUTH MODE 플레이어 수
#define dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER  15		// 게임서버 GAME MODE 플레이어 수
#define dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS  16		// 게임서버 Accept 처리 초당 횟수
#define dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS  17		// 게임서버 패킷처리 초당 횟수
#define dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS  18		// 게임서버 패킷 보내기 초당 완료 횟수
#define dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS  19		// 게임서버 DB 저장 메시지 초당 처리 횟수
#define dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG  20		// 게임서버 DB 저장 메시지 큐 개수 (남은 수)
#define dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS  21		// 게임서버 AUTH 스레드 초당 프레임 수 (루프 수)
#define dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS  22		// 게임서버 GAME 스레드 초당 프레임 수 (루프 수)
#define dfMONITOR_DATA_TYPE_GAME_PACKET_POOL  23		// 게임서버 패킷풀 사용량

#define dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN  30		// 채팅서버 ChatServer 실행 여부 ON / OFF
#define dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU  31		// 채팅서버 ChatServer CPU 사용률
#define dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM  32		// 채팅서버 ChatServer 메모리 사용 MByte
#define dfMONITOR_DATA_TYPE_CHAT_SESSION  33		// 채팅서버 세션 수 (컨넥션 수)
#define dfMONITOR_DATA_TYPE_CHAT_PLAYER  34		// 채팅서버 인증성공 사용자 수 (실제 접속자)
#define dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS  35		// 채팅서버 UPDATE 스레드 초당 초리 횟수
#define dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL  36		// 채팅서버 패킷풀 사용량
#define dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL  37		// 채팅서버 UPDATE MSG 풀 사용량

#define dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL  40		// 서버컴퓨터 CPU 전체 사용률
#define dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY  41		// 서버컴퓨터 논페이지 메모리 MByte
#define dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV  42		// 서버컴퓨터 네트워크 수신량 KByte
#define dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND  43 	// 서버컴퓨터 네트워크 송신량 KByte
#define dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY  44		// 서버컴퓨터 사용가능 메모리

#define dfMONITOR_TOOL_LOGIN_OK  1		// 로그인 성공
#define dfMONITOR_TOOL_LOGIN_ERR_NOSERVER  2		// 서버이름 오류 (매칭미스)
#define dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY  3		// 로그인 세션키 오류

#define GameServerDataStart 10
#define GameServerDataEnd 23

#define ChatServerDataStart 30
#define ChatServerDataEnd   37

#define LoginServerDataStart 1
#define LoginServerDataEnd   6

#define ServerComDataStart 40
#define ServerComDataEnd 44


#define ChatServerNum 3
#define GameServerNum 2
#define LoginServerNum 1