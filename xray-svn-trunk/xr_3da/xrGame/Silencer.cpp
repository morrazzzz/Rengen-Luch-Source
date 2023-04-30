///////////////////////////////////////////////////////////////
// Silencer.cpp
// Silencer - апгрейд оружия глушитель 
///////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "silencer.h"
//#include "PhysicsShell.h"

CSilencer::CSilencer()
{
}

CSilencer::~CSilencer() 
{
}

BOOL CSilencer::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	return(inherited::SpawnAndImportSOData(data_containing_so));
}

void CSilencer::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);
}

void CSilencer::DestroyClientObj() 
{
	inherited::DestroyClientObj();
}

void CSilencer::UpdateCL() 
{
	inherited::UpdateCL();
}

void CSilencer::AfterAttachToParent() 
{
	inherited::AfterAttachToParent();
}

void CSilencer::BeforeDetachFromParent(bool just_before_destroy) 
{
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void CSilencer::renderable_Render(IRenderBuffer& render_buffer) 
{
	inherited::renderable_Render(render_buffer);
}