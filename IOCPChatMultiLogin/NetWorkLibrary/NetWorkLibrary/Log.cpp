#include "pch.h"
#include "Log.h"

LogManager g_logManager;

LockFreeQueue<PacketBuffer*> LogJobQueue;

LogManager::LogManager()
{
    time_t t = time(nullptr);
    struct tm tm;
    localtime_s(&tm, &t);
    char buf[64];
    strftime(buf, sizeof(buf), "log_%Y%m%d_%H%M%S.txt", &tm);
    logFileName = buf;

    hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    hStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    unsigned threadID;
    hThread = (HANDLE)_beginthreadex(nullptr, 0, threadFunc, this, 0, &threadID);

}

LogManager::~LogManager()
{
    CloseHandle(hEvent);
}

void LogManager::inputLog(uint64_t sessionID, const std::string& message)
{
    PacketBuffer* buffer = PacketBuffer::Alloc();
    std::string line = std::to_string(sessionID) + ": " + message + "\n";
    buffer->EnqueueData(line.data(), static_cast<int>(line.size()));

    buffer->AddRef();
    LogJobQueue.Enqueue(buffer);
    SetEvent(hEvent);
    buffer->Release();
}

void LogManager::inputLog(uint64_t sessionID, int err, const std::string& message)
{
    PacketBuffer* buffer = PacketBuffer::Alloc();
    std::string line = std::to_string(sessionID) + ": " + message + ": "+to_string(err)+ "\n";
    buffer->EnqueueData(line.data(), static_cast<int>(line.size()));

    buffer->AddRef();
    LogJobQueue.Enqueue(buffer);
    SetEvent(hEvent);
    buffer->Release();
}

void LogManager::inputLog(const std::string& message)
{
    PacketBuffer* buffer = PacketBuffer::Alloc();
    buffer->EnqueueData(message.data(), static_cast<int>(message.size()));

    buffer->AddRef();
    LogJobQueue.Enqueue(buffer);
    SetEvent(hEvent);
    buffer->Release();
}

void LogManager::start()
{
    time_t t = time(nullptr);
    struct tm tm;
    localtime_s(&tm, &t);

    // 2) 포맷팅
    char timeBuf[64];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);

    // 3) 로그 남기기
    inputLog(std::string("Server started at ") + timeBuf + "\n");
}

void LogManager::stop()
{
    if (hStopEvent)
    {
        SetEvent(hStopEvent);
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = nullptr;
    }
}

unsigned __stdcall LogManager::threadFunc(void* param)
{
    LogManager* mgr = static_cast<LogManager*>(param);
    HANDLE handles[2] = { mgr->hStopEvent, mgr->hEvent };

    while (1)
    {
        DWORD idx = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        if (idx == WAIT_OBJECT_0)
            break;

        mgr->processLogs();
    }

    mgr->processLogs();
    printf("logThread exit\n");
    return 0;
}

void LogManager::processLogs()
{
    PacketBuffer* buffer;
    while (1)
    {
        int ret = LogJobQueue.Dequeue(buffer);
        if (ret == -1)
            break;

        FILE* fp = nullptr;
        errno_t err = fopen_s(&fp, logFileName.c_str(), "a");
        if (err == 0 && fp)
        {
            int len = buffer->GetDataSize();
            fwrite(buffer->GetBufferPtr(), 1, len, fp);
            fclose(fp);
        }
        buffer->Release();
    }
}
