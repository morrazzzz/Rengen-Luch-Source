#include "stdafx.h"
#pragma hdrstop

#include "xrdebug.h"
#include "os_clipboard.h"

#include <sal.h>
#include "dxerr.h"

#include <malloc.h>
#include <direct.h>

extern bool shared_str_initialized;

#ifdef __BORLANDC__

#include "d3d9.h"
#include "d3dx9.h"
#include "D3DX_Wrapper.h"
#pragma comment(lib,"EToolsB.lib")

#endif
extern bool GbEditor ;
#include "StackWalker/StackWalker.h"

class StackWalkerToConsole : public StackWalker
{
	bool m_tocopy;
	LPSTR assertion_info;
	LPSTR buffer_base;
	LPSTR buffer;
	u32 assertion_info_size;
public:
	StackWalkerToConsole() : StackWalker(0, Core.ApplicationPath)
	{
		m_tocopy = false;
		assertion_info = nullptr;
		buffer = nullptr;
		buffer_base = nullptr;
		assertion_info_size = 0;
	}

	void SetParams(bool tocopy = false, LPSTR buf = nullptr, LPSTR info = nullptr, u32 size = 0)
	{
		m_tocopy = tocopy;
		assertion_info = info;
		buffer = buf;
		buffer_base = info;
		assertion_info_size = size;
	};
protected:
	virtual void OnOutput(LPCSTR szText)
	{
		// to error window buffer
		if (buffer)
		{
			if (sizeof(assertion_info) - u32(buffer - &assertion_info[0]) < 512)
			{
				Msg("%u %u %p %p", sizeof(assertion_info) - u32(buffer - &assertion_info[0]), buffer, &assertion_info[0], sizeof(assertion_info));
				MessageBox(NULL, "Almost cant't fit the assertion info into buffer. Raise the buffer", "INTERNAL DEBUG INFO GATHERING ERROR", MB_CANCELTRYCONTINUE | MB_ICONWARNING | MB_DEFBUTTON2 | MB_TASKMODAL | MB_TOPMOST);
			}
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
			buffer += xr_sprintf(buffer, assertion_info_size - u32(buffer - buffer_base), "%s\n", szText);
#else
			if (strstr(Core.Params, "-use_error_window"))
				buffer += xr_sprintf(buffer, assertion_info_size - u32(buffer - buffer_base), "%s\n", szText);
#endif
		}

		// log
		if (shared_str_initialized)
			Msg("%s", szText);

		// to clipboard
		if (m_tocopy)
		{
			string4096 buf;
			xr_sprintf(buf, sizeof(buf), "%s\r\n", szText);

			os_clipboard::update_clipboard(buf);
		}
	}
};

StackWalkerToConsole stackWalker_;

#ifndef _M_AMD64
#	ifndef __BORLANDC__
#		pragma comment(lib, "dxerr.lib")
#	endif
#endif

#include <dbghelp.h> // MiniDump flags

#include <new.h>		// for _set_new_mode
#include <signal.h>		// for signals like "OnAbort" handler

#if __BORLANDC__
#	define USE_OWN_ERROR_MESSAGE_WINDOW
#else
#	define USE_OWN_MINI_DUMP
#endif

XRCORE_API	xrDebug	Debug;

static bool	error_after_dialog = false;

RMutex protectStackTrace_;

void ScriptStackTrace()
{
#ifndef _EDITOR
	typedef void __cdecl extern_func_1();

	HMODULE xrGameModule_ = NULL;
	extern_func_1* ScriptStackTrace = NULL;
	xrGameModule_ = LoadLibrary("xrGame.dll");

	if (xrGameModule_)
	{
		ScriptStackTrace = (extern_func_1*)GetProcAddress(xrGameModule_, "ScriptStackTraceExternalCall");

		if (ScriptStackTrace)
			ScriptStackTrace();

		FreeLibrary(xrGameModule_);
	}
#endif
}

void LogStackTrace(LPCSTR header)
{
	if (!shared_str_initialized)
		return;

	protectStackTrace_.lock();

	Msg("%s",header);

	stackWalker_.SetParams();
	stackWalker_.ShowCallstack();

	protectStackTrace_.unlock();
}

void xrDebug::log_stack_trace()
{
	LogStackTrace("Error");
}

//extern LPCTSTR ConvertSimpleException(DWORD dwExcept);

LPCSTR xrDebug::exception_name(DWORD code)
{
	return /*ConvertSimpleException(code);*/ "";
}

void xrDebug::exception_stacktrace(_EXCEPTION_POINTERS* ep)
{
	Msg("%s:", "exception stacktrace");

	stackWalker_.SetParams();
	stackWalker_.ShowCallstack();
}

void xrDebug::gather_info(const char *expression, const char *description, const char *argument0, const char *argument1, const char *file, int line, const char *function, LPSTR assertion_info, u32 const assertion_info_size)
{
	LPSTR buffer_base = assertion_info;
	LPSTR buffer = assertion_info;
	int assertion_size = (int)assertion_info_size;

	LPCSTR endline = "\n";
	LPCSTR prefix = "[error]";
	bool extended_description = (description && !argument0 && strchr(description,'\n'));

	buffer += xr_sprintf(assertion_info, sizeof(assertion_info), "");

	// Get the basic info into buffer
	for (int i = 0; i < 1; ++i)
	{
		if (!i)
			buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sERROR%s%s", endline, endline, endline);

		buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sExpression    : %s%s", prefix, expression, endline);
		buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sFunction      : %s%s", prefix, function, endline);
		buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sFile          : %s%s", prefix, file, endline);
		buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sLine          : %d%s", prefix, line, endline);

		if (extended_description)
		{
			buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%s%s%s", endline, description, endline);

			if (argument0)
			{
				if (argument1)
				{
					buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%s%s", argument0, endline);
					buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%s%s", argument1, endline);
				}
				else
					buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%s%s", argument0, endline);
			}
		}
		else
		{
			buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sDescription   : %s%s", prefix, description, endline);

			if (argument0)
			{
				if (argument1)
				{
					buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sArgument 0    : %s%s", prefix, argument0, endline);
					buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sArgument 1    : %s%s", prefix, argument1, endline);
				}
				else
					buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%sArguments     : %s%s", prefix, argument0, endline);
			}
		}

		buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "%s", endline);

		if (!i)
		{
			if (shared_str_initialized)
			{
				Msg("%s", assertion_info);

				FlushLog();
			}

			endline		= "\r\n";
			prefix		= "";
		}
	}

#ifdef USE_MEMORY_MONITOR
	memory_monitor::flush_each_time	(true);
	memory_monitor::flush_each_time	(false);
#endif


	// Get the stack trace into buffer

	if (!IsDebuggerPresent() && !strstr(GetCommandLine(),"-no_call_stack_assert"))
	{
		if (shared_str_initialized)
			Msg("stack trace:\n");

#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
		buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "stack trace:%s%s", endline, endline);
#else
		if (strstr(Core.Params, "-use_error_window"))
			buffer += xr_sprintf(buffer, assertion_size - u32(buffer - buffer_base), "stack trace:%s%s", endline, endline);
#endif

		if (!xr_strcmp(Core.ApplicationName, "xr_3da") && GetCurrentThreadId() == Core.mainThreadID) // xr_3da only and main thread.
			ScriptStackTrace(); // Try to get script stack trace from xrGame or other script handlers

		protectStackTrace_.lock();

		stackWalker_.SetParams(false, buffer, assertion_info, assertion_info_size);
		stackWalker_.ShowCallstack();

		protectStackTrace_.unlock();

		if (shared_str_initialized)
			FlushLog();

		// copy to clipboard, so that user can ctrl+V the error message
		os_clipboard::update_clipboard(assertion_info);
	}

	FlushLog();
}


void xrDebug::do_exit	(const std::string &message)
{
	FlushLog();

	MessageBox(NULL, message.c_str(), "Error", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);

	TerminateProcess(GetCurrentProcess(), 1);
}


void xrDebug::backend	(const char *expression, const char *description, const char *argument0, const char *argument1, const char *file, int line, const char *function, bool &ignore_always)
{
	if (IsDebuggerPresent() && strstr(Core.Params, "-assert_debug_break"))
	{
		DebugBreak(); return;
	}
	static AccessLock CS;

	CS.Enter();

	error_after_dialog	= true;

	string16384 assertion_info = "";

	gather_info(expression, description, argument0, argument1, file, line, function, assertion_info);

	LPCSTR endline = "\r\n";
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW

	LPSTR buffer = assertion_info + xr_strlen(assertion_info);

	buffer				+= xr_sprintf(buffer, sizeof(assertion_info) - u32(buffer - &assertion_info[0]), "%sPress CANCEL to EXIT app%s", endline, endline);
	buffer				+= xr_sprintf(buffer, sizeof(assertion_info) - u32(buffer - &assertion_info[0]), "Press TRY AGAIN to ignore error ONE TIME%s", endline);

#ifdef __BORLANDC__
	buffer += xr_sprintf(buffer, sizeof(assertion_info) - u32(buffer - &assertion_info[0]), "Press CONTINUE to ignore ALL errors from SDK BINS(Not Recomended)%s%s", endline, endline);
#else
	buffer += xr_sprintf(buffer, sizeof(assertion_info) - u32(buffer - &assertion_info[0]), "Press CONTINUE to ignore ALL errors from SHARED BINS(Not Recomended)%s%s", endline, endline);
#endif

#endif
	FlushLog();

	if (handler)
		handler();

	if (get_on_dialog())
		get_on_dialog()(true);

	FlushLog();

#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
#ifdef __BORLANDC__
		int	result = 
			MessageBox(NULL, assertion_info, "ASSERTION FAILED IN SDK BINS", MB_CANCELTRYCONTINUE | MB_ICONWARNING | MB_TASKMODAL| MB_DEFBUTTON | MB_TOPMOST2);
#else
		int	result =
			MessageBox(NULL, assertion_info, "ASSERTION FAILED IN SHARED BINS", MB_CANCELTRYCONTINUE | MB_ICONWARNING | MB_DEFBUTTON2 | MB_TASKMODAL | MB_TOPMOST);
#endif

		switch (result)
		{
			case IDCANCEL : {
				TerminateProcess(GetCurrentProcess(), 1); // sometimes editor does not want to completely exit. This resolves the problem
				break;
			}
			case IDTRYAGAIN : {
				error_after_dialog	= false;
				break;
			}
			case IDCONTINUE : {
				error_after_dialog	= false;
				ignore_always	= true;
				break;
			}
			default : NODEFAULT;
		}

		if (get_on_dialog())
			get_on_dialog()	(false);


		CS.Leave();
#else

		if (strstr(Core.Params, "-use_error_window"))
		{
			LPSTR buffer = assertion_info + xr_strlen(assertion_info);

			buffer += xr_sprintf(buffer, sizeof(assertion_info) - u32(buffer - &assertion_info[0]), "%sPress CANCEL to EXIT app%s", endline, endline);
			buffer += xr_sprintf(buffer, sizeof(assertion_info) - u32(buffer - &assertion_info[0]), "Press TRY AGAIN to ignore error ONE TIME%s", endline);
			buffer += xr_sprintf(buffer, sizeof(assertion_info) - u32(buffer - &assertion_info[0]), "Press CONTINUE to ignore ALL errors (Not Recomended)%s%s", endline, endline);

			int	result = MessageBox(NULL, assertion_info, "ASSERTION FAILED IN GAME BINS", MB_CANCELTRYCONTINUE | MB_ICONWARNING | MB_DEFBUTTON2 | MB_TASKMODAL | MB_TOPMOST);

			switch (result)
			{
			case IDCANCEL: {
				TerminateProcess(GetCurrentProcess(), 1); // sometimes editor does not want to completely exit. This resolves the problem
				break;
			}
			case IDTRYAGAIN: {
				error_after_dialog = false;
				break;
			}
			case IDCONTINUE: {
				error_after_dialog = false;
				ignore_always = true;
				break;
			}
			default: NODEFAULT;
			}
		}
		else
		{
			Msg("! Try using '-use_error_window' to be able to skip some errors");

			MessageBox(NULL, "Critical Error", description, MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);

			TerminateProcess(GetCurrentProcess(), 1);
		}

		if (get_on_dialog())
			get_on_dialog()	(false);


		CS.Leave();
#endif

		Msg("%s End of Debug Info %s ", endline, endline);

		FlushLog();

//#endif
}

LPCSTR xrDebug::error2string(long code)
{
	LPCSTR result = 0;
	static string1024 desc_storage;

#if defined(_M_AMD64) || defined(_EDITOR)
#else
	result = DXGetErrorDescription(code);
#endif

	if (0==result) 
	{
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,code,0,desc_storage,sizeof(desc_storage)-1,0);
		result = desc_storage;
	}

	return result;
}

void xrDebug::error(long hr, const char* expr, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(error2string(hr), expr, 0, 0, file, line, function, ignore_always);
}

void xrDebug::error(long hr, const char* expr, const char* e2, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(error2string(hr), expr, e2, 0, file, line, function, ignore_always);
}

void xrDebug::fail(const char *e1, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		("assertion failed", e1 , 0, 0, file,line, function, ignore_always);
}

void xrDebug::fail(const char *e1, const std::string &e2, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(e1, e2.c_str(), 0, 0, file, line, function, ignore_always);
}

void xrDebug::fail(const char *e1, const char *e2, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(e1, e2, 0, 0, file, line, function, ignore_always);
}

void xrDebug::fail(const char *e1, const char *e2, const char *e3, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(e1, e2, e3, 0, file, line, function, ignore_always);
}

void xrDebug::fail(const char *e1, const char *e2, const char *e3, const char *e4, const char *file, int line, const char *function, bool &ignore_always)
{
	backend		(e1, e2, e3, e4, file, line, function, ignore_always);
}

void __cdecl xrDebug::fatal(const char *file, int line, const char *function, const char* F,...)
{
	string1024	buffer;

	va_list		p;
	va_start	(p,F);
	vsprintf	(buffer,F,p);
	va_end		(p);

	bool		ignore_always = true;

	backend		("fatal error","<no expression>",buffer,0,file,line,function,ignore_always);
}

typedef void (*full_memory_stats_callback_type)();
XRCORE_API full_memory_stats_callback_type g_full_memory_stats_callback = 0;

int out_of_memory_handler(size_t size)
{
	if (g_full_memory_stats_callback)
		g_full_memory_stats_callback();
	else
	{
		Memory.mem_compact();
#ifndef _EDITOR
		u32					crt_heap		= mem_usage_impl((HANDLE)_get_heap_handle(), 0, 0);
#else
		u32					crt_heap		= 0;
#endif
		u32					process_heap	= mem_usage_impl(GetProcessHeap(), 0, 0);
		int					eco_strings		= (int)g_pStringContainer->stat_economy();
		int					eco_smem		= (int)g_pSharedMemoryContainer->stat_economy();

		Msg					("* [x-ray]: crt heap[%d K], process heap[%d K]",crt_heap/1024,process_heap/1024);
		Msg					("* [x-ray]: economy: strings[%d K], smem[%d K]",eco_strings/1024,eco_smem);
	}

	Debug.fatal(DEBUG_INFO,"Out of memory. Memory request: %d K",size/1024);

	return 1;
}

extern LPCSTR log_name();

XRCORE_API string_path g_bug_report_file;

#if 1

typedef LONG WINAPI UnhandledExceptionFilterType(struct _EXCEPTION_POINTERS *pExceptionInfo);
typedef LONG (__stdcall *PFNCHFILTFN) (EXCEPTION_POINTERS* pExPtrs);

extern "C" BOOL __stdcall SetCrashHandlerFilter (PFNCHFILTFN pFn);

static UnhandledExceptionFilterType	*previous_filter = 0;

#ifdef USE_OWN_MINI_DUMP
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
										 );

void FlushLog(LPCSTR file_name);

void save_mini_dump(_EXCEPTION_POINTERS *pExceptionInfo)
{
	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;
	string_path	szDbgHelpPath;

	if (GetModuleFileName(NULL, szDbgHelpPath, _MAX_PATH))
	{
		char *pSlash = strchr( szDbgHelpPath, '\\' );
		if (pSlash)
		{
			xr_strcpy	(pSlash+1, sizeof(szDbgHelpPath) - (pSlash - szDbgHelpPath), "DBGHELP.DLL" );
			hDll = ::LoadLibrary( szDbgHelpPath );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary( "DBGHELP.DLL" );
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			string_path	szDumpPath;
			string_path	szScratch;
			string64	t_stemp;

			timestamp	(t_stemp);

			xr_strcpy		(szDumpPath, Core.ApplicationName);
			xr_strcat		(szDumpPath, "_");
			xr_strcat		(szDumpPath, Core.UserName);
			xr_strcat		(szDumpPath, "_");
			xr_strcat		(szDumpPath, t_stemp);
			xr_strcat		(szDumpPath, ".mdmp");

			__try
			{
				if (FS.path_exist("$logs$"))
					FS.update_path(szDumpPath,"$logs$",szDumpPath);
			}
            __except( EXCEPTION_EXECUTE_HANDLER )
			{
				string_path	temp;

				xr_strcpy		(temp, szDumpPath);
				xr_strcpy		(szDumpPath, "logs/");
				xr_strcat		(szDumpPath, temp);
            }

			string_path	log_file_name;
			strconcat(sizeof(log_file_name), log_file_name, szDumpPath, ".log");

			FlushLog(log_file_name);

			// create the file
			HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE==hFile)	
			{
				// try to place into current directory
				MoveMemory(szDumpPath,szDumpPath+5,strlen(szDumpPath));

				hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			}
			if (hFile!=INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId				= ::GetCurrentThreadId();
				ExInfo.ExceptionPointers	= pExceptionInfo;
				ExInfo.ClientPointers		= NULL;

				// write the dump
				MINIDUMP_TYPE dump_flags	= MINIDUMP_TYPE(MiniDumpNormal | MiniDumpFilterMemory | MiniDumpScanMemory);

				BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, dump_flags, &ExInfo, NULL, NULL);
				if (bOK)
				{
					xr_sprintf( szScratch, "Saved dump file to '%s'", szDumpPath );
					szResult = szScratch;
//					retval = EXCEPTION_EXECUTE_HANDLER;
				}
				else
				{
					xr_sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, GetLastError());
					szResult = szScratch;
				}
				::CloseHandle(hFile);
			}
			else
			{
				xr_sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, GetLastError() );
				szResult = szScratch;
			}
		}
		else
		{
			szResult = "DBGHELP.DLL too old";
		}
	}
	else
	{
		szResult = "DBGHELP.DLL not found";
	}
}
#endif

void format_message	(LPSTR buffer, const u32 &buffer_size)
{
    LPVOID message;
    DWORD error_code = GetLastError();

	if (!error_code)
	{
		*buffer	= 0;
		return;
	}

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)& message,
        0,
		NULL
	);

	xr_sprintf(buffer,buffer_size,"[error][%8d]    : %s",error_code,message);
    LocalFree(message);
}

LONG WINAPI UnhandledFilter	(_EXCEPTION_POINTERS *pExceptionInfo)
{
	//if (GbEditor) 	TerminateThread(GetCurrentThread(), 1);
	string256 error_message;
	format_message(error_message, sizeof(error_message));
	/*if (*error_message)
	{
		string4096 buffer;
		if (shared_str_initialized)
			Msg("\n%s", error_message);
		FlushLog();
		xr_strcat(error_message, sizeof(error_message), "\r\n");

		os_clipboard::update_clipboard(error_message);
	}*/

	if (!error_after_dialog && !strstr(GetCommandLine(), "-no_call_stack_assert"))
	{
		CONTEXT save = *pExceptionInfo->ContextRecord;

		if (!xr_strcmp(Core.ApplicationName, "xr_3da") && GetCurrentThreadId() == Core.mainThreadID) // xr_3da only
			ScriptStackTrace(); // Try to get script stack trace from xrGame or other script handlers

		*pExceptionInfo->ContextRecord = save;

		if (shared_str_initialized)
			Msg("stack trace: Unhandled Error \n");
		else
			MessageBox(NULL, "Try to Ctrl+V in any text editor to see Call Stack", "Unhandled Error", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);

		os_clipboard::update_clipboard("Unhandled Error: Stack trace:\r\n\r\n");

		string4096 buffer;

		protectStackTrace_.lock();

		stackWalker_.SetParams(true);
		stackWalker_.ShowCallstack();

		protectStackTrace_.unlock();

		if (*error_message)
		{
			if (shared_str_initialized)
				Msg("\n%s",error_message);

			xr_strcat(error_message,sizeof(error_message),"\r\n");

			os_clipboard::update_clipboard(buffer);
		}
	}


	if (shared_str_initialized)
		FlushLog();

#	ifdef USE_OWN_MINI_DUMP
	save_mini_dump(pExceptionInfo);
#	endif

	if (Debug.get_on_dialog())
		Debug.get_on_dialog() (true);

	if (strstr(GetCommandLine(), "-error_hang"))
		while (true) Sleep(1000); // to be able to run debuger
	else
		MessageBox(NULL, "Critical Error", "Unhandled Error", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);

	if (Debug.get_on_dialog())
		Debug.get_on_dialog() (false);

	TerminateProcess(GetCurrentProcess(), 1);

	return (EXCEPTION_CONTINUE_SEARCH);
}
#endif

//////////////////////////////////////////////////////////////////////
#ifdef M_BORLAND
	namespace std{
		extern new_handler _RTLENTRY _EXPFUNC set_new_handler( new_handler new_p );
	};

	static void __cdecl def_new_handler() 
    {
		FATAL		("Out of memory.");
    }

    void	xrDebug::_initialize		(const bool &dedicated)
    {
		handler							= 0;
		m_on_dialog						= 0;
        std::set_new_handler			(def_new_handler);	// exception-handler for 'out of memory' condition
//		::SetUnhandledExceptionFilter	(UnhandledFilter);	// exception handler to all "unhandled" exceptions
    }
#else
    typedef int		(__cdecl * _PNH)( size_t );
    _CRTIMP int		__cdecl _set_new_mode(_In_ int _NewMode);
  //  _CRTIMP _PNH	__cdecl _set_new_handler(_In_opt_ _PNH _NewHandler);

	void _terminate()
	{
		R_ASSERT(false);
		MessageBox(NULL, "Termination of app called", "Fuckeh", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);

		abort();
	}

	void _terminate_unexpected()
	{
		MessageBox(NULL, "Error is not in dynamic-exception-specification", "Handler of unexpected is invoked", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);

		abort();
	}

	void debug_on_thread_spawn()
	{
		std::set_terminate(_terminate);
	}

	static void handler_base(LPCSTR reason_string)
	{
		bool ignore_always = false;
		Debug.backend(
			"error handler is invoked!",
			reason_string,
			0,
			0,
			DEBUG_INFO,
			ignore_always
		);
	}

	static void invalid_parameter_handler(
			const wchar_t *expression,
			const wchar_t *function,
			const wchar_t *file,
			unsigned int line,
			uintptr_t reserved
		)
	{
		bool							ignore_always = false;

		string4096						expression_;
		string4096						function_;
		string4096						file_;
		size_t							converted_chars = 0;
//		errno_t							err = 

		if (expression)
			wcstombs_s(
				&converted_chars, 
				expression_,
				sizeof(expression_),
				expression,
				(wcslen(expression) + 1)*2*sizeof(char)
			);
		else
			xr_strcpy					(expression_,"");

		if (function)
			wcstombs_s(
				&converted_chars, 
				function_,
				sizeof(function_),
				function,
				(wcslen(function) + 1)*2*sizeof(char)
			);
		else
			xr_strcpy					(function_,__FUNCTION__);

		if (file)
			wcstombs_s(
				&converted_chars, 
				file_,
				sizeof(file_),
				file,
				(wcslen(file) + 1)*2*sizeof(char)
			);
		else
		{
			line = __LINE__;
			xr_strcpy(file_,__FILE__);
		}

		Debug.backend(
			"error handler is invoked!",
			expression_,
			0,
			0,
			file_,
			line,
			function_,
			ignore_always
		);
	}

	static void std_new_error_handler()
	{
		u32 curr_free_blocks	= 0, curr_used_blocks = 0;
		u32 curr_mem_usage		= Memory.mem_usage(&curr_used_blocks, &curr_free_blocks);
		u32	process_heap		= mem_usage_impl(GetProcessHeap(), 0, 0);

		std::string str = make_string("Error handler for operator 'new' is called: Error while allocating memory or system is out of memory. Memory manager: Usage: %d kb, Used blocks: %d, Free blocks: %d, Heap: %d kb",
			curr_mem_usage / 1024, curr_used_blocks, curr_free_blocks, process_heap / 1024);
		
		handler_base(str.c_str());
	}

	static void pure_call_handler()
	{
		handler_base("pure virtual function call");
	}

#ifdef CS_USE_EXCEPTIONS
	static void unexpected_handler()
	{
		handler_base("unexpected program termination");
	}
#endif

	static void floating_point_handler(int signal)
	{
		handler_base("floating point error");
	}

	static void illegal_instruction_handler(int signal)
	{
		handler_base("illegal instruction");
	}

	static void storage_access_handler		(int signal)
	{
		handler_base("illegal storage access");
	}

	static void termination_handler(int signal)
	{
		handler_base("termination with exit code 3");
	}

	extern "C" void OnAbortFunction(int signal_number) // Called when abort() is called
	{
		if (IsDebuggerPresent())DebugBreak();
		try
		{
			Msg(LINE_SPACER);
			Msg(LINE_SPACER);
			Msg(LINE_SPACER);
			Msg("!!!An abort() is called, trying to save log!!!");
			Msg(LINE_SPACER);
			Msg(LINE_SPACER);
			Msg(LINE_SPACER);

			LogStackTrace("Abort call stack");

			FlushLog();
		}
		catch (...)
		{
			MessageBox(NULL, "If you see this msg box, that means log is probably empty. Try to check all abort calls in source code", "Abort is called", MB_OK | MB_ICONWARNING | MB_TASKMODAL | MB_TOPMOST);
		}

	}

    void xrDebug::_initialize(const bool &dedicated,bool editor)
    {
		debug_on_thread_spawn();

		_set_abort_behavior				(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

		signal							(SIGABRT,			&OnAbortFunction); // initialize "on abort" function for our process
		signal							(SIGABRT_COMPAT,	&OnAbortFunction); // Same as SIGABRT. For compability for other platforms

		signal							(SIGFPE,			floating_point_handler);
		signal							(SIGILL,			illegal_instruction_handler);
		signal							(SIGINT,			0);
		//signal							(SIGSEGV,			storage_access_handler);
		signal							(SIGTERM,			termination_handler);

		_set_invalid_parameter_handler	(&invalid_parameter_handler);

		_set_new_mode					(1);
//		_set_new_handler				(&out_of_memory_handler);
		std::set_new_handler			(&std_new_error_handler);

		_set_purecall_handler			(&pure_call_handler);


#if 1// should be if we use exceptions
		std::set_unexpected(_terminate_unexpected);
#endif

		static bool is_dedicated= dedicated;

		*g_bug_report_file= 0;

		debug_on_thread_spawn();
		//sif(!editor)
		previous_filter = ::SetUnhandledExceptionFilter(UnhandledFilter);	// exception handler to all "unhandled" exceptions

#if 0
		struct foo {static void	recurs	(const u32 &count)
		{
			if (!count)
				return;

			_alloca			(4096);
			recurs			(count - 1);
		}};
		foo::recurs			(u32(-1));
		std::terminate		();
#endif
	}
#endif