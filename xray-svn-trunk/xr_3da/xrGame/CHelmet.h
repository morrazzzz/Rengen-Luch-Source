#pragma once

#include "OutfitBase.h"


struct SBoneProtections;

class CHelmet
	: public COutfitBase 
{
public:
	
	CHelmet();

	void LoadCfg(LPCSTR section) override;

	float m_fShowNearestEnemiesDistance;
	float					m_fPowerLossAdd;

protected:

	virtual bool install_upgrade_impl(LPCSTR section, bool test);
	
private:

	typedef	COutfitBase inherited;
};