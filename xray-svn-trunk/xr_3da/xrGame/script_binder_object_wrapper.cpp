////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder_object_wrapper.cpp
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object binder wrapper
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_binder_object_wrapper.h"
#include "script_game_object.h"
#include "xrServer_Objects_ALife.h"

CScriptBinderObjectWrapper::CScriptBinderObjectWrapper	(CScriptGameObject *object) :
	CScriptBinderObject	(object)
{
}

CScriptBinderObjectWrapper::~CScriptBinderObjectWrapper ()
{
}

void CScriptBinderObjectWrapper::reinit					()
{
	luabind::call_member<void>		(this,"reinit");
}

void CScriptBinderObjectWrapper::reinit_static			(CScriptBinderObject *script_binder_object)
{
	script_binder_object->CScriptBinderObject::reinit	();
}

void CScriptBinderObjectWrapper::reload					(LPCSTR section)
{
	luabind::call_member<void>		(this,"reload",section);
}

void CScriptBinderObjectWrapper::reload_static			(CScriptBinderObject *script_binder_object, LPCSTR section)
{
	script_binder_object->CScriptBinderObject::reload	(section);
}

bool CScriptBinderObjectWrapper::SpawnAndImportSOData(SpawnType data_containing_so)
{
	return(luabind::call_member<bool>(this,"net_spawn",data_containing_so));
}

bool CScriptBinderObjectWrapper::SpawnAndImportSOData_static(CScriptBinderObject *script_binder_object, SpawnType data_containing_so)
{
	return(script_binder_object->CScriptBinderObject::SpawnAndImportSOData(data_containing_so));
}

void CScriptBinderObjectWrapper::DestroyClientObj()
{
	luabind::call_member<void>		(this,"net_destroy");
}

void CScriptBinderObjectWrapper::DestroyClientObj_static		(CScriptBinderObject *script_binder_object)
{
	script_binder_object->CScriptBinderObject::DestroyClientObj();
}

void CScriptBinderObjectWrapper::ExportDataToServer(NET_Packet *net_packet)
{
	luabind::call_member<void>(this,"ExportDataToServer",net_packet);
}

void CScriptBinderObjectWrapper::ExportDataToServer_static(CScriptBinderObject *script_binder_object, NET_Packet *net_packet)
{
	script_binder_object->CScriptBinderObject::ExportDataToServer(net_packet);
}

#include "GameObject.h"

extern ENGINE_API BOOL logPerfomanceProblems_;

void CScriptBinderObjectWrapper::ScheduledUpdate(u32 time_delta)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	luabind::call_member<void>(this, "update", time_delta);


#ifdef MEASURE_UPDATES

	float time = measure_sc_update.GetElapsed_sec();
	Device.Statistic->scheduler_ScriptBinderObjectWrapper_ += time;

	if (logPerfomanceProblems_ && time * 1000.f > 1.f)
		Msg("!Perfomance Warning: Script binder update of %s took %f ms", *m_object->ObjectName(), time * 1000.f);

#endif
}

void CScriptBinderObjectWrapper::ScheduledUpdate_static	(CScriptBinderObject *script_binder_object, u32 time_delta)
{
	script_binder_object->CScriptBinderObject::ScheduledUpdate	(time_delta);
}

void CScriptBinderObjectWrapper::save					(NET_Packet *output_packet)
{
	luabind::call_member<void>		(this,"save",output_packet);
}

void CScriptBinderObjectWrapper::save_static			(CScriptBinderObject *script_binder_object, NET_Packet *output_packet)
{
	script_binder_object->CScriptBinderObject::save		(output_packet);
}

void CScriptBinderObjectWrapper::load					(IReader *input_packet)
{
	luabind::call_member<void>		(this,"load",input_packet);
}

void CScriptBinderObjectWrapper::load_static			(CScriptBinderObject *script_binder_object, IReader *input_packet)
{
	script_binder_object->CScriptBinderObject::load		(input_packet);
}

bool CScriptBinderObjectWrapper::net_SaveRelevant		()
{
	return							(luabind::call_member<bool>(this,"net_save_relevant"));
}

bool CScriptBinderObjectWrapper::net_SaveRelevant_static(CScriptBinderObject *script_binder_object)
{
	return							(script_binder_object->CScriptBinderObject::net_SaveRelevant());
}

void CScriptBinderObjectWrapper::RemoveLinksToCLObj(CScriptGameObject *object)
{
	luabind::call_member<void>		(this,"net_Relcase",object);
}

void CScriptBinderObjectWrapper::RemoveLinksToCLObj_static(CScriptBinderObject *script_binder_object, CScriptGameObject *object)
{
	script_binder_object->CScriptBinderObject::RemoveLinksToCLObj(object);
}