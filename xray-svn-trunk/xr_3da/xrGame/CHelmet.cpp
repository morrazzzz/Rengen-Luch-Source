#include "stdafx.h"
#include "CHelmet.h"


CHelmet::CHelmet()
{
	m_baseSlot = HELMET_SLOT;
	m_fPowerLossAdd = 0.f;
}

void CHelmet::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);

	m_fShowNearestEnemiesDistance = READ_IF_EXISTS(pSettings, r_float, section, "nearest_enemies_show_dist", 0.0f);
	m_fPowerLossAdd					= READ_IF_EXISTS(pSettings, r_float, section, "power_loss_add", 0.0f);
}

//------------>------Upgrades------<-------------

bool CHelmet::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result = inherited::install_upgrade_impl(section, test);

	result |= process_if_exists(section, "nearest_enemies_show_dist", &CInifile::r_float, m_fShowNearestEnemiesDistance, test);
	result |= process_if_exists(section, "power_loss_add",			  &CInifile::r_float, m_fPowerLossAdd, test );

	return result;
}