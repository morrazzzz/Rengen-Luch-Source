#include "stdafx.h"
#include "etools.h"

//for total process ram usage
#include "windows.h"
#include "psapi.h"
#pragma comment( lib, "psapi.lib" )

namespace ETOOLS
{
	ETOOLS_API u64 __stdcall GetTotalUsedProcessRAM()
	{
		PROCESS_MEMORY_COUNTERS pmc;
		BOOL result = GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
		u64 RAMUsed = pmc.WorkingSetSize;

		return RAMUsed;
	}

	ETOOLS_API void __stdcall AlertMsg(const char *format, ...)
	{
		Msg(format);

		R_ASSERT2(false, make_string("%s; See xrSE_Factory log", format));
	}
}