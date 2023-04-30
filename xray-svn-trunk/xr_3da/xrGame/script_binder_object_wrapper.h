////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder_object_wrapper.h
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object binder wrapper
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_binder_object.h"
#include "script_game_object.h"

class CScriptGameObject;

class CScriptBinderObjectWrapper : public CScriptBinderObject, public luabind::wrap_base {
public:
						CScriptBinderObjectWrapper	(CScriptGameObject *object);
	virtual				~CScriptBinderObjectWrapper	();
	
	virtual void		reinit						();
	static  void		reinit_static				(CScriptBinderObject *script_binder_object);
	virtual void		reload						(LPCSTR section);
	static  void		reload_static				(CScriptBinderObject *script_binder_object, LPCSTR section);
	
	virtual bool		SpawnAndImportSOData		(SpawnType data_containing_so);
	static  bool		SpawnAndImportSOData_static	(CScriptBinderObject *script_binder_object, SpawnType data_containing_so);
	virtual void		DestroyClientObj			();
	static  void		DestroyClientObj_static		(CScriptBinderObject *script_binder_object);
	virtual void		ExportDataToServer			(NET_Packet *net_packet);
	static  void		ExportDataToServer_static	(CScriptBinderObject *script_binder_object, NET_Packet *net_packet);
	virtual void		RemoveLinksToCLObj			(CScriptGameObject *object);
	static	void		RemoveLinksToCLObj_static	(CScriptBinderObject *script_binder_object, CScriptGameObject *object);
	
	virtual void		ScheduledUpdate				(u32 time_delta);
	static  void		ScheduledUpdate_static		(CScriptBinderObject *script_binder_object, u32 time_delta);
	
	virtual void		save						(NET_Packet *output_packet);
	static	void		save_static					(CScriptBinderObject *script_binder_object, NET_Packet *output_packet);
	virtual void		load						(IReader *input_packet);
	static	void		load_static					(CScriptBinderObject *script_binder_object, IReader *input_packet);
	
	virtual bool		net_SaveRelevant			();
	static  bool		net_SaveRelevant_static		(CScriptBinderObject *script_binder_object);
};
