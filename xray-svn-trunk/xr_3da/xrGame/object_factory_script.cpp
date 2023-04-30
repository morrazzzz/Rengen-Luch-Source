////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory_script.cpp
//	Created 	: 27.05.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "object_factory.h"
#include "ai_space.h"
#include "script_engine.h"
#include "object_item_script.h"

void ReportPossibleCauses()
{
	Msg(LINE_SPACER);
	Msg("Possible causes:");
	Msg("Forgot to add function header file of your class into script_engine_export.h");
	Msg("Forgot to add function '::script_register(lua_State *L)' into your class cpp file");
	Msg("Forgot to add DECLARE_SCRIPT_REGISTER_FUNCTION or DECLARE_SCRIPT_REGISTER_FUNCTION_STRUCT into your class h file");
	Msg("Forgot to add: ");
	Msg(LINE_SPACER);
	Msg("add_to_type_list(Your class)");
	Msg("#undef script_type_list");
	Msg("#define script_type_list");
	Msg(LINE_SPACER);
	Msg(" into your class h file");
	Msg(LINE_SPACER);
}

void CObjectFactory::register_script_class	(LPCSTR client_class, LPCSTR server_class, LPCSTR clsid, LPCSTR script_clsid)
{
#ifdef DEBUG
	Msg("* register script class: %s %s %s %s", client_class, server_class, clsid, script_clsid);
#endif
	
#ifndef NO_XR_GAME
	luabind::object				client;
	if (!ai().script_engine().function_object(client_class, client, LUA_TUSERDATA))
	{
		Msg("! register script class: Fail");

		ReportPossibleCauses();

		ai().script_engine().script_log	(eLuaMessageTypeError,"Cannot register class %s",client_class);

		return;
	}
#endif
	luabind::object				server;
	if (!ai().script_engine().function_object(server_class, server, LUA_TUSERDATA))
	{
		Msg("! register script class: Fail");

		ReportPossibleCauses();

		ai().script_engine().script_log	(eLuaMessageTypeError, "Cannot register class %s", server_class);

		return;
	}
	
	add							(
		xr_new <CObjectItemScript>(
#ifndef NO_XR_GAME
			client,
#endif
			server,
			TEXT2CLSID(clsid),
			script_clsid
		)
	);
}

void CObjectFactory::register_script_class			(LPCSTR unknown_class, LPCSTR clsid, LPCSTR script_clsid)
{
#ifdef DEBUG
	Msg("* register script class: %s %s %s", unknown_class, clsid, script_clsid);
#endif
	luabind::object				creator;
	if (!ai().script_engine().function_object(unknown_class,creator,LUA_TUSERDATA))
	{
		Msg("! register script class: Fail");

		ReportPossibleCauses();

		ai().script_engine().script_log	(eLuaMessageTypeError,"Cannot register class %s",unknown_class);

		return;
	}
	add							(
		xr_new <CObjectItemScript>(
#ifndef NO_XR_GAME
			creator,
#endif
			creator,
			TEXT2CLSID(clsid),
			script_clsid
		)
	);
}

void CObjectFactory::register_script_classes()
{
	ai();
}

using namespace luabind;

struct CInternal{};

void CObjectFactory::register_script	() const
{
	actualize					();

	luabind::class_<CInternal>	instance("clsid");

	const_iterator				I = clsids().begin(), B = I;
	const_iterator				E = clsids().end();
#ifdef DEBUG
//	Msg("~ Exporting clsid......");
#endif
	for ( ; I != E; ++I)
	{
		instance.enum_			("_clsid")[luabind::value(*(*I)->script_clsid(),int(I - B))];
#ifdef DEBUG
//		Msg						("# %s = %d", *(*I)->script_clsid(), int(I - B));
#endif
	}
	luabind::module				(ai().script_engine().lua())[instance];
}

#pragma optimize("s",on)
void CObjectFactory::script_register(lua_State *L)
{
	module(L)
	[
		class_<CObjectFactory>("object_factory")
			.def("register",	(void (CObjectFactory::*)(LPCSTR,LPCSTR,LPCSTR,LPCSTR))(&CObjectFactory::register_script_class))
			.def("register",	(void (CObjectFactory::*)(LPCSTR,LPCSTR,LPCSTR))(&CObjectFactory::register_script_class))
	];
}
