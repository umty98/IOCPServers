#pragma once

//extern thread_local cpp_redis::client g_redisClient;
//extern thread_local bool g_redisInited;

cpp_redis::client& GetRedisClient();
bool& GetRedisInited();

extern DWORD totalRedisConnect;