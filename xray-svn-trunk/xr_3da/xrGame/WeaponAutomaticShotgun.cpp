#include "stdafx.h"
#include "WeaponAutomaticShotgun.h"
#include "entity.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "level.h"
#include "actor.h"
#include "weaponmiscs.h"

CWeaponAutomaticShotgun::CWeaponAutomaticShotgun()
{
	m_eSoundClose			= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
	m_eSoundAddCartridge	= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
}

CWeaponAutomaticShotgun::~CWeaponAutomaticShotgun()
{
}

void CWeaponAutomaticShotgun::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);

	if(pSettings->line_exist(section, "tri_state_reload")){
		m_bTriStateReload = !!pSettings->r_bool(section, "tri_state_reload");
	};
	if(m_bTriStateReload){
		m_sounds.LoadSound(section, "snd_open_weapon", "sndOpen", 0, false, m_eSoundOpen);

		m_sounds.LoadSound(section, "snd_add_cartridge", "sndAddCartridge", 0, false, m_eSoundAddCartridge);

		m_sounds.LoadSound(section, "snd_close_weapon", "sndClose", 0, false, m_eSoundClose);
	};

}

bool CWeaponAutomaticShotgun::Action(u16 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;

	if(	m_bTriStateReload && GetState()==eReload &&
		cmd==kWPN_FIRE && flags&CMD_START &&
		m_sub_state==eSubstateReloadInProcess		)//остановить перезагрузку
	{
		AddCartridge(1);
		m_sub_state = eSubstateReloadEnd;
		return true;
	}
	return false;
}

void CWeaponAutomaticShotgun::OnAnimationEnd(u32 state) 
{
	if(!m_bTriStateReload || state != eReload)
		return inherited::OnAnimationEnd(state);

	switch(m_sub_state){
		case eSubstateReloadBegin:{
			m_sub_state = eSubstateReloadInProcess;
			SwitchState(eReload);
		}break;

		case eSubstateReloadInProcess:{
			if( 0 != AddCartridge(1) ){
				m_sub_state = eSubstateReloadEnd;
			}
			SwitchState(eReload);
		}break;

		case eSubstateReloadEnd:{
			m_sub_state = eSubstateReloadBegin;
			SwitchState(eIdle);
		}break;
		
	};
}

void CWeaponAutomaticShotgun::Reload() 
{
	if(m_bTriStateReload){
		TriStateReload();
	}else
		inherited::Reload();
}

void CWeaponAutomaticShotgun::TriStateReload()
{
	if (m_magazine.size() == (u32)maxMagazineSize_ || !HaveCartridgeInInventory(1))return;
	CWeapon::Reload		();
	m_sub_state			= eSubstateReloadBegin;
	SwitchState			(eReload);
}

void CWeaponAutomaticShotgun::OnStateSwitch	(u32 S)
{
	if(!m_bTriStateReload || S != eReload){
		inherited::OnStateSwitch(S);
		return;
	}

	CWeapon::OnStateSwitch(S);

	if (m_magazine.size() == (u32)maxMagazineSize_ || !HaveCartridgeInInventory(1)){
			switch2_EndReload		();
			m_sub_state = eSubstateReloadEnd;
			return;
	};

	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		if( HaveCartridgeInInventory(1) )
			switch2_StartReload	();
		break;
	case eSubstateReloadInProcess:
			if( HaveCartridgeInInventory(1) )
				switch2_AddCartgidge	();
		break;
	case eSubstateReloadEnd:
			switch2_EndReload		();
		break;
	};
}

void CWeaponAutomaticShotgun::switch2_StartReload()
{
	PlaySound			("sndOpen",get_LastFP());
	PlayAnimOpenWeapon	();
	SetPending			(TRUE);
}

void CWeaponAutomaticShotgun::switch2_AddCartgidge	()
{
	PlaySound("sndAddCartridge", get_LastFP(), Random.randF(MIN_RND_FREQ__ADD_CARTRIDGE, MAX_RND_FREQ__ADD_CARTRIDGE));
	PlayAnimAddOneCartridgeWeapon();
	SetPending			(TRUE);
}

void CWeaponAutomaticShotgun::switch2_EndReload	()
{
	SetPending			(FALSE);
	PlaySound			("sndClose",get_LastFP());
	PlayAnimCloseWeapon	();
}

void CWeaponAutomaticShotgun::PlayAnimOpenWeapon()
{
	VERIFY(GetState()==eReload);
	PlayHUDMotion("anm_open",FALSE,this,GetState());
}
void CWeaponAutomaticShotgun::PlayAnimAddOneCartridgeWeapon()
{
	VERIFY(GetState()==eReload);
	PlayHUDMotion("anm_add_cartridge",FALSE,this,GetState());
}
void CWeaponAutomaticShotgun::PlayAnimCloseWeapon()
{
	VERIFY(GetState()==eReload);
	PlayHUDMotion("anm_close",FALSE,this,GetState());
}

bool CWeaponAutomaticShotgun::HaveCartridgeInInventory(u8 cnt)
{
	if (unlimited_ammo()) return true;
	m_pCurrentAmmo = NULL;
	if (m_pCurrentInventory)
	{
		//попытаться найти в инвентаре патроны текущего типа 
		m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[m_ammoType]));

		if (!m_pCurrentAmmo)
		{
			for (u8 i = 0; i < m_ammoTypes.size(); ++i)
			{
				//проверить патроны всех подходящих типов
				m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[i]));
				if (m_pCurrentAmmo)
				{
					m_ammoType = i;
					break;
				}
			}
		}
	}
	return (m_pCurrentAmmo != NULL) && (m_pCurrentAmmo->m_boxCurr >= cnt);
}


u8 CWeaponAutomaticShotgun::AddCartridge		(u8 cnt)
{
	if (IsMisfire())	bMisfire = false;

	if (m_changed_ammoType_on_reload != u8(-1)){
		m_ammoType = m_changed_ammoType_on_reload;
		SetAmmoOnReload(u8(-1));

	}

	if (!HaveCartridgeInInventory(1))
		return 0;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());


	if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
		m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));
	CCartridge l_cartridge = m_DefaultCartridge;
	while (cnt)// && m_pCurrentAmmo->Get(l_cartridge)) 
	{
		if (!unlimited_ammo())
		{
			if (!m_pCurrentAmmo->Get(l_cartridge)) break;
		}
		--cnt;
		++iAmmoElapsed;
		l_cartridge.m_LocalAmmoType = u8(m_ammoType);
		m_magazine.push_back(l_cartridge);
		//		m_fCurrentCartirdgeDisp = l_cartridge.m_kDisp;
	}
	m_ammoName = (m_pCurrentAmmo) ? m_pCurrentAmmo->m_nameShort : NULL;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	//выкинуть коробку патронов, если она пустая
	if (m_pCurrentAmmo && !m_pCurrentAmmo->m_boxCurr)
		m_pCurrentAmmo->SetDropManual(TRUE);

	return cnt;
}

void	CWeaponAutomaticShotgun::ExportDataToServer(NET_Packet& P)
{
	inherited::ExportDataToServer(P);	
}
