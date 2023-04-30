#include "stdafx.h"
#include "HudItem.h"
#include "physic_item.h"
#include "actor.h"
#include "Missile.h"
#include "xrmessages.h"
#include "level.h"
#include "inventory.h"
#include "../CameraBase.h"
#include "player_hud.h"
#include "../SkeletonMotions.h"
#include "weaponmiscs.h"

CHudItem::CHudItem()
{
	EnableHudInertion			(TRUE);
	AllowHudInertion			(TRUE);
	m_bStopAtEndAnimIsRunning	= false;
	m_current_motion_def		= NULL;
	m_started_rnd_anim_idx		= u8(-1);
}

DLL_Pure *CHudItem::_construct()
{
	m_object			= smart_cast<CPhysicItem*>(this);
	VERIFY				(m_object);

	m_item				= smart_cast<CInventoryItem*>(this);
	VERIFY				(m_item);

	return				(m_object);
}

CHudItem::~CHudItem()
{
}

void CHudItem::LoadCfg(LPCSTR section)
{
	hud_sect				= pSettings->r_string		(section,"hud");
	m_animation_slot		= pSettings->r_u32			(section,"animation_slot");

	m_sounds.LoadSound(section, "snd_bore", "sndBore", 0, true);

	EnableHudCollide(!!READ_IF_EXISTS(pSettings,r_bool,hud_sect,"apply_collide",TRUE));
	EnableHudStrafeInertion(!!READ_IF_EXISTS(pSettings,r_bool,hud_sect,"apply_strafe_inertion",FALSE));
	EnableHudBobbing(!!READ_IF_EXISTS(pSettings,r_bool,hud_sect,"apply_bobbing",FALSE));
}

SSnd* CHudItem::PlaySound(LPCSTR alias, const Fvector& position, float freq)
{
	return m_sounds.PlaySound(alias, position, object().H_Root(), !!GetHUDmode(), false, u8(-1), freq);
}

void CHudItem::renderable_Render(IRenderBuffer& render_buffer)
{
#pragma todo("MT Warning: better to move this call to main thread. May be into updateCL?")
	UpdateXForm();

	BOOL _hud_render = render_buffer.isForHud_ && GetHUDmode();
	
	if(_hud_render && !IsHidden())
	{ 
	}
	else 
	{
		if (!object().H_Parent() || (!_hud_render && !IsHidden()))
		{
			on_renderable_Render(render_buffer);
			//debug_draw_firedeps(); // Mt bugs here
		}
		else if (object().H_Parent()) 
		{
			CInventoryOwner* owner = smart_cast<CInventoryOwner*>(object().H_Parent());

			VERIFY(owner);

			CInventoryItem* self = smart_cast<CInventoryItem*>(this);

			if (owner->attached(self) || (owner->NeedToRenderSecordary() && (item().BaseSlot() == RIFLE_SLOT || item().BaseSlot() == RIFLE_2_SLOT)))
				on_renderable_Render(render_buffer);
		}
	}
}

void CHudItem::SwitchState(u32 S)
{
	SetNextState(S);

	if (!object().getDestroy())	
		OnStateSwitch(S);
}

void CHudItem::OnEvent(NET_Packet& P, u16 type)
{
}

void CHudItem::OnStateSwitch(u32 S)
{
	SetState(S);

	switch (S)
	{
	case eBore:
		SetPending(FALSE);

		PlayAnimBore();
		if(HudItemData())
		{
			Fvector P = HudItemData()->m_item_transform.c;

			m_sounds.PlaySound("sndBore", P, object().H_Root(), !!GetHUDmode(), false, m_started_rnd_anim_idx);
		}

		break;
	}
}

void CHudItem::PlayAnimBore()
{
	PlayHUDMotion("anm_bore", TRUE, this, GetState());
}

bool CHudItem::ActivateItem()
{
	OnActiveItem();

	return true;
}

void CHudItem::DeactivateItem()
{
	OnHiddenItem();
}

void CHudItem::OnMoveToRuck()
{
	SwitchState(eHidden);
}

void CHudItem::SendDeactivateItem	()
{
	SendHiddenItem();
}

void CHudItem::SendHiddenItem()
{
	if (!object().getDestroy())
	{
		OnStateSwitch(eHiding);
	}
}

void CHudItem::UpdateHudAdditonal(Fmatrix& hud_trans)
{
}

void CHudItem::UpdateCL()
{
	if(m_current_motion_def)
	{
		if(m_bStopAtEndAnimIsRunning)
		{
			const xr_vector<motion_marks>& marks = m_current_motion_def->marks;

			if(!marks.empty())
			{
				float motion_prev_time = ((float)m_dwMotionCurrTm - (float)m_dwMotionStartTm)/1000.0f;
				float motion_curr_time = ((float)EngineTimeU() - (float)m_dwMotionStartTm)/1000.0f;
				
				xr_vector<motion_marks>::const_iterator it = marks.begin();
				xr_vector<motion_marks>::const_iterator it_e = marks.end();

				for(; it != it_e; ++it)
				{
					const motion_marks&	M = (*it);

					if(M.is_empty())
						continue;
	
					const motion_marks::interval* Iprev = M.pick_mark(motion_prev_time);
					const motion_marks::interval* Icurr = M.pick_mark(motion_curr_time);

					if(Iprev == NULL && Icurr != NULL /* || M.is_mark_between(motion_prev_time, motion_curr_time)*/)
					{
						OnMotionMark				(m_startedMotionState, M);
					}
				}
			
			}

			m_dwMotionCurrTm = EngineTimeU();

			if(m_dwMotionCurrTm > m_dwMotionEndTm)
			{
				m_current_motion_def				= NULL;
				m_dwMotionStartTm					= 0;
				m_dwMotionEndTm						= 0;
				m_dwMotionCurrTm					= 0;
				m_bStopAtEndAnimIsRunning			= false;

				OnAnimationEnd(m_startedMotionState);
			}
		}
	}
}

void CHudItem::AfterAttachToParent()
{}

void CHudItem::BeforeAttachToParent()
{
	StopCurrentAnimWithoutCallback();
}

void CHudItem::BeforeDetachFromParent(bool just_before_destroy)
{
	m_sounds.StopAllSounds(DONT_STOP_SOUNDS);

	UpdateXForm();
}

void CHudItem::AfterDetachFromParent()
{
	if(HudItemData())
		g_player_hud->detach_item(this);

	StopCurrentAnimWithoutCallback();
}

void CHudItem::on_b_hud_detach()
{
	m_sounds.StopAllSounds(DONT_STOP_SOUNDS);
}

void CHudItem::on_a_hud_attach()
{
	if(m_current_motion_def)
	{
		PlayHUDMotion_noCB(m_current_motion, FALSE);
	}
	else
	{

	}
}

u32 CHudItem::PlayHUDMotion(const shared_str& M, BOOL bMixIn, CHudItem*  W, u32 state)
{
	u32 anim_time = PlayHUDMotion_noCB(M, bMixIn);

	if (anim_time > 0)
	{
		m_bStopAtEndAnimIsRunning	= true;
		m_dwMotionStartTm			= EngineTimeU();
		m_dwMotionCurrTm			= m_dwMotionStartTm;
		m_dwMotionEndTm				= m_dwMotionStartTm + anim_time;
		m_startedMotionState		= state;
	}
	else
		m_bStopAtEndAnimIsRunning	= false;

	return anim_time;
}


u32 CHudItem::PlayHUDMotion_noCB(const shared_str& motion_name, BOOL bMixIn)
{
	m_current_motion = motion_name;

	if(bDebug && item().m_pCurrentInventory)
	{
		Msg("-[%s] as[%d] [%d]anim_play [%s][%d]",
			HudItemData()?"HUD":"Simulating", 
			item().m_pCurrentInventory->GetActiveSlot(), 
			item().object_id(),
			motion_name.c_str(), 
			CurrentFrame());
	}
	if( HudItemData() )
	{
		return HudItemData()->anim_play		(motion_name, bMixIn, m_current_motion_def, m_started_rnd_anim_idx);
	}else
	{
		m_started_rnd_anim_idx				= 0;
		return g_player_hud->motion_length	(motion_name, HudSection(), m_current_motion_def );
	}
}

void CHudItem::StopCurrentAnimWithoutCallback()
{
	m_dwMotionStartTm			= 0;
	m_dwMotionEndTm				= 0;
	m_dwMotionCurrTm			= 0;
	m_bStopAtEndAnimIsRunning	= false;
	m_current_motion_def		= NULL;
}

BOOL CHudItem::GetHUDmode()
{
	if(object().H_Parent())
	{
		CActor* A = smart_cast<CActor*>(object().H_Parent());

		return (A && A->HUDview() && HudItemData() && HudItemData());
	}
	else
		return FALSE;
}

void CHudItem::PlayAnimIdle()
{
	if (TryPlayAnimIdle())
		return;

	PlayHUDMotion("anm_idle", TRUE, NULL, GetState());
}

bool CHudItem::TryPlayAnimIdle()
{
	if(MovingAnimAllowedNow())
	{
		CActor* pActor = smart_cast<CActor*>(object().H_Parent());

		if(pActor)
		{
			CEntity::SEntityState st;
			pActor->g_State(st);

			if(st.bSprint)
			{
				PlayAnimIdleSprint();

				return true;
			}
			else if(!st.bCrouch && pActor->AnyMove())
			{
				PlayAnimIdleMoving();

				return true;
			}
		}
	}

	return false;
}

void CHudItem::PlayAnimIdleMoving()
{
	if (!HudBobbingEnabled())
		PlayHUDMotion("anm_idle_moving", TRUE, NULL, GetState());
	else
		PlayHUDMotion("anm_idle", TRUE, NULL, GetState());
}

void CHudItem::PlayAnimIdleSprint()
{
	PlayHUDMotion("anm_idle_sprint", TRUE, NULL,GetState());
}

void CHudItem::OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd)
{
	if(GetState()==eIdle && !m_bStopAtEndAnimIsRunning)
	{
		if((cmd == ACTOR_DEFS::mcSprint) || (cmd == ACTOR_DEFS::mcAnyMove))
		{
			PlayAnimIdle();
			ResetSubStateTime();
		}
	}
}

attachable_hud_item* CHudItem::HudItemData()
{
	attachable_hud_item* hi = NULL;

	if(!g_player_hud)		
		return hi;

	hi = g_player_hud->attached_item(0);

	if (hi && hi->m_parent_hud_item == this)
		return hi;

	hi = g_player_hud->attached_item(1);

	if (hi && hi->m_parent_hud_item == this)
		return hi;

	return NULL;
}

Fvector& CHudItem::hands_offset_pos()
{
	attachable_hud_item* hi = HudItemData();
	R_ASSERT(hi);

	u8 idx = GetCurrentHudOffsetIdx();

	return hi->m_measures.m_hands_offset_ironsight[0][idx];
}

Fvector& CHudItem::hands_offset_rot()
{
	attachable_hud_item* hi = HudItemData();
	R_ASSERT(hi);

	u8 idx = GetCurrentHudOffsetIdx();

	return hi->m_measures.m_hands_offset_ironsight[1][idx];
}

