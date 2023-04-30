#include "stdafx.h"
#include "WeaponZoomable.h"

CWeaponZoomable::CWeaponZoomable() : CWeaponBinoculars()
{
}

CWeaponZoomable::~CWeaponZoomable()
{
}

void CWeaponZoomable::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);
}

bool CWeaponZoomable::GetBriefInfo(II_BriefInfo& info)
{
	return CWeaponCustomPistol::GetBriefInfo(info);
}

bool CWeaponZoomable::Action(u16 cmd, u32 flags) 
{
	return CWeaponCustomPistol::Action(cmd, flags);
}

void CWeaponZoomable::OnZoomIn		()
{
		inherited::OnZoomIn();
}

void CWeaponZoomable::OnZoomOut		()
{
		inherited::OnZoomOut();
}

void CWeaponZoomable::ZoomInc()
{
	inherited::ZoomInc();
}

void CWeaponZoomable::ZoomDec()
{
	inherited::ZoomDec();
}