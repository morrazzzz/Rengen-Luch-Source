#pragma once

#include "weapon.h"
#include "hudsound.h"
#include "ai_sounds.h"

class ENGINE_API CMotionDef;

//������ ������� ��������� �������������
//����������� ��������, ������, ���� ��������� �������
#define WEAPON_ININITE_QUEUE -1


class CWeaponMagazined: public CWeapon
{
private:
	typedef CWeapon inherited;
protected:
	//�������������� ���������� � ���������
	LPCSTR			m_sSilencerFlameParticles;
	LPCSTR			m_sSilencerSmokeParticles;

	ESoundTypes		m_eSoundShow;
	ESoundTypes		m_eSoundHide;
	ESoundTypes		m_eSoundShot;
	ESoundTypes		m_eSoundEmptyClick;
	ESoundTypes		m_eSoundReload;
	ESoundTypes		m_eSoundReloadJammed;
	ESoundTypes		m_eSoundReloadNotEmpty;

	bool			m_sounds_enabled;

	// General
	//���� ������� ��������� UpdateSounds
	u32				dwUpdateSounds_Frame;

	bool			hasDistantShotSnd_;
protected:
	virtual void	OnMagazineEmpty	();

	virtual void	switch2_Idle	();
	virtual void	switch2_Fire	();
	virtual void	switch2_Empty	();
	virtual void	switch2_Reload	();
	virtual void	switch2_Hiding	();
	virtual void	switch2_Hidden	();
	virtual void	switch2_Showing	();
	bool			m_bChamberStatus; 
	bool			m_chamber;
	virtual void	Chamber();

	virtual void	OnShot			() { OnShot(true); }
			void	OnShot			(bool hasRecoil);
	
	virtual void	OnEmptyClick	();

	virtual void	OnAnimationEnd	(u32 state);
	virtual void	OnStateSwitch	(u32 S);

	virtual void	UpdateSounds	();

	bool			TryReload		();

protected:
	virtual void	ReloadMagazine();
			void	ApplySilencerKoeffs();
			void	ResetSilencerKoeffs();

	virtual void	state_Fire		(float dt);
	virtual void	state_MagEmpty	(float dt);
	virtual void	state_Misfire	(float dt);
	virtual bool	DelayedShotIsAllowed();
public:
					CWeaponMagazined	(ESoundTypes eSoundType=SOUND_TYPE_WEAPON_SUBMACHINEGUN);
	virtual			~CWeaponMagazined	();

	virtual CWeaponMagazined*cast_weapon_magazined	()		 {return this;}
	
	virtual void	LoadCfg			(LPCSTR section);
			void	LoadSilencerKoeffs();

	// Happens when object gets online. Import saved data from server here. Do oject initialization based on data imported from server
	virtual BOOL	SpawnAndImportSOData(CSE_Abstract* data_containing_so);
	virtual void	DestroyClientObj	();
	// constant export of various weapon data to server. Here should be stuff, needed to be saved before object switches offline. Than it should be imported in SpawnAndImportSOData, when object gets online again
	virtual void	ExportDataToServer	(NET_Packet& P);

	// save stuff that is needed while weapon is in alife radius
	virtual void	save(NET_Packet &output_packet);
	// load stuff needed in alife radius
	virtual void	load(IReader &input_packet);

	virtual void	AfterAttachToParent		();
			void	BeforeDetachFromParent	(bool) override;
			
	virtual	void	UpdateCL		();

	virtual void	renderable_Render(IRenderBuffer & render_buffer);
	virtual void	render_hud_mode	(IKinematics* hud_model, IRenderBuffer& render_buffer);
		
	virtual void	SetDefaults		();
	virtual void	FireStart		();
	virtual void	FireEnd			();
	virtual void	Reload			();

	virtual bool	Attach			(PIItem pIItem, bool b_send_event);
	virtual bool	Detach			(const char* item_section_name, bool b_spawn_item);
			bool	DetachScope		(const char* item_section_name, bool b_spawn_item);
	virtual bool	CanAttach		(PIItem pIItem);
	virtual bool	CanDetach		(const char* item_section_name);

	virtual void	InitAddons();

	virtual bool	Action			(u16 cmd, u32 flags);
	virtual LPCSTR	getAmmoName		();
	bool			IsAmmoAvailable	();

	virtual void	UnloadMagazine	(bool spawn_ammo = true, u32 into_who_id = u32(-1), bool unload_secondary = false);
	virtual void	UnloadMagazineEx(xr_vector<CCartridge>& from_where, bool spawn_ammo = true, u32 into_who_id = u32(-1), bool decline_ammo_counter = true);

	virtual bool	GetBriefInfo	(II_BriefInfo& info);


	//////////////////////////////////////////////
	// ��� �������� ��������� ��� ����������
	//////////////////////////////////////////////
public:
	virtual bool	SwitchMode				();
	virtual bool	SingleShotMode			()			{return 1 == m_iQueueSize;}
	virtual void	SetQueueSize			(int size);
	IC		int		GetQueueSize			() const	{return m_iQueueSize;};
	virtual bool	StopedAfterQueueFired	()			{return m_bStopedAfterQueueFired; }
	virtual void	StopedAfterQueueFired	(bool value){m_bStopedAfterQueueFired = value; }

protected:
	//������������ ������ �������, ������� ����� ����������
	int				m_iQueueSize;
	//���������� ������� ����������� ��������
	int				m_iShotNum;
	//����� ������ ��������, ��� ����������� ��������, ���������� ������ (������� ��-�� �������)
	int				m_iRecoilStartShotNum;
	//���������������� ��� ��������, �� ������� �� ������ ������ (������� ��-�� �������)
	float			m_fNoRecoilTimeToFire;
	//����������� �������� (����������������) ����� ��������� ��� ���������� ����������
	float			m_fTimeToFireSemi;
	//�������� ���� ����� �������� ������������ ����� ������������
	Fvector			m_vFireDirectionOffset;
	//  [7/20/2005]
	//���� ����, ��� �� ������������ ����� ���� ��� ����������
	//����� ������� ��������, ������� ���� ������ � m_iQueueSize
	bool			m_bStopedAfterQueueFired;
	//���� ����, ��� ���� �� ���� ������� �� ������ �������
	//(���� ���� ����� ������ ������ �� ����� � ��������� FireEnd)
	bool			m_bFireSingleShot;
	//������������ ���������� �������� ����� ������� �������� (������) � ������ ���������
	float			m_fShotMaxDelay;
	//������ ��������
	bool			m_bHasDifferentFireModes;
	xr_vector<int>	m_aFireModes;
	int				m_iCurFireMode;
	string16		m_sCurFireMode;
	int				m_iPrefferedFireMode;
	
	float 			m_conditionDecreasePerQueueShot;	//���������� ����������� ��� �������� ��������

	//���������� ��������� �������������
	//������ ������ ����� ��������
	bool m_bLockType;

	//////////////////////////////////////////////
	// ����� �����������
	//////////////////////////////////////////////
public:
	virtual void	OnZoomIn			();
	virtual void	OnZoomOut			();
	virtual	void	OnNextFireMode		();
	virtual	void	OnPrevFireMode		();
	virtual bool	HasFireModes		() { return m_bHasDifferentFireModes; };
	virtual	int		GetCurrentFireMode	() { return m_aFireModes[m_iCurFireMode]; };	
	virtual LPCSTR	GetCurrentFireModeStr	() {return m_sCurFireMode;};

protected:
	virtual bool	install_upgrade_impl( LPCSTR section, bool test );

protected:
	virtual bool	AllowFireWhileWorking() {return false;}

	//����������� ������� ��� ������������ �������� HUD
	virtual void	PlayAnimShow		();
	virtual void	PlayAnimHide		();
	virtual void	PlayAnimBore		();
	virtual void	PlayAnimReload		();
	virtual void	PlayAnimIdle		();
	virtual void	PlayAnimIdleMoving	();
	virtual void	PlayAnimIdleSprint	();
	virtual void	PlayAnimShoot		();
	virtual void	PlayReloadSound		();
	virtual void	PlayAnimAim			();

	virtual	int		ShotsFired			() { return m_iShotNum; }
	virtual float	GetWeaponDeterioration	();
	virtual void	RemoveZoomInertionEffector();

};
