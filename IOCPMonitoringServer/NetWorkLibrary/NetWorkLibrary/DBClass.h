#pragma once

class DataBase
{
public:
	DataBase();
	~DataBase();

	bool Connect(const char* host, const char* user, const char* password, const char* db, int port);
	void Disconnect();

	bool ExecuteQuery(const char* query);

	bool GetAccountInfo(INT64 accountNo, std::string& outUserID, std::string& outNickname);

	bool GetAccountInfo(INT64 accountNo, WCHAR outUserID[20], WCHAR outNickname[20]);

	void StartCheckTime()
	{
		checkTime = timeGetTime();
	}
	void EndCheckTime()
	{
		DWORD checkedTime = timeGetTime() - checkTime;
		//printf("checkedTime : %d\n", checkedTime);
		//이런식으로 로깅
		//if (checkedTime > 1000)
		//{

		//}
	}

	std::string MakeMonitorLogTableName()
	{
		time_t now = time(nullptr);
		struct tm t;
		localtime_s(&t, &now);

		char buf[64];
		sprintf_s(buf, sizeof(buf), "monitorLog_%04d%02d", t.tm_year + 1900, t.tm_mon + 1);
		//m_MonitorLogTableName = std::string(buf);
		return std::string(buf);
	}

	bool EnsureMonitorLogTableExists(const std::string& tableName);


	bool InsertMonitorLog(INT64 serverNo, int type, float param1, float param2, float param3, const std::string& paramStr);

	bool BeginTransaction()
	{
		return ExecuteQuery("START TRANSACTION;");
	}

	bool Commit()
	{
		return ExecuteQuery("COMMIT;");
	}

private:
	MYSQL m_conn;
	MYSQL* m_connection;;
	DWORD checkTime;
	std::string m_MonitorLogTableName;
};