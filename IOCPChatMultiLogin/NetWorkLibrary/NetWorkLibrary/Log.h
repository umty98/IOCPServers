#pragma once

class LogManager
{
public:
	LogManager();
	~LogManager();

	void inputLog(uint64_t sessionID, const std::string& message);
	void inputLog(uint64_t sessionID, int err, const std::string& message);
	void inputLog(const std::string& message);

	void start();
	void stop();
private:
	static unsigned __stdcall threadFunc(void* param);
	void processLogs();
	std::string logFileName;

	HANDLE hEvent;
	HANDLE hStopEvent;
	HANDLE hThread;
};

extern LogManager g_logManager;