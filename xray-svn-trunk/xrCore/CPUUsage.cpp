#include "stdafx.h"
#include "windows.h"
#include "CPUUsage.h"

CPUUsage cpuUsageHandler_;

CPUUsage::CPUUsage()
{
	initialized_ = false;

	numProcessors_ = 1;
	currentProcessHandle_ = nullptr;
}

void CPUUsage::InitMonitoring()
{
	SYSTEM_INFO sysInfo;
	FILETIME ftime, fsys, fuser;

	GetSystemInfo(&sysInfo);
	numProcessors_ = sysInfo.dwNumberOfProcessors;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&lastCPU_, &ftime, sizeof(FILETIME));

	currentProcessHandle_ = GetCurrentProcess();
	GetProcessTimes(currentProcessHandle_, &ftime, &ftime, &fsys, &fuser);
	memcpy(&lastSysCPU_, &fsys, sizeof(FILETIME));
	memcpy(&lastUserCPU_, &fuser, sizeof(FILETIME));

	initialized_ = true;
}

double CPUUsage::GetCurrentCPUUsage()
{
	if (!initialized_)
		return -0.0;

	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(currentProcessHandle_, &ftime, &ftime, &fsys, &fuser);

	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));

	percent = double((sys.QuadPart - lastSysCPU_.QuadPart) + (user.QuadPart - lastUserCPU_.QuadPart));
	percent /= (now.QuadPart - lastCPU_.QuadPart);
	percent /= numProcessors_;

	lastCPU_ = now;
	lastUserCPU_ = user;
	lastSysCPU_ = sys;

	return percent * 100;
}