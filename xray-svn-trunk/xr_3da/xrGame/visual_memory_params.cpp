
//	Created 	: 09.12.2004
//	Author		: Dmitriy Iassenev

#include "stdafx.h"
#include "visual_memory_params.h"
#include "memory_space.h"

CVisionParameters::CVisionParameters()
{
	m_transparency_threshold	= 0.0f;
	m_still_visible_time		= 0;
	m_min_view_distance			= 0.0f;
	m_max_view_distance			= 0.0f;
	m_visibility_threshold		= 0.0f;
	m_always_visible_distance	= 0.0f;
	m_time_quant				= 0.0f;
	m_decrease_value			= 0.0f;
	m_velocity_factor			= 0.0f;
	m_luminocity_factor			= 0.0f;
}

void CVisionParameters::LoadCfg	(LPCSTR section, bool not_a_stalker)
{
	m_transparency_threshold	= pSettings->r_float(section, "transparency_threshold");
	m_still_visible_time		= READ_IF_EXISTS(pSettings, r_u32, section, "still_visible_time", 0);

#ifndef USE_STALKER_VISION_FOR_MONSTERS
	if (!not_a_stalker)
		return;
#endif
	m_min_view_distance			= pSettings->r_float(section, "min_view_distance");
	m_max_view_distance			= pSettings->r_float(section, "max_view_distance");
	m_visibility_threshold		= pSettings->r_float(section, "visibility_threshold");
	m_always_visible_distance	= pSettings->r_float(section, "always_visible_distance");
	m_time_quant				= pSettings->r_float(section, "time_quant");
	m_decrease_value			= pSettings->r_float(section, "decrease_value");
	m_velocity_factor			= pSettings->r_float(section, "velocity_factor");
	m_luminocity_factor			= pSettings->r_float(section, "luminocity_factor");
}

SVisionParametersKoef::SVisionParametersKoef()
{
	transparencyThresholdK_		= 1.0f;
	stillVisibleTimeK_			= 1.0f;
	minViewDistanceK_			= 1.0f;
	maxViewDistanceK_			= 1.0f;
	visibilityThresholdK_		= 1.0f;
	alwaysVisibleDistanceK_		= 1.0f;
	timeQuantK_					= 1.0f;
	decreaseValueK_				= 1.0f;
	velocityFactorK_			= 1.0f;
	luminocityFactorK_			= 1.0f;

	eyeFovK_					= 1.0f;
	eyeRangeK_					= 1.0f;
}

void SVisionParametersKoef::LoadCfg(LPCSTR section)
{
	transparencyThresholdK_		= pSettings->r_float(section, "transparency_threshold_K");
	stillVisibleTimeK_			= pSettings->r_float(section, "still_visible_time_K");
	minViewDistanceK_			= pSettings->r_float(section, "min_view_distance_K");
	maxViewDistanceK_			= pSettings->r_float(section, "max_view_distance_K");
	visibilityThresholdK_		= pSettings->r_float(section, "visibility_threshold_K");
	alwaysVisibleDistanceK_		= pSettings->r_float(section, "always_visible_distance_K");
	timeQuantK_					= pSettings->r_float(section, "time_quant_K");
	decreaseValueK_				= pSettings->r_float(section, "decrease_value_K");
	velocityFactorK_			= pSettings->r_float(section, "velocity_factor_K");
	luminocityFactorK_			= pSettings->r_float(section, "luminocity_factor_K");

	eyeFovK_					= pSettings->r_float(section, "eye_fov_K");
	eyeRangeK_					= pSettings->r_float(section, "eye_range_K");
}
