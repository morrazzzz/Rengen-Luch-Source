#include "stdafx.h"
#pragma hdrstop

#include <time.h>
#include "resource.h"
#include "log.h"
#ifdef _EDITOR
	#include "malloc.h"
#endif

extern BOOL					LogExecCB		= TRUE;
static string_path			logFName		= "engine.log";
static string_path			log_file_name	= "engine.log";
extern BOOL 				no_log			= FALSE;
extern BOOL 				no_log_overflow			= FALSE;
XRCORE_API BOOL				log_sending_thread_ID = false;

AccessLock logCS;
AccessLock syncLogFileDestuction_;

xr_vector<shared_str>*		LogFile			= NULL;
static LogCallback			LogCB			= 0;

#ifdef _EDITOR // For flushing SE factory log in SDK
HMODULE hXRSE_FACTORY = NULL;

typedef void __cdecl extern_func_1();

extern_func_1* FlushSELog = NULL;
#endif


void FlushLog			(LPCSTR file_name)
{
#ifdef _EDITOR
	if (FlushSELog)
		FlushSELog(); // Save xr_SE_Factory log
#endif

	if (no_log)
		return;

	Msg("* Saving log %s", file_name);

	logCS.Enter();

	try
	{
		IWriter				*f = FS.w_open(file_name);
		if (f) {
			for (u32 it = 0; it<LogFile->size(); it++)	{
				LPCSTR		s = *((*LogFile)[it]);
				f->w_string(s ? s : "");
			}
			FS.w_close(f);
		}
	}
	catch (...)
	{
		MessageBox(NULL, "Could not perform log saving", "Error while saving log", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
	}
	
	logCS.Leave();
}

void FlushLog			(bool report_myself)
{
#ifdef _EDITOR
	if (FlushSELog)
		FlushSELog(); // Save xr_SE_Factory log
#endif

	if (!no_log)
	{
		if (report_myself) Msg("* Saving log");

		logCS.Enter();

		string_path log_name;

#ifndef _EDITOR
		if (GetCurrentThreadId() == Core.mainThreadID)
			xr_sprintf(log_name, "%s", logFName); // A log flushed from main thread
		else
		{
			// Logs flushed from secondary threads
			xr_sprintf(log_name, "%s_%s_Thread_ID_%u.log", Core.ApplicationName, Core.UserName, GetCurrentThreadId());

			if (FS.path_exist("$logs$"))
				FS.update_path(log_name, "$logs$", log_name);
		}
#else
		sprintf(log_name, "%s", logFName); // A log flushed from editors
#endif

		IWriter *f = FS.w_open(log_name);

        if (f)
		{
            for (u32 it=0; it<LogFile->size(); it++)
			{
				LPCSTR		s	= *((*LogFile)[it]);
				f->w_string	(s?s:"");
			}

            FS.w_close(f);
        }

		logCS.Leave();
    }
}

void AddOne				(const char *split) 
{
	if(!LogFile)		
		return;

	logCS.Enter			();

#ifdef DEBUG
	OutputDebugString	(split);
	OutputDebugString	("\n");
#endif

//	DUMP_PHASE;
	{
		u32	length = xr_strlen(split);
		shared_str temp;

		if (length>0 && log_sending_thread_ID)
		{

#ifndef __BORLANDC__
			string4096 str;
			xr_sprintf(str, "%s @Thread[%u]", split, GetCurrentThreadId());
			temp = shared_str(str);
#else
			temp = shared_str(split);
#endif

		}
		else
		{
			temp = shared_str(split);
		}

		LogFile->push_back(temp);
//		DUMP_PHASE;

	}

	//exec CallBack
	if (LogExecCB&&LogCB)LogCB(split);

	logCS.Leave				();
}

void Log				(const char *s) 
{
	int		i,j;

	u32			length = xr_strlen( s );
#ifndef _EDITOR    
	PSTR split  = (PSTR)_alloca( (length + 1) * sizeof(char) );
#else
	PSTR split  = (PSTR)alloca( (length + 1) * sizeof(char) );
#endif
	for (i=0,j=0; s[i]!=0; i++) {
		if (s[i]=='\n') {
			split[j]=0;	// end of line
			if (split[0]==0) { split[0]=' '; split[1]=0; }
			AddOne(split);
			j=0;
		} else {
			split[j++]=s[i];
		}
	}
	split[j]=0;
	AddOne(split);
}

void __cdecl Msg		( const char *format, ...)
{
	if (!no_log_overflow && LogFile->size()>5000)  //skyloader: clear log
	{
		LogFile->clear();
		Log("Log overflow! Log cleared successfully.");
	}
	va_list		mark;
	string4096	buf;

	va_start	(mark, format );
	int sz = _vsnprintf(buf, sizeof(buf) - 1, format, mark);
	buf[sizeof(buf)-1] = 0;
    va_end		(mark);

	if (sz)		Log(buf);
}

void Log				(const char *msg, const char *dop) {
	if (!dop) {
		Log		(msg);
		return;
	}

	u32			buffer_size = (xr_strlen(msg) + 1 + xr_strlen(dop) + 1) * sizeof(char);
	PSTR buf	= (PSTR)_alloca( buffer_size );
	strconcat	(buffer_size, buf, msg, " ", dop);
	Log			(buf);
}

void Log				(const char *msg, u32 dop) {
	u32			buffer_size = (xr_strlen(msg) + 1 + 10 + 1) * sizeof(char);
	PSTR buf	= (PSTR)_alloca( buffer_size );

	xr_sprintf	(buf, buffer_size, "%s %d", msg, dop);
	Log			(buf);
}

void Log				(const char *msg, int dop) {
	u32			buffer_size = (xr_strlen(msg) + 1 + 11 + 1) * sizeof(char);
	PSTR buf	= (PSTR)_alloca( buffer_size );

	xr_sprintf	(buf, buffer_size, "%s %i", msg, dop);
	Log			(buf);
}

void Log				(const char *msg, float dop) {
	// actually, float string representation should be no more, than 40 characters,
	// but we will count with slight overhead
	u32			buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
	PSTR buf	= (PSTR)_alloca( buffer_size );

	xr_sprintf	(buf, buffer_size, "%s %f", msg, dop);
	Log			(buf);
}

void Log				(const char *msg, const Fvector &dop) {
	u32			buffer_size = (xr_strlen(msg) + 2 + 3*(64 + 1) + 1) * sizeof(char);
	PSTR buf	= (PSTR)_alloca( buffer_size );

	xr_sprintf	(buf, buffer_size,"%s (%f,%f,%f)",msg, VPUSH(dop) );
	Log			(buf);
}

void Log				(const char *msg, const Fmatrix &dop)	{
	u32			buffer_size = (xr_strlen(msg) + 2 + 4*( 4*(64 + 1) + 1 ) + 1) * sizeof(char);
	PSTR buf	= (PSTR)_alloca( buffer_size );

	xr_sprintf	(buf, buffer_size,"%s:\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n",
		msg,
		dop.i.x, dop.i.y, dop.i.z, dop._14_,
		dop.j.x, dop.j.y, dop.j.z, dop._24_,
		dop.k.x, dop.k.y, dop.k.z, dop._34_,
		dop.c.x, dop.c.y, dop.c.z, dop._44_
	);
	Log			(buf);
}

void LogWinErr			(const char *msg, long err_code)	{
	Msg					("%s: %s",msg,Debug.error2string(err_code)	);
}

LogCallback SetLogCB	(LogCallback cb)
{
	LogCallback	result	= LogCB;
	LogCB				= cb;
	return				(result);
}

LPCSTR log_name			()
{
	return				(log_file_name);
}

void InitLog()
{
	R_ASSERT			(LogFile==NULL);
	LogFile				= xr_new <xr_vector<shared_str> >();
	LogFile->reserve	(1000);

	Msg("* Log is initialized");

#ifdef _EDITOR
	hXRSE_FACTORY = LoadLibrary("xrSE_Factory.dll");

	VERIFY3(hXRSE_FACTORY, "Can't load library:", "xrSE_Factory.dll");

	FlushSELog = (extern_func_1*)GetProcAddress(hXRSE_FACTORY, "FlushSEFactoryLog");

	VERIFY3(FlushSELog, "Can't find func:", "FlushSEFactoryLog");
#endif
}

void CreateLog			(BOOL nl, BOOL nlo)
{
	no_log = nl;
	no_log_overflow = nlo;

	strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, "_MAIN", ".log");

	if (FS.path_exist("$logs$"))
		FS.update_path(logFName,"$logs$",log_file_name);

	if (!no_log)
	{
        IWriter *f = FS.w_open(logFName);
        if (f == NULL)
		{
			MessageBox(NULL, "If this error Apeared - Coders need to consider displaying call stack in this box", "Can't create log file.", MB_OK | MB_ICONERROR | MB_TASKMODAL);

			abort();
        }
        FS.w_close(f);
    }

	Msg("* Log file is created at %s", logFName);
	FlushLog(false);
}

void CloseLog(void)
{
	syncLogFileDestuction_.Enter();

	FlushLog		();
 	LogFile->clear	();
	
	xr_delete(LogFile);

	syncLogFileDestuction_.Leave();

#ifdef _EDITOR
	FreeLibrary(hXRSE_FACTORY);
#endif
}

bool IsLogFilePresentMT()
{
	bool res = false;

	syncLogFileDestuction_.Enter();

	if (LogFile)
		res = true;

	syncLogFileDestuction_.Leave();

	return res;
}