////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder.cpp
//	Created 	: 26.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script objects binder
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_space.h"
#include "script_engine.h"
#include "script_binder.h"
#include "xrServer_Objects_ALife.h"
#include "script_binder_object.h"
#include "script_game_object.h"
#include "gameobject.h"
#include "level.h"

//#define DBG_DISABLE_SCRIPTS

extern void NotifyLUAErrorScriptCall(LPCSTR caMessage, bool message_box);

CScriptBinder::CScriptBinder		()
{
	init					();
}

CScriptBinder::~CScriptBinder		()
{
	VERIFY					(!m_object);
}

void CScriptBinder::init			()
{
	m_object				= 0;
}

void CScriptBinder::clear			()
{
	Msg("Exception catched for [%s]", smart_cast<CGameObject*>(this) ? *smart_cast<CGameObject*>(this)->ObjectName() : "");
	
	NotifyLUAErrorScriptCall("Script binder error", true);

	LogStackTrace("");

	try {
		xr_delete			(m_object);
	}
	catch(...) {
		m_object			= 0;
	}

	init					();
}

void CScriptBinder::reinit			()
{
#ifdef DEBUG_MEMORY_MANAGER
	u32									start = 0;
	if (g_bMEMO)
		start							= Memory.mem_usage();
#endif // DEBUG_MEMORY_MANAGER
	if (m_object) {
		m_object->reinit	();
	}
#ifdef DEBUG_MEMORY_MANAGER
	if (g_bMEMO) {
//		lua_gc				(ai().script_engine().lua(),LUA_GCCOLLECT,0);
//		lua_gc				(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		Msg					("CScriptBinder::reinit() : %d",Memory.mem_usage() - start);
	}
#endif // DEBUG_MEMORY_MANAGER
}

void CScriptBinder::LoadCfg			(LPCSTR section)
{
}

void CScriptBinder::reload			(LPCSTR section)
{
#ifdef DEBUG_MEMORY_MANAGER
	u32									start = 0;
	if (g_bMEMO)
		start							= Memory.mem_usage();
#endif // DEBUG_MEMORY_MANAGER
#ifndef DBG_DISABLE_SCRIPTS
	VERIFY					(!m_object);
	if (!pSettings->line_exist(section,"script_binding"))
		return;
	
	luabind::functor<void>	lua_function;
	if (!ai().script_engine().functor(pSettings->r_string(section,"script_binding"),lua_function)) {
		ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"function %s is not loaded!",pSettings->r_string(section,"script_binding"));
		return;
	}
	
	CGameObject				*game_object = smart_cast<CGameObject*>(this);

	lua_function		(game_object ? game_object->lua_game_object() : 0);

	if (m_object) {
		m_object->reload(section);
	}
#endif
#ifdef DEBUG_MEMORY_MANAGER
	if (g_bMEMO) {
//		lua_gc				(ai().script_engine().lua(),LUA_GCCOLLECT,0);
//		lua_gc				(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		Msg					("CScriptBinder::reload() : %d",Memory.mem_usage() - start);
	}
#endif // DEBUG_MEMORY_MANAGER
}

BOOL CScriptBinder::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
#ifdef DEBUG_MEMORY_MANAGER
	u32									start = 0;
	if (g_bMEMO)
		start							= Memory.mem_usage();
#endif // DEBUG_MEMORY_MANAGER
	CSE_Abstract			*abstract = (CSE_Abstract*)data_containing_so;
	CSE_ALifeObject			*object = smart_cast<CSE_ALifeObject*>(abstract);
	if (object && m_object) {
		return			((BOOL)m_object->SpawnAndImportSOData(object));
	}

#ifdef DEBUG_MEMORY_MANAGER
	if (g_bMEMO) {
//		lua_gc				(ai().script_engine().lua(),LUA_GCCOLLECT,0);
//		lua_gc				(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		Msg					("CScriptBinder::SpawnAndImportSOData() : %d",Memory.mem_usage() - start);
	}
#endif // DEBUG_MEMORY_MANAGER

	return					(TRUE);
}

void CScriptBinder::DestroyClientObj()
{
	if (m_object) {
#ifdef _DEBUG
		Msg						("* Core object %s is UNbinded from the script object",smart_cast<CGameObject*>(this) ? *smart_cast<CGameObject*>(this)->ObjectName() : "");
#endif // _DEBUG
		m_object->DestroyClientObj();
	}
	xr_delete				(m_object);
}

void CScriptBinder::set_object		(CScriptBinderObject *object)
{
	R_ASSERT2			(!m_object, "Object is already binded!");
	m_object			= object;
}

void CScriptBinder::ScheduledUpdate(u32 time_delta)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	if (m_object)
	{
		m_object->ScheduledUpdate(time_delta);
	}


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_ScriptBinder_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CScriptBinder::save			(NET_Packet &output_packet)
{
	if (m_object) {
		m_object->save	(&output_packet);
	}
}

void CScriptBinder::load			(IReader &input_packet)
{
	if (m_object) {
		m_object->load	(&input_packet);
	}
}

BOOL CScriptBinder::net_SaveRelevant()
{
	if (m_object) {
		return			(m_object->net_SaveRelevant());
	}
	return							(FALSE);
}

void CScriptBinder::RemoveLinksToCLObj(CObject *object)
{
	CGameObject						*game_object = smart_cast<CGameObject*>(object);
	if (m_object && game_object) {
		m_object->RemoveLinksToCLObj(game_object->lua_game_object());
	}
}
