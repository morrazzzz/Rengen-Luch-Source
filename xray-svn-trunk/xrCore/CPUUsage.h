#pragma once

class XRCORE_API CPUUsage
{
	ULARGE_INTEGER lastCPU_; 
	ULARGE_INTEGER lastSysCPU_;
	ULARGE_INTEGER lastUserCPU_;

	HANDLE currentProcessHandle_;

	bool		initialized_;

public:
	CPUUsage();

	int			numProcessors_;

	void		InitMonitoring();
	double		GetCurrentCPUUsage();
};

extern XRCORE_API CPUUsage cpuUsageHandler_;