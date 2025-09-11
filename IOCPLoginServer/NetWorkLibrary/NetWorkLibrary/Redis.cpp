#include "pch.h"

//Redis

//thread_local cpp_redis::client g_redisClient;
//thread_local bool g_redisInited = false;

cpp_redis::client& GetRedisClient()
{
    thread_local cpp_redis::client redisClient;
    return redisClient;
}

bool& GetRedisInited()
{
    thread_local bool redisInited = false;
    return redisInited;
}


DWORD totalRedisConnect = 0;