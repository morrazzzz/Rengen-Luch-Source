#include "stdafx.h"
#include "grenade.h"
#include "../../xrphysics/PhysicsShell.h"
#include "entity.h"
#include "actor.h"
#include "inventory.h"
#include "level.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "xrserver_objects_alife.h"
#include "UI/UIInventoryUtilities.h"
#include "UIGameCustom.h"
#include "UI/UIInventoryWnd.h"
#include "GameConstants.h"

#define GRENADE_REMOVE_TIME		30000
const float default_grenade_detonation_threshold_hit=100;
CGrenade::CGrenade(void) 
{

	m_eSoundCheckout = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);

	nextGrenade_ = nullptr;
}

CGrenade::~CGrenade(void) 
{
}

void CGrenade::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);
	CExplosive::LoadCfg(section);

	m_sounds.LoadSound(section, "snd_checkout", "sndCheckout", 0, false, m_eSoundCheckout);

	//////////////////////////////////////
	//время убирания оружия с уровня
	if(pSettings->line_exist(section,"grenade_remove_time"))
		m_dwGrenadeRemoveTime = pSettings->r_u32(section,"grenade_remove_time");
	else
		m_dwGrenadeRemoveTime = GRENADE_REMOVE_TIME;
	m_grenade_detonation_threshold_hit=READ_IF_EXISTS(pSettings,r_float,section,"detonation_threshold_hit",default_grenade_detonation_threshold_hit);
}

void CGrenade::Hit					(SHit* pHDS)
{
	if( ALife::eHitTypeExplosion==pHDS->hit_type && m_grenade_detonation_threshold_hit<pHDS->damage()&&CExplosive::Initiator()==u16(-1)) 
	{
		CExplosive::SetCurrentParentID(pHDS->who->ID());
		Destroy();
	}
	inherited::Hit(pHDS);
}

BOOL CGrenade::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	m_dwGrenadeIndependencyTime			= 0;
	
	BOOL ret= inherited::SpawnAndImportSOData(data_containing_so);
	
	Fvector box;BoundingBox().getsize	(box);
	float max_size						= _max(_max(box.x,box.y),box.z);
	box.set								(max_size,max_size,max_size);
	box.mul								(3.f);
	CExplosive::SetExplosionSize		(box);
	m_thrown							= false;
	return								ret;
}

void CGrenade::DestroyClientObj() 
{
	inherited::DestroyClientObj();
	CExplosive::DestroyClientObj();
}

void CGrenade::BeforeDetachFromParent(bool just_before_destroy) 
{
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void CGrenade::AfterDetachFromParent() 
{
	m_dwGrenadeIndependencyTime			= Level().timeServer();
	inherited::AfterDetachFromParent		();	
}

void CGrenade::AfterAttachToParent()
{
	m_dwGrenadeIndependencyTime			= 0;
	m_dwDestroyTime						= 0xffffffff;
	inherited::AfterAttachToParent		();
}

void CGrenade::State(u32 state) 
{
	switch (state)
	{
	case eThrowStart:
		{
			Fvector						C;
			Center						(C);
			PlaySound					("sndCheckout", C);
		}break;
	case eThrowEnd:
		{
			if (!m_thrown)
			{
				Throw();

				Msg("! Who broke hud motion marks in grenade anims?");
			}

			if(m_thrown)
			{
				if (m_pPhysicsShell)	m_pPhysicsShell->Deactivate();
				xr_delete				(m_pPhysicsShell);
				m_dwDestroyTime			= 0xffffffff;
				PutNextToSlot		();

				#ifdef DEBUG
				Msg("Destroying local grenade[%d][%d]",ID(), CurrentFrame());
				#endif

				DestroyObject		();
				
			};
		}break;
	};
	inherited::State(state);
}

bool CGrenade::DropGrenade()
{
	EMissileStates grenade_state = static_cast<EMissileStates>(GetState());
	if (((grenade_state == eThrowStart) ||
		(grenade_state == eReady) ||
		(grenade_state == eThrow)) &&
		(!m_thrown)
		)
	{
		Throw();
		return true;
	}
	return false;
}

void CGrenade::DiscardState()
{
	if(GetState()==eReady || GetState()==eThrow)
		OnStateSwitch(eIdle);
}

void CGrenade::SendHiddenItem						()
{
	if (GetState()==eThrow)
	{
		Throw				();
	}
	CActor* pActor = smart_cast<CActor*>( m_pCurrentInventory->GetOwner());
	if (pActor && (GetState()==eReady || GetState()==eThrow))
	{
		return;
	}

	inherited::SendHiddenItem();
}


void CGrenade::Throw() 
{
	if (!m_fake_missile || m_thrown)
		return;

	CGrenade					*pGrenade = smart_cast<CGrenade*>(m_fake_missile);
	VERIFY						(pGrenade);
	
	if (pGrenade) {
		pGrenade->set_destroy_time(m_dwDestroyTimeMax);
		//установить ID того кто кинул гранату
		pGrenade->SetInitiator( H_Parent()->ID() );
	}
	inherited::Throw			();
	m_fake_missile->processing_activate();//@sliph
	m_thrown = true;
}

void CGrenade::Destroy() 
{
	//Generate Expode event
	Fvector						normal;

	if(m_destroy_callback)
	{
		m_destroy_callback		(this);
		m_destroy_callback	=	destroy_callback(NULL);
	}

	FindNormal					(normal);
	CExplosive::GenExplodeEvent	(Position(), normal);
}



bool CGrenade::Useful() const
{

	bool res = (/* !m_throw && */ m_dwDestroyTime == 0xffffffff && CExplosive::Useful() && TestServerFlag(CSE_ALifeObject::flCanSave));

	return res;
}

void CGrenade::OnEvent(NET_Packet& P, u16 type) 
{
	inherited::OnEvent			(P,type);
	CExplosive::OnEvent			(P,type);
}

void CGrenade::PutNextToSlot()
{
	VERIFY(!getDestroy());

	//выкинуть гранату из инвентаря
	if (m_pCurrentInventory)
	{
		m_pCurrentInventory->MoveToRuck(this);
	}
	else
		Msg("! PutNextToSlot : m_pInventory = NULL [%d][%d]", ID(), CurrentFrame());

	if (smart_cast<CInventoryOwner*>(H_Parent()) && m_pCurrentInventory)
	{
		u16 actor_id = Level().CurrentEntity() ? Level().CurrentEntity()->ID() : 65535;

		CGrenade *pNext = smart_cast<CGrenade*>(m_pCurrentInventory->GetNotSameBySect(SectionNameStr(), this, m_pCurrentInventory->GetOwner()->object_id() == actor_id ? m_pCurrentInventory->belt_ : m_pCurrentInventory->ruck_));
		
		if(!pNext) 
			pNext = smart_cast<CGrenade*>(m_pCurrentInventory->GetNotSameBySlotNum(GRENADE_SLOT, this, m_pCurrentInventory->GetOwner()->object_id() == actor_id ? m_pCurrentInventory->belt_ : m_pCurrentInventory->ruck_));

		VERIFY(pNext != this);
		

		if (pNext && m_pCurrentInventory->MoveToSlot(pNext->BaseSlot(), pNext))
		{
			m_pCurrentInventory->SetActiveSlot(pNext->BaseSlot());
			pNext->ActivateItem();

			if (CurrentGameUI()->m_InventoryMenu->IsShown())
				CurrentGameUI()->m_InventoryMenu->ReinitInventoryWnd(); // needed to update belt cell items
		}
		else
		{
			CActor* pActor = smart_cast<CActor*>( m_pCurrentInventory->GetOwner());

			if(pActor)
				pActor->OnPrevWeaponSlot();
		}

		m_thrown				= false;
	}
}

void CGrenade::OnAnimationEnd(u32 state) 
{
	switch(state)
	{
	case eThrowEnd: 
		SwitchState(eHidden);
		break;

	case eHiding:
	{
		// If grenade gets hidden - put it back to Belt or to Ruck. It gets to slot again, when its activated again (in Inventory::TryActivate)
		// Only For Actor
		u16 actor_id = Level().CurrentEntity() ? Level().CurrentEntity()->ID() : 65535;

		if(m_pCurrentInventory->GetOwner()->object_id() == actor_id)
			RemoveSelfFromSlot();

		if (nextGrenade_)
		{
			if (m_pCurrentInventory->MoveToSlot(nextGrenade_->BaseSlot(), nextGrenade_, true))
			{
				m_pCurrentInventory->SetActiveSlot(nextGrenade_->BaseSlot());
				nextGrenade_->ActivateItem();
				nextGrenade_ = nullptr;
			}
			else
				nextGrenade_ = nullptr;
		}

		inherited::OnAnimationEnd(state);
		break; 
	}

	default : 
		inherited::OnAnimationEnd(state);
	}
}


void CGrenade::UpdateCL() 
{
	inherited::UpdateCL();
	CExplosive::UpdateCL();
}


bool CGrenade::Action(u16 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;

	switch(cmd) 
	{
	//переключение типа гранаты
	case kWPN_NEXT:
		{
            if(flags&CMD_START) 
			{
				if(m_pCurrentInventory)
				{
					CGrenade* grenade = GetNextGrenadeType();

					if (grenade && xr_strcmp(grenade->SectionName(), SectionName()))
					{
						nextGrenade_ = grenade;
						m_pCurrentInventory->ActivateSlot(NO_ACTIVE_SLOT);

						return true;
					}

					return true;
				}
			}
			return true;
		};
	}
	return false;
}

BOOL CGrenade::UsedAI_Locations()
{
	return inherited::UsedAI_Locations();
}

void CGrenade::RemoveLinksToCLObj(CObject* O )
{
	CExplosive::RemoveLinksToCLObj(O);
	inherited::RemoveLinksToCLObj(O);
}

void CGrenade::DeactivateItem()
{
	//Drop grenade if primed
	StopCurrentAnimWithoutCallback();
	if ( !GetTmpPreDestroy() && ( GetState()==eThrowStart || GetState()==eReady || GetState()==eThrow ) )
	{
		if (m_fake_missile)
		{
			CGrenade*		pGrenade	= smart_cast<CGrenade*>(m_fake_missile);
			if (pGrenade)
			{
				if (m_pCurrentInventory->GetOwner())
				{
					CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
					if (pActor)
					{
						if (!pActor->g_Alive())
						{
							m_constpower			= false;
							m_fThrowForce			= 0;
						}
					}
				}				
				Throw					();
			};
		};
	};

	inherited::DeactivateItem();
}

bool CGrenade::GetBriefInfo(II_BriefInfo& info)
{
	u32 same_grnds_count = m_pCurrentInventory->GetSameItemCountBySlot(GRENADE_SLOT, m_pCurrentInventory->belt_); //Calculate grenades on Belt
	if (m_pCurrentInventory->ItemFromSlot(GRENADE_SLOT))
		same_grnds_count += 1; // Add one that is in hands

	string16 stmp;
	xr_sprintf(stmp, "%d", same_grnds_count);

	info.name = NameShort();
	info.cur_ammo = stmp;
	info.section = SectionName();

	return true;
}

CGrenade* CGrenade::GetNextGrenadeType()
{
	xr_vector<shared_str>* known_types = &GameConstants::GetHandGrandesTypes();

	u32 grenade_id = 0;
	for (u32 i = 0; i < known_types->size(); ++i) // find current_grenade index in list
	{
		if (!xr_strcmp(known_types->at(i), SectionName()))
		{
			grenade_id = i;
			break;
		}
	}

	PIItem item = nullptr;
	for (u32 i = 0; i < known_types->size(); i++) // try to find next grenade type in inventory
	{
		grenade_id += 1;

		if (grenade_id >= known_types->size())
		{
			grenade_id = 0;
		}

		item = m_pCurrentInventory->GetItemBySect(known_types->at(grenade_id).c_str(), m_pCurrentInventory->belt_);
		if (item)
		{
			break;
		}

	}

	if (!item)
		return 0;

	CGrenade* grenade = smart_cast<CGrenade*>(item);

	return grenade;
}

void CGrenade::RemoveSelfFromSlot()
{
	TSlotId next_active_slot = m_pCurrentInventory->GetNextActiveSlot();

	bool in_belt_ok = m_pCurrentInventory->MoveToBelt(this);
	if (!in_belt_ok)
		m_pCurrentInventory->MoveToRuck(this);

	if (CurrentGameUI()->m_InventoryMenu->IsShown())
		CurrentGameUI()->m_InventoryMenu->ReinitInventoryWnd(); // needed to update belt cell items

	if (next_active_slot != NO_ACTIVE_SLOT && next_active_slot != GRENADE_SLOT) m_pCurrentInventory->ActivateSlot(next_active_slot); // because moving grenade out of slot causes NO_ACTIVE_SLOT to be activated... In case we need to activate, for example, weapon slot
}