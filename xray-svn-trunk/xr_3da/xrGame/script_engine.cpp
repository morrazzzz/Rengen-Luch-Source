////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine.cpp
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Engine
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_engine.h"
#include "ai_space.h"
#include "object_factory.h"
#include "script_process.h"

#ifdef USE_DEBUGGER
#	include "script_debugger.h"
#endif

#ifndef XRSE_FACTORY_EXPORTS
#	ifdef DEBUG
#		include "ai_debug.h"
		extern Flags32 psAI_Flags;
#	endif
#endif

extern void export_classes(lua_State *L);

CScriptEngine::CScriptEngine()
{
	m_stack_level			= 0;
	m_reload_modules		= false;

#ifdef USE_DEBUGGER
	m_scriptDebugger = NULL;
	restartDebugger();	
#endif
}

CScriptEngine::~CScriptEngine()
{
	while (!m_script_processes.empty())
		remove_script_process(m_script_processes.begin()->first);

#ifdef DEBUG
	flush_log();
#endif

#ifdef USE_DEBUGGER
	xr_delete (m_scriptDebugger);
#endif
}

void CScriptEngine::unload()
{
	lua_settop				(lua(),m_stack_level);
}

int CScriptEngine::lua_panic(lua_State *L)
{
	print_output(L, "PANIC", LUA_ERRRUN);

	return(0);
}

void CScriptEngine::lua_error(lua_State *L)
{
	print_output(L, "", LUA_ERRRUN);

#if !XRAY_EXCEPTIONS
	Debug.fatal	(DEBUG_INFO, "LUA error: %s", lua_tostring(L, -1));
#else
	throw lua_tostring(L, -1);
#endif
}

int  CScriptEngine::lua_pcall_failed(lua_State *L)
{
	Msg("! Script Engine Error : P Call Failed");

	print_output(L, "", LUA_ERRRUN);

#if !XRAY_EXCEPTIONS
	Debug.fatal(DEBUG_INFO, "LUA error: %s", lua_isstring(L, -1) ? lua_tostring(L, -1) : "");
#endif

	if (lua_isstring(L, -1))
		lua_pop	(L, 1);

	return(LUA_ERRRUN);
}

void lua_cast_failed(lua_State *L, LUABIND_TYPE_INFO info)
{
	CScriptEngine::print_output	(L, "", LUA_ERRRUN);

	Debug.fatal	(DEBUG_INFO, "LUA error: cannot cast lua value to %s", info->name());
}

void CScriptEngine::setup_callbacks()
{

#ifdef USE_DEBUGGER
	if(debugger())
		debugger()->PrepareLuaBind();
#endif

#ifdef USE_DEBUGGER
	if (!debugger() || !debugger()->Active()) 
#endif
	{
#if !XRAY_EXCEPTIONS
		luabind::set_error_callback		(CScriptEngine::lua_error);
#endif
		luabind::set_pcall_callback		(CScriptEngine::lua_pcall_failed);
	}

#if !XRAY_EXCEPTIONS
	luabind::set_cast_failed_callback	(lua_cast_failed);
#endif
	lua_atpanic							(lua(),CScriptEngine::lua_panic);
}

#ifdef DEBUG
#	include "script_thread.h"
void CScriptEngine::lua_hook_call(lua_State *L, lua_Debug *dbg)
{
	if (ai().script_engine().current_thread())
		ai().script_engine().current_thread()->script_hook(L,dbg);
	else
		ai().script_engine().m_stack_is_ready = true;
}
#endif

int auto_load(lua_State *L)
{
	if ((lua_gettop(L) < 2) || !lua_istable(L,1) || !lua_isstring(L,2))
	{
		lua_pushnil(L);

		return(1);
	}

	ai().script_engine().process_file_if_exists(lua_tostring(L,2),false);
	lua_rawget(L, 1);

	return(1);
}

void CScriptEngine::setup_auto_load	()
{
	luaL_newmetatable					(lua(),"XRAY_AutoLoadMetaTable");
	lua_pushstring						(lua(),"__index");
	lua_pushcfunction					(lua(), auto_load);
	lua_settable						(lua(),-3);

	lua_pushstring 						(lua(),"_G"); 
	lua_gettable 						(lua(),LUA_GLOBALSINDEX); 
	luaL_getmetatable					(lua(),"XRAY_AutoLoadMetaTable");
	lua_setmetatable					(lua(),-2);
	//. ??????????
	// lua_settop							(lua(),-0);
}

void CScriptEngine::init()
{
	CScriptStorage::reinit();

	luabind::open						(lua());
	setup_callbacks						();
	export_classes						(lua());
	setup_auto_load						();

	size_t cnt = LoadScriptList();

	Msg("- Found %d script files", cnt);

#ifdef DEBUG
	m_stack_is_ready					= true;
#endif

#ifdef DEBUG
#	ifdef USE_DEBUGGER
		if(!debugger() || !debugger()->Active())
#	endif
			lua_sethook					(lua(), lua_hook_call, LUA_MASKLINE|LUA_MASKCALL|LUA_MASKRET, 0);
#endif

	bool								save = m_reload_modules;
	m_reload_modules					= true;
	process_file_if_exists				("_g",false);
	m_reload_modules					= save;

	register_script_classes();
	object_factory().register_script();

#ifdef XRGAME_EXPORTS
	load_common_scripts();
#endif
	m_stack_level						= lua_gettop(lua());
}

size_t CScriptEngine::LoadScriptList()
{
	scriptsList_.clear();

	FS_FileSet fset;
	FS.file_list(fset, "$game_scripts$", FS_ListFiles, "*.script");

	auto fit = fset.begin();
	auto fit_e = fset.end();

	for (; fit != fit_e; ++fit)
	{
		string_path	fn1, fn2;
		_splitpath((*fit).name.c_str(), 0, fn1, fn2, 0);

		FS.update_path(fn1, "$game_scripts$", fn1);
		strconcat(sizeof(fn1), fn1, fn1, fn2, ".script");

		scriptsList_.insert(std::make_pair(xr_string(fn2), xr_string(fn1)));
	}

	return scriptsList_.size();
}

const CLocatorAPI::file* CScriptEngine::GetScriptFile(LPCSTR script_name)
{
	R_ASSERT(script_name);

	string128 script_name_lwr;
	xr_strcpy(script_name_lwr, script_name);

	xr_strlwr(script_name_lwr);

	LPSTR _ext = strext(script_name_lwr);

	if (_ext && (!stricmp(_ext, ".script")))
		*_ext = 0;

	ScriptsList::iterator it = scriptsList_.find(xr_string(script_name_lwr));

	if (it != scriptsList_.end())
	{
		//Msg("Found %s %s", script_name_lwr, it->second.c_str());

		return FS.exist(it->second.c_str());
	}
	else
		R_ASSERT2(false, make_string("Script not found: %s", script_name_lwr));

	return nullptr;
}

void CScriptEngine::remove_script_process	(const EScriptProcessors &process_id)
{
	CScriptProcessStorage::iterator	I = m_script_processes.find(process_id);

	if (I != m_script_processes.end())
	{
		xr_delete((*I).second);

		m_script_processes.erase(I);
	}
}

void CScriptEngine::load_common_scripts()
{
#ifdef DBG_DISABLE_SCRIPTS
	return;
#endif

	string_path S;

	FS.update_path(S, "$game_config$", "script.ltx");

	Msg("# Script initialization config file is %s", S);

	R_ASSERT(FS.exist(S));

	CInifile* l_tpIniFile = xr_new <CInifile>(S);

	R_ASSERT(l_tpIniFile);

	if (!l_tpIniFile->section_exist("common"))
	{
		Msg("! Warning: Script initialization config file %s is mising section [common], which is used to initialize start up scripts and important script pointers, like class_registrator.script", S);

		xr_delete(l_tpIniFile);

		return;
	}

	if (l_tpIniFile->line_exist("common", "script"))
	{
		LPCSTR caScriptString = l_tpIniFile->r_string("common", "script");

		u32	n = _GetItemCount(caScriptString);
		string256 I;

		for (u32 i = 0; i < n; ++i)
		{
			process_file(_GetItem(caScriptString, i, I));
			xr_strcat(I, "_initialize");

			if (object("_G", I, LUA_TFUNCTION))
			{
//				lua_dostring			(lua(),strcat(I,"()"));
				luabind::functor<void>	f;

				R_ASSERT				(functor(I, f));

				f();
			}
		}
	}
	else
	{
		Msg("! Warning: Script initialization config file %s is mising 'script' parametre, which is used to initialize start up scripts like _G.script", S);
	}

	xr_delete(l_tpIniFile);
}

void CScriptEngine::process_file_if_exists(LPCSTR file_name, bool warn_if_not_exist)
{
	R_ASSERT(file_name);

	if (m_reload_modules || !namespace_loaded(file_name))
	{
		string128 script_name_lwr;
		xr_strcpy(script_name_lwr, file_name);

		xr_strlwr(script_name_lwr);

		ScriptsList::iterator it = scriptsList_.find(xr_string(script_name_lwr));

		if (it != scriptsList_.end())
		{
			Msg("* Loading script: %s | Using path: %s", file_name, it->second.c_str());

			m_reload_modules = false;

			load_file_into_namespace(it->second.c_str(), xr_strcmp(file_name, "_g") == 0 ? "_G" : file_name);
		}
		else if(warn_if_not_exist)
		{
			R_ASSERT2(false, make_string("Loading of %s script failed. Script is not registered or does not exist", file_name));
		}
	}
}

void CScriptEngine::process_file	(LPCSTR file_name)
{
	process_file_if_exists	(file_name, true);
}

void CScriptEngine::process_file	(LPCSTR file_name, bool reload_modules)
{
	m_reload_modules		= reload_modules;
	process_file_if_exists	(file_name, true);
	m_reload_modules		= false;
}

void CScriptEngine::register_script_classes()
{
#ifdef DBG_DISABLE_SCRIPTS
	return;
#endif
	string_path	S;

	FS.update_path(S, "$game_config$", "script.ltx");

	Msg("# Script Class Registrtion: Script initialization config file is %s", S);

	R_ASSERT2(FS.exist(S), make_string("cfg file %s is missing", S));

	CInifile* l_tpIniFile = xr_new <CInifile>(S);

	R_ASSERT(l_tpIniFile);

	if (!l_tpIniFile->section_exist("common"))
	{
		Msg("! Warning: Script initialization config file %s is mising section [common], which is used to initialize start up scripts and important script pointers, like class_registrator.script", S);

		xr_delete(l_tpIniFile);

		return;
	}

	if (!l_tpIniFile->line_exist("common", "class_registrators"))
		R_ASSERT2(false, make_string("Engine Can't initialize scripted classes, since the %s is missing 'class_registrators' list parametr"));


	m_class_registrators = l_tpIniFile->r_string("common", "class_registrators");

	xr_delete(l_tpIniFile);

	u32	n = _GetItemCount(*m_class_registrators);

	string256 I;

	for (u32 i=0; i<n; ++i)
	{
		_GetItem(*m_class_registrators,i,I);

		luabind::functor<void>	result;
		if (!functor(I,result))
		{
			script_log(eLuaMessageTypeError,"Cannot load class registrator %s!",I);

			continue;
		}

		result(const_cast<CObjectFactory*>(&object_factory()));
	}
}

bool CScriptEngine::function_object(LPCSTR function_to_call, luabind::object &object, int type)
{
	if (!xr_strlen(function_to_call))
	{
		Msg("ScriptEngine: Funciton Object: FALSE");
		return(false);
	}

	string256				name_space, function;

	parse_script_namespace(function_to_call, name_space, 256, function, 256);

	if (xr_strcmp(name_space, "_G"))
		process_file(name_space);


	if (!this->object(name_space, function, type))
	{
		Msg("ScriptEngine: Funciton Object: FALSE 2 (%s, %s)", name_space, function);
		return(false);
	}

	luabind::object lua_namespace = this->name_space(name_space);
	object = lua_namespace[function];

	return(true);
}

#ifdef USE_DEBUGGER
void CScriptEngine::stopDebugger()
{
	if (debugger())
	{
		xr_delete(m_scriptDebugger);

		Msg("Script debugger succesfully stoped.");
	}
	else
		Msg("Script debugger not present.");
}

void CScriptEngine::restartDebugger()
{
	if(debugger())
		stopDebugger();

	m_scriptDebugger = xr_new <CScriptDebugger>();
	debugger()->PrepareLuaBind();

	Msg("Script debugger succesfully restarted.");
}
#endif

void CScriptEngine::collect_all_garbage()
{
	lua_gc(lua(), LUA_GCCOLLECT, 0);
	lua_gc(lua(), LUA_GCCOLLECT, 0);
}
