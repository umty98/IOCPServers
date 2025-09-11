#pragma once

class CPUUsageProcessor
{
public:
    CPUUsageProcessor();

    void Update();

    int GetTotal()  const { return m_total; }
    int GetUser()   const { return m_user; }
    int GetKernel() const { return m_kernel; }

private:
    int m_numProcessors;
    ULARGE_INTEGER m_lastIdle, m_lastKernel, m_lastUser;
    float m_total, m_user, m_kernel;

};


class CPUUsageProcess
{
public:
    CPUUsageProcess(HANDLE hProcess = INVALID_HANDLE_VALUE);

    void Update();

    int GetTotal()  const { return m_total; }
    int GetUser()   const { return m_user; }
    int GetKernel() const { return m_kernel; }

private:
    HANDLE m_hProcess;
    int    m_numProcessors;
    ULARGE_INTEGER m_lastTime, m_lastKernel, m_lastUser;
    float m_total, m_user, m_kernel;
};

extern CPUUsageProcessor ProcessorTime;
extern CPUUsageProcess ProcessTime;