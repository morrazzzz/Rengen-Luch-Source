#pragma once

// For rank dependance
struct SSoundParametersKoef
{
	float						soundPowerK_;
	float						soundThresholdK_;

	SSoundParametersKoef();

	void		LoadCfg(LPCSTR section);
};