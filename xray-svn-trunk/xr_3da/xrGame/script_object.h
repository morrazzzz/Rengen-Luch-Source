////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object.h
//	Created 	: 06.10.2003
//  Modified 	: 14.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "gameobject.h"
#include "script_entity.h"

class CScriptObject : 
	public CGameObject,
	public CScriptEntity
{
public:
								CScriptObject			();
	virtual						~CScriptObject			();
	
	virtual DLL_Pure			*_construct				();
	
	virtual	void				reinit					();
	
	virtual BOOL				SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void				DestroyClientObj		();
	
	virtual BOOL				UsedAI_Locations		();
	
	virtual void				ScheduledUpdate			(u32 DT);
	virtual void				UpdateCL				();
	
	virtual CScriptEntity*		cast_script_entity		()	{return this;}
};
