#include "StdAfx.h"
#include "sound_memory_rank_K.h"

SSoundParametersKoef::SSoundParametersKoef()
{
	soundPowerK_				= 1.0f;
	soundThresholdK_			= 1.0f;
}

void SSoundParametersKoef::LoadCfg(LPCSTR section)
{
	soundThresholdK_	= pSettings->r_float(section, "sound_threshold_K");
	soundPowerK_		= pSettings->r_float(section, "sound_power_K");
}