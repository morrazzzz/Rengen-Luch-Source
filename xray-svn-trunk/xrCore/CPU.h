#pragma once

namespace CPU
{
	XRCORE_API extern u32 qpc_inquiry_counter;

	XRCORE_API extern u8 processors_;

	XRCORE_API extern u8 physicalCores_;
	XRCORE_API extern u8 logicalCores_;

	XRCORE_API extern u64 totalRam_;

	XRCORE_API extern string512 cpuName_;

	IC u64 QPC_Freq()
	{
		u64 qpc_freq;
		QueryPerformanceFrequency((PLARGE_INTEGER)&qpc_freq);

		return qpc_freq;
	}

	IC u64 QPC()
	{
		u64 _dest;
		QueryPerformanceCounter((PLARGE_INTEGER)&_dest);
		qpc_inquiry_counter++;

		return _dest;
	}

	IC u8 ProcessorsNum()
	{
		return processors_;
	}

	IC u8 GetLogicalCoresNum()
	{
		return logicalCores_;
	}

	IC u8 GetPhysicalCoresNum()
	{
		return physicalCores_;
	}

	IC u64 GetTotalRam()
	{
		return totalRam_;
	}

	IC LPCSTR GetCPUName()
	{
		return cpuName_;
	}

	XRCORE_API extern DWORD_PTR	GetMainThreadBestAffinity();
	XRCORE_API extern DWORD_PTR	GetSecondaryThreadBestAffinity();

	XRCORE_API extern int		InitProcessorInfo();
	XRCORE_API extern void		DumpInfo();
};