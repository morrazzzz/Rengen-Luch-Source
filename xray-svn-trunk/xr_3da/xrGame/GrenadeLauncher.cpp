///////////////////////////////////////////////////////////////
// GrenadeLauncher.cpp
// GrenadeLauncher - апгрейд оружия поствольный гранатомет
///////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "grenadelauncher.h"
//#include "../../xrphysics/PhysicsShell.h"

CGrenadeLauncher::CGrenadeLauncher()
{
	m_fGrenadeVel = 0.f;
}

CGrenadeLauncher::~CGrenadeLauncher() 
{
}

BOOL CGrenadeLauncher::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	return		(inherited::SpawnAndImportSOData(data_containing_so));
}

void CGrenadeLauncher::LoadCfg(LPCSTR section) 
{
	m_fGrenadeVel = pSettings->r_float(section, "grenade_vel");
	inherited::LoadCfg(section);
}

void CGrenadeLauncher::DestroyClientObj() 
{
	inherited::DestroyClientObj();
}

void CGrenadeLauncher::UpdateCL() 
{
	inherited::UpdateCL();
}


void CGrenadeLauncher::AfterAttachToParent() 
{
	inherited::AfterAttachToParent();
}

void CGrenadeLauncher::BeforeDetachFromParent(bool just_before_destroy) 
{
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void CGrenadeLauncher::renderable_Render(IRenderBuffer& render_buffer) 
{
	inherited::renderable_Render(render_buffer);
}