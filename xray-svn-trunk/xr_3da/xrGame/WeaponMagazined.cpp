#include "stdafx.h"
#include "WeaponMagazined.h"
#include "entity.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "level.h"
#include "object_broker.h"
#include "string_table.h"
#include "ui/UIXmlInit.h"
#include "ui/UIWindow.h"
#include "UIGameCustom.h"
#include "hudmanager.h"
#include "..\CameraBase.h"
#include "weaponmiscs.h"
#include "GameConstants.h"

CUIXml*				pWpnScopeXml = NULL;

extern int sndMaxShotSounds_;

void createWpnScopeXML()
{
	if(!pWpnScopeXml)
	{
		pWpnScopeXml			= xr_new <CUIXml>();
		pWpnScopeXml->Load		(CONFIG_PATH, UI_PATH, "scopes.xml");
	}
}

CWeaponMagazined::CWeaponMagazined(ESoundTypes eSoundType) : CWeapon()
{
	m_eSoundShow		= ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
	m_eSoundHide		= ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
	m_eSoundShot		= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
	m_eSoundEmptyClick	= ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
	m_eSoundReload		= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);

	m_sounds_enabled			= true;

	m_sSilencerFlameParticles = m_sSilencerSmokeParticles = NULL;

	m_bFireSingleShot = false;
	m_iShotNum = 0;
	m_iQueueSize = WEAPON_ININITE_QUEUE;
	m_bLockType = false;

	m_chamber = false;

	hasDistantShotSnd_ = false;
}

CWeaponMagazined::~CWeaponMagazined()
{
}

void CWeaponMagazined::DestroyClientObj()
{
	inherited::DestroyClientObj();
}


void CWeaponMagazined::LoadCfg	(LPCSTR section)
{
	inherited::LoadCfg		(section);
		
	// Sounds
	m_sounds.LoadSound(section, "snd_draw", "sndShow", 0, false, m_eSoundShow);
	m_sounds.LoadSound(section, "snd_holster", "sndHide", 0, false, m_eSoundHide);
	m_sounds.LoadSound(section, "snd_shoot", "sndShot", (u8)sndMaxShotSounds_, false, m_eSoundShot);

	if (pSettings->line_exist(section, "snd_shoot_dist")) // distant sound
	{
		m_sounds.LoadSound(section, "snd_shoot_dist", "sndShotDist", (u8)sndMaxShotSounds_, false, m_eSoundShot);
		hasDistantShotSnd_ = true;
	}

	m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", 0, false, m_eSoundEmptyClick);
	m_sounds.LoadSound(section, "snd_reload", "sndReload", 0, true, m_eSoundReload);

	if (pSettings->line_exist(section, "snd_reload_empty"))
		m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", 0, true, m_eSoundReload);
	else
		m_sounds.LoadSound(section, "snd_reload", "sndReloadEmpty", 0, true, m_eSoundReload);

	//звуки и партиклы глушителя, еслит такой есть
	if (pSettings->line_exist(section, "silencer_flame_particles"))
	{
		m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
	}
	if (pSettings->line_exist(section, "silencer_smoke_particles"))
	{
		m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");
	}
	if (pSettings->line_exist(section, "snd_silncer_shot"))
	{
		m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", (u8)sndMaxShotSounds_, false, m_eSoundShot);
	}

	m_iRecoilStartShotNum = READ_IF_EXISTS(pSettings, r_u8, section, "dispersion_start", 1);
	m_fNoRecoilTimeToFire = READ_IF_EXISTS(pSettings, r_float, section, "rpm_no_disp", 0);
	m_fNoRecoilTimeToFire = (m_fNoRecoilTimeToFire > 0.f) 
		? 60.f / m_fNoRecoilTimeToFire
		: fOneShotTime;

	m_fTimeToFireSemi = READ_IF_EXISTS(pSettings, r_float, section, "rpm_semi", 0);
	m_fTimeToFireSemi = (m_fTimeToFireSemi > 0.f) 
		? 60.f / m_fTimeToFireSemi
		: fOneShotTime;

	m_fShotMaxDelay = READ_IF_EXISTS(pSettings, r_float, section, "shot_max_delay", 0);

	m_conditionDecreasePerQueueShot = READ_IF_EXISTS(pSettings, r_float, section, "condition_queue_shot_dec", conditionDecreasePerShot);

	Fvector3 nullVec = {0.f, 0.f, 0.f};
	m_vFireDirectionOffset = READ_IF_EXISTS(pSettings, r_fvector3, section, "fire_direction_offset", nullVec);

	if (pSettings->line_exist(section, "fire_modes"))
	{
		m_bHasDifferentFireModes = true;
		shared_str FireModesList = pSettings->r_string(section, "fire_modes");
		int ModesCount = _GetItemCount(FireModesList.c_str());
		m_aFireModes.clear();
		for (int i=0; i<ModesCount; i++)
		{
			string16 sItem;
			_GetItem(FireModesList.c_str(), i, sItem);
			int FireMode = atoi(sItem);
			m_aFireModes.push_back(FireMode);			
		}
		m_iCurFireMode = ModesCount - 1;
		m_iPrefferedFireMode = READ_IF_EXISTS(pSettings, r_s16,section,"preffered_fire_mode",-1);
	}
	else
	{ 
		m_aFireModes.clear();
		m_aFireModes.push_back(1);
		m_iCurFireMode = (1+m_aFireModes.size()) % m_aFireModes.size();
		m_bHasDifferentFireModes = true;
	}
	LoadSilencerKoeffs();

	m_bChamberStatus = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_chamber", FALSE);
}

void CWeaponMagazined::FireStart		()
{
	if(!IsMisfire())
	{
		if(IsValid()) 
		{
			if(!IsWorking() || AllowFireWhileWorking())
			{
				if(GetState()==eReload) return;
				if(GetState()==eShowing) return;
				if(GetState()==eHiding) return;
				if(GetState()==eMisfire) return;

				inherited::FireStart();
				
				if (iAmmoElapsed == 0) 
					OnMagazineEmpty();
				else
					SwitchState(eFire);
			}
		}else 
		{
			if(eReload!=GetState()) 
				OnMagazineEmpty();
		}
	}else
	{//misfire
		if(smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity()==H_Parent()) )
		{
			SDrawStaticStruct* s	= HUD().GetGameUI()->AddCustomStatic("gun_jammed", true);
			s->m_endTime = EngineTime() + 3.0f;// 3sec
		}

		OnEmptyClick();
	}
}

void CWeaponMagazined::FireEnd() 
{
	if (!IsPending())
	{
		if (m_iShotNum > 0)
		{
			// induce semi-auto delay after stopped shooting (eg. fire button unpressed)
			fShotTimeCounter = m_fTimeToFireSemi;

			// Make sure recoil is applied even if stopped firing before RecoilStartShotNum
			if (GetCurrentFireMode() == -1 && m_iShotNum < m_iRecoilStartShotNum)
			{
				AddShotEffector();
			}
		}
		inherited::FireEnd();

		CActor	*actor = smart_cast<CActor*>(H_Parent());
		if (m_pCurrentInventory && !iAmmoElapsed && actor && GetState() != eReload)
		{
			if (actor != g_actor || psActorFlags.is(AF_WPN_AUTORELOAD))
				Reload();
		}
	}
}

void CWeaponMagazined::Reload()
{
	inherited::Reload();

	TryReload();
}

bool CWeaponMagazined::TryReload() 
{
	if(m_pCurrentInventory) 
	{
		m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[m_ammoType]));

		
		if(IsMisfire() && iAmmoElapsed)
		{
			SetPending			(TRUE);
			SwitchState(eReload); 
			return true;
		}

		if(m_pCurrentAmmo || unlimited_ammo())
		{
			SetPending			(TRUE);
			SwitchState(eReload); 
			return true;
		} 
		else for(u8 i = 0; i < m_ammoTypes.size(); ++i) 
		{
			m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[i]));
			if(m_pCurrentAmmo)
			{ 
				m_ammoType = i; 
				SetPending			(TRUE);
				SwitchState(eReload);
				return true; 
			}
		}
	}

	if(GetState()!=eIdle)
		SwitchState(eIdle);

	return false;
}

bool CWeaponMagazined::IsAmmoAvailable()
{
	if (smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[m_ammoType])))
		return	(true);
	else
		for(u32 i = 0; i < m_ammoTypes.size(); ++i)
			if (smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[i])))
				return	(true);
	return		(false);
}

void CWeaponMagazined::OnMagazineEmpty() 
{
	//попытка стрелять когда нет патронов
	if(GetState() == eIdle) 
	{
		OnEmptyClick			();
		return;
	}

	if( GetNextState() != eMagEmpty && GetNextState() != eReload)
	{
		SwitchState(eMagEmpty);
	}

	inherited::OnMagazineEmpty();
}

void CWeaponMagazined::UnloadMagazine(bool spawn_ammo, u32 into_who_id, bool unload_secondary)
{
	UnloadMagazineEx(m_magazine, spawn_ammo, into_who_id);
}

void CWeaponMagazined::UnloadMagazineEx(xr_vector<CCartridge>& from_where, bool spawn_ammo, u32 into_who_id, bool decline_ammo_counter)
{
	xr_map<LPCSTR, u16> l_ammo;
	
	while (!from_where.empty())
	{
		CCartridge &l_cartridge = from_where.back();
		xr_map<LPCSTR, u16>::iterator l_it;
		for(l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it) 
		{
            if(!xr_strcmp(*l_cartridge.m_ammoSect, l_it->first)) 
            { 
				 ++(l_it->second); 
				 break; 
			}
		}

		if(l_it == l_ammo.end()) l_ammo[*l_cartridge.m_ammoSect] = 1;
		from_where.pop_back();

		if (decline_ammo_counter)
			--iAmmoElapsed;
	}

	if (decline_ammo_counter)
		R_ASSERT((u32)iAmmoElapsed == from_where.size());
	
	if (!spawn_ammo)
		return;

	CInventory* into_where = m_pCurrentInventory;

	if (into_who_id != u32(-1))
	{
		CObject* O = Level().Objects.net_Find(into_who_id);

		if (O)
		{
			CInventoryOwner* inv_owner = smart_cast<CInventoryOwner*>(O);

			if (inv_owner)
				into_where = &inv_owner->inventory();
		}
	}

	xr_map<LPCSTR, u16>::iterator l_it;

	for(l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it) 
	{
		if (into_where)
		{
			CWeaponAmmo *l_pA = smart_cast<CWeaponAmmo*>(into_where->GetAmmoFromInv(l_it->first));

			if(l_pA) 
			{
				u16 l_free = l_pA->m_boxSize - l_pA->m_boxCurr;
				l_pA->m_boxCurr = l_pA->m_boxCurr + (l_free < l_it->second ? l_free : l_it->second);
				l_it->second = l_it->second - (l_free < l_it->second ? l_free : l_it->second);
			}
		}

		if (l_it->second && !unlimited_ammo())
			SpawnAmmo(l_it->second, l_it->first, into_who_id);
	}
}

void CWeaponMagazined::ReloadMagazine() 
{
	m_BriefInfo_CalcFrame = 0;

	//устранить осечку при перезарядке
	if(IsMisfire())
		bMisfire = false;
	
	//переменная блокирует использование
	//только разных типов патронов

	if (!m_bLockType)
	{
		m_ammoName	= NULL;
		m_pCurrentAmmo = NULL;
	}
	
	if (!m_pCurrentInventory)
		return;

	if (m_changed_ammoType_on_reload != u8(-1)){
		m_ammoType = m_changed_ammoType_on_reload;
		SetAmmoOnReload(u8(-1));
	}

	if(!unlimited_ammo()) 
	{
		//попытаться найти в инвентаре патроны текущего типа 
		m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[m_ammoType]));
		
		if(!m_pCurrentAmmo && !m_bLockType)
		{
			for(u8 i = 0; i < m_ammoTypes.size(); ++i) 
			{
				//проверить патроны всех подходящих типов
				m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[i]));
				if(m_pCurrentAmmo)
				{ 
					m_ammoType = i; 
					break; 
				}
			}
		}
	}

	//нет патронов для перезарядки
	if(!m_pCurrentAmmo && !unlimited_ammo())
		return;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
		m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));

	CCartridge l_cartridge = m_DefaultCartridge;

	while (iAmmoElapsed < maxMagazineSize_)
	{
		if (!unlimited_ammo())
		{
			if (!m_pCurrentAmmo->Get(l_cartridge))
				break;
		}

		++iAmmoElapsed;

		l_cartridge.m_LocalAmmoType = u8(m_ammoType);
		m_magazine.push_back(l_cartridge);
	}

	m_ammoName = (m_pCurrentAmmo) ? m_pCurrentAmmo->m_nameShort : NULL;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	//выкинуть коробку патронов, если она пустая
	if(m_pCurrentAmmo && !m_pCurrentAmmo->m_boxCurr)
		m_pCurrentAmmo->SetDropManual(TRUE);

	if (maxMagazineSize_ > iAmmoElapsed) // if ammo box did not fully load magazine - try to find another ammo box
	{ 
		m_bLockType = true;
		ReloadMagazine();
		m_bLockType = false; 
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeaponMagazined::OnStateSwitch	(u32 S)
{
	inherited::OnStateSwitch(S);
	switch (S)
	{
	case eIdle:
		switch2_Idle	();
		break;
	case eFire:
		switch2_Fire	();
		break;
	case eMisfire:
		if(smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity()==H_Parent()) )
		{
			SDrawStaticStruct* s	= CurrentGameUI()->AddCustomStatic("gun_jammed", true);
			s->m_endTime = EngineTime() + 3.0f;// 3sec
		}
		break;
	case eMagEmpty:
		switch2_Empty	();
		break;
	case eReload:
		switch2_Reload	();
		break;
	case eShowing:
		switch2_Showing	();
		break;
	case eHiding:
		switch2_Hiding	();
		break;
	case eHidden:
		switch2_Hidden	();
		break;
	}
}

void CWeaponMagazined::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif
	
	
	inherited::UpdateCL	();

	float dt = TimeDelta();

	//когда происходит апдейт состояния оружия
	//ничего другого не делать
	if(GetNextState() == GetState())
	{
		switch (GetState())
		{
		case eShowing:
		case eHiding:
		case eReload:
		case eIdle:
			fShotTimeCounter -= dt;

			if (fShotTimeCounter < 0)
				fShotTimeCounter = 0;
			break;
		case eFire:			
			if(iAmmoElapsed > 0)
				state_Fire(dt);
			
			if(fShotTimeCounter <= 0)
			{
				if(iAmmoElapsed == 0)
					OnMagazineEmpty();

				StopShooting();
			}
			else
			{
				fShotTimeCounter -= dt;
			}

			break;

		case eMisfire:		state_Misfire	(dt);	break;
		case eMagEmpty:		state_MagEmpty	(dt);	break;
		case eHidden:		break;
		}
	}

	UpdateSounds();

	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CWeaponMagazined::UpdateSounds	()
{
	if (CurrentFrame() == dwUpdateSounds_Frame)
		return;
	
	dwUpdateSounds_Frame = CurrentFrame();

	Fvector P						= get_LastFP();
	m_sounds.SetPosition("sndShow", P);
	m_sounds.SetPosition("sndHide", P);
	m_sounds.SetPosition("sndReload", P);
	m_sounds.SetPosition("sndReloadEmpty", P);
}

void CWeaponMagazined::state_Fire	(float dt)
{
	if(iAmmoElapsed > 0)
	{
		VERIFY(fOneShotTime >0.f);

		if (!H_Parent()) return;

		Fvector p1, d;
		p1.set(get_LastFP());
		d.set(get_LastFD());

		CInventoryOwner* io		= smart_cast<CInventoryOwner*>(H_Parent());
		if(NULL == io->inventory().ActiveItem())
		{
				Log("current_state", GetState() );
				Log("next_state", GetNextState());
				Log("item_sect", SectionName().c_str());
				Log("H_Parent", H_Parent()->SectionName().c_str());
		}

		CEntity* E = smart_cast<CEntity*>(H_Parent());
		E->g_fireParams	(this, p1,d);

		if( !E->g_stateFire() )
			StopShooting();

		d.add(m_vFireDirectionOffset);

		int fireMode = GetCurrentFireMode();
		VERIFY(!m_magazine.empty());

		// NOTE: Queue size equals to fire mode (number of shots or -1 for full-auto) for Actor but different for NPC's (like number of shots in a row for semi-auto)
		while (!m_magazine.empty() && fShotTimeCounter <=0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
		{
				if( CheckForMisfire() )
				{
					StopShooting();
					return;
				}
			m_bFireSingleShot = false;

			VERIFY(fOneShotTime >0.f);
			VERIFY(m_fNoRecoilTimeToFire>0.f);
		
			++m_iShotNum;

			if (fireMode > 0 && (m_iShotNum % fireMode) == 0)
			{
				// last shot in controlled burst or semi-automatic mode
				fShotTimeCounter		+= m_fTimeToFireSemi;
				OnShot		(true);
			}
			else if (m_iShotNum < m_iRecoilStartShotNum)
			{
				// no-recoil shot (for Abakan)
				fShotTimeCounter += m_fNoRecoilTimeToFire;
				OnShot		(false);
			}
			else
			{
				// normal shot
				fShotTimeCounter += fOneShotTime;
				OnShot		(true);
			}
			FireTrace(p1, d);

			if (m_iShotNum == m_iQueueSize)
			{
				m_bStopedAfterQueueFired = true;
			}
		}

		UpdateSounds		();
	}
}

void CWeaponMagazined::state_Misfire	(float dt)
{
	OnEmptyClick			();
	SwitchState				(eIdle);
	
	bMisfire				= true;

	UpdateSounds			();
}

void CWeaponMagazined::state_MagEmpty	(float dt)
{
}

void CWeaponMagazined::SetDefaults	()
{
	CWeapon::SetDefaults		();
}


void CWeaponMagazined::OnShot		(bool hasRecoil)
{
	// Sound
	if (IsSilencerAttached() && SilencerAttachable()) //skyloader: dont touch SilencerAttachable(), it needs for pb, vss, val
		PlaySound("sndSilencerShot", get_LastFP(), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));
	else
	{
		if (hasDistantShotSnd_)
		{
			float dist_f = Position().distance_to(Device.vCameraPosition) / GameConstants::GetDistantSndApplyDistance();

			float valume_far = dist_f;
			clamp(valume_far, 0.1f, 1.f);

			float valume_close = 1.f - dist_f;
			clamp(valume_close, 0.1f, 1.f);

			if (valume_far > 0.1f)
			{
				SSnd* distant = PlaySound("sndShotDist", get_LastFP(), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));

				if (distant)
					distant->snd.set_volume(valume_far);
			}

			if (valume_close > 0.1f)
			{
				SSnd* close = PlaySound("sndShot", get_LastFP(), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));

				if (close)
					close->snd.set_volume(valume_close);
			}
		}
		else
			PlaySound("sndShot", get_LastFP(), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));
	}

	if (hasRecoil)
	{
		// Camera	
		AddShotEffector		();
	}

	// Animation
	PlayAnimShoot		();
	
	// Shell Drop
	Fvector vel; 
	PHGetLinearVell(vel);
	OnShellDrop					(get_LastSP(), vel);
	
	// Огонь из ствола
	StartFlameParticles	();

	//дым из ствола
	ForceUpdateFireParticles	();
	StartSmokeParticles			(get_LastFP(), vel);
}


void CWeaponMagazined::OnEmptyClick	()
{
	PlaySound("sndEmptyClick", get_LastFP(), Random.randF(MIN_RND_FREQ__EMPTY_SHOT, MAX_RND_FREQ__EMPTY_SHOT));
}

void CWeaponMagazined::OnAnimationEnd(u32 state) 
{
	switch(state)
	{
		case eReload:
		{
			Chamber();

			u32 ammo_in_chamber_pos = u32(-1);
			CCartridge ammo_in_chamber;

			if (iAmmoElapsed > 0) // store chambered bullet
			{
				ammo_in_chamber_pos = m_magazine.size() - 1;
				ammo_in_chamber = m_magazine.back();
			}

			ReloadMagazine();

			if (m_chamber && ammo_in_chamber_pos != u32(-1)) // move chambered bullet at front
			{
				m_magazine.erase(m_magazine.begin() + ammo_in_chamber_pos);
				m_magazine.push_back(ammo_in_chamber);
			}

			SwitchState(eIdle);

		} break;	// End of reload animation

		case eHiding:	SwitchState(eHidden);   break;	// End of Hide
		case eShowing:	SwitchState(eIdle);		break;	// End of Show
		case eIdle:		switch2_Idle();			break;  // Keep showing idle

	}
	inherited::OnAnimationEnd(state);
}
void CWeaponMagazined::switch2_Idle	()
{
	m_iShotNum = 0;
	SetPending			(FALSE);
	PlayAnimIdle		();
}

#ifdef DEBUG
#include "ai\stalker\ai_stalker.h"
#include "object_handler_planner.h"
#endif
void CWeaponMagazined::switch2_Fire	()
{
	CInventoryOwner* io		= smart_cast<CInventoryOwner*>(H_Parent());

#ifdef LOG_PLANNER
	CInventoryItem* ii = smart_cast<CInventoryItem*>(this);

	if (!io)
		return;
	//VERIFY2					(io,make_string("no inventory owner, item %s",*ObjectName()));

	if (ii != io->inventory().ActiveItem())
		Msg					("! not an active item, item %s, owner %s, active item %s",*ObjectName(),*H_Parent()->ObjectName(),io->inventory().ActiveItem() ? *io->inventory().ActiveItem()->object().ObjectName() : "no_active_item");

	if ( !(io && (ii == io->inventory().ActiveItem())) ) 
	{
		CAI_Stalker			*stalker = smart_cast<CAI_Stalker*>(H_Parent());
		if (stalker) {
			stalker->planner().show						();
			stalker->planner().show_current_world_state	();
			stalker->planner().show_target_world_state	();
		}
	}
#else
	if (!io)
		return;
#endif // DEBUG

//
//	VERIFY2(
//		io && (ii == io->inventory().ActiveItem()),
//		make_string(
//			"item[%s], parent[%s]",
//			*ObjectName(),
//			H_Parent() ? *H_Parent()->ObjectName() : "no_parent"
//		)
//	);

	m_bStopedAfterQueueFired = false;
	m_bFireSingleShot = DelayedShotIsAllowed();
	m_iShotNum = 0;
}

void CWeaponMagazined::switch2_Empty()
{
	if(psActorFlags.is(AF_WPN_AUTORELOAD))
	{
		OnZoomOut();

		if(!TryReload())
		{
			OnEmptyClick();
		}
		else
		{
			inherited::FireEnd();
		}
	}
}
void CWeaponMagazined::PlayReloadSound()
{
	if(m_sounds_enabled)
	{
		if(iAmmoElapsed==0)
			PlaySound	("sndReloadEmpty",get_LastFP());
		else
			PlaySound	("sndReload",get_LastFP());
	}
}

void CWeaponMagazined::switch2_Reload()
{
	CWeapon::FireEnd();

	PlayReloadSound	();
	PlayAnimReload	();
	SetPending			(TRUE);
}
void CWeaponMagazined::switch2_Hiding()
{
	OnZoomOut();
	CWeapon::FireEnd();
	
	if(m_sounds_enabled)
		PlaySound			("sndHide",get_LastFP());

	PlayAnimHide();
	SetPending			(TRUE);
}

void CWeaponMagazined::switch2_Hidden()
{
	CWeapon::FireEnd();

	StopCurrentAnimWithoutCallback();

	signal_HideComplete		();
	RemoveShotEffector		();
	RemoveZoomInertionEffector();
}

void CWeaponMagazined::switch2_Showing()
{
	if(m_sounds_enabled)
		PlaySound			("sndShow",get_LastFP());

	SetPending			(TRUE);
	PlayAnimShow		();
}

void CWeaponMagazined::Chamber()  //Kondr48 алгоритм 
{
	if (m_bChamberStatus == true && iAmmoElapsed >= 1 && m_chamber == false)
	{
		m_chamber = true;
		maxMagazineSize_++;
	}
	else if (m_bChamberStatus == true && iAmmoElapsed == 0 && m_chamber == true)
	{
		m_chamber = false;
		maxMagazineSize_--;
	}
}

bool CWeaponMagazined::Action(u16 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;
	
	//если оружие чем-то занято, то ничего не делать
	if(IsPending()) return false;
	
	switch(cmd) 
	{
	case kWPN_RELOAD:
		{
			if(flags&CMD_START) 
				if (iAmmoElapsed < maxMagazineSize_ || IsMisfire() || (m_bChamberStatus && !m_chamber && iAmmoElapsed == maxMagazineSize_))
					Reload();
		} 
		return true;
	case kWPN_FIREMODE_PREV:
		{
			if(flags&CMD_START) 
			{
				OnPrevFireMode();
				return true;
			};
		}break;
	case kWPN_FIREMODE_NEXT:
		{
			if(flags&CMD_START) 
			{
				OnNextFireMode();
				return true;
			};
		}break;
	}
	return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
	CScope*				pScope				= smart_cast<CScope*>(pIItem);
	CSilencer*			pSilencer			= smart_cast<CSilencer*>(pIItem);
	CGrenadeLauncher*	pGrenadeLauncher	= smart_cast<CGrenadeLauncher*>(pIItem);

	if(			pScope &&
				 m_eScopeStatus == ALife::eAddonAttachable &&
				(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
        {
		SCOPES_VECTOR_IT it = m_scopes.begin();
		for(; it!=m_scopes.end(); it++)
		{
			if(pSettings->r_string((*it),"scope_name")==pIItem->object().SectionName())
				return true;
		}
		return false;
	}
	else if(	pSilencer &&
				m_eSilencerStatus == ALife::eAddonAttachable &&
				(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
				(m_sSilencerName == pIItem->object().SectionName()) )
       return true;
	else if (	pGrenadeLauncher &&
				m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
				(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
				(m_sGrenadeLauncherName  == pIItem->object().SectionName()) )
		return true;
	else
		return inherited::CanAttach(pIItem);
}

bool CWeaponMagazined::CanDetach(const char* item_section_name)
{
	if( m_eScopeStatus == ALife::eAddonAttachable &&
	   0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope))
	{
		SCOPES_VECTOR_IT it = m_scopes.begin();
		for(; it!=m_scopes.end(); it++)
		{
			if(pSettings->r_string((*it),"scope_name")==item_section_name)
				return true;
		}
		return false;
	}
	else if(m_eSilencerStatus == ALife::eAddonAttachable &&
	   0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
	   (m_sSilencerName == item_section_name))
       return true;
	else if(m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
	   0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
	   (m_sGrenadeLauncherName == item_section_name))
       return true;
	else
		return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazined::Attach(PIItem pIItem, bool b_send_event)
{
	bool result = false;

	CScope*				pScope					= smart_cast<CScope*>(pIItem);
	CSilencer*			pSilencer				= smart_cast<CSilencer*>(pIItem);
	CGrenadeLauncher*	pGrenadeLauncher		= smart_cast<CGrenadeLauncher*>(pIItem);
	
	if(pScope &&
	   m_eScopeStatus == ALife::eAddonAttachable &&
	   (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
	{
		SCOPES_VECTOR_IT it = m_scopes.begin();
		for(; it!=m_scopes.end(); it++)
		{
			if(pSettings->r_string((*it),"scope_name")==pIItem->object().SectionName())
				m_cur_scope = u8(it-m_scopes.begin());
		}
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
		result = true;
	}
	else if(pSilencer &&
	   m_eSilencerStatus == ALife::eAddonAttachable &&
	   (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
	   (m_sSilencerName == pIItem->object().SectionName()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
		result = true;
	}
	else if(pGrenadeLauncher &&
	   m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
	   (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
	   (m_sGrenadeLauncherName == pIItem->object().SectionName()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
		result = true;
	}

	if(result)
	{
		if (b_send_event)
		{
			//уничтожить подсоединенную вещь из инвентаря
//.			pIItem->Drop					();
			pIItem->object().DestroyObject	();
		};

		UpdateAddonsVisibility();
		InitAddons();

		return true;
	}
	else
        return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazined::DetachScope(const char* item_section_name, bool b_spawn_item)
{
	bool detached = false;
	SCOPES_VECTOR_IT it = m_scopes.begin();
	for(; it!=m_scopes.end(); it++)
	{
		LPCSTR iter_scope_name = pSettings->r_string((*it),"scope_name");
		if(!xr_strcmp(iter_scope_name, item_section_name))
		{
			m_cur_scope = 0;
			detached = true;
		}
	}
	return detached;
}

bool CWeaponMagazined::Detach(const char* item_section_name, bool b_spawn_item)
{
	if(		m_eScopeStatus == ALife::eAddonAttachable &&
			DetachScope(item_section_name, b_spawn_item))
	{
		if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
		{
			Msg("ERROR: scope addon already detached.");
			return true;
		}
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonScope;
		
		UpdateAddonsVisibility();
		InitAddons();

		return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
	}
	else if(m_eSilencerStatus == ALife::eAddonAttachable &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
			(m_sSilencerName == item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;

		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
	}
	else if(m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
			(m_sGrenadeLauncherName == item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
	}
	else
		return inherited::Detach(item_section_name, b_spawn_item);;
}

#include "UIStaticItem.h"
#include "player_hud.h"
void CWeaponMagazined::InitAddons()
{

	bNeedUpdateAddons = true;

	if (IsScopeAttached())
	{
		if (m_eScopeStatus == ALife::eAddonAttachable)
		{
			shared_str scope_tex_name;

			scope_tex_name = pSettings->r_string(GetScopeName().c_str(), "scope_texture");
			m_zoom_params.m_fScopeZoomFactor = pSettings->r_float(GetScopeName().c_str(), "scope_zoom_factor");
			m_zoom_params.m_bUseDynamicZoom = READ_IF_EXISTS(pSettings, r_bool, GetScopeName().c_str(), "scope_dynamic_zoom", FALSE);

			m_zoom_params.m_sUseZoomPostprocess = READ_IF_EXISTS(pSettings, r_string, GetScopeName().c_str(), "scope_nightvision", 0);
			m_zoom_params.m_sUseBinocularVision = READ_IF_EXISTS(pSettings, r_string, GetScopeName().c_str(), "scope_alive_detector", 0);

			m_zoom_params.m_fSecondVPZoomFactor = READ_IF_EXISTS(pSettings, r_float, GetScopeName().c_str(), "svp_zoom_factor", 0.0f);
			m_zoom_params.m_fSecondVPWorldFOV	= READ_IF_EXISTS(pSettings, r_float, GetScopeName().c_str(), "svp_world_fov", 100.0f);

			scopeAttachBone = READ_IF_EXISTS(pSettings, r_string, GetScopeName().c_str(), "scope_attach_bone", "wpn_scope");
			fakeAttachScopeBone = READ_IF_EXISTS(pSettings, r_string, GetScopeName().c_str(), "fake_skeleton_bone", "wpn_body");

			scopeAttachOffset[0] = READ_IF_EXISTS(pSettings, r_fvector3, m_scopes[m_cur_scope].c_str(), "w_attach_pos", scopeAttachOffset[0]);
			scopeAttachOffset[1] = READ_IF_EXISTS(pSettings, r_fvector3, m_scopes[m_cur_scope].c_str(), "w_attach_rot", scopeAttachOffset[1]);

			if (m_UIScope)
			{
				xr_delete(m_UIScope);
			}

			m_UIScope = xr_new <CUIWindow>();
			createWpnScopeXML();

			CUIXmlInit::InitWindow(*pWpnScopeXml, scope_tex_name.c_str(), 0, m_UIScope);
		}
	}
	else
	{
		m_zoom_params.m_fSecondVPZoomFactor = 0.0f;

		if (m_UIScope)
		{
			xr_delete(m_UIScope);
		}
	}

	if ( IsSilencerAttached() && SilencerAttachable()) //skyloader: dont touch SilencerAttachable(), it needs for pb, vss, val
	{
		m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
		m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;

		//подсветка от выстрела
		LoadLights(*SectionName(), "silencer_");
		ApplySilencerKoeffs();

		silencerAttachBone = READ_IF_EXISTS(pSettings, r_string, GetSilencerName().c_str(), "silencer_attach_bone", "wpn_silencer");

		silencerAttachOffset[0] = READ_IF_EXISTS(pSettings, r_fvector3, m_scopes[m_cur_scope].c_str(), "sil_w_attach_pos", silencerAttachOffset[0]);
		silencerAttachOffset[1] = READ_IF_EXISTS(pSettings, r_fvector3, m_scopes[m_cur_scope].c_str(), "sil_w_attach_rot", silencerAttachOffset[1]);

		m_iSilencerX = READ_IF_EXISTS(pSettings, r_s32, GetSilencerName().c_str(), "silencer_x", 0);
		m_iSilencerY = READ_IF_EXISTS(pSettings, r_s32, GetSilencerName().c_str(), "silencer_y", 0);
	}
	else
	{
		m_sFlameParticlesCurrent = m_sFlameParticles;
		m_sSmokeParticlesCurrent = m_sSmokeParticles;

		//подсветка от выстрела
		LoadLights(*SectionName(), "");
		ResetSilencerKoeffs();
	}

	inherited::InitAddons();
}

void CWeaponMagazined::LoadSilencerKoeffs()
{
	if (m_eSilencerStatus == ALife::eAddonAttachable)
	{
		LPCSTR sect = m_sSilencerName.c_str();
		m_silencer_koef.hit_power = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_hit_power_k", 1.0f);
		m_silencer_koef.hit_impulse = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_hit_impulse_k", 1.0f);
		m_silencer_koef.bullet_speed = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_speed_k", 1.0f);
		m_silencer_koef.fire_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "fire_dispersion_base_k", 1.0f);
		m_silencer_koef.cam_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_k", 1.0f);
		m_silencer_koef.cam_disper_inc = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_inc_k", 1.0f);
	}

	clamp(m_silencer_koef.hit_power, 0.0f, 1.0f);
	clamp(m_silencer_koef.hit_impulse, 0.0f, 1.0f);
	clamp(m_silencer_koef.bullet_speed, 0.0f, 1.0f);
	clamp(m_silencer_koef.fire_dispersion, 0.0f, 3.0f);
	clamp(m_silencer_koef.cam_dispersion, 0.0f, 1.0f);
	clamp(m_silencer_koef.cam_disper_inc, 0.0f, 1.0f);
}

void CWeaponMagazined::ApplySilencerKoeffs()
{
	cur_silencer_koef = m_silencer_koef;
}

void CWeaponMagazined::ResetSilencerKoeffs()
{
	cur_silencer_koef.Reset();
}

//виртуальные функции для проигрывания анимации HUD
void CWeaponMagazined::PlayAnimShow()
{
	VERIFY(GetState()==eShowing);
	if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_show_empty"))
		PlayHUDMotion("anm_show_empty", FALSE, this, GetState());
	else
		PlayHUDMotion("anm_show", FALSE, this, GetState());		
}

void CWeaponMagazined::PlayAnimHide()
{
	VERIFY(GetState()==eHiding);
	if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_hide_empty"))
		PlayHUDMotion("anm_hide_empty", TRUE, this, GetState());
	else
		PlayHUDMotion("anm_hide", TRUE, this, GetState());
}

void CWeaponMagazined::PlayAnimBore()
{
	if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_bore_empty"))
		PlayHUDMotion	("anm_bore_empty", TRUE, this, GetState());
	else
		inherited::PlayAnimBore();
}

void CWeaponMagazined::PlayAnimReload()
{
	VERIFY(GetState()==eReload);
	if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_reload_empty"))
	{
		PlayHUDMotion("anm_reload_empty", TRUE, this, GetState());
	}else
	{
		PlayHUDMotion("anm_reload", TRUE, this, GetState());
	}
}

void CWeaponMagazined::PlayAnimAim()
{
	if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_idle_aim_empty"))
		PlayHUDMotion("anm_idle_aim_empty", TRUE, NULL, GetState());
	else
		PlayHUDMotion("anm_idle_aim", TRUE, NULL, GetState());
}

void CWeaponMagazined::PlayAnimIdle()
{
	if(GetState() != eIdle)
		return;

	if(IsZoomed())
	{
		PlayAnimAim();
	}
	else
	{
		if (inherited::TryPlayAnimIdle())
			return;

		if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_idle_empty"))
			PlayHUDMotion("anm_idle_empty", TRUE, NULL, GetState());
		else
			PlayHUDMotion("anm_idle", TRUE, NULL, GetState());
	}
}

void CWeaponMagazined::PlayAnimIdleMoving()
{
	if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_idle_moving_empty"))
	{
		PlayHUDMotion("anm_idle_moving_empty", TRUE, NULL, GetState());
	}else
	{
		PlayHUDMotion("anm_idle_moving", TRUE, NULL, GetState());
	}
}

void CWeaponMagazined::PlayAnimIdleSprint()
{
	if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_idle_sprint_empty"))
	{
		PlayHUDMotion("anm_idle_sprint_empty", TRUE, NULL, GetState());
	}else
	{
		PlayHUDMotion("anm_idle_sprint", TRUE, NULL, GetState());
	}
}

void CWeaponMagazined::PlayAnimShoot()
{
	VERIFY(GetState()==eFire);
	if(iAmmoElapsed <= 1 && pSettings->line_exist(hud_sect, "anm_shot_1"))
		PlayHUDMotion("anm_shot_1", FALSE, this, GetState());
	else
		PlayHUDMotion("anm_shots", FALSE, this, GetState());
}

void CWeaponMagazined::OnZoomIn			()
{
	inherited::OnZoomIn();

	if(GetState() == eIdle)
		PlayAnimIdle();


	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if(pActor)
	{
		CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>	(pActor->Cameras().GetCamEffector(eCEZoom));
		if (!S)	
		{
			S = (CEffectorZoomInertion*)pActor->Cameras().AddCamEffector(xr_new <CEffectorZoomInertion>());
			S->Init(this);
		};
		S->SetRndSeed(pActor->GetZoomRndSeed());
		S->Enable(true);
		R_ASSERT				(S);
	}
}
void CWeaponMagazined::OnZoomOut		()
{
	if (!IsZoomed())
		return;

	inherited::OnZoomOut();

	if(GetState() == eIdle)
		PlayAnimIdle();

	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if(pActor)
	{
		auto S = smart_cast<CEffectorZoomInertion*>(pActor->Cameras().GetCamEffector(eCEZoom));
		if (S)
		{
			S->Enable(false, m_zoom_params.m_fZoomRotateTime);
		}
	}
}

//переключение режимов стрельбы одиночными и очередями
bool CWeaponMagazined::SwitchMode			()
{
	if(eIdle != GetState() || IsPending()) return false;

	if(SingleShotMode())
		m_iQueueSize = WEAPON_ININITE_QUEUE;
	else
		m_iQueueSize = 1;
	
	PlaySound	("sndEmptyClick", get_LastFP());

	return true;
}

LPCSTR CWeaponMagazined::getAmmoName()
{
	return m_ammoTypes[m_ammoType].c_str();
}

void	CWeaponMagazined::OnNextFireMode		()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode+1+m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
};

void	CWeaponMagazined::OnPrevFireMode		()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode-1+m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());	
};

void	CWeaponMagazined::AfterAttachToParent()
{
	if (m_bHasDifferentFireModes)
	{
		CActor	*actor = smart_cast<CActor*>(H_Parent());
		if (!actor) SetQueueSize(-1);
		else SetQueueSize(GetCurrentFireMode());
	};	
	inherited::AfterAttachToParent();
}

void CWeaponMagazined::BeforeDetachFromParent(bool jbd)
{
	RemoveZoomInertionEffector();
	inherited::BeforeDetachFromParent(jbd);
}

void	CWeaponMagazined::SetQueueSize			(int size)  
{
	m_iQueueSize = size; 
	if (m_iQueueSize == -1)
		xr_strcpy(m_sCurFireMode, "A");
	else
		xr_sprintf(m_sCurFireMode, "%d", m_iQueueSize);
};

float	CWeaponMagazined::GetWeaponDeterioration	()
{
	if (!m_bHasDifferentFireModes || m_iPrefferedFireMode == -1 || u32(GetCurrentFireMode()) <= u32(m_iPrefferedFireMode))
	{	
		return (u32(GetCurrentFireMode()) > 1)
			? m_conditionDecreasePerQueueShot
			: inherited::GetWeaponDeterioration();
	}
	return m_iShotNum*m_conditionDecreasePerQueueShot;
}

void CWeaponMagazined::RemoveZoomInertionEffector()
{
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (pActor)
	{
		pActor->Cameras().RemoveCamEffector(eCEZoom);
	}
}

BOOL CWeaponMagazined::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	BOOL bResult = inherited::SpawnAndImportSOData(data_containing_so);
	CSE_ALifeItemWeaponMagazined* weapon_mgznd = smart_cast<CSE_ALifeItemWeaponMagazined*>(data_containing_so);

	R_ASSERT(weapon_mgznd);

	m_iCurFireMode = weapon_mgznd->m_u8CurFireMode;

	return bResult;
}

void CWeaponMagazined::save(NET_Packet &output_packet)
{
	inherited::save	(output_packet);
	save_data		(m_iQueueSize, output_packet);
	save_data		(m_iShotNum, output_packet);
}

void CWeaponMagazined::load(IReader &input_packet)
{
	inherited::load	(input_packet);
	load_data		(m_iQueueSize, input_packet);SetQueueSize(m_iQueueSize);
	load_data		(m_iShotNum, input_packet);
}

void CWeaponMagazined::ExportDataToServer(NET_Packet& P)
{
	inherited::ExportDataToServer(P);

	P.w_u8(u8(m_iCurFireMode&0x00ff));
}

#include "string_table.h"
bool CWeaponMagazined::GetBriefInfo(II_BriefInfo& info)
{
	VERIFY(m_pCurrentInventory);
	string32	int_str;

	int	ae = GetAmmoElapsed();
	int	ac = GetSuitableAmmoTotal();
	
	if (!unlimited_ammo())
		xr_sprintf(int_str, "%d/%d",ae,ac - ae);
	else
		xr_sprintf(int_str, "%d/--",ae);

	info.cur_ammo = int_str;

	if (HasFireModes())
	{
		if (m_iQueueSize == WEAPON_ININITE_QUEUE)
		{
			info.fire_mode = "A";
		}
		else
		{
			xr_sprintf(int_str, "%d", m_iQueueSize);
			info.fire_mode = int_str;
		}
	}
	else
		info.fire_mode = "";

	//if (m_pCurrentInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
	//	return false;

	GetSuitableAmmoTotal();//update m_BriefInfo_CalcFrame
	info.grenade = "";

	u32 at_size = m_ammoTypes.size();
	if (unlimited_ammo() || at_size == 0)
	{
		info.fmj_ammo._set("--");
		info.ap_ammo._set("--");
	}
	else
	{
		//GetSuitableAmmoTotal(); //mp = all type

		xr_sprintf(int_str, "%d", GetAmmoCount(0)); // !!!!!!!!!!! == 0 temp
		if (m_ammoType == 0)
			info.fmj_ammo = int_str;
		else
			info.ap_ammo = int_str;

		if (at_size == 2)
		{
			xr_sprintf(int_str, "%d", GetAmmoCount(1));
			if (m_ammoType == 0)
				info.ap_ammo = int_str;
			else
				info.fmj_ammo = int_str;
		}
		else
		{
			info.ap_ammo = "";
		}
	}

	if (ae != 0 && m_magazine.size() != 0)
	{
		LPCSTR ammo_type = m_ammoTypes[m_magazine.back().m_LocalAmmoType].c_str();
		info.name = CStringTable().translate(pSettings->r_string(ammo_type, "inv_name_short"));
		info.icon = ammo_type;
	}
	else
	{
		LPCSTR ammo_type = m_ammoTypes[m_ammoType].c_str();
		info.name = CStringTable().translate(pSettings->r_string(ammo_type, "inv_name_short"));
		info.icon = ammo_type;
	}
	return true;
}

bool CWeaponMagazined::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result = inherited::install_upgrade_impl( section, test );
	
	LPCSTR str;
	// fire_modes = 1, 2, -1
	bool result2 = process_if_exists_set( section, "fire_modes", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{
		int ModesCount = _GetItemCount( str );
		m_aFireModes.clear();
		for ( int i = 0; i < ModesCount; ++i )
		{
			string16 sItem;
			_GetItem( str, i, sItem );
			m_aFireModes.push_back( (s8)atoi(sItem) );
		}
		m_iCurFireMode = ModesCount - 1;
	}
	result |= result2;

	result |= process_if_exists_set(section, "dispersion_start", &CInifile::r_s32, m_iRecoilStartShotNum, test);
	result |= ProcessRpmUpgrade( section, "rpm_no_disp", m_fNoRecoilTimeToFire, test );
	result |= ProcessRpmUpgrade( section, "rpm_semi", m_fTimeToFireSemi, test );

	// sounds (name of the sound, volume (0.0 - 1.0), delay (sec))
	result2 = process_if_exists_set( section, "snd_draw", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{ 
		m_sounds.LoadSound(section, "snd_draw", "sndShow", 0, false, m_eSoundShow);
	}
	result |= result2;

	result2 = process_if_exists_set( section, "snd_holster", &CInifile::r_string, str, test );
	if ( result2 && !test ) 
	{
		m_sounds.LoadSound(section, "snd_holster", "sndHide", 0, false, m_eSoundHide);
	}
	result |= result2;

	result2 = process_if_exists_set( section, "snd_shoot", &CInifile::r_string, str, test );
	if ( result2 && !test ) 
	{
		m_sounds.LoadSound(section, "snd_shoot", "sndShot", (u8)sndMaxShotSounds_, false, m_eSoundShot);
	}
	result |= result2;

	result2 = process_if_exists_set(section, "snd_shoot_dist", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_shoot_dist", "sndShotDist", (u8)sndMaxShotSounds_, false, m_eSoundShot);
		hasDistantShotSnd_ = true;
	}
	result |= result2;

	result2 = process_if_exists_set( section, "snd_empty", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{ 
		m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", 0, false, m_eSoundEmptyClick);
	}
	result |= result2;

	result2 = process_if_exists_set( section, "snd_reload", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{ 
		m_sounds.LoadSound(section, "snd_reload", "sndReload", 0, true, m_eSoundReload);
	}
	result |= result2;

	result2 = process_if_exists_set( section, "snd_reload_empty", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{ 
		m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", 0, true, m_eSoundReload);
	}
	result |= result2;

	if ( m_eSilencerStatus == ALife::eAddonAttachable || m_eSilencerStatus == ALife::eAddonPermanent )
	{
		result |= process_if_exists_set( section, "silencer_flame_particles", &CInifile::r_string, m_sSilencerFlameParticles, test );
		result |= process_if_exists_set( section, "silencer_smoke_particles", &CInifile::r_string, m_sSilencerSmokeParticles, test );

		result2 = process_if_exists_set( section, "snd_silncer_shot", &CInifile::r_string, str, test );
		if ( result2 && !test ) 
		{ 
			m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", (u8)sndMaxShotSounds_, false, m_eSoundShot);
		}
		result |= result2;
	}

	// fov for zoom mode
	result |= process_if_exists( section, "ironsight_zoom_factor", &CInifile::r_float, m_zoom_params.m_fIronSightZoomFactor, test );

	if( IsScopeAttached() )
	{
		//if ( m_eScopeStatus == ALife::eAddonAttachable )
		{
			result |= process_if_exists( section, "scope_zoom_factor", &CInifile::r_float, m_zoom_params.m_fScopeZoomFactor, test );
		}
	}

	result |= process_if_exists(section, "condition_queue_shot_dec", &CInifile::r_float, m_conditionDecreasePerQueueShot, test );

	return result;
}

bool CWeaponMagazined::DelayedShotIsAllowed()
{
	return fShotTimeCounter <= m_fShotMaxDelay;
}

#include "../Include/xrRender/Kinematics.h"

void CWeaponMagazined::renderable_Render(IRenderBuffer& render_buffer)
{
	if (scopeVisual)
	{
		render_buffer.renderingMatrix_ = scopeAttachTransform;
		::Render->add_Visual(scopeVisual, render_buffer);
	}

	if (silencerVisual)
	{
		render_buffer.renderingMatrix_ = silencerAttachTransform;
		::Render->add_Visual(silencerVisual, render_buffer);
	}

	if (glauncherVisual)
	{
		render_buffer.renderingMatrix_ = glauncherAttachTransform;
		::Render->add_Visual(glauncherVisual, render_buffer);
	}

	inherited::renderable_Render(render_buffer);
}

void CWeaponMagazined::render_hud_mode(IKinematics* hud_model, IRenderBuffer& render_buffer)
{
	if (fakeViual)
	{
		render_buffer.renderingMatrix_ = HudItemData()->m_item_transform;
		::Render->add_Visual(fakeViual, render_buffer);
	}

	if (scopeVisual)
	{ 
		render_buffer.renderingMatrix_ = scopeAttachTransform;
		::Render->add_Visual(scopeVisual, render_buffer);
	}

	if (silencerVisual)
	{
		render_buffer.renderingMatrix_ = silencerAttachTransform;
		::Render->add_Visual(silencerVisual, render_buffer);
	}

	if (glauncherVisual)
	{
		render_buffer.renderingMatrix_ = glauncherAttachTransform;
		::Render->add_Visual(glauncherVisual, render_buffer);
	}

	inherited::render_hud_mode(hud_model, render_buffer);
}