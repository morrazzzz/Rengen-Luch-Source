
//	Created 	: 09.12.2004
//	Author		: Dmitriy Iassenev

#pragma once

struct CVisionParameters
{
	float						m_min_view_distance;
	float						m_max_view_distance;
	float						m_visibility_threshold;
	float						m_always_visible_distance;
	float						m_time_quant;
	float						m_decrease_value;
	float						m_velocity_factor;
	float						m_transparency_threshold;
	float						m_luminocity_factor;
	u32							m_still_visible_time;

	CVisionParameters();

	void		LoadCfg		(LPCSTR section, bool not_a_stalker);
};

// For rank dependance
struct SVisionParametersKoef
{
	float						minViewDistanceK_;
	float						maxViewDistanceK_;
	float						visibilityThresholdK_;
	float						alwaysVisibleDistanceK_;
	float						timeQuantK_;
	float						decreaseValueK_;
	float						velocityFactorK_;
	float						transparencyThresholdK_;
	float						luminocityFactorK_;
	float						stillVisibleTimeK_;

	float						eyeFovK_;
	float						eyeRangeK_;

	SVisionParametersKoef();

	void		LoadCfg		(LPCSTR section);
};
