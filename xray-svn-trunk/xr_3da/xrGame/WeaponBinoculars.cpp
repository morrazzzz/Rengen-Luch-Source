#include "stdafx.h"
#include "WeaponBinoculars.h"

#include "xr_level_controller.h"

#include "level.h"
#include "WeaponBinocularsVision.h"
#include "object_broker.h"
#include "inventory.h"

CWeaponBinoculars::CWeaponBinoculars()
{
}

CWeaponBinoculars::~CWeaponBinoculars()
{
}

void CWeaponBinoculars::LoadCfg	(LPCSTR section)
{
	inherited::LoadCfg(section);

	// Sounds
	m_sounds.LoadSound(section, "snd_zoomin", "sndZoomIn", 0, false, SOUND_TYPE_ITEM_USING);
	m_sounds.LoadSound(section, "snd_zoomout", "sndZoomOut", 0, false, SOUND_TYPE_ITEM_USING);
}

bool CWeaponBinoculars::Action(u16 cmd, u32 flags) 
{
	switch(cmd) 
	{
	case kWPN_FIRE : 
		return inherited::Action(kWPN_ZOOM, flags);
	}

	return inherited::Action(cmd, flags);
}

void CWeaponBinoculars::OnZoomIn		()
{
	if(H_Parent() && !IsZoomed())
	{
		m_sounds.StopSound("sndZoomOut");
		bool b_hud_mode = (Level().CurrentEntity() == H_Parent());
		m_sounds.PlaySound("sndZoomIn", H_Parent()->Position(), H_Parent(), b_hud_mode);
	}
	inherited::OnZoomIn		();
}

void CWeaponBinoculars::OnZoomOut		()
{
	if(H_Parent() && IsZoomed() && !IsRotatingToZoom())
	{
		m_sounds.StopSound("sndZoomIn");
		bool b_hud_mode = (Level().CurrentEntity() == H_Parent());	
		m_sounds.PlaySound("sndZoomOut", H_Parent()->Position(), H_Parent(), b_hud_mode);
	}

	inherited::OnZoomOut();
}

BOOL CWeaponBinoculars::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	inherited::SpawnAndImportSOData(data_containing_so);
	
	return TRUE;
}

void GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor)
{
	float def_fov = float(camFov);
	float min_zoom_k = 0.3f;
	float zoom_step_count = 3.0f;
	float delta_factor_total = def_fov-scope_factor;
	VERIFY(delta_factor_total>0);
	min_zoom_factor = def_fov-delta_factor_total*min_zoom_k;
	delta = (delta_factor_total*(1-min_zoom_k) )/zoom_step_count;

}

void CWeaponBinoculars::ZoomInc()
{
	float delta,min_zoom_factor;
	GetZoomData(m_zoom_params.m_fScopeZoomFactor, delta, min_zoom_factor);

	float f					= GetZoomFactor()-delta;
	clamp					(f,m_zoom_params.m_fScopeZoomFactor,min_zoom_factor);
	SetZoomFactor			( f );
}

void CWeaponBinoculars::ZoomDec()
{
	float delta,min_zoom_factor;
	GetZoomData(m_zoom_params.m_fScopeZoomFactor,delta,min_zoom_factor);

	float f					= GetZoomFactor()+delta;
	clamp					(f,m_zoom_params.m_fScopeZoomFactor,min_zoom_factor);
	SetZoomFactor			( f );

}

//tatarinrafa: Cweapon alredy has this

//void CWeaponBinoculars::save(NET_Packet &output_packet)
//{
//	inherited::save(output_packet);
//	save_data		(m_fRTZoomFactor,output_packet);
//}

//void CWeaponBinoculars::load(IReader &input_packet)
//{
//	inherited::load(input_packet);
//	load_data		(m_fRTZoomFactor,input_packet);
//}

bool CWeaponBinoculars::GetBriefInfo(II_BriefInfo& info)
{
	info.name		= NameShort();
	info.cur_ammo	= "";
	info.icon		= *SectionName();

	return true;
}
