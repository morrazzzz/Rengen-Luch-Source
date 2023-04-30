#include "stdafx.h"
#include "bolt.h"
#include "../../xrphysics/PhysicsShell.h"
#include "level.h"
#include "GameConstants.h"

CBolt::CBolt(void) 
{
	m_weight					= .1f;
	m_baseSlot						= BOLT_SLOT;
	m_flags.set					(Fruck, FALSE);
	m_thrower_id				=u16(-1);
}

CBolt::~CBolt(void) 
{
}

void CBolt::AfterAttachToParent() 
{
	inherited::AfterAttachToParent();
	
	CObject* o= H_Parent()->H_Parent();
	
	if(o)
		SetInitiator(o->ID());
}

void CBolt::Throw() 
{
	CMissile					*l_pBolt = smart_cast<CMissile*>(m_fake_missile);
	if(!l_pBolt)				return;
	l_pBolt->set_destroy_time	(u32(m_dwDestroyTimeMax/phTimefactor));
	inherited::Throw			();
	spawn_fake_missile			();
}

bool CBolt::Useful() const
{
	return false;
}

bool CBolt::Action(u16 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;
/*
	switch(cmd) 
	{
	case kDROP:
		{
			if(flags&CMD_START) 
			{
				m_throw = false;
				if(State() == MS_IDLE) State(MS_THREATEN);
			} 
			else if(State() == MS_READY || State() == MS_THREATEN) 
			{
				m_throw = true; 
				if(State() == MS_READY) State(MS_THROW);
			}
		} 
		return true;
	}
*/
	return false;
}

void CBolt::activate_physic_shell	()
{
	inherited::activate_physic_shell	();
	m_pPhysicsShell->SetAirResistance	(.0001f);
}

void CBolt::SetInitiator			(u16 id)
{
	m_thrower_id=id;
}

u16	CBolt::Initiator				()
{
	return m_thrower_id;
}

bool CBolt::CheckDestruction()
{
	if (!H_Parent() && getVisible() && m_pPhysicsShell)
	{
		if (m_dwDestroyTime <= Level().timeServer())
		{
			if (Device.GetDistFromCamera(Position()) > GameConstants::GetDontRemoveBoltDist())
			{
				m_dwDestroyTime = 0xffffffff;
				VERIFY(!m_pCurrentInventory);

				Destroy();
			}

			return true;
		}
	}

	return false;
}