////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder_object_script.cpp
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object binder script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_binder_object.h"
#include "script_export_space.h"
#include "script_binder_object_wrapper.h"
#include "xrServer_Objects_ALife.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptBinderObject::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptBinderObject,CScriptBinderObjectWrapper>("object_binder")
			.def_readonly("object",				&CScriptBinderObject::m_object)
			.def(								constructor<CScriptGameObject*>())
			.def("reinit",						&CScriptBinderObject::reinit,			&CScriptBinderObjectWrapper::reinit_static)
			.def("reload",						&CScriptBinderObject::reload,			&CScriptBinderObjectWrapper::reload_static)
			.def("net_spawn",					&CScriptBinderObject::SpawnAndImportSOData, &CScriptBinderObjectWrapper::SpawnAndImportSOData_static)
			.def("net_destroy",					&CScriptBinderObject::DestroyClientObj,		&CScriptBinderObjectWrapper::DestroyClientObj_static)
			.def("ExportDataToServer",			&CScriptBinderObject::ExportDataToServer, 	&CScriptBinderObjectWrapper::ExportDataToServer_static)
			.def("update",						&CScriptBinderObject::ScheduledUpdate,	&CScriptBinderObjectWrapper::ScheduledUpdate_static)
			.def("save",						&CScriptBinderObject::save,				&CScriptBinderObjectWrapper::save_static)
			.def("load",						&CScriptBinderObject::load,				&CScriptBinderObjectWrapper::load_static)
			.def("net_save_relevant",			&CScriptBinderObject::net_SaveRelevant,	&CScriptBinderObjectWrapper::net_SaveRelevant_static)
			.def("net_Relcase",					&CScriptBinderObject::RemoveLinksToCLObj,	&CScriptBinderObjectWrapper::RemoveLinksToCLObj_static)
	];
}