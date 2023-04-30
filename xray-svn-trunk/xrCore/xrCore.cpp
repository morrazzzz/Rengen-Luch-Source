// xrCore.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#pragma hdrstop
#include <mmsystem.h>
#include <objbase.h>
#include "xrCore.h"
 
#pragma comment(lib,"winmm.lib")

#ifdef DEBUG
#	include	<malloc.h>
#endif // DEBUG

XRCORE_API		xrCore	Core;

static u32	init_counter	= 0;

XRCORE_API		u32		build_id = 0;
XRCORE_API		LPCSTR	build_date = NULL;

// In xrCore, because not all dlls depend on xr_3da
Flags32 XRCORE_API engineState;

#ifndef __BORLANDC__

#include <thread>

xrCore::xrCore()
{

}

#endif
bool GbEditor = false;
void xrCore::_initialize	(LPCSTR _ApplicationName, LogCallback cb, BOOL init_fs, LPCSTR fs_fname, bool editor)
{
	GbEditor = editor;
	CTimer T; T.Start();

	xr_strcpy(ApplicationName, _ApplicationName);

	mainThreadID = GetCurrentThreadId(); // Store Main thread id
	CreateThreadRuntimeParams(mainThreadID);

	thread_name("X-RAY Primary thread");

	engineState.zero();

	if (0==init_counter) {
#ifdef XRCORE_STATIC	
		_clear87	();
		_control87	( _PC_53,   MCW_PC );
		_control87	( _RC_CHOP, MCW_RC );
		_control87	( _RC_NEAR, MCW_RC );
		_control87	( _MCW_EM,  MCW_EM );
#endif
		// Init COM so we can use CoCreateInstance
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion < 6)								//skyloader: if not windows vista, 7, 8, etc.
			CoInitializeEx (NULL, COINIT_MULTITHREADED);

		xr_strcpy			(Params,sizeof(Params),GetCommandLine());
		_strlwr_s			(Params,sizeof(Params));

		string_path		fn,dr,di;

		// application path
        GetModuleFileName(GetModuleHandle(MODULE_NAME),fn,sizeof(fn));
        _splitpath		(fn,dr,di,0,0);
        strconcat		(sizeof(ApplicationPath),ApplicationPath,dr,di);
#ifndef _EDITOR
	//	xr_strcpy		(g_application_path,sizeof(g_application_path),ApplicationPath);
#endif

#ifdef _EDITOR
		// working path
        if( strstr(Params,"-wf") )
        {
            string_path				c_name;
            sscanf					(strstr(Core.Params,"-wf ")+4,"%[^ ] ",c_name);
            SetCurrentDirectory     (c_name);
        }
#endif

		GetCurrentDirectory(sizeof(WorkingPath),WorkingPath);

#ifdef DEBUG
		isDebugMode = true;
#else
		isDebugMode = false;
#endif	

		// User/Comp Name
		DWORD	sz_user		= sizeof(UserName);
		GetUserName			(UserName,&sz_user);

		DWORD	sz_comp		= sizeof(CompName);
		GetComputerName		(CompName,&sz_comp);

		CPU::InitProcessorInfo();

		Memory._initialize	(strstr(Params,"-mem_debug") ? TRUE : FALSE);

		DUMP_PHASE;

		InitLog				();
		_initialize_cpu		();

		CPU::DumpInfo();

		Msg("* Core Params: %s", Core.Params);
		Msg("* Core preinitialization completed: %f ms \n", T.GetElapsed_sec() * 1000.f);
		Msg("* Core location: %s", Core.ApplicationPath);

		T.Start();

		rtc_initialize		();

		xr_FS				= xr_new <CLocatorAPI>();

		xr_EFS				= xr_new <EFS_Utils>();
	}

	if (init_fs)
	{
		u32 flags			= 0;

		if (0!=strstr(Params,"-build"))	 flags |= CLocatorAPI::flBuildCopy;
		if (0!=strstr(Params,"-ebuild")) flags |= CLocatorAPI::flBuildCopy|CLocatorAPI::flEBuildCopy;

#ifdef DEBUG
		if (strstr(Params,"-cache"))  flags |= CLocatorAPI::flCacheFiles;
		else flags &= ~CLocatorAPI::flCacheFiles;
#endif

#ifdef _EDITOR // for EDITORS - no cache
		flags 				&=~ CLocatorAPI::flCacheFiles;
#endif
		flags |= CLocatorAPI::flScanAppRoot;

#ifndef	_EDITOR
	#ifndef ELocatorAPIH
		if (0!=strstr(Params,"-file_activity"))	 flags |= CLocatorAPI::flDumpFileActivity;
	#endif
#endif
		FS._initialize		(flags,0,fs_fname);
		BuildId = build_id;

#ifndef _EDITOR

		if (build_id != 0 && build_date)
			Msg("Rentgen-Luch: %s build %d, %s\n", ApplicationName, build_id, build_date);
#endif

		EFS._initialize		();

#ifdef DEBUG
    #ifndef	_EDITOR
		Msg					("CRT heap 0x%08x",_get_heap_handle());
		Msg					("Process heap 0x%08x",GetProcessHeap());
    #endif
#endif

	}

	SetLogCB(cb);
	init_counter++;

	Msg("* Core initialization for '%s' is completed: %f ms \n", ApplicationName, T.GetElapsed_sec() * 1000.f);
	Msg("^ Main thread ID: [%u]", Core.mainThreadID);
	FlushLog(false);
}

#ifndef	_EDITOR
#include "compression_ppmd_stream.h"
extern compression::ppmd::stream	*trained_model;
#endif
void xrCore::_destroy		()
{
	--init_counter;
	if (0==init_counter){
		FS._destroy			();
		EFS._destroy		();
		xr_delete			(xr_FS);
		xr_delete			(xr_EFS);

#ifndef	_EDITOR
		if (trained_model) {
			void			*buffer = trained_model->buffer();
			xr_free			(buffer);
			xr_delete		(trained_model);
		}
#endif

		Memory._destroy		();

		DeleteThreadRuntimeParams(mainThreadID);
	}
}

#ifndef XRCORE_STATIC

//. why ??? 
#ifdef _EDITOR
	BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
#else
	BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
#endif
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			_clear87		();
			_control87		( _PC_53,   MCW_PC );
			_control87		( _RC_CHOP, MCW_RC );
			_control87		( _RC_NEAR, MCW_RC );
			_control87		( _MCW_EM,  MCW_EM );
		}
//.		LogFile.reserve		(256);
		break;
	case DLL_THREAD_ATTACH:
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion < 6)								//skyloader: if not windows vista, 7, 8, etc.
			CoInitializeEx	(NULL, COINIT_MULTITHREADED);

		timeBeginPeriod	(1);
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
#ifdef USE_MEMORY_MONITOR
		memory_monitor::flush_each_time	(true);
#endif // USE_MEMORY_MONITOR
		break;
	}
    return TRUE;
}
#endif // XRCORE_STATIC
