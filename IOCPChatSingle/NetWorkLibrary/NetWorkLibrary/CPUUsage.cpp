//#include <Windows.h>
//#include "CPUUsage.h"
#include "pch.h"

CPUUsageProcessor ProcessorTime;
CPUUsageProcess ProcessTime;

CPUUsageProcessor::CPUUsageProcessor()
    : m_total(0), m_user(0), m_kernel(0)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    m_numProcessors = si.dwNumberOfProcessors;
    m_lastIdle.QuadPart = m_lastKernel.QuadPart = m_lastUser.QuadPart = 0;
    Update();  // 초기화
}

void CPUUsageProcessor::Update()
{
    ULARGE_INTEGER idle, kernel, user;
    if (!GetSystemTimes(
        reinterpret_cast<PFILETIME>(&idle),
        reinterpret_cast<PFILETIME>(&kernel),
        reinterpret_cast<PFILETIME>(&user)))
        return;

    auto kd = kernel.QuadPart - m_lastKernel.QuadPart;
    auto ud = user.QuadPart - m_lastUser.QuadPart;
    auto id = idle.QuadPart - m_lastIdle.QuadPart;
    auto total = kd + ud;

    if (total == 0) 
    {
        m_total = m_user = m_kernel = 0.0f;
    }
    else 
    {
        m_total = float((double)(total - id) / double(total) * 100.0);
        m_user = float((double)ud / double(total) * 100.0);
        m_kernel = float((double)(kd - id) / double(total) * 100.0);
    }

    m_lastIdle = idle;
    m_lastKernel = kernel;
    m_lastUser = user;
}

CPUUsageProcess::CPUUsageProcess(HANDLE hProcess)
    : m_total(0), m_user(0), m_kernel(0)
{
    m_hProcess = (hProcess == INVALID_HANDLE_VALUE)
        ? GetCurrentProcess()
        : hProcess;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    m_numProcessors = si.dwNumberOfProcessors;

    m_lastTime.QuadPart = 0;
    m_lastKernel.QuadPart = 0;
    m_lastUser.QuadPart = 0;

    Update();  // 초기화
}

void CPUUsageProcess::Update()
{
    FILETIME ftNow;
    GetSystemTimeAsFileTime(&ftNow);
    ULARGE_INTEGER now;
    now.LowPart = ftNow.dwLowDateTime;
    now.HighPart = ftNow.dwHighDateTime;

    // 2) 프로세스가 사용한 커널/유저 시간
    FILETIME ftDummy, ftKer, ftUsr;
    if (!GetProcessTimes(m_hProcess,
        &ftDummy, &ftDummy,
        &ftKer, &ftUsr))
        return;

    ULARGE_INTEGER ker, usr;
    ker.LowPart = ftKer.dwLowDateTime;  ker.HighPart = ftKer.dwHighDateTime;
    usr.LowPart = ftUsr.dwLowDateTime;  usr.HighPart = ftUsr.dwHighDateTime;

    // 3) 차이 계산
    auto elapsed = now.QuadPart - m_lastTime.QuadPart;
    auto kd = ker.QuadPart - m_lastKernel.QuadPart;
    auto ud = usr.QuadPart - m_lastUser.QuadPart;
    auto total = kd + ud;

    //double scale = 100.0 / (double(m_numProcessors) * double(elapsed));
    //double scale = (double)(m_numProcessors) / double(elapsed) * 100.0;

    m_total = float((double)total / (double)m_numProcessors / double(elapsed) * 100.0f);
    m_kernel = float((double)kd / (double)m_numProcessors / double(elapsed) * 100.0f);
    m_user = float((double)ud / (double)m_numProcessors / double(elapsed) * 100.0f);

    // 4) 다음 갱신을 위해 저장
    m_lastTime = now;
    m_lastKernel = ker;
    m_lastUser = usr;
}
