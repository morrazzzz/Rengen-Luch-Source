////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object.cpp
//	Created 	: 06.10.2003
//  Modified 	: 14.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_object.h"

CScriptObject::CScriptObject			()
{
}

CScriptObject::~CScriptObject			()
{
}

DLL_Pure *CScriptObject::_construct		()
{
	CGameObject::_construct			();
	CScriptEntity::_construct		();
	return							(this);
}

void CScriptObject::reinit				()
{
	CScriptEntity::reinit			();
	CGameObject::reinit				();
}

BOOL CScriptObject::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	return	(
		CGameObject::SpawnAndImportSOData(data_containing_so) &&
		CScriptEntity::SpawnAndImportSOData(data_containing_so)
	);
}

void CScriptObject::DestroyClientObj()
{
	CGameObject::DestroyClientObj();
	CScriptEntity::DestroyClientObj();
}

BOOL CScriptObject::UsedAI_Locations	()
{
	return							(FALSE);
}

void CScriptObject::ScheduledUpdate(u32 DT)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	CGameObject::ScheduledUpdate(DT);
	CScriptEntity::ScheduledUpdate(DT);


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_ScriptObject_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CScriptObject::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	CGameObject::UpdateCL();
	CScriptEntity::UpdateCL();

	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_ScriptObject_ += measure_updatecl.GetElapsed_sec();
#endif
}
