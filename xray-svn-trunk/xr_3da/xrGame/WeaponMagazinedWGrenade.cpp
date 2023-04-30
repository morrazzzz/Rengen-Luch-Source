#include "stdafx.h"
#include "weaponmagazinedwgrenade.h"
#include "player_hud.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "GrenadeLauncher.h"
#include "xrserver_objects_alife_items.h"
#include "ExplosiveRocket.h"
#include "xr_level_controller.h"
#include "level.h"
#include "object_broker.h"
#include "weaponmiscs.h"
#include "GameConstants.h"

#ifdef DEBUG
#include "phdebug.h"
#endif

CWeaponMagazinedWGrenade::CWeaponMagazinedWGrenade(ESoundTypes eSoundType) : CWeaponMagazined(eSoundType)
{
	inactiveAmmoIndex_ = 0;
	grenadeMode_ = false;

	hasDistantShotGSnd_ = false;
}

CWeaponMagazinedWGrenade::~CWeaponMagazinedWGrenade(void)
{
}
void CWeaponMagazinedWGrenade::LoadCfg	(LPCSTR section)
{
	inherited::LoadCfg			(section);
	CRocketLauncher::LoadCfg	(section);
	
	
	m_sounds.LoadSound(section, "snd_shoot_grenade", "sndShotG", 0, false, m_eSoundShot);

	if (pSettings->line_exist(section, "snd_shoot_grenade_dist")) // distant sound
	{
		m_sounds.LoadSound(section, "snd_shoot_grenade_dist", "sndShotGDist", 0, false, m_eSoundShot);
		hasDistantShotGSnd_ = true;
	}

	m_sounds.LoadSound(section, "snd_reload_grenade", "sndReloadG", 0, true, m_eSoundReload);
	m_sounds.LoadSound(section, "snd_switch", "sndSwitch", 0, true, m_eSoundReload);

	m_sFlameParticles2 = pSettings->r_string(section, "grenade_flame_particles");

	if(m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
	{
		CRocketLauncher::m_fLaunchSpeed = pSettings->r_float(section, "grenade_vel");
	}

	// load ammo classes SECOND (grenade_class)
	ammoList2_.clear();
	LPCSTR				S = pSettings->r_string(section,"grenade_class");
	if (S && S[0]) 
	{
		string128		_ammoItem;
		int				count		= _GetItemCount	(S);
		for (int it=0; it<count; ++it)	
		{
			_GetItem				(S,it,_ammoItem);
			ammoList2_.push_back(_ammoItem);
		}
	}
	magMaxSize1 = maxMagazineSize_;
	magMaxSize2 = 1;
}

void CWeaponMagazinedWGrenade::DestroyClientObj()
{
	inherited::DestroyClientObj();
}

void CWeaponMagazinedWGrenade::switch2_Reload()
{
	VERIFY(GetState()==eReload);

	if (grenadeMode_)
	{
		PlaySound("sndReloadG", get_LastFP2());
		PlayHUDMotion("anm_reload_g", FALSE, this, GetState());
		SetPending			(TRUE);
	}
	else 
	     inherited::switch2_Reload();
}

void CWeaponMagazinedWGrenade::OnShot		()
{
	if (grenadeMode_)
		GrenadeShot();

	else inherited::OnShot();
}

void CWeaponMagazinedWGrenade::GrenadeShot()
{
	PlayAnimShoot();

	if (hasDistantShotGSnd_)
	{
		float dist_f = Position().distance_to(Device.vCameraPosition) / GameConstants::GetDistantSndApplyDistance();

		float valume_far = dist_f;
		clamp(valume_far, 0.1f, 1.f);

		float valume_close = 1.f - dist_f;
		clamp(valume_close, 0.1f, 1.f);

		if (valume_far > 0.1f)
		{
			SSnd* distant = PlaySound("sndShotGDist", get_LastFP(), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));

			if (distant)
				distant->snd.set_volume(valume_far);
		}

		if (valume_close > 0.1f)
		{
			SSnd* close = PlaySound("sndShotG", get_LastFP(), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));

			if (close)
				close->snd.set_volume(valume_close);
		}
	}
	else
		PlaySound("sndShotG", get_LastFP(), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));

	AddShotEffector();

	StartFlameParticles2();
}

//переход в режим подствольника или выход из него
//если мы в режиме стрельбы очередями, переключиться
//на одиночные, а уже потом на подствольник
bool CWeaponMagazinedWGrenade::SwitchMode() 
{
	bool bUsefulStateToSwitch = ((eIdle==GetState())||(eHidden==GetState())||(eMisfire==GetState())||(eMagEmpty==GetState())) && (!IsPending());

	if(!bUsefulStateToSwitch)
		return false;

	if(!IsGrenadeLauncherAttached()) 
		return false;

	OnZoomOut();
	SetPending				(TRUE);

	PerformSwitchGL			();
	
	PlaySound				("sndSwitch",get_LastFP());

	PlayAnimModeSwitch		();

	m_BriefInfo_CalcFrame = 0;

	return					true;
}

void  CWeaponMagazinedWGrenade::PerformSwitchGL()
{
	grenadeMode_		= !grenadeMode_;

	maxMagazineSize_	= grenadeMode_ ? magMaxSize2 : magMaxSize1;

	m_ammoTypes.swap	(ammoList2_);

	swap				(m_ammoType,inactiveAmmoIndex_);
	swap				(m_DefaultCartridge, m_DefaultCartridge2);

	xr_vector<CCartridge> l_magazine; // temp_storage

	while(m_magazine.size()) { l_magazine.push_back(m_magazine.back()); m_magazine.pop_back(); } // copy and clear 'now'

	while (inactiveMagazine_.size()) { m_magazine.insert(m_magazine.begin(), inactiveMagazine_.back()); inactiveMagazine_.pop_back(); } // push inactive into 'now'

	while (l_magazine.size()) { inactiveMagazine_.push_back(l_magazine.back()); l_magazine.pop_back(); } // push copied 'now' to inactive

	iAmmoElapsed = (int)m_magazine.size();

	m_BriefInfo_CalcFrame = 0;
}

bool CWeaponMagazinedWGrenade::Action(u16 cmd, u32 flags) 
{
	if (grenadeMode_ && cmd == kWPN_FIRE)
	{
		if(IsPending())		
			return				false;

		if(flags&CMD_START)
		{
			if(iAmmoElapsed)
				LaunchGrenade		();
			else
				Reload				();

			if(GetState() == eIdle) 
				OnEmptyClick			();
		}
		return					true;
	}
	if(inherited::Action(cmd, flags))
		return true;
	
	switch(cmd) 
	{
	case kWPN_FUNC: 
		{
            if(flags&CMD_START && !IsPending()) 
				SwitchState(eSwitch);
			return true;
		}
	}
	return false;
}

#include "inventory.h"
#include "actor.h"
#include "inventoryOwner.h"
void CWeaponMagazinedWGrenade::state_Fire(float dt) 
{
	VERIFY(fOneShotTime>0.f);

	inherited::state_Fire(dt);
}

void CWeaponMagazinedWGrenade::OnEvent(NET_Packet& P, u16 type) 
{
	inherited::OnEvent(P,type);
	u16 id;
	switch (type) 
	{
		case GE_OWNERSHIP_TAKE: 
			{
				P.r_u16(id);
				CRocketLauncher::AttachRocket(id, this);
			}
			break;
		case GE_OWNERSHIP_REJECT :
		case GE_LAUNCH_ROCKET : 
			{
				bool bLaunch	= (type==GE_LAUNCH_ROCKET);
				P.r_u16			(id);
				CRocketLauncher::DetachRocket(id, bLaunch);

				if(bLaunch)
					GrenadeShot();

				break;
			}
	}
}

void  CWeaponMagazinedWGrenade::LaunchGrenade()
{
	if(!getRocketCount())	return;
	R_ASSERT(grenadeMode_);

	{
		Fvector						p1, d; 
		p1.set						(get_LastFP2());
		d.set						(get_LastFD());
		CEntity*					E = smart_cast<CEntity*>(H_Parent());

		if (E){
			CInventoryOwner* io		= smart_cast<CInventoryOwner*>(H_Parent());
			if(NULL == io->inventory().ActiveItem())
			{
				Log("current_state", GetState() );
				Log("next_state", GetNextState());
				Log("item_sect", SectionName().c_str());
				Log("H_Parent", H_Parent()->SectionName().c_str());
			}
			E->g_fireParams		(this, p1,d);
		}
		p1.set						(get_LastFP2());
		
		Fmatrix launch_matrix;
		launch_matrix.identity();
		launch_matrix.k.set(d);
		Fvector::generate_orthonormal_basis(launch_matrix.k,
											launch_matrix.j, 
											launch_matrix.i);

		launch_matrix.c.set				(p1);

		if(IsZoomed() && smart_cast<CActor*>(H_Parent()))
		{
			H_Parent()->setEnabled(FALSE);
			setEnabled(FALSE);

			collide::rq_result RQ;
			BOOL HasPick = Level().ObjectSpace.RayPick(p1, d, 300.0f, collide::rqtStatic, RQ, this);

			setEnabled(TRUE);
			H_Parent()->setEnabled(TRUE);

			if (HasPick)
			{
				Fvector Transference;
				Transference.mul(d, RQ.range);
				Fvector res[2];
#ifdef		DEBUG
//.				DBG_OpenCashedDraw();
//.				DBG_DrawLine(p1,Fvector().add(p1,d),D3DCOLOR_XRGB(255,0,0));
#endif
				u8 canfire0 = TransferenceAndThrowVelToThrowDir(Transference, 
																CRocketLauncher::m_fLaunchSpeed, 
																EffectiveGravity(), 
																res);
#ifdef DEBUG
//.				if(canfire0>0)DBG_DrawLine(p1,Fvector().add(p1,res[0]),D3DCOLOR_XRGB(0,255,0));
//.				if(canfire0>1)DBG_DrawLine(p1,Fvector().add(p1,res[1]),D3DCOLOR_XRGB(0,0,255));
//.				DBG_ClosedCashedDraw(30000);
#endif
				
				if (canfire0 != 0)
				{
					d = res[0];
				};
			}
		};
		
		d.normalize();
		d.mul(CRocketLauncher::m_fLaunchSpeed);
		VERIFY2(_valid(launch_matrix),"CWeaponMagazinedWGrenade::SwitchState. Invalid launch_matrix!");
		CRocketLauncher::LaunchRocket(launch_matrix, d, zero_vel);

		CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket()/*m_pRocket*/);
		VERIFY(pGrenade);
		pGrenade->SetInitiator(H_Parent()->ID());

		
		VERIFY				(m_magazine.size());
		m_magazine.pop_back	();
		--iAmmoElapsed;
		VERIFY((u32)iAmmoElapsed == m_magazine.size());

		NET_Packet					P;
		u_EventGen					(P,GE_LAUNCH_ROCKET,ID());
		P.w_u16						(getCurrentRocket()->ID());
		u_EventSend					(P);

	}
}

void CWeaponMagazinedWGrenade::FireEnd() 
{
	if (grenadeMode_)
	{
		CWeapon::FireEnd();
	}else
		inherited::FireEnd();
}

void CWeaponMagazinedWGrenade::OnMagazineEmpty() 
{
	if(GetState() == eIdle) 
	{
		OnEmptyClick			();
	}
}

void CWeaponMagazinedWGrenade::ReloadMagazine() 
{
	inherited::ReloadMagazine();

	//перезарядка подствольного гранатомета
	if (iAmmoElapsed && !getRocketCount() && grenadeMode_)
	{
		shared_str fake_grenade_name = pSettings->r_string(m_ammoTypes[m_ammoType].c_str(), "fake_grenade_name");
		
		CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
	}
}

void CWeaponMagazinedWGrenade::Chamber()
{
	if (!grenadeMode_)
	{
		if (m_bChamberStatus == true && iAmmoElapsed >= 1 && m_chamber == false)
		{
			m_chamber = true;
			maxMagazineSize_++;
			magMaxSize1++;
		}
		else if (m_bChamberStatus == true && iAmmoElapsed == 0 && m_chamber == true)
		{
			m_chamber = false;
			maxMagazineSize_--;
			magMaxSize1--;
		}
	}
}

void CWeaponMagazinedWGrenade::OnStateSwitch(u32 S) 
{

	switch (S)
	{
	case eSwitch:
		{
			if( !SwitchMode() ){
				SwitchState(eIdle);
				return;
			}
		}break;
	}
	
	inherited::OnStateSwitch(S);
	UpdateGrenadeVisibility(!!iAmmoElapsed || S == eReload);
}


void CWeaponMagazinedWGrenade::OnAnimationEnd(u32 state)
{
	switch (state)
	{
	case eSwitch:
		{
			SwitchState(eIdle);
		}break;
	case eFire:
		{
			if (grenadeMode_)
				Reload();
		}break;
	}
	inherited::OnAnimationEnd(state);
}


void CWeaponMagazinedWGrenade::BeforeDetachFromParent(bool just_before_destroy)
{
	inherited::BeforeDetachFromParent(just_before_destroy);

	SetPending			(FALSE);
	if (grenadeMode_) {
		SetState		( eIdle );
		SetPending		(FALSE);
	}
}

bool CWeaponMagazinedWGrenade::CanAttach(PIItem pIItem)
{
	CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);
	
	if(pGrenadeLauncher &&
	   ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
	   0 == (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
	   !xr_strcmp(*m_sGrenadeLauncherName, pIItem->object().SectionName()))
       return true;
	else
		return inherited::CanAttach(pIItem);
}

bool CWeaponMagazinedWGrenade::CanDetach(const char* item_section_name)
{
	if(ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
	   0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
	   !xr_strcmp(*m_sGrenadeLauncherName, item_section_name))
	   return true;
	else
	   return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazinedWGrenade::Attach(PIItem pIItem, bool b_send_event)
{
	CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);
	
	if(pGrenadeLauncher &&
		ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
	   0 == (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
	   !xr_strcmp(*m_sGrenadeLauncherName, pIItem->object().SectionName()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

		CRocketLauncher::m_fLaunchSpeed = pGrenadeLauncher->GetGrenadeVel();

 		//уничтожить подствольник из инвентаря
		if(b_send_event)
		{
			pIItem->object().DestroyObject	();
		}
		InitAddons				();
		UpdateAddonsVisibility	();

		if(GetState()==eIdle)
			PlayAnimIdle		();

		return					true;
	}
	else
        return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazinedWGrenade::Detach(const char* item_section_name, bool b_spawn_item)
{
	if (ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
	   0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
	   !xr_strcmp(*m_sGrenadeLauncherName, item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
		if (!grenadeMode_)
		{
			PerformSwitchGL();
		}
		UnloadMagazine();
		PerformSwitchGL();

		UpdateAddonsVisibility();

		if(GetState()==eIdle)
			PlayAnimIdle		();

		return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
	}
	else
		return inherited::Detach(item_section_name, b_spawn_item);
}




void CWeaponMagazinedWGrenade::InitAddons()
{	
	inherited::InitAddons();

	if(GrenadeLauncherAttachable())
	{
		if(IsGrenadeLauncherAttached())
		{
			CRocketLauncher::m_fLaunchSpeed = pSettings->r_float(*m_sGrenadeLauncherName,"grenade_vel");

			glAttachBone = READ_IF_EXISTS(pSettings, r_string, GetGrenadeLauncherName().c_str(), "gl_attach_bone", "wpn_launcher");

			glauncherAttachOffset[0] = READ_IF_EXISTS(pSettings, r_fvector3, m_scopes[m_cur_scope].c_str(), "gl_w_attach_pos", glauncherAttachOffset[0]);
			glauncherAttachOffset[1] = READ_IF_EXISTS(pSettings, r_fvector3, m_scopes[m_cur_scope].c_str(), "gl_w_attach_rot", glauncherAttachOffset[1]);
		}
	}
}

bool	CWeaponMagazinedWGrenade::UseScopeTexture()
{
	if (IsGrenadeLauncherAttached() && grenadeMode_) return false;
	
	if (IsSecondVPZoomPresent()) return false;

	return true;
};

float	CWeaponMagazinedWGrenade::CurrentZoomFactor	()
{
	if (IsGrenadeLauncherAttached() && grenadeMode_) return m_zoom_params.m_fIronSightZoomFactor;
	return inherited::CurrentZoomFactor();
}

//виртуальные функции для проигрывания анимации HUD
void CWeaponMagazinedWGrenade::PlayAnimShow()
{
	VERIFY(GetState()==eShowing);
	if(IsGrenadeLauncherAttached())
	{
		if(grenadeMode_)
		{
			if(inactiveMagazine_.size() == 0 && pSettings->line_exist(hud_sect, "anm_show_g_empty"))
				PlayHUDMotion("anm_show_g_empty", FALSE, this, GetState());
			else
				PlayHUDMotion("anm_show_g", FALSE, this, GetState());
		}else
		{
			if(iAmmoElapsed == 0 && pSettings->line_exist(hud_sect, "anm_show_w_gl_empty"))
				PlayHUDMotion("anm_show_w_gl_empty", FALSE, this, GetState());
			else
				PlayHUDMotion("anm_show_w_gl", FALSE, this, GetState());
		}
	}else
	{
		if(iAmmoElapsed == 0 && pSettings->line_exist(hud_sect, "anm_show_empty"))
			PlayHUDMotion("anm_show_empty", FALSE, this, GetState());
		else
			PlayHUDMotion("anm_show", FALSE, this, GetState());
	}
}

void CWeaponMagazinedWGrenade::PlayAnimHide()
{
	VERIFY(GetState()==eHiding);
	if(IsGrenadeLauncherAttached())
	{
		if(grenadeMode_)
		{
			if(inactiveMagazine_.size() == 0 && pSettings->line_exist(hud_sect, "anm_hide_g_empty"))
				PlayHUDMotion("anm_hide_g_empty", FALSE, this, GetState());
			else
				PlayHUDMotion("anm_hide_g", FALSE, this, GetState());
		}else
		{
			if(iAmmoElapsed == 0 && pSettings->line_exist(hud_sect, "anm_hide_w_gl_empty"))
				PlayHUDMotion("anm_hide_w_gl_empty", FALSE, this, GetState());
			else
				PlayHUDMotion("anm_hide_w_gl", FALSE, this, GetState());
		}
	}else
	{
		if(iAmmoElapsed == 0 && pSettings->line_exist(hud_sect, "anm_hide_empty"))
			PlayHUDMotion("anm_hide_empty", FALSE, this, GetState());
		else
			PlayHUDMotion("anm_hide", FALSE, this, GetState());
	}
}

void CWeaponMagazinedWGrenade::PlayAnimReload()
{
	VERIFY(GetState()==eReload);
	if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_reload_empty"))
	{
		if(IsGrenadeLauncherAttached())
			PlayHUDMotion("anm_reload_w_gl_empty", TRUE, this, GetState());
		else
		PlayHUDMotion("anm_reload_empty", TRUE, this, GetState());
	}else
	{
		if(IsGrenadeLauncherAttached())
			PlayHUDMotion("anm_reload_w_gl", TRUE, this, GetState());
		else
			PlayHUDMotion("anm_reload", TRUE, this, GetState());
	}
}

void CWeaponMagazinedWGrenade::PlayAnimIdle()
{
	if(IsGrenadeLauncherAttached())
	{
		if(IsZoomed())
		{
			if(grenadeMode_)
			{
				if(inactiveMagazine_.size() == 0 && pSettings->line_exist(hud_sect, "anm_idle_g_aim_empty"))
					PlayHUDMotion("anm_idle_g_aim_empty", TRUE, NULL, GetState());
				else
					PlayHUDMotion("anm_idle_g_aim", TRUE, NULL, GetState());
			}else
			{
				if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_idle_w_gl_aim_empty"))
					PlayHUDMotion("anm_idle_w_gl_aim_empty", TRUE, NULL, GetState());
				else
					PlayHUDMotion("anm_idle_w_gl_aim", TRUE, NULL, GetState());
			}
		}else
		{
			int act_state = 0;
			CActor* pActor = smart_cast<CActor*>(H_Parent());
			if(pActor)
			{
				CEntity::SEntityState st;
				pActor->g_State(st);
				if(st.bSprint)
				{
					act_state = 1;
				}else
				if(pActor->AnyMove())
				{
					act_state = 2;
				}
			}

			if(grenadeMode_)
			{
				if(inactiveMagazine_.size() == 0 && pSettings->line_exist(hud_sect, "anm_idle_g_empty"))
				{
					if(act_state==0)
						PlayHUDMotion("anm_idle_g_empty", TRUE, NULL, GetState());
					else
					if(act_state==1)
						PlayHUDMotion("anm_idle_sprint_g_empty", TRUE, NULL,GetState());
					else
					if(act_state==2)
						PlayHUDMotion("anm_idle_moving_g_empty", TRUE, NULL,GetState());
				}else
				{
					if(act_state==0)
						PlayHUDMotion("anm_idle_g", TRUE, NULL, GetState());
					else
					if(act_state==1)
						PlayHUDMotion("anm_idle_sprint_g", TRUE, NULL,GetState());
					else
					if(act_state==2)
						PlayHUDMotion("anm_idle_moving_g", TRUE, NULL,GetState());
				}
			}else
			{
				if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_idle_w_gl_empty"))
				{
					if(act_state==0)
						PlayHUDMotion("anm_idle_w_gl_empty", TRUE, NULL, GetState());
					else
					if(act_state==1)
						PlayHUDMotion("anm_idle_sprint_w_gl_empty", TRUE, NULL,GetState());
					else
					if(act_state==2)
						PlayHUDMotion("anm_idle_moving_w_gl_empty", TRUE, NULL,GetState());
				}else
				{
					if(act_state==0)
						PlayHUDMotion("anm_idle_w_gl", TRUE, NULL, GetState());
					else
					if(act_state==1)
						PlayHUDMotion("anm_idle_sprint_w_gl", TRUE, NULL,GetState());
					else
					if(act_state==2)
						PlayHUDMotion("anm_idle_moving_w_gl", TRUE, NULL,GetState());
				}
			}
		
		}
	}
	else
		inherited::PlayAnimIdle();
}

void CWeaponMagazinedWGrenade::PlayAnimShoot()
{
	if(grenadeMode_)
	{
		if(inactiveMagazine_.size() == 0 && pSettings->line_exist(hud_sect, "anm_shots_g_empty"))
			PlayHUDMotion("anm_shots_g_empty" ,FALSE, this, eFire);
		else
			PlayHUDMotion("anm_shots_g" ,FALSE, this, eFire);
	}
	else
	{
		VERIFY(GetState()==eFire);
		if(IsGrenadeLauncherAttached())
		{
			if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_shots_w_gl_empty"))
				PlayHUDMotion("anm_shots_w_gl_empty" ,FALSE, this, GetState());
			else
				PlayHUDMotion("anm_shots_w_gl" ,FALSE, this, GetState());
		}else
		{
			inherited::PlayAnimShoot();
		}
	}
}

void  CWeaponMagazinedWGrenade::PlayAnimModeSwitch()
{
	if(grenadeMode_)
	{
		if(inactiveMagazine_.size() == 0 && pSettings->line_exist(hud_sect, "anm_switch_g_empty"))
			PlayHUDMotion("anm_switch_g_empty" , FALSE, this, eSwitch);
		else
			PlayHUDMotion("anm_switch_g" , FALSE, this, eSwitch);
	}else
	{
		if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_switch_empty"))
			PlayHUDMotion("anm_switch_empty" , FALSE, this, eSwitch);
		else
			PlayHUDMotion("anm_switch" , FALSE, this, eSwitch);
	}
}

void CWeaponMagazinedWGrenade::PlayAnimBore()
{
	if(IsGrenadeLauncherAttached())
	{
		if(grenadeMode_)
		{
			if(inactiveMagazine_.size() == 0 && pSettings->line_exist(hud_sect, "anm_bore_g_empty"))
				PlayHUDMotion	("anm_bore_g_empty", TRUE, this, GetState());
			else
				PlayHUDMotion	("anm_bore_g", TRUE, this, GetState());
		}else
		{
			if(iAmmoElapsed==0 && pSettings->line_exist(hud_sect, "anm_bore_w_gl_empty"))
				PlayHUDMotion	("anm_bore_w_gl_empty", TRUE, this, GetState());
			else
				PlayHUDMotion	("anm_bore_w_gl", TRUE, this, GetState());
		}
	}else
		inherited::PlayAnimBore();
}


void CWeaponMagazinedWGrenade::UpdateSounds	()
{
	inherited::UpdateSounds			();

	Fvector P						= get_LastFP();

	m_sounds.SetPosition("sndShotG", P);
	if (hasDistantShotGSnd_)
		m_sounds.SetPosition("sndShotDistG", P);
	m_sounds.SetPosition("sndReloadG", P);
	m_sounds.SetPosition("sndSwitch", P);
}

void CWeaponMagazinedWGrenade::UpdateGrenadeVisibility(bool visibility)
{
	if(!GetHUDmode())							return;
	HudItemData()->set_bone_visible				("grenade", visibility, TRUE);
}

BOOL CWeaponMagazinedWGrenade::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	BOOL bResult = CHudItemObject::SpawnAndImportSOData(data_containing_so); // обходим нет спавн базового оружия(чтобы не перетолковать одни и те же переменные) отрабатываем нетспавн худ итема и нетспавним этот класс по собственному алгоритму

	CSE_ALifeItemWeaponMagazinedWGL* const weapon_gl = smart_cast<CSE_ALifeItemWeaponMagazinedWGL*>(data_containing_so);

	VERIFY(weapon_gl);

	// Загрузить с сервера то, что мы экспортировали через ::ExportDataToServer
	m_iCurFireMode		= weapon_gl->m_u8CurFireMode;

	m_flagsAddOnState	= weapon_gl->m_addon_flags.get();
	m_cur_scope			= weapon_gl->m_cur_scope;

	SetState			(weapon_gl->wpn_state);
	SetNextState		(weapon_gl->wpn_state);

	grenadeMode_		= weapon_gl->grenadeMode_server;

	m_magazine.clear();
	inactiveMagazine_.clear();

	// Тут немного сложно, но пока по другому никак. В зависимости от того, включен ли режим гранатомета, для магазина патронов и магазина подствола выберается активный или неактивный магазины для хранения зарядов
	xr_vector<CCartridge>& grenade_mag = grenadeMode_ ? m_magazine : inactiveMagazine_; // указатель на переменную хронящую заряды для подствольника
	xr_vector<CCartridge>& regular_ammo_mag = grenadeMode_ ? inactiveMagazine_ : m_magazine; // указатель на переменную хронящую заряды для обычного магазина

	u8& grenade_ammo_index = grenadeMode_ ? m_ammoType : inactiveAmmoIndex_; // указатель на переменную хронящую тип гранат
	u8& regular_ammo_index = grenadeMode_ ? inactiveAmmoIndex_ : m_ammoType; // указатель на переменную хронящую тип патронов

	if (grenadeMode_)
		maxMagazineSize_ = 1;

	// Загрузить подствольник
	grenade_ammo_index = weapon_gl->grndID_;

	if (grenade_ammo_index >= ammoList2_.size())
	{
		Msg("! grenade ammo index %d is out of ammo types, should be less than %d. Item section is [%s]. Probably default ammo upgrade is the reason", m_ammoType, ammoList2_.size(), SectionNameStr());
		grenade_ammo_index = 0;
	}

	if (weapon_gl->grndIsLoaded_)
	{
		m_DefaultCartridge2.Load(ammoList2_[grenade_ammo_index].c_str(), grenade_ammo_index);
		grenade_mag.push_back(m_DefaultCartridge2);
	}


	// Загрузить обычный магазин
	regular_ammo_index = weapon_gl->ammo_type;
	if (regular_ammo_index >= m_ammoTypes.size())
	{
		Msg("! ammo index %d is out of ammo types, should be less than %d. Item section is [%s]. Probably default ammo upgrade is the reason", regular_ammo_index, m_ammoTypes.size(), SectionNameStr());
		regular_ammo_index = 0;
	}

	iAmmoElapsed = weapon_gl->a_elapsed;

	ImportSOMagazine(weapon_gl, regular_ammo_mag);

	R_ASSERT(weapon_gl->grndIsLoaded_ == grenade_mag.size());
	R_ASSERT(weapon_gl->a_elapsed == regular_ammo_mag.size());

	iAmmoElapsed = grenadeMode_ ? grenade_mag.size() : regular_ammo_mag.size();

	//Get apropriate ammo list for active magazine
	if (grenadeMode_)
		m_ammoTypes.swap(ammoList2_); // if not grenade mode - no need to swap because default is regular ammo list

	PlayAnimModeSwitch(); // switch to apropriate animation type
	
	UpdateGrenadeVisibility(grenade_mag.size()>0);
	UpdateAddonsVisibility();

	SetPending(FALSE);

	InitAddons();

	if (weapon_gl->grndIsLoaded_) // load actual missle into gr launcher, if its magazine is not empty
	{
		shared_str fake_grenade_name = pSettings->r_string(grenade_mag.back().m_ammoSect, "fake_grenade_name");

		CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
	}

	return bResult;
}

void CWeaponMagazinedWGrenade::ExportDataToServer(NET_Packet& P)
{
	// for weapon with grenade addon we cant call inherited weapon export, we have to store values in different way taking in attention active/inactive magazines
	CHudItemObject::ExportDataToServer(P);

	VERIFY2(inactiveMagazine_.size() <= 65535 && m_magazine.size() <= 65535, "Possible bug here, contact coders");

	auto regular_mag = grenadeMode_ ? inactiveMagazine_ : m_magazine;

	//server weapon data
	u8 need_upd	= IsUpdating() ? 1 : 0;
	P.w_u8		(need_upd);
	P.w_u16		((u16)regular_mag.size());
	P.w_u8		(m_cur_scope);
	P.w_u8		(m_flagsAddOnState);
	P.w_u8		(grenadeMode_ ? inactiveAmmoIndex_ : m_ammoType);

	PackAndExportMagazine(P, regular_mag);

	u8 state = (GetState() == eMisfire || GetState() == eMagEmpty) ? (u8)GetState() : (u8)eHidden; // save only same states
	P.w_u8		(state);
	P.w_u8		((u8)IsZoomed());

	//server w_magazined data
	P.w_u8		(u8(m_iCurFireMode & 0x00ff));

	//server w_grenade_laucher data
	P.w_u8		(grenadeMode_ ? 1 : 0);
	P.w_u8		(grenadeMode_ ? (u8)m_magazine.size() : (u8)inactiveMagazine_.size());
	P.w_u8		(grenadeMode_ ? m_ammoType : inactiveAmmoIndex_);
}

void CWeaponMagazinedWGrenade::save(NET_Packet &output_packet)
{
	inherited::save				(output_packet);
}

void CWeaponMagazinedWGrenade::load(IReader &input_packet)
{
	inherited::load				(input_packet);
}

bool CWeaponMagazinedWGrenade::IsNecessaryItem	    (const shared_str& item_sect)
{
	return (	std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end() ||
				std::find(ammoList2_.begin(), ammoList2_.end(), item_sect) != ammoList2_.end() 
			);
}

u8 CWeaponMagazinedWGrenade::GetCurrentHudOffsetIdx()
{
	bool b_aiming		= 	((IsZoomed() && m_zoom_params.m_fZoomRotationFactor<=1.f) ||
							(!IsZoomed() && m_zoom_params.m_fZoomRotationFactor>0.f));
	
	if(!b_aiming)
		return		0;
	else
	if(grenadeMode_)
		return		2;
	else
		return		1;
}

bool CWeaponMagazinedWGrenade::install_upgrade_ammo_class	( LPCSTR section, bool test )
{
	LPCSTR str;

	bool result = process_if_exists(section, "ammo_mag_size", &CInifile::r_s32, magMaxSize1, test);
	maxMagazineSize_ = grenadeMode_ ? 1 : magMaxSize1;

	//	ammo_class = ammo_5.45x39_fmj, ammo_5.45x39_ap  // name of the ltx-section of used ammo
	bool result2 = process_if_exists_set( section, "ammo_class", &CInifile::r_string, str, test );
	if ( result2 && !test ) 
	{
		xr_vector<shared_str>& ammo_types = grenadeMode_ ? ammoList2_ : m_ammoTypes;
		ammo_types.clear					(); 
		for ( int i = 0, count = _GetItemCount( str ); i < count; ++i )	
		{
			string128						ammo_item;
			_GetItem						( str, i, ammo_item );
			ammo_types.push_back			( ammo_item );
		}

		m_ammoType  = 0;
		inactiveAmmoIndex_ = 0;
	}
	result |= result2;

	return result2;
}

bool CWeaponMagazinedWGrenade::install_upgrade_impl( LPCSTR section, bool test )
{
	LPCSTR str;
	bool result = inherited::install_upgrade_impl( section, test );
	
	//	grenade_class = ammo_vog-25, ammo_vog-25p          // name of the ltx-section of used grenades
	bool result2 = process_if_exists_set( section, "grenade_class", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{
		xr_vector<shared_str>& ammo_types = !grenadeMode_ ? ammoList2_ : m_ammoTypes;
		ammo_types.clear					(); 
		for ( int i = 0, count = _GetItemCount( str ); i < count; ++i )	
		{
			string128						ammo_item;
			_GetItem						( str, i, ammo_item );
			ammo_types.push_back			( ammo_item );
		}

		m_ammoType  = 0;
		inactiveAmmoIndex_ = 0;
	}
	result |= result2;

	result |= process_if_exists( section, "launch_speed", &CInifile::r_float, m_fLaunchSpeed, test );

	result2 = process_if_exists_set( section, "snd_shoot_grenade", &CInifile::r_string, str, test );
	if ( result2 && !test ) { m_sounds.LoadSound( section, "snd_shoot_grenade", "sndShotG", 0, false, m_eSoundShot );	}
	result |= result2;

	result2 = process_if_exists_set(section, "snd_shoot_grenade_dist", &CInifile::r_string, str, test);
	if (result2 && !test) { m_sounds.LoadSound(section, "snd_shoot_grenade_dist", "sndShotGDist", 0, false, m_eSoundShot); hasDistantShotGSnd_ = true; }
	result |= result2;

	result2 = process_if_exists_set( section, "snd_reload_grenade", &CInifile::r_string, str, test );
	if ( result2 && !test ) { m_sounds.LoadSound( section, "snd_reload_grenade", "sndReloadG", 0, true, m_eSoundReload );	}
	result |= result2;

	result2 = process_if_exists_set( section, "snd_switch", &CInifile::r_string, str, test );
	if ( result2 && !test ) { m_sounds.LoadSound( section, "snd_switch", "sndSwitch", 0, true, m_eSoundReload );	}
	result |= result2;

	return result;
}

void CWeaponMagazinedWGrenade::UnloadMagazine(bool spawn_ammo, u32 into_who_id, bool unload_secondary)
{
	inherited::UnloadMagazineEx(m_magazine, spawn_ammo, into_who_id); // call for first, active magazine

	if (unload_secondary)
		inherited::UnloadMagazineEx(inactiveMagazine_, spawn_ammo, into_who_id, false); // call for second, inactive magazine
}

void CWeaponMagazinedWGrenade::UpdateSecondVP(bool bCond_4)
{
	inherited::UpdateSecondVP(!grenadeMode_);
}

#include "string_table.h"
bool CWeaponMagazinedWGrenade::GetBriefInfo(II_BriefInfo& info)
{
	VERIFY(m_pCurrentInventory);
	/*
		if(!inherited::GetBriefInfo(info))
			return false;
	*/ 
	string32 int_str;
	
	int	ae = GetAmmoElapsed();
	int	ac = GetSuitableAmmoTotal();
	
	if (!unlimited_ammo())
		xr_sprintf(int_str, "%d/%d",ae,ac - ae);
	else
		xr_sprintf(int_str, "%d/--",ae);
	
	info.cur_ammo._set(int_str);
	
	if (HasFireModes())
	{
		if (m_iQueueSize == WEAPON_ININITE_QUEUE)
			info.fire_mode._set("A");
		else
		{
			xr_sprintf(int_str, "%d", m_iQueueSize);
			info.fire_mode._set(int_str);
		}
	}
	//if (m_pCurrentInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
	//	return false;

	GetSuitableAmmoTotal();

	u32 at_size = grenadeMode_ ? ammoList2_.size() : m_ammoTypes.size();
	if (unlimited_ammo() || at_size == 0)
	{
		info.fmj_ammo._set("--");
		info.ap_ammo._set("--");
	}
	else
	{
		u8 ammo_type = grenadeMode_ ? inactiveAmmoIndex_ : m_ammoType;
		xr_sprintf(int_str, "%d", grenadeMode_ ? GetAmmoCount2(0) : GetAmmoCount(0));
		if (ammo_type == 0)
			info.fmj_ammo._set(int_str);
		else
			info.ap_ammo._set(int_str);

		if (at_size == 2)
		{
			xr_sprintf(int_str, "%d", grenadeMode_ ? GetAmmoCount2(1) : GetAmmoCount(1));
			if (ammo_type == 0)
				info.ap_ammo._set(int_str);
			else
				info.fmj_ammo._set(int_str);
		}
		else
			info.ap_ammo._set("");
	}

	if (ae != 0 && m_magazine.size() != 0)
	{
		LPCSTR ammo_type = m_ammoTypes[m_magazine.back().m_LocalAmmoType].c_str();
		info.name._set(CStringTable().translate(pSettings->r_string(ammo_type, "inv_name_short")));
		info.icon._set(ammo_type);
	}
	else
	{
		LPCSTR ammo_type = m_ammoTypes[m_ammoType].c_str();
		info.name._set(CStringTable().translate(pSettings->r_string(ammo_type, "inv_name_short")));
		info.icon._set(ammo_type);
	}

	if (!IsGrenadeLauncherAttached())
	{
		info.grenade = "";
		return false;
	}

	int total2 = grenadeMode_ ? GetAmmoCount(0) : GetAmmoCount2(0);
	if (unlimited_ammo())
		xr_sprintf(int_str, "--");
	else
	{
		if (total2)
			xr_sprintf(int_str, "%d", total2);
		else
			xr_sprintf(int_str, "X");
	}
	info.grenade = int_str;

	return true;
}

int CWeaponMagazinedWGrenade::GetAmmoCount2(u8 ammo2_type) const
{
	VERIFY(m_pCurrentInventory);
	R_ASSERT(ammo2_type < ammoList2_.size());

	return GetAmmoCount_forType(ammoList2_[ammo2_type]);
}
