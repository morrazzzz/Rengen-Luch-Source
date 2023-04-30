#include "stdafx.h"
#include "CPU.h"
#include "CPU_Misc.h"

extern bool shared_str_initialized;

namespace CPU
{
	XRCORE_API u32 qpc_inquiry_counter = 0;
	XRCORE_API u8 processors_ = 0;
	XRCORE_API u8 physicalCores_ = 0;
	XRCORE_API u8 logicalCores_ = 0;
	XRCORE_API u64 totalRam_ = 0;
	XRCORE_API string512 cpuName_ = "";

	XRCORE_API int InitProcessorInfo()
	{
		// For windows platform only, sorry MacLinux :(

		INFO_RES res;
		xr_sprintf(cpuName_, "CPU name");

		try
		{
			res = RoutineCPUInfo(processors_, physicalCores_, logicalCores_);
		}
		catch (...){
			MessageBox(NULL, "1", "CPU info error", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);
		}

		try
		{
			CPUName(cpuName_);
		}
		catch (...) {
			MessageBox(NULL, "2", "CPU info error", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);
		}

		try
		{
			totalRam_ = TotalRam();
		}
		catch (...) {
			MessageBox(NULL, "3", "CPU info error", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);
		}

		if (logicalCores_ <= 0) // backup
			logicalCores_ = (u8)std::thread::hardware_concurrency();

		return 0;
	}

	XRCORE_API void DumpInfo()
	{
		if(shared_str_initialized)
			Msg("\n* CPU count: %u\n* %s Cores/Threads %u/%u\n* RAM: %u MB\n", ProcessorsNum(), GetCPUName(), GetPhysicalCoresNum(), GetLogicalCoresNum(), GetTotalRam() / 1024/ 1024);
	}


	XRCORE_API DWORD_PTR GetMainThreadBestAffinity()
	{
		u32 core_number = GetLogicalCoresNum();
		DWORD_PTR res = 0;

		if (core_number >= 16)
			res = (1 << 14) + (1 << 15);
		else if (core_number >= 12)
			res = (1 << 10) + (1 << 11);
		else if (core_number >= 8)
			res = (1 << 6) + (1 << 7);
		else if (core_number >= 6)
			res = (1 << 4) + (1 << 5);
		else if (core_number >= 4)
			res = (1 << 2) + (1 << 3);
		else if (core_number >= 2)
			res = (1 << 0) + (1 << 1);
		else
			res = (1 << 0);

		return res;
	}

	XRCORE_API DWORD_PTR GetSecondaryThreadBestAffinity()
	{
		u32 core_number = GetLogicalCoresNum();
		DWORD_PTR res = 0;

		if (core_number >= 16)
			res = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 4) + (1 << 5) + (1 << 6) + (1 << 7) + (1 << 8) + (1 << 9) + (1 << 10) + (1 << 11) + (1 << 12) + (1 << 13);
		else if (core_number >= 12)
			res = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 4) + (1 << 5) + (1 << 6) + (1 << 7) + (1 << 8) + (1 << 9);
		else if (core_number >= 8)
			res = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3) + (1 << 4) + (1 << 5);
		else if (core_number >= 6)
			res = (1 << 0) + (1 << 1) + (1 << 2) + (1 << 3);
		else if (core_number >= 4)
			res = (1 << 0) + (1 << 1) + (1 << 2);
		else if (core_number >= 2)
			res = (1 << 0) + (1 << 1);
		else
			res = (1 << 0);

		return res;
	}
};