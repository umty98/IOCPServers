//#include <stdio.h>
//#include <Windows.h>
//#include <string>
//#include <strsafe.h>
//#include <Pdh.h>
//#pragma comment(lib,"Pdh.lib")
//
//#include "PDHMonitor.h"

#include "pch.h"

PDHMonitor pdhMonitor;

PDHMonitor::PDHMonitor()
{
	wchar_t modulePath[MAX_PATH];
	GetModuleFileNameW(NULL, modulePath, MAX_PATH);
	std::wstring exe(modulePath);
	size_t pos = exe.find_last_of(L"\\\\/");
	if (pos != std::wstring::npos) {
		exe = exe.substr(pos + 1);
	}
	size_t dot = exe.rfind(L'.');
	if (dot != std::wstring::npos) {
		exe.erase(dot);
	}

	std::wstring totalCpuPath = L"\\Processor(_Total)\\% Processor Time";
	//std::wstring cpuProcessPath;
	std::wstring processUserMemoryPath = L"\\Process(" + exe + L")\\Private Bytes";
	std::wstring processNonPagedMemoryPath = L"\\Process(" + exe + L")\\Pool Nonpaged Bytes";
	std::wstring useableMemoryPath = L"\\Memory\\Available MBytes";
	std::wstring nonPagedMemoryPath = L"\\Memory\\Pool Nonpaged Bytes";

	PdhOpenQuery(NULL, NULL, &pdhQuery);

	//totalCpu
	//PdhOpenQuery(NULL, NULL, &totalCpuQuery);
	//PdhAddCounter(totalCpuQuery, totalCpuPath.c_str(), NULL, &totalCpu);
	PdhAddCounter(pdhQuery, totalCpuPath.c_str(), NULL, &totalCpu);
	//PdhCollectQueryData(totalCpuQuery);
	//ProcessCpu

	//ProcessUserMemory
	//PdhOpenQuery(NULL, NULL, &processUserMemoryQuery);
	//PdhAddCounter(processUserMemoryQuery, processUserMemoryPath.c_str(), NULL, &processUserMemory);
	PdhAddCounter(pdhQuery, processUserMemoryPath.c_str(), NULL, &processUserMemory);
	//PdhCollectQueryData(processUserMemoryQuery);

	//ProcessNonPagedMemory
	//PdhOpenQuery(NULL, NULL, &processNonPagedMemoryQuery);
	//PdhAddCounter(processNonPagedMemoryQuery, processNonPagedMemoryPath.c_str(), NULL, &processNonPagedMemory);
	PdhAddCounter(pdhQuery, processNonPagedMemoryPath.c_str(), NULL, &processNonPagedMemory);
	//PdhCollectQueryData(processNonPagedMemoryQuery);

	//useableMemory
	//PdhOpenQuery(NULL, NULL, &useableMemoryQuery);
	//PdhAddCounter(useableMemoryQuery, useableMemoryPath.c_str(), NULL, &useableMemory);
	PdhAddCounter(pdhQuery, useableMemoryPath.c_str(), NULL, &useableMemory);
	//PdhCollectQueryData(useableMemoryQuery);

	//nonPagedMemory
	//PdhOpenQuery(NULL, NULL, &nonPagedMemoryQuery);
	//PdhAddCounter(nonPagedMemoryQuery, nonPagedMemoryPath.c_str(), NULL, &nonPagedMemory);
	PdhAddCounter(pdhQuery, nonPagedMemoryPath.c_str(), NULL, &nonPagedMemory);
	//PdhCollectQueryData(nonPagedMemoryQuery);


	//NetWorkPath	//std::wstring NetWorkPath;

	int iCnt = 0;
	bool bErr = false;
	WCHAR* szCur = NULL;
	WCHAR* szCounters = NULL;
	WCHAR* szInterfaces = NULL;
	DWORD dwCounterSize = 0, dwInterfaceSize = 0;
	WCHAR szQuery[1024] = { 0, };

	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);

	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];

	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD,
		0) != ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
		DebugBreak();
	}

	iCnt = 0;
	szCur = szInterfaces;

	for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
	{
		EthernetStruct[iCnt].bUse = true;
		EthernetStruct[iCnt].szName[0] = L'\0';
		wcscpy_s(EthernetStruct[iCnt].szName, szCur);
		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
		PdhAddCounter(pdhQuery, szQuery, NULL, &EthernetStruct[iCnt].recvBytesValue);
		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
		PdhAddCounter(pdhQuery, szQuery, NULL, &EthernetStruct[iCnt].sendBytesValue);
	}

	PdhCollectQueryData(pdhQuery);
}

void PDHMonitor::Update()
{
	PdhCollectQueryData(pdhQuery);
	//totalCpu
	//PdhCollectQueryData(totalCpuQuery);
	PdhGetFormattedCounterValue(totalCpu, PDH_FMT_DOUBLE, NULL, &totalCpuValue);

	//ProcessUserMemory
	//PdhCollectQueryData(processUserMemoryQuery);
	PdhGetFormattedCounterValue(processUserMemory, PDH_FMT_LARGE, NULL, &processUserMemoryValue);

	//ProcessNonPagedMemory
	//PdhCollectQueryData(processNonPagedMemoryQuery);
	PdhGetFormattedCounterValue(processNonPagedMemory, PDH_FMT_LARGE, NULL, &processNonPageMermoryValue);

	//useableMemory
	//PdhCollectQueryData(useableMemoryQuery);
	PdhGetFormattedCounterValue(useableMemory, PDH_FMT_LARGE, NULL, &useableMemoryValue);

	//nonPagedMemory
	//PdhCollectQueryData(nonPagedMemoryQuery);
	PdhGetFormattedCounterValue(nonPagedMemory, PDH_FMT_LARGE, NULL, &nonPagedMemoryValue);

	netWork_recvBytes = 0;
	netWork_sendBytes = 0;

	//NetWork
	for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
	{
		int Status;
		if (EthernetStruct[iCnt].bUse)
		{
			Status = PdhGetFormattedCounterValue(EthernetStruct[iCnt].recvBytesValue,
				PDH_FMT_LARGE, NULL, &netWorkValue);
			if (Status == 0) netWork_recvBytes += netWorkValue.largeValue;

			Status = PdhGetFormattedCounterValue(EthernetStruct[iCnt].sendBytesValue,
				PDH_FMT_LARGE, NULL, &netWorkValue);
			if (Status == 0) netWork_sendBytes += netWorkValue.largeValue;

		}
	}
}
