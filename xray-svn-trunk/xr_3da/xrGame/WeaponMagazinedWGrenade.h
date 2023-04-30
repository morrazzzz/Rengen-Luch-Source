#pragma once
#include "weaponmagazined.h"
#include "rocketlauncher.h"


class CWeaponFakeGrenade;


class CWeaponMagazinedWGrenade : public CWeaponMagazined,
								 public CRocketLauncher
{
	typedef CWeaponMagazined inherited;
public:
					CWeaponMagazinedWGrenade	(ESoundTypes eSoundType=SOUND_TYPE_WEAPON_SUBMACHINEGUN);
	virtual			~CWeaponMagazinedWGrenade	();

	virtual void	LoadCfg				(LPCSTR section);
	
	// Happens when object gets online. Import saved data from server here. Do oject initialization based on data imported from server
	virtual BOOL	SpawnAndImportSOData(CSE_Abstract* data_containing_so); // Note, this particular class does not call inherided weapon net Spawn, instead it calls huditem object net Spawn directly
	virtual void	DestroyClientObj	();
	// constant export of various weapon data to server. Here should be stuff, needed to be saved before object switches offline. Than it should be imported in SpawnAndImportSOData, when object gets online again
	virtual void	ExportDataToServer	(NET_Packet& P); // Note, this particular class does not call inherided weapon net Export, instead it calls huditem object net Export directly

	virtual void	BeforeDetachFromParent	(bool just_before_destroy);

	// save stuff that is needed while weapon is in alife radius
	virtual void	save				(NET_Packet &output_packet);
	// load stuff needed in alife radius
	virtual void	load				(IReader &input_packet);


	virtual bool	Attach(PIItem pIItem, bool b_send_event);
	virtual bool	Detach(const char* item_section_name, bool b_spawn_item);
	virtual bool	CanAttach(PIItem pIItem);
	virtual bool	CanDetach(const char* item_section_name);
	virtual void	InitAddons();
	virtual bool	UseScopeTexture();
	virtual	float	CurrentZoomFactor	();

	// SecondVP code
	virtual void	UpdateSecondVP			(bool bCond_4 = true);

	virtual u8		GetCurrentHudOffsetIdx();
	virtual void	FireEnd					();
			void	LaunchGrenade			();
	
	virtual void	OnStateSwitch	(u32 S);
	
	virtual void	switch2_Reload	();
	virtual void	state_Fire		(float dt);
	virtual void	OnShot			();
	virtual void	OnEvent			(NET_Packet& P, u16 type);
	virtual void	ReloadMagazine	();

	virtual void	Chamber			();

			void	GrenadeShot		();

	virtual void	UnloadMagazine	(bool spawn_ammo = true, u32 into_who_id = u32(-1), bool unload_secondary = false);

	virtual bool	Action			(u16 cmd, u32 flags);

	virtual void	UpdateSounds	();

	//переключение в режим подствольника
	virtual bool	SwitchMode		();
	void			PerformSwitchGL	();
	void			OnAnimationEnd	(u32 state);
	virtual void	OnMagazineEmpty	();
	virtual bool	GetBriefInfo			(II_BriefInfo& info);

	virtual bool	IsNecessaryItem	    (const shared_str& item_sect);

	//виртуальные функции для проигрывания анимации HUD
	virtual void	PlayAnimShow		();
	virtual void	PlayAnimHide		();
	virtual void	PlayAnimReload		();
	virtual void	PlayAnimIdle		();
	virtual void	PlayAnimShoot		();
	virtual void	PlayAnimModeSwitch	();
	virtual void	PlayAnimBore		();

	//Fake grenade visability
	void			UpdateGrenadeVisibility(bool visibility);

private:
	virtual bool	install_upgrade_impl		( LPCSTR section, bool test );
	virtual	bool	install_upgrade_ammo_class	( LPCSTR section, bool test );

	bool					hasDistantShotGSnd_;
			int		GetAmmoCount2				( u8 ammo2_type ) const;

public:
	//дополнительные параметры патронов 
	//для подствольника
//-	CWeaponAmmo*			m_pAmmo2;
	xr_vector<shared_str>	ammoList2_; // переменная для временного хранения типов патронов неактивного магазина. Так же при инициализации хранит типы патронов для подствольника
	u8						inactiveAmmoIndex_; // переменная для временного хранения индекса типа патрона неактивного магазина.

	xr_vector<CCartridge>	inactiveMagazine_; // переменная для временного хранения содержимого неактивного магазина
	bool					grenadeMode_;

	CCartridge				m_DefaultCartridge2;

	int						magMaxSize1; // regular
	int						magMaxSize2; // gl
};