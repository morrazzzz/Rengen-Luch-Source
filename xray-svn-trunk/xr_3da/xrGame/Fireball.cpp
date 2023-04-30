#include "stdafx.h"
#include "Fireball.h"
#include "xr_level_controller.h"

CFireball::CFireball() : CWeaponKnife()
{
}

CFireball::~CFireball(void)
{
}

void CFireball::LoadCfg(LPCSTR section)
{
	//inherited::LoadCfg(section);
	CWeapon::LoadCfg(section);

	fWallmarkSize = pSettings->r_float(section,"wm_size");

	m_sounds.LoadSound(section, "snd_shoot", "sndShot", 0, false, SOUND_TYPE_WEAPON_SHOOTING);
	
	knife_material_idx =  GMLib.GetMaterialIdx(pSettings->r_string(*hud_sect,"fireball_material"));
}

bool CFireball::Action(u16 cmd, u32 flags)
{
	switch(cmd){
		case kWPN_FIRE: {
			if (flags&CMD_START){
				SwitchState(ePreFire);
				return true;
			} else if (flags&CMD_STOP && GetState()==ePreFire) {
				SwitchState(eIdle);
				return true;
			}
		} return true;
		case kWPN_ZOOM: return true;

	}
	return inherited::Action(cmd, flags);
}

void CFireball::OnStateSwitch		(u32 S)
{
	inherited::OnStateSwitch(S);

	switch (S) {
		case eFire: {
			StartFlameParticles();
			fShotTimeCounter			+=	fOneShotTime;
		}return;
		case eFire2: {
			//Msg("eFire2 CFireball called!!! WTF???");
					 }return;
		case ePreFire: {
			switch2_PreFire	();
		}break;
	}
}



void CFireball::switch2_PreFire	()
{
	if(IsPending())	return;

	PlayHUDMotion("anm_activate", FALSE, this, ePreFire);
	SetPending		(TRUE);
}

void CFireball::OnAnimationEnd		(u32 state)
{
	switch (state)
	{
	case eFire: {
				Fvector	p1, d; 
				p1.set(get_LastFP()); 
				d.set(get_LastFD());

				KnifeStrike(p1,d);

				SwitchState(eIdle);
		}return;
		case ePreFire:  SetPending	(FALSE); SwitchState(eFire); return;

	}
	inherited::OnAnimationEnd(state);
}


void CFireball::LoadFireParams(LPCSTR section)
{
	CWeapon::LoadFireParams(section);

	fvHitPower_1		= fvHitPower;
	fHitImpulse_1		= fHitImpulse;
	m_eHitType_1		= ALife::g_tfString2HitType(pSettings->r_string(section, "hit_type"));
}

void CFireball::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	inherited::UpdateCL();

	float dt = TimeDelta();

	//когда происходит апдейт состояния оружия
	//ничего другого не делать
	if(GetNextState() == GetState())
	{
		switch (GetState())
		{
		case eShowing:
		case eHiding:
		case eIdle:
			fShotTimeCounter -= dt;

			if (fShotTimeCounter < 0)
				fShotTimeCounter = 0;

			break;
		case eFire:			
			//if(iAmmoElapsed>0)
			//	state_Fire		(dt);
			
			if(fShotTimeCounter <= 0)
				StopShooting();
			else
				fShotTimeCounter -= dt;

			break;
		case eHidden:		break;
		}
	}
	
	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
#endif
}

#include "pch_script.h"

using namespace luabind;

void CFireball::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CFireball,CGameObject>("CFireball")
			.def(constructor<>())
	];
}



