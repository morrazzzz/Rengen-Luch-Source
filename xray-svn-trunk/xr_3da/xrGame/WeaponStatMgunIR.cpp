#include "stdafx.h"
#include "WeaponStatMgun.h"
#include "xr_level_controller.h"


void CWeaponStatMgun::OnMouseMove			(int dx, int dy)
{
	float scale		= psMouseSens * psMouseSensScale/50.f;
	float h,p;
	m_destEnemyDir.getHP(h,p);
	if (dx){
		float d		= float(dx)*scale;
		h			-= d;
		SetDesiredDir						(h,p);
	}
	if (dy){
		float d		= ((psMouseInvert.test(1))?-1:1)*float(dy)*scale*3.f/4.f;
		p			-= d;
		SetDesiredDir						(h,p);
	}
}

void CWeaponStatMgun::OnKeyboardPress		(int dik)
{
	switch (dik)	
	{
	case kWPN_FIRE:					
		FireStart();
		break;
	};
}

void CWeaponStatMgun::OnKeyboardRelease	(int dik)
{
	switch (dik)	
	{
	case kWPN_FIRE:
		FireEnd();
		break;
	};
}

void CWeaponStatMgun::OnKeyboardHold		(int dik)
{}
