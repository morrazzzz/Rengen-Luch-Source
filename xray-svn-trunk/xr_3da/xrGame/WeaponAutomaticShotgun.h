#pragma once
#include "WeaponMagazined.h"
#include "WeaponShotgun.h"
#include "script_export_space.h"

class CWeaponAutomaticShotgun :	public CWeaponMagazined
{
	typedef CWeaponMagazined inherited;
public:
					CWeaponAutomaticShotgun	();
	virtual			~CWeaponAutomaticShotgun();

	virtual void	LoadCfg					(LPCSTR section);
	
	// constant export of various weapon data to server. Here should be stuff, needed to be saved before object switches offline. Than it should be imported in SpawnAndImportSOData, when object gets online again
	virtual void	ExportDataToServer		(NET_Packet& P);

	virtual void	Reload					();
	void			switch2_StartReload		();
	void			switch2_AddCartgidge	();
	void			switch2_EndReload		();

	virtual void	PlayAnimOpenWeapon		();
	virtual void	PlayAnimAddOneCartridgeWeapon();
	void			PlayAnimCloseWeapon		();

	virtual bool	Action					(u16 cmd, u32 flags);
	virtual	int		GetCurrentFireMode	() { return m_aFireModes[m_iCurFireMode]; };

protected:
	virtual void	OnAnimationEnd			(u32 state);
	void			TriStateReload			();
	virtual void	OnStateSwitch			(u32 S);

	bool			HaveCartridgeInInventory(u8 cnt);
	virtual u8		AddCartridge			(u8 cnt);

	ESoundTypes		m_eSoundOpen;
	ESoundTypes		m_eSoundAddCartridge;
	ESoundTypes		m_eSoundClose;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
