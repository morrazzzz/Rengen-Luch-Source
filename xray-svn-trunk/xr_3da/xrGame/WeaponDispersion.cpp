// WeaponDispersion.cpp: ������ ��� ��������
// 
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "weapon.h"
#include "inventoryowner.h"
#include "actor.h"
#include "inventory_item_impl.h"



//���������� 1, ���� ������ � �������� ��������� � >1 ���� ����������
float CWeapon::GetConditionDispersionFactor() const
{
	return (1.f + fireDispersionConditionFactor*(1.f-GetCondition()));
}

float CWeapon::GetFireDispersion	(bool with_cartridge) 
{
	if (!with_cartridge) return GetFireDispersion(1.0f);
	if (!m_magazine.empty()) m_fCurrentCartirdgeDisp = m_magazine.back().param_s.kDisp;
	return GetFireDispersion	(m_fCurrentCartirdgeDisp);
}

// ������� ������ �������� � ������ � ������ ���������, ������� � ������
float CWeapon::GetFireDispersion(float cartridge_k)
{
	return fireDispersionBase * cur_silencer_koef.fire_dispersion * cartridge_k * GetConditionDispersionFactor();
}

//////////////////////////////////////////////////////////////////////////
// ��� ������� ������ ������
void CWeapon::AddShotEffector		()
{
	R_ASSERT(m_pCurrentInventory);
	R_ASSERT(m_pCurrentInventory->GetOwner());

	inventory_owner().on_weapon_shot_start	(this);
/**
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if(pActor){
		CCameraShotEffector* S	= smart_cast<CCameraShotEffector*>	(pActor->EffectorManager().GetEffector(eCEShot)); 
		if (!S)	S				= (CCameraShotEffector*)pActor->EffectorManager().AddEffector(xr_new<CCameraShotEffectorX> (camMaxAngleVert,camRelaxSpeed, camMaxAngleHorz, camStepAngleHorz, camDispertionFrac));
		R_ASSERT				(S);
		S->SetRndSeed(pActor->GetShotRndSeed());
		S->SetActor(pActor);
		S->Shot					(camDispersion+camDispersionInc*float(ShotsFired()));
	}
/**/
}

void  CWeapon::RemoveShotEffector	()
{
	CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if (pInventoryOwner)
		pInventoryOwner->on_weapon_shot_remove	(this);
}

void	CWeapon::ClearShotEffector	()
{
	CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if (pInventoryOwner)
		pInventoryOwner->on_weapon_hide	(this);

};

void	CWeapon::StopShotEffector()
{
	CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if (pInventoryOwner)
		pInventoryOwner->on_weapon_shot_stop();
}
/**
const Fvector& CWeapon::GetRecoilDeltaAngle()
{
	CActor* pActor		= smart_cast<CActor*>(H_Parent());

	CCameraShotEffector* S = NULL;
	if(pActor)
		S = smart_cast<CCameraShotEffector*>(pActor->EffectorManager().GetEffector(eCEShot)); 

	if(S)
	{
		S->GetDeltaAngle(m_vRecoilDeltaAngle);
		return m_vRecoilDeltaAngle;
	}
	else
	{
		m_vRecoilDeltaAngle.set(0.f,0.f,0.f);
		return m_vRecoilDeltaAngle;
	}
}
/**/