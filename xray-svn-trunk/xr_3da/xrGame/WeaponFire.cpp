// WeaponFire.cpp: implementation of the CWeapon class.
// function responsible for firing with CWeapon
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Weapon.h"
#include "ParticlesObject.h"
#include "entity.h"
#include "actor.h"

#include "level_bullet_manager.h"

#define FLAME_TIME 0.05f


float _nrand(float sigma)
{
#define ONE_OVER_SIGMA_EXP (1.0f / 0.7975f)

	if(sigma == 0) return 0;

	float y;
	do{
		y = -logf(Random.randF());
	}while(Random.randF() > expf(-_sqr(y - 1.0f)*0.5f));
	if(rand() & 0x1)	return y * sigma * ONE_OVER_SIGMA_EXP;
	else				return -y * sigma * ONE_OVER_SIGMA_EXP;
}

void random_dir(Fvector& tgt_dir, const Fvector& src_dir, float dispersion)
{
	float sigma			= dispersion/3.f;
	float alpha			= clampr		(_nrand(sigma),-dispersion,dispersion);
	float theta			= Random.randF	(0,PI);
	float r 			= tan			(alpha);
	Fvector 			U,V,T;
	Fvector::generate_orthonormal_basis	(src_dir,U,V);
	U.mul				(r*_sin(theta));
	V.mul				(r*_cos(theta));
	T.add				(U,V);
	tgt_dir.add			(src_dir,T).normalize();
}

float CWeapon::GetWeaponDeterioration	()
{
	return conditionDecreasePerShot;
};

void CWeapon::FireTrace		(const Fvector& P, const Fvector& D)
{
	VERIFY		(m_magazine.size());

	CCartridge &l_cartridge = m_magazine.back();
//	Msg("ammo - %s", l_cartridge.m_ammoSect.c_str());
	VERIFY		(u16(-1) != l_cartridge.bullet_material_idx);
	//-------------------------------------------------------------	
	bool is_tracer	= m_bHasTracers && !!l_cartridge.m_flags.test(CCartridge::cfTracer);

	l_cartridge.m_flags.set	(CCartridge::cfTracer, is_tracer );
	if (m_u8TracerColorID != u8(-1))
		l_cartridge.param_s.u8ColorID	= m_u8TracerColorID;
	//-------------------------------------------------------------
	//�������� ������������ ������ � ������ ������� ����������� �������
//	float Deterioration = GetWeaponDeterioration();
//	Msg("Deterioration = %f", Deterioration);
	ChangeCondition(-GetWeaponDeterioration()*l_cartridge.param_s.impair);

	// ������� ������ ������
	float barrel_disp				= GetFireDispersion(true);
	// ������� �������� ��������
	const CInventoryOwner* pOwner = smart_cast<const CInventoryOwner*>(H_Parent());
	float shooter_disp = pOwner->GetWeaponAccuracy();

	bool SendHit = SendHitAllowed(H_Parent());
	Fvector3 shootDir;
	random_dir(shootDir, D, shooter_disp);
	//���������� ���� (� ������ ��������� �������� ������)
	for(int i = 0; i < l_cartridge.param_s.buckShot; ++i) 
	{
		FireBullet(P, shootDir, barrel_disp, l_cartridge, H_Parent()->ID(), ID(), SendHit);
	}

	StartShotParticles		();
	
	if(m_bLightShotEnabled) 
		Light_Start			();

	
	// Ammo
	m_magazine.pop_back	();
	--iAmmoElapsed;

	//��������� �� ��������� �� ������
	CheckForMisfire();

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeapon::Fire2Start()				
{ 
	bWorking2=true;	
}
void CWeapon::Fire2End	()
{ 
	//������������� ������������� ����������� ��������
	if(m_pFlameParticles2 && m_pFlameParticles2->IsLooped()) 
		StopFlameParticles2	();

	bWorking2=false;
}

void CWeapon::StopShooting		()
{
	SetPending			(TRUE); 

	//������������� ������������� ����������� ��������
	if(m_pFlameParticles && m_pFlameParticles->IsLooped())
		StopFlameParticles	();	

	SwitchState(eIdle);

	bWorking = false;
	//if(IsWorking()) FireEnd();
}
#include "pch_script.h"
#include "script_callback_ex.h"
#include "script_game_object.h"

void CWeapon::FireStart()
{
	if (H_Parent())
	{
		CGameObject* game_object = smart_cast<CGameObject*>(H_Parent());

		if (game_object)
			game_object->callback(GameObject::eActionTypeWeaponFire)(game_object->lua_game_object(), lua_game_object());
	}

	CShootingObject::FireStart();
}

void CWeapon::FireEnd				()
{
	CShootingObject::FireEnd();
	ClearShotEffector();
}


void CWeapon::StartFlameParticles2	()
{
	CShootingObject::StartParticles (m_pFlameParticles2, *m_sFlameParticles2, get_LastFP2());
}
void CWeapon::StopFlameParticles2	()
{
	CShootingObject::StopParticles (m_pFlameParticles2);
}
void CWeapon::UpdateFlameParticles2	()
{
	if (m_pFlameParticles2)			CShootingObject::UpdateParticles (m_pFlameParticles2, get_LastFP2());
}

void CWeapon::CalcEffectiveDistance()
{
	if (m_magazine.size())
	{
		CCartridge& current_patron = m_magazine.back();

		float val = aiAproximateEffectiveDistanceBase_ * current_patron.param_s.aiAproximateEffectiveDistanceK_;

		aproxEffectiveDistance_ = val;
	}
	else
		aproxEffectiveDistance_ = aiAproximateEffectiveDistanceBase_;
};