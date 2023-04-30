#include "stdafx.h"
#include "NightVisionEffector.h"
#include "actor.h"
#include "inventory.h"
#include "actorEffector.h"
#include "../CustomHUD.h"


CNightVisionEffector::CNightVisionEffector(const char* snd_sect)
	: m_pActor(nullptr)
{
	if (m_has_sounds = (pSettings->line_exist(snd_sect, "snd_night_vision_on") &&
		pSettings->line_exist(snd_sect, "snd_night_vision_off") &&
		pSettings->line_exist(snd_sect, "snd_night_vision_idle") &&
		pSettings->line_exist(snd_sect, "snd_night_vision_broken")))
	{
		m_sounds.LoadSound(snd_sect, "snd_night_vision_on", "NightVisionOnSnd", 0, false, SOUND_TYPE_ITEM_USING);
		m_sounds.LoadSound(snd_sect, "snd_night_vision_off", "NightVisionOffSnd", 0, false, SOUND_TYPE_ITEM_USING);
		m_sounds.LoadSound(snd_sect, "snd_night_vision_idle", "NightVisionIdleSnd", 0, false, SOUND_TYPE_ITEM_USING);
		m_sounds.LoadSound(snd_sect, "snd_night_vision_broken", "NightVisionBrokenSnd", 0, false, SOUND_TYPE_ITEM_USING);
	}
}

CNightVisionEffector::~CNightVisionEffector()
{
}

void CNightVisionEffector::StartPPE(const shared_str& sect, CActor* pA, bool play_sound)
{
	m_pActor = pA;
	AddEffector(m_pActor, effNightvision, sect);
	if (play_sound && m_has_sounds)
	{
		PlaySounds(eStartSound);
		PlaySounds(eIdleSound);
	}
}

void CNightVisionEffector::StopPPE(const float factor, bool play_sound)
{
	if (!m_pActor)		return;
	CEffectorPP* pp = m_pActor->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
	if (pp)
	{
		pp->Stop(factor);
		if (play_sound && m_has_sounds)
		{
			PlaySounds(eStopSound);
			m_sounds.StopSound("NightVisionIdleSnd");
		}
	}
}

void CNightVisionEffector::StartShader(const shared_str& sect, CActor* pA, bool play_sound)
{
	m_pActor = pA;

	g_hud->nightVisionEffect_.needNVShading_ = true;

	g_hud->nightVisionEffect_.nvColor_					= pSettings->r_fvector3(sect.c_str(), "color");
	g_hud->nightVisionEffect_.nvSaturation_				= pSettings->r_float(sect.c_str(), "saturation");
	g_hud->nightVisionEffect_.nvSaturationOut_			= pSettings->r_float(sect.c_str(), "saturation_out");
	g_hud->nightVisionEffect_.borderShadowingStop_		= pSettings->r_float(sect.c_str(), "shadowing_border_stop");
	g_hud->nightVisionEffect_.borderShadowingOut_		= pSettings->r_float(sect.c_str(), "shadowing_border_out");
	g_hud->nightVisionEffect_.monocularRadius_			= pSettings->r_float(sect.c_str(), "monocular_radius");
	g_hud->nightVisionEffect_.nightVisionGoogleType_	= pSettings->r_u8(sect.c_str(), "form_type");
	g_hud->nightVisionEffect_.nvGrainPower_				= pSettings->r_float(sect.c_str(), "grain_power");
	g_hud->nightVisionEffect_.nvGrainSize_				= pSettings->r_float(sect.c_str(), "grain_size");
	g_hud->nightVisionEffect_.nvGrainClrIntensity_		= pSettings->r_float(sect.c_str(), "grain_clr_intensity");
	g_hud->nightVisionEffect_.nvHasBrokenCamEffect_ =	!!READ_IF_EXISTS(pSettings, r_bool, sect.c_str(), "has_broken_cam_eff", FALSE);

	if (play_sound && m_has_sounds)
	{
		PlaySounds(eStartSound);
		PlaySounds(eIdleSound);
	}
}

void CNightVisionEffector::StopShader(bool play_sound)
{
	if (!m_pActor)
		return;

	if (g_hud->nightVisionEffect_.needNVShading_)
	{
		g_hud->nightVisionEffect_.needNVShading_ = false;

		if (play_sound && m_has_sounds)
		{
			PlaySounds(eStopSound);
			m_sounds.StopSound("NightVisionIdleSnd");
		}
	}
}

bool CNightVisionEffector::IsActive()
{
	if (!m_pActor)
		return false;

	CEffectorPP* pp = m_pActor->Cameras().GetPPEffector((EEffectorPPType)effNightvision);

	return (pp != nullptr || g_hud->nightVisionEffect_.needNVShading_ == true);
}

void CNightVisionEffector::OnDisabled(CActor* pA, bool play_sound)
{
	m_pActor = pA;
	if (play_sound && m_has_sounds)
		PlaySounds(eBrokeSound);
}

void CNightVisionEffector::PlaySounds(EPlaySounds which)
{
	if (!m_pActor)
		return;

	bool bPlaySoundFirstPerson = !!m_pActor->HUDview();
	switch (which)
	{
	case eStartSound:
	{
		m_sounds.PlaySound("NightVisionOnSnd", m_pActor->Position(), nullptr, bPlaySoundFirstPerson);
	}break;
	case eStopSound:
	{
		m_sounds.PlaySound("NightVisionOffSnd", m_pActor->Position(), nullptr, bPlaySoundFirstPerson);
	}break;
	case eIdleSound:
	{
		m_sounds.PlaySound("NightVisionIdleSnd", m_pActor->Position(), nullptr, bPlaySoundFirstPerson, true);
	}break;
	case eBrokeSound:
	{
		m_sounds.PlaySound("NightVisionBrokenSnd", m_pActor->Position(), nullptr, bPlaySoundFirstPerson);
	}break;
	default: NODEFAULT;
	}
}