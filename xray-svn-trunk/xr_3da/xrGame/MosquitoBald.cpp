#include "stdafx.h"
#include "mosquitobald.h"
#include "physicsshellholder.h"
#include "../xr_collide_form.h"
#include "Actor.h"
#include "Car.h"

CMosquitoBald::CMosquitoBald(void) 
{
	m_fHitImpulseScale		= 1.f;

	m_bLastBlowoutUpdate	= false;
}

CMosquitoBald::~CMosquitoBald(void) 
{
}

void CMosquitoBald::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);

	m_killCarEngine = !!READ_IF_EXISTS(pSettings, r_bool, section, "car_kill_engine", false);
	m_hitKoefCar = READ_IF_EXISTS(pSettings, r_float, section, "hit_koef_car", 1.f);
}


void CMosquitoBald::Postprocess(f32 /**val/**/) 
{
}

bool CMosquitoBald::BlowoutState()
{
	bool result = inherited::BlowoutState();
	if(!result)
	{
		m_bLastBlowoutUpdate = false;
		UpdateBlowout();
	}
	else if(!m_bLastBlowoutUpdate)
	{
		m_bLastBlowoutUpdate = true;
		UpdateBlowout();
	}

	return result;
}

bool CMosquitoBald::ShouldIgnoreObject(CGameObject* pObject)
{
	auto pCar = smart_cast<CCar*>(pObject);
	if (pCar) return false;

	return inherited::ShouldIgnoreObject(pObject);
}

void CMosquitoBald::Affect(SZoneObjectInfo* O) 
{
	CPhysicsShellHolder *pGameObject = smart_cast<CPhysicsShellHolder*>(O->object);
	if(!pGameObject) return;

	if(O->zone_ignore) return;

	Fvector P; 
	XFORM().transform_tiny(P,CFORM()->getSphere().P);

	Fvector hit_dir; 
	hit_dir.set(	::Random.randF(-.5f,.5f), 
					::Random.randF(.0f,1.f), 
					::Random.randF(-.5f,.5f)); 
	hit_dir.normalize();

	Fvector position_in_bone_space;

	VERIFY(!pGameObject->getDestroy());

	float dist = pGameObject->Position().distance_to(P) - pGameObject->Radius();
	float power = Power(dist>0.f?dist:0.f, Radius());
	float impulse = m_fHitImpulseScale*power*pGameObject->GetMass();


	if(power > 0.01f) 
	{
		position_in_bone_space.set(0.f,0.f,0.f);

		auto car = smart_cast<CCar*>(pGameObject);
		if (car != nullptr)
		{
			if (m_killCarEngine)
			{
				car->KillEngine();
			}
			power *= m_hitKoefCar;
		}
		CreateHit(pGameObject->ID(),ID(),hit_dir,power,0,position_in_bone_space,impulse,m_eHitTypeBlowout);

		PlayHitParticles(pGameObject);
	}
}

void CMosquitoBald::UpdateSecondaryHit()
{
	if (m_dwAffectFrameNum == CurrentFrame())
		return;

	m_dwAffectFrameNum = CurrentFrame();
	if (Device.dwPrecacheFrame)
		return;

	OBJECT_INFO_VEC_IT it;
	for (it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it)
	{
		if (!(*it).object->getDestroy())
		{
			CPhysicsShellHolder *pGameObject = smart_cast<CPhysicsShellHolder*>((&(*it))->object);
			if (!pGameObject) return;

			if ((&(*it))->zone_ignore) return;
			Fvector P;
			XFORM().transform_tiny(P, CFORM()->getSphere().P);

			Fvector hit_dir;
			hit_dir.set(::Random.randF(-.5f, .5f),
				::Random.randF(.0f, 1.f),
				::Random.randF(-.5f, .5f));
			hit_dir.normalize();

			Fvector position_in_bone_space;

			VERIFY(!pGameObject->getDestroy());

			float dist = pGameObject->Position().distance_to(P) - pGameObject->Radius();
			float power = m_fSecondaryHitPower * Power(dist > 0.f ? dist : 0.f, Radius());
			if (power < 0.0f)
				return;

			float impulse = m_fHitImpulseScale * power*pGameObject->GetMass();
			position_in_bone_space.set(0.f, 0.f, 0.f);
			CreateHit(pGameObject->ID(), ID(), hit_dir, power, 0, position_in_bone_space, impulse, m_eHitTypeBlowout);
		}
	}
}