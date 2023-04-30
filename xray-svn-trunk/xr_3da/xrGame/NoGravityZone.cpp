#include "stdafx.h"
#include "nogravityzone.h"
#include "../../xrphysics/physicsshell.h"
#include "entity_alive.h"
#include "PHMovementControl.h"
#include "../../xrphysics/iPhWorld.h"
#include "CharacterPhysicsSupport.h"

CNoGravityZone::CNoGravityZone()
{
	p_inside_gravity = 0.001f;
	p_impulse_factor = 0.01f;
}

void CNoGravityZone::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);
	p_inside_gravity = READ_IF_EXISTS(pSettings, r_float, section, "inside_gravity", 0.001f);
	p_impulse_factor = READ_IF_EXISTS(pSettings, r_float, section, "impulse_factor", 0.01f);
}

void CNoGravityZone::enter_Zone(SZoneObjectInfo& io)
{
	inherited::enter_Zone(io);
	switchGravity(io, false);
}

void CNoGravityZone::exit_Zone(SZoneObjectInfo& io)
{
	switchGravity(io, true);
	inherited::exit_Zone(io);
}

void CNoGravityZone::UpdateWorkload(u32 dt)
{
	OBJECT_INFO_VEC_IT i=m_ObjectInfoMap.begin(),e=m_ObjectInfoMap.end();

	for(; e!=i; i++)
		switchGravity(*i, false);
}

void CNoGravityZone::switchGravity(SZoneObjectInfo& io, bool val)
{
	if(io.object->getDestroy())
		return;

	CPhysicsShellHolder* sh = smart_cast<CPhysicsShellHolder*>(io.object);

	if(!sh)
		return;

	CPhysicsShell* shell = sh->PPhysicsShell();

	if(shell && shell->isActive())
	{

		if (val)
			shell->SetGravity_K(1.0f);
		else
			shell->SetGravity_K(p_inside_gravity);

		if(!val && shell->get_ApplyByGravity())
		{
			CPhysicsElement* e = shell->get_ElementByStoreOrder(u16(Random.randI(0, shell->get_ElementsNumber())));
			if(e->isActive())
			{
				e->applyImpulseTrace(Fvector().random_point(e->getRadius()), Fvector().random_dir(), shell->getMass() * physics_world()->Gravity() * p_impulse_factor, e->m_SelfID);
			}
		}
		//shell->SetAirResistance(0.f, 0.f);
		//shell->set_DynamicScales(1.f);

		return;
	}

	if(!io.nonalive_object)
	{
		CEntityAlive* ea = smart_cast<CEntityAlive*>(io.object);
		CPHMovementControl* mc = ea->character_physics_support()->movement();

		if (val)
			mc->SetCharGravityK(1.0f);
		else
			mc->SetCharGravityK(p_inside_gravity);

		mc->SetForcedPhysicsControl(!val);

		if(!val && mc->Environment()==CPHMovementControl::peOnGround)
		{
			Fvector gn;
			mc->GroundNormal(gn);
			mc->ApplyImpulse(gn, mc->GetMass() * physics_world()->Gravity() * p_impulse_factor);
		}
	}
}