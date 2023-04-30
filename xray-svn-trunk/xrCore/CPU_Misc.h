
// Various functions for detecting CPU and other SYS info. These function are from various websites (with our changes) and are for Windows platform

#pragma once

#include "stdafx.h"
#include <windows.h>
#include <malloc.h>    
#include <stdio.h>
#include <tchar.h>

#include "intrin.h"

/// from https://docs.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation

typedef BOOL(WINAPI *LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
	DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
	DWORD bit_set_count = 0;
	ULONG_PTR bit_test = (ULONG_PTR)1 << LSHIFT;
	DWORD i;

	for (i = 0; i <= LSHIFT; ++i)
	{
		bit_set_count += ((bitMask & bit_test) ? 1 : 0);
		bit_test /= 2;
	}

	return bit_set_count;
}

enum INFO_RES
{
	SEXSEXFULL,
	FAIL_1, // GetLogicalProcessorInformation is not supported
	FAIL_2, // Error: Allocation failure
	FAIL_3, // Error: Use GetLastError()
	HAS_WARNING // Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value
};

INFO_RES RoutineCPUInfo(u8& processors_count, u8& ph_cores_count, u8& log_cores_count)
{
	INFO_RES res;

	LPFN_GLPI glpi;
	bool done = false;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	DWORD return_length = 0;

	log_cores_count = 0;
	ph_cores_count = 0;
	processors_count = 0;

	DWORD byte_offset = 0;

	glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");

	if (!glpi)
		return FAIL_1;

	while (!done)
	{
		DWORD rc = glpi(buffer, &return_length);

		if (FALSE == rc)
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (buffer)
					free(buffer);

				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(return_length);

				if (!buffer)
					return FAIL_2;
			}
			else
				return FAIL_3;
		}
		else
		{
			done = true;
		}
	}

	ptr = buffer;

	while (byte_offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= return_length)
	{
		switch (ptr->Relationship)
		{
		case RelationProcessorCore:
			ph_cores_count++;

			// A hyperthreaded core supplies more than one logical processor.
			log_cores_count += (u8)CountSetBits(ptr->ProcessorMask);
			break;

		case RelationProcessorPackage:
			// Logical processors share a physical package.
			processors_count++;
			break;

		default:
			res = HAS_WARNING;

			break;
		}
		byte_offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

	free(buffer);

	res = SEXSEXFULL;

	return res;
}

/// from https://stackoverflow.com/questions/850774/how-to-determine-the-hardware-cpu-and-ram-on-a-machine

// string includes manufacturer, model and clockspeed
void CPUName(LPSTR buf)
{
	int cpu_info[4] = { -1 };
	u32 n_ex_ids, i = 0;

	// Get the information associated with each extended ID.
	__cpuid(cpu_info, 0x80000000);

	n_ex_ids = cpu_info[0];

	for (i = 0x80000000; i <= n_ex_ids; ++i)
	{
		__cpuid(cpu_info, i);
		// Interpret CPU brand string
		if (i == 0x80000002)
			memcpy(buf, cpu_info, sizeof(cpu_info));
		else if (i == 0x80000003)
			memcpy(buf + 16, cpu_info, sizeof(cpu_info));
		else if (i == 0x80000004)
			memcpy(buf + 32, cpu_info, sizeof(cpu_info));
	}
}

u64 TotalRam()
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);

	GlobalMemoryStatusEx(&statex);

	return statex.ullTotalPhys;
}