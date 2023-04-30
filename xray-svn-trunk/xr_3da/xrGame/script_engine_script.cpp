////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine_script.cpp
//	Created 	: 25.12.2002
//  Modified 	: 13.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator script engine export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stdafx.h"
#include "script_engine.h"
#include "script_storage.h"
#include "script_thread.h"
#include "ai_space.h"
#include "script_debugger.h"
//#include <ostream>

using namespace luabind;

bool ignoringLauErrorsEnabled_ = false;

void NotifyLUAError(LPCSTR caMessage, bool message_box = true)
{
	Msg("--- Script Stack Trace");

	Msg(LINE_SPACER); Msg(LINE_SPACER);

	ai().script_engine().print_stack_simple();

	if (message_box && !ignoringLauErrorsEnabled_)
	{
		string4096 info;

		xr_sprintf(info, sizeof(info), "\r\nError has occured in scripts logic. See Log for more info: \r\n%s\r\n", caMessage);

		xr_sprintf(info, sizeof(info), "%s\r\nPress Abort to EXIT app\r\n", info);
		xr_sprintf(info, sizeof(info), "%sPress Retry to ignore error ONE TIMEr\n", info);
		xr_sprintf(info, sizeof(info), "%sPress Ignore to ignore ALL scripts runtime errors (Not Recomended)\r\n\r\n", info);

		int	result = MessageBox(NULL, info, "Lua Error Notification", MB_ABORTRETRYIGNORE | MB_ICONWARNING | MB_DEFBUTTON2 | MB_TASKMODAL);

		switch (result)
		{
		case IDABORT: {
			FlushLog();

			TerminateProcess(GetCurrentProcess(), 1);
			break;
		}
		case IDRETRY: {
			break;
		}
		case IDIGNORE: {
			ignoringLauErrorsEnabled_ = true;
			break;
		}
		default: NODEFAULT;
		}
	}

	Msg(LINE_SPACER); Msg(LINE_SPACER);
}

void NotifyLUAErrorScriptCall(LPCSTR caMessage, bool message_box = true)
{
	Msg("! --------------- Script Runtime Error Notification -----------------");

	NotifyLUAError(caMessage, message_box);

	FlushLog();
}

// For getting script call stack from other parrent modules(ddls)
extern "C" {
	DLL_API void	__cdecl	ScriptStackTraceExternalCall()
	{
		try
		{
			if (g_ai_space)
			{
				Msg("* External Call for Script Stack Trace");

				NotifyLUAError("External Call for Script Stack Trace", false);
			}
		}
		catch (...)
		{
			MessageBox(NULL, "Could not get script stack trace", "Error", MB_OK | MB_ICONERROR | MB_TASKMODAL);
		}
	}
};

#ifdef XRSE_FACTORY_EXPORTS

void print1(LPCSTR str)
{
	lua_State* L = ai().script_engine().lua();
	lua_Debug l_tDebugInfo;
	lua_getstack(L, 2, &l_tDebugInfo);
	lua_getinfo(L, "nSl", &l_tDebugInfo);

	Msg("== [%s:%s:%i]: %s", l_tDebugInfo.short_src, l_tDebugInfo.name ? l_tDebugInfo.name : "", l_tDebugInfo.currentline, str);
}

#else

void print2(LPCSTR str)
{
	float time = Device.rateControlingTimer_.GetElapsed_ms_f();

	lua_State* L = ai().script_engine().lua();
	lua_Debug l_tDebugInfo;
	lua_getstack(L, 2, &l_tDebugInfo);
	lua_getinfo(L, "nSl", &l_tDebugInfo);

	Msg("== [#%u][%fms][%s:%s:%i]: %s", CurrentFrame(), time, l_tDebugInfo.short_src, l_tDebugInfo.name ? l_tDebugInfo.name : "", l_tDebugInfo.currentline, str);
}

void print3(LPCSTR str)
{
	float time = Device.rateControlingTimer_.GetElapsed_ms_f();

	lua_State* L = ai().script_engine().lua();
	lua_Debug l_tDebugInfo;
	
	lua_getstack(L, 2, &l_tDebugInfo);
	lua_getinfo(L, "S", &l_tDebugInfo);

	Msg("== [#%u][%fms][%s]: %s", CurrentFrame(), time, l_tDebugInfo.short_src, str);
}
#endif

void lua_debug_print(LPCSTR str)
{
	if (!xr_strlen(str))
		return;

#ifdef XRSE_FACTORY_EXPORTS
	print1(str);
#else
#ifdef DEBUG
	print2(str);
#else
	print3(str);
#endif
#endif
}

void log_(LPCSTR str)
{
	if (!xr_strlen(str))
		return;

	Msg("== %s", str);
}

void FlushLogs()
{
	FlushLog();

#ifdef DEBUG
	ai().script_engine().flush_log();
#endif
}

void verify_if_thread_is_running()
{
	THROW2(ai().script_engine().current_thread(),"coroutine.yield() is called outside the LUA thread!");
}

bool is_editor()
{
#ifdef XRGAME_EXPORTS
	return(false);
#else
	return(true);
#endif
}

#ifdef XRGAME_EXPORTS
CRenderDevice *get_device()
{
	return(&Device);
}
#endif

int bit_and(int i, int j)
{
	return(i & j);
}

int bit_or(int i, int j)
{
	return(i | j);
}

int bit_xor(int i, int j)
{
	return(i ^ j);
}

int bit_not(int i)
{
	return(~i);
}

int bit_lshift(int n, int s)
{
	return n << s;
}

int bit_rshift(int n, int s)
{
	return n >> s;
}

LPCSTR user_name()
{
	return(Core.UserName);
}

void prefetch_module(LPCSTR file_name)
{
	ai().script_engine().process_file(file_name);
}

const CLocatorAPI::file* GetScriptFileScript(LPCSTR name)
{
	return ai().script_engine().GetScriptFile(name);
}

int RandomInteger0()
{
	int rondo = Random.randI();

	return rondo;
}

int RandomInteger1(int max)
{
	int rondo = Random.randI(max + 1);

	return rondo;
}

int RandomInteger2(int min, int max)
{
	int mx = max;

	if (max < min) // since CRandom is not aware of this behavior
	{
		max = min;
		min = mx;
	}

	return Random.randI(min, max + 1);
}

float RandomFloat0()
{
	return Random.randF();
}

float RandomFloat1(float max)
{
	return Random.randF(max);
}

float RandomFloat2(float min, float max)
{
	return Random.randF(min, max);
}

struct profile_timer_script
{
	u64							m_accumulator;
	u64							m_count;
	int							m_recurse_mark;

	CTimer measure;
	
	IC profile_timer_script()
	{
		m_accumulator			= 0;
		m_count					= 0;
		m_recurse_mark			= 0;
	}

	IC profile_timer_script(const profile_timer_script &profile_timer)
	{
		*this = profile_timer;
	}

	IC profile_timer_script& operator=(const profile_timer_script &profile_timer)
	{
		measure					= profile_timer.measure;

		m_accumulator			= profile_timer.m_accumulator;
		m_count					= profile_timer.m_count;
		m_recurse_mark			= profile_timer.m_recurse_mark;

		return					(*this);
	}

	IC bool operator<(const profile_timer_script &profile_timer) const
	{
		return(m_accumulator < profile_timer.m_accumulator);
	}

	IC void start()
	{
		if (m_recurse_mark)
		{
			++m_recurse_mark;

			return;
		}

		++m_recurse_mark;
		++m_count;

		measure.Start();
	}

	IC void stop()
	{
		THROW(m_recurse_mark);

		--m_recurse_mark;
		
		if (m_recurse_mark)
			return;

		m_accumulator += measure.GetElapsed_mcs();
	}

	IC float time () const
	{
		float res = float(double(m_accumulator));

		return m_accumulator;
	}
};

IC profile_timer_script operator+(const profile_timer_script &portion0, const profile_timer_script &portion1)
{
	profile_timer_script	result;
	result.m_accumulator	= portion0.m_accumulator + portion1.m_accumulator;
	result.m_count			= portion0.m_count + portion1.m_count;
	return					(result);
}

//IC	std::ostream& operator<<(std::ostream &stream, profile_timer_script &timer)
//{
//	stream					<< timer.time();
//	return					(stream);
//}

#ifdef XRGAME_EXPORTS
ICF	u32	script_time_global	()	{ return EngineTimeU(); }
ICF	u32	script_time_global_async	()	{ return Device.TimerAsync_MMT(); }
#else
ICF	u32	script_time_global	()	{ return 0; }
ICF	u32	script_time_global_async	()	{ return 0; }
#endif

#pragma optimize("s",on)
void CScriptEngine::script_register(lua_State *L)
{
	module(L)[
		class_<profile_timer_script>("profile_timer")
			.def(constructor<>())
			.def(constructor<profile_timer_script&>())
			.def(const_self + profile_timer_script())
			.def(const_self < profile_timer_script())
//			.def(tostring(self))
			.def("start",&profile_timer_script::start)
			.def("stop",&profile_timer_script::stop)
			.def("time",&profile_timer_script::time)
	];

	function	(L,	"log_",							log_);
	function	(L,	"log_print",					lua_debug_print);

	function	(L,	"flush",						FlushLogs);
	function	(L,	"notify_error",					NotifyLUAErrorScriptCall);

	function	(L,	"prefetch",						prefetch_module);
	function	(L,	"verify_if_thread_is_running",	verify_if_thread_is_running);
	function	(L,	"editor",						is_editor);

	function	(L,	"bit_and",						bit_and);
	function	(L,	"bit_or",						bit_or);
	function	(L,	"bit_xor",						bit_xor);
	function	(L,	"bit_not",						bit_not);
	function    (L, "bit_lshift",					bit_lshift);
	function	(L, "bit_rshift",					bit_rshift);

	function	(L, "user_name",					user_name);

	function	(L, "time_global",					script_time_global);
	function	(L, "time_global_async",			script_time_global_async);

	function	(L, "script_exist",					GetScriptFileScript);

#ifdef XRGAME_EXPORTS
	function	(L,	"device",						get_device);
#endif

	module(L, "Random")
		[
			def("I", &RandomInteger0),
			def("I", &RandomInteger1),
			def("I", &RandomInteger2),
			def("F", &RandomFloat0),
			def("F", &RandomFloat1),
			def("F", &RandomFloat2)
		];
}
