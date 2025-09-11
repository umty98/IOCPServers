#include "pch.h"

std::unordered_map<int, MonitoringData*> MonitoringDataMap;

std::unordered_map<int, MonitoringDataStats*> MonitoringStatsMap;

std::unordered_map<int, std::string> MonitoringNameMap;

void MakeAllDataSet()
{
    MonitoringDataMap.reserve(50);
    MonitoringStatsMap.reserve(50);

   /* constexpr int keys[] = {
     dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN,
     dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU,
     dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM,
     dfMONITOR_DATA_TYPE_CHAT_SESSION,
     dfMONITOR_DATA_TYPE_CHAT_PLAYER,
     dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS,
     dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,
     dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL
    };*/

    constexpr int keys[] = {
        // 로그인 서버
        dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN,
        dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU,
        dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM,
        dfMONITOR_DATA_TYPE_LOGIN_SESSION,
        dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS,
        dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL,

        // 게임 서버
        dfMONITOR_DATA_TYPE_GAME_SERVER_RUN,
        dfMONITOR_DATA_TYPE_GAME_SERVER_CPU,
        dfMONITOR_DATA_TYPE_GAME_SERVER_MEM,
        dfMONITOR_DATA_TYPE_GAME_SESSION,
        dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER,
        dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER,
        dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS,
        dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS,
        dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS,
        dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS,
        dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG,
        dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS,
        dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS,
        dfMONITOR_DATA_TYPE_GAME_PACKET_POOL,

        // 채팅 서버
        dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN,
        dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU,
        dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM,
        dfMONITOR_DATA_TYPE_CHAT_SESSION,
        dfMONITOR_DATA_TYPE_CHAT_PLAYER,
        dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS,
        dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,
        dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL,

        // 모니터링 서버 전체
        dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL,
        dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY,
        dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV,
        dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND,
        dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY
    };

    for (int key : keys)
    {
        MonitoringDataMap.emplace(key, new MonitoringData());
        MonitoringStatsMap.emplace(key, new MonitoringDataStats());
    }
    //login
    MonitoringNameMap[dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU] = "loginCPU";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM] = "loginMEM"; 
	MonitoringNameMap[dfMONITOR_DATA_TYPE_LOGIN_SESSION] = "loginSession";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS] = "loginAuthTPS";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL] = "loginPacketPool";
    //game
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_SERVER_CPU] = "gameCPU";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_SERVER_MEM] = "gameMEM";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_SESSION] = "gameSession";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER] = "gameAuthPlayer";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER] = "gameGamePlayer";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS] = "gameAcceptTPS";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS] = "gameRecvTPS";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS] = "gameSendTPS";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS] = "gameDBWriteTPS";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG] = "gameDBWriteMsg";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS] = "gameAuthThreadFPS";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS] = "gameGameThreadFPS";
    MonitoringNameMap[dfMONITOR_DATA_TYPE_GAME_PACKET_POOL] = "gamePacketPool";
	//chat
	MonitoringNameMap[dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU] = "chatCPU";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM] = "chatMEM";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_CHAT_SESSION] = "chatSession";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_CHAT_PLAYER] = "chatPlayer";  
	MonitoringNameMap[dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS] = "chatUpdateTPS";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL] = "chatPacketPool";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL] = "chatUpdateMsgPool";
    //total
	MonitoringNameMap[dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL] = "totalCPU";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY] = "totalNonPagedMemory";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV] = "totalNetworkRecv";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND] = "totalNetworkSend";
	MonitoringNameMap[dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY] = "totalAvailableMemory";
}
