#pragma once

#define df_PDH_ETHERNET_MAX	8

struct st_Ethernet
{
	bool bUse;
	WCHAR szName[128];

	PDH_HCOUNTER recvBytesValue;
	PDH_HCOUNTER sendBytesValue;
};


class PDHMonitor
{
public:
	PDHMonitor();

	void Update();

	int GetTotalCpu() const { return totalCpuValue.doubleValue; }
	int GetProcessUserMemory() const { return processUserMemoryValue.largeValue / (1024ull * 1024ull); }
	int GetProcessNonpagedMemory() const { return processNonPageMermoryValue.largeValue / (1024ull * 1024ull); }
	int GetUseableMemory() const { return useableMemoryValue.largeValue; }
	int GetNonPagedMemory() const { return nonPagedMemoryValue.largeValue / (1024ull * 1024ull); }
	int GetRecvBytes() const { return netWork_recvBytes / (1024ull); }
	int GetSendBytes() const { return netWork_sendBytes / (1024ull); }


private:
	PDH_HQUERY pdhQuery;
	//PDH_HQUERY totalCpuQuery;
	PDH_HCOUNTER totalCpu;
	PDH_FMT_COUNTERVALUE totalCpuValue{};

	//PDH_HQUERY processUserMemoryQuery;
	PDH_HCOUNTER processUserMemory;
	PDH_FMT_COUNTERVALUE processUserMemoryValue{};

	//PDH_HQUERY processNonPagedMemoryQuery;
	PDH_HCOUNTER processNonPagedMemory;
	PDH_FMT_COUNTERVALUE processNonPageMermoryValue{};

	//PDH_HQUERY useableMemoryQuery;
	PDH_HCOUNTER useableMemory;
	PDH_FMT_COUNTERVALUE useableMemoryValue{};

	//PDH_HQUERY nonPagedMemoryQuery;
	PDH_HCOUNTER nonPagedMemory;
	PDH_FMT_COUNTERVALUE nonPagedMemoryValue{};

	st_Ethernet EthernetStruct[df_PDH_ETHERNET_MAX];
	uint64_t netWork_recvBytes = 0;
	uint64_t netWork_sendBytes = 0;
	PDH_FMT_COUNTERVALUE netWorkValue{};

};

extern PDHMonitor pdhMonitor;
