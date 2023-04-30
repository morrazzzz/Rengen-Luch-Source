////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder_object.h
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object binder
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

class CSE_ALifeObject;
class CScriptGameObject;
class NET_Packet;

class CScriptBinderObject {
public:
	typedef CSE_ALifeObject*	SpawnType;
	CScriptGameObject			*m_object;

public:
						CScriptBinderObject	(CScriptGameObject *object);
	virtual				~CScriptBinderObject();
	virtual void		reinit				();
	virtual void		reload				(LPCSTR section);
	
	virtual bool		SpawnAndImportSOData(SpawnType data_containing_so);
	virtual void		DestroyClientObj	();
	virtual void		ExportDataToServer	(NET_Packet *net_packet);
	virtual bool		net_SaveRelevant	();
	virtual void		RemoveLinksToCLObj	(CScriptGameObject *object);
	
	virtual void		ScheduledUpdate		(u32 time_delta);
	
	virtual void		save				(NET_Packet *output_packet);
	virtual void		load				(IReader	*input_packet);


	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptBinderObject)
#undef script_type_list
#define script_type_list save_type_list(CScriptBinderObject)
