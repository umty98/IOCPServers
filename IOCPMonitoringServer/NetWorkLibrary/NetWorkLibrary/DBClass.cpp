//#include <mysql.h>
//#include <errmsg.h>
//#include <stdio.h>
//#include <ctime> 
#include "pch.h"


DataBase::DataBase()
{
	mysql_init(&m_conn);
	m_connection = nullptr;

	m_MonitorLogTableName = MakeMonitorLogTableName();
}

DataBase::~DataBase()
{
	Disconnect();
}

bool DataBase::Connect(const char* host, const char* user, const char* password, const char* db, int port)
{
	m_connection = mysql_real_connect(&m_conn, host, user, password, db, port, NULL, 0);
	if (m_connection == nullptr)
	{
		//std::cerr << "MySQL connection error: " << mysql_error(&m_conn) << std::endl;
		return false;
	}
	return true;
}

void DataBase::Disconnect()
{
	if (m_connection)
	{
		mysql_close(m_connection);
		m_connection = nullptr;
	}
}

bool DataBase::ExecuteQuery(const char* query)
{
	if (mysql_query(m_connection, query) != 0)
	{
		//printf("Query Error: %s\n", mysql_error(&m_conn));
		return false;
	}


	return true;
}

bool DataBase::GetAccountInfo(INT64 accountNo, std::string& outUserID, std::string& outNickname)
{
	char query[256];
	sprintf_s(query, sizeof(query),
		"SELECT userid, usernick FROM v_account WHERE accountno = %lld;", accountNo);

	if (mysql_query(m_connection, query) != 0)
	{
		return false;  // 쿼리 실행 실패
	}

	MYSQL_RES* result = mysql_store_result(m_connection);
	if (result == nullptr)
	{
		return false;  // 결과 가져오기 실패
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	if (row)
	{
		outUserID = row[0] ? row[0] : "";
		outNickname = row[1] ? row[1] : "";
		mysql_free_result(result);
		return true;  // 정상 조회
	}

	mysql_free_result(result);
	return false;  // 데이터 없음
}

bool DataBase::GetAccountInfo(INT64 accountNo, WCHAR outUserID[20], WCHAR outNickname[20])
{
	char query[256];
	sprintf_s(query, sizeof(query),
		"SELECT userid, usernick FROM v_account WHERE accountno = %lld;", accountNo);

	if (mysql_query(m_connection, query) != 0)
		return false;

	MYSQL_RES* result = mysql_store_result(m_connection);
	if (result == nullptr)
		return false;

	MYSQL_ROW row = mysql_fetch_row(result);
	if (row)
	{
		// ANSI → WCHAR 변환
		MultiByteToWideChar(CP_UTF8, 0, row[0], -1, outUserID, 20);
		MultiByteToWideChar(CP_UTF8, 0, row[1], -1, outNickname, 20);

		mysql_free_result(result);
		return true;
	}

	mysql_free_result(result);
	return false;
}

bool DataBase::EnsureMonitorLogTableExists(const std::string& tableName)
{
	char checkQuery[128];
	sprintf_s(checkQuery, sizeof(checkQuery), "SHOW TABLES LIKE '%s';", tableName.c_str());
	if (mysql_query(m_connection, checkQuery) != 0)
		return false;

	MYSQL_RES* result = mysql_store_result(m_connection);
	bool exists = (mysql_num_rows(result) > 0);
	mysql_free_result(result);

	if (!exists)
	{
		char createQuery[256];
		sprintf_s(createQuery, sizeof(createQuery), "CREATE TABLE `%s` LIKE monitorLog_template", tableName.c_str());
		if (mysql_query(m_connection, createQuery) != 0)
		{
			//printf("Create table failed: %s\n", mysql_error(m_connection));
			return false;
		}
	}
	return true;
}

bool DataBase::InsertMonitorLog(INT64 serverNo, int type, float param1, float param2, float param3, const std::string& paramStr)
{
	if (m_MonitorLogTableName.empty())
		return false;

	if (!EnsureMonitorLogTableExists(m_MonitorLogTableName))
		return false;

	char escapedStr[256];
	mysql_real_escape_string(m_connection, escapedStr, paramStr.c_str(), (unsigned long)paramStr.length());

	char insertQuery[1024];
	sprintf_s(insertQuery, sizeof(insertQuery),
		"INSERT INTO `%s` (logtime, serverno, type, param1, param2, param3, paramstr) "
		"VALUES (NOW(), %lld, %d, %f, %f, %f, '%s')",
		m_MonitorLogTableName.c_str(), serverNo, type, param1, param2, param3, escapedStr);

	if (mysql_query(m_connection, insertQuery) != 0)
	{
		//printf("Insert failed: %s\n", mysql_error(m_connection));
		return false;
	}

	return true;
}














