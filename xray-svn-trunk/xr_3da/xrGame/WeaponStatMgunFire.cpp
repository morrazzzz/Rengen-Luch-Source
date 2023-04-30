#include "stdafx.h"
#include "WeaponStatMgun.h"
#include "level.h"
#include "entity_alive.h"
#include "hudsound.h"
#include "actor.h"
#include "actorEffector.h"
#include "EffectorShot.h"
#include "weaponmiscs.h"

#include "pch_script.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "Weapon.h"

const Fvector&	CWeaponStatMgun::get_CurrentFirePoint()
{
	return m_fire_pos;
}

const Fmatrix&	CWeaponStatMgun::get_ParticlesXFORM	()						
{
	return m_fire_bone_xform;
}

void CWeaponStatMgun::FireStart()
{
	if (Owner())
		Owner()->callback(GameObject::eActionTypeWeaponFire)(Owner()->lua_game_object(), lua_game_object());

	m_dAngle.set(0.0f,0.0f);
	inheritedShooting::FireStart();
}

void CWeaponStatMgun::FireEnd()	
{
	m_dAngle.set(0.0f,0.0f);
	inheritedShooting::FireEnd();
	StopFlameParticles	();
	RemoveShotEffector ();
}

void CWeaponStatMgun::UpdateFire()
{
	fShotTimeCounter -= TimeDelta();
	

	inheritedShooting::UpdateFlameParticles();
	inheritedShooting::UpdateLight();

	if(!IsWorking()){
		clamp(fShotTimeCounter,0.0f, flt_max);
		return;
	}

	if(fShotTimeCounter<=0)
	{
		OnShot			();
		fShotTimeCounter		+= fOneShotTime;
	}else
	{
		angle_lerp		(m_dAngle.x,0.f,5.f,TimeDelta());
		angle_lerp		(m_dAngle.y,0.f,5.f,TimeDelta());
	}
}


void CWeaponStatMgun::OnShot()
{
	VERIFY(Owner());

	FireBullet				(	m_fire_pos, m_fire_dir, fireDispersionBase, *m_Ammo, 
								Owner()->ID(),ID(), SendHitAllowed(Owner()));

	StartShotParticles		();
	
	if(m_bLightShotEnabled) 
		Light_Start			();

	StartFlameParticles		();
	StartSmokeParticles		(m_fire_pos, zero_vel);
	OnShellDrop				(m_fire_pos, zero_vel);

	bool b_hud_mode =			(Level().CurrentEntity() == smart_cast<CObject*>(Owner()));
	m_sounds.PlaySound		("sndShot", m_fire_pos, Owner(), b_hud_mode, false, u8(-1), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));

	AddShotEffector			();
	m_dAngle.set			(	::Random.randF(-fireDispersionBase,fireDispersionBase),
								::Random.randF(-fireDispersionBase,fireDispersionBase));
}

void CWeaponStatMgun::AddShotEffector				()
{
	if(OwnerActor())
	{
		CCameraShotEffector* S	= smart_cast<CCameraShotEffector*>(OwnerActor()->Cameras().GetCamEffector(eCEShot)); 
		CameraRecoil		camera_recoil;
		//( camMaxAngle,camRelaxSpeed, 0.25f, 0.01f, 0.7f )
		camera_recoil.camMaxAngleVert		= camMaxAngle;
		camera_recoil.camRelaxSpeed			= camRelaxSpeed;
		camera_recoil.camMaxAngleHorz		= 0.25f;
		camera_recoil.camStepAngleHorz		= ::Random.randF(-1.0f, 1.0f) * 0.01f;
		camera_recoil.camDispersionFrac	= 0.7f;

		if (!S)	S			= (CCameraShotEffector*)OwnerActor()->Cameras().AddCamEffector(xr_new<CCameraShotEffector>(camera_recoil) );
		R_ASSERT			(S);
		S->Initialize		(camera_recoil);
		S->Shot2			(0.01f);
	}
}

void  CWeaponStatMgun::RemoveShotEffector	()
{
	if(OwnerActor())
		OwnerActor()->Cameras().RemoveCamEffector	(eCEShot);
}