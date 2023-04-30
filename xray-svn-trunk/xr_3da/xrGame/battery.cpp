#include "stdafx.h"
#include "battery.h"
#include "Actor.h"
#include "torch.h"

CBattery::CBattery()
{
}

CBattery::~CBattery()
{
}

BOOL CBattery::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	return		(inherited::SpawnAndImportSOData(data_containing_so));
}

void CBattery::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);
}

void CBattery::DestroyClientObj() 
{
	inherited::DestroyClientObj();
}

void CBattery::ScheduledUpdate(u32 dt) 
{
	inherited::ScheduledUpdate(dt);

}

void CBattery::UpdateCL() 
{
	inherited::UpdateCL();
}


void CBattery::AfterAttachToParent() 
{
	inherited::AfterAttachToParent();
}

void CBattery::BeforeDetachFromParent(bool just_before_destroy) 
{
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void CBattery::renderable_Render(IRenderBuffer& render_buffer) 
{
	inherited::renderable_Render(render_buffer);
}

bool CBattery::UseBy (CEntityAlive* entity_alive)
{
	CInventoryOwner *IO				= smart_cast<CInventoryOwner*>(entity_alive);
	CActor			*actor			= NULL;
	
	R_ASSERT						(IO);

	actor							= smart_cast<CActor*>(IO);
	R_ASSERT						(actor);

	CTorch *flashlight = actor->GetCurrentTorch();
	if (!flashlight) return false;

	actor->RechargeTorchBattery();

	bool used = inherited::UseBy(entity_alive);
	return used;
}

bool CBattery::Empty() const
{
	CTorch *flashlight = Actor()->GetCurrentTorch();
	if (!flashlight)
		return false;

	return inherited::Empty();
}
