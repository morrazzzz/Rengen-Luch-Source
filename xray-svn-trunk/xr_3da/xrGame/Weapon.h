// Weapon.h: interface for the CWeapon class.
#pragma once

#include "../../xrphysics/PhysicsShell.h"
#include "weaponammo.h"
#include "PHShellCreator.h"

#include "ShootingObject.h"
#include "NightVisionUsable.h"
#include "hud_item_object.h"
#include "ActorFlags.h"
#include "../../Include/xrRender/KinematicsAnimated.h"
#include "game_cl_base.h"
#include "firedeps.h"
#include "CameraRecoil.h"

// refs
class CEntity;
class ENGINE_API CMotionDef;
class CSE_ALifeItemWeapon;
class CSE_ALifeItemWeaponAmmo;
class CWeaponMagazined;
class CParticlesObject;
class CUIWindow;

class CNightVisionEffector;
class CBinocularsVision;

class CWeaponAddons
{
public:
	CWeaponAddons();
	virtual ~CWeaponAddons();

	IRenderVisual * mesh;
	Fmatrix		    render_pos;

	shared_str      addon_name;
	shared_str      addon_class;
	shared_str		addon_section;
};

class CWeapon 
	: public CHudItemObject
	, public CShootingObject
	//, public CNightVisionUsable		--	Abdulla: all the functionality of NV is available via 'm_pNight_vision'. This ptr 
	//										shares NV with contrast upgrade. As long as they are mutually incompatible 
	//										(a technician cannot make both upgrades), everything is OK. 
{
private:
	typedef CHudItemObject inherited;

public:
							CWeapon				();
	virtual					~CWeapon			();

	virtual CWeapon			*cast_weapon			()					{return this;}
	virtual CWeaponMagazined*cast_weapon_magazined	()					{return 0;}

	xr_vector<CWeaponAddons> current_addons_list;
	xr_vector<shared_str>    avaible_addons_list;


	// Generic
	virtual void			LoadCfg				(LPCSTR section);

	// Happens when object gets online. Import saved data from server here. Do oject initialization based on data imported from server
	virtual BOOL			SpawnAndImportSOData(CSE_Abstract* data_containing_so);
	virtual void			DestroyClientObj	();
	// constant export of various weapon data to server. Here should be stuff, needed to be saved before object switches offline. Than it should be imported in SpawnAndImportSOData, when object gets online again
	virtual void			ExportDataToServer	(NET_Packet& P);
			void			RemoveLinksToCLObj	(CObject *object) override;

			void			ImportSOMagazine	(CSE_ALifeItemWeapon* data_containing_so, xr_vector<CCartridge>& mag_to_pack);

			void			PackAndExportMagazine(NET_Packet& P, xr_vector<CCartridge>& mag_to_pack);

	// save stuff that is needed while weapon is in alife radius
	virtual void			save				(NET_Packet &output_packet);
	// load stuff needed in alife radius
	virtual void			load				(IReader &input_packet);
	
	virtual BOOL			net_SaveRelevant	()								{return inherited::net_SaveRelevant();}

	virtual void			UpdateCL			();
	virtual void			ScheduledUpdate		(u32 dt);

	virtual void			renderable_Render	(IRenderBuffer& render_buffer);
	virtual void			render_hud_mode		(IKinematics* hud_model, IRenderBuffer& render_buffer);
	virtual bool			need_renderable		();


	// SecondVP code
	virtual void			UpdateSecondVP		(bool bCond_4 = true);

	float					GetSecondVPFov		() const;

	// It needed for turn off scope texture in shader if we disabled SecondVP
	float					GetInZoomNow() const;

	float					GetSecondVPZoomFactor	() const { return m_zoom_params.m_fSecondVPZoomFactor; }
	bool					IsSecondVPZoomPresent	() const { return (GetSecondVPZoomFactor() > 0.000f && psActorFlags.test(AF_USE_3D_SCOPES)); }

	void					ZoomDynamicMod			(bool bIncrement, bool bForceLimit);

	virtual void			render_item_ui		();
	virtual bool			render_item_ui_query();

	virtual void			BeforeAttachToParent	();
	virtual void			AfterAttachToParent		();
	virtual void			BeforeDetachFromParent	(bool just_before_destroy);
	virtual void			AfterDetachFromParent	();
	
	virtual void			OnEvent				(NET_Packet& P, u16 type);// {inherited::OnEvent(P,type);}

	virtual	void			Hit					(SHit* pHDS);

	virtual void			reinit				();
	virtual void			reload				(LPCSTR section);
	virtual void			create_physic_shell	();
	virtual void			activate_physic_shell();
	virtual void			setup_physic_shell	();

	virtual void			SwitchState			(u32 S);

	virtual void			Hide				();
	virtual void			Show				();

	//инициализация если вещь в активном слоте или спрятана на BeforeAttachToParent
	virtual void			OnActiveItem		();
	virtual void			OnHiddenItem		();
	virtual void			SendHiddenItem		();

public:
	virtual bool			can_kill			() const;
	virtual CInventoryItem	*can_kill			(CInventory *inventory) const;
	virtual const CInventoryItem *can_kill		(const xr_vector<const CGameObject*> &items) const;
	virtual bool			ready_to_kill		() const;

	AccessLock				protectWeaponRender_;
protected:
	virtual bool			IsHudModeNow		();

public:
	void					signal_HideComplete	();
	virtual bool			Action(u16 cmd, u32 flags);

	enum EWeaponStates {
		eFire		= eLastBaseState+1,
		eFire2,
		eReload,
		eMisfire,
		eMagEmpty,
		eSwitch,
		ePreFire,
	};
	enum EWeaponSubStates{
		eSubstateReloadBegin		=0,
		eSubstateReloadInProcess,
		eSubstateReloadEnd,
	};

	IC BOOL					IsValid				()	const		{	return iAmmoElapsed;						}
	// Does weapon need's update?
	BOOL					IsUpdating			();


	BOOL					IsMisfire			() const;
	BOOL					CheckForMisfire		();

	virtual bool			MovingAnimAllowedNow	();

	BOOL					AutoSpawnAmmo		() const		{ return m_bAutoSpawnAmmo; };
	bool					IsTriStateReload	() const		{ return m_bTriStateReload;}
	EWeaponSubStates		GetReloadState		() const		{ return (EWeaponSubStates)m_sub_state;}
protected:
	bool					m_bTriStateReload;
	u8						m_sub_state;
	// Weapon fires now
	bool					bWorking2;
	// a misfire happens, you'll need to rearm weapon
	bool					bMisfire;				

	BOOL					m_bAutoSpawnAmmo;

	virtual bool			AllowBore		();

	xr_vector<u16>			magazinePacked; // server export purpose only

//////////////////////////////////////////////////////////////////////////
//  Weapon Addons
//////////////////////////////////////////////////////////////////////////
public:
	///////////////////////////////////////////
	// работа с аддонами к оружию
	//////////////////////////////////////////


			bool IsGrenadeLauncherAttached	() const;
			bool IsScopeAttached			() const;
			bool IsSilencerAttached			() const;

	virtual bool GrenadeLauncherAttachable();
	virtual bool ScopeAttachable();
	virtual bool SilencerAttachable();
	virtual bool UseScopeTexture();

	ALife::EWeaponAddonStatus	get_GrenadeLauncherStatus	() const { return m_eGrenadeLauncherStatus; }
	ALife::EWeaponAddonStatus	get_ScopeStatus				() const { return m_eScopeStatus; }
	ALife::EWeaponAddonStatus	get_SilencerStatus			() const { return m_eSilencerStatus; }

	//обновление видимости для косточек аддонов
			void UpdateAddonsVisibility();
			void UpdateHUDAddonsVisibility();
	//инициализация свойств присоединенных аддонов
	virtual void InitAddons();

	//для отоброажения иконок апгрейдов в интерфейсе
	int	GetScopeX() {return pSettings->r_s32(m_scopes[m_cur_scope], "scope_x");}
	int	GetScopeY() {return pSettings->r_s32(m_scopes[m_cur_scope], "scope_y");}
	int	GetSilencerX() {return m_iSilencerX;}
	int	GetSilencerY() {return m_iSilencerY;}
	int	GetGrenadeLauncherX() {return m_iGrenadeLauncherX;}
	int	GetGrenadeLauncherY() {return m_iGrenadeLauncherY;}

	const shared_str& GetGrenadeLauncherName	() const		{return m_sGrenadeLauncherName;}
	const shared_str GetScopeName() const		{ shared_str res = pSettings->r_string(m_scopes[m_cur_scope], "scope_name"); R_ASSERT2(res.c_str(), make_string("m_scopes[m_cur_scope] = %s m_cur_scope %u section %s", m_scopes[m_cur_scope].c_str(), m_cur_scope, SectionNameStr())); return res; }
	const shared_str& GetSilencerName			() const		{return m_sSilencerName;}

	bool	IsScopeIconForced					() const		{ return m_bScopeForceIcon; }
	bool	IsSilencerIconForced				() const		{ return m_bSilencerForceIcon; }
	bool	IsGrenadeLauncherIconForced			() const		{ return m_bGrenadeLauncherForceIcon; }
	IC void	ForceUpdateAmmo						()		{ m_BriefInfo_CalcFrame = 0; }

	u8		GetAddonsState						()		const		{return m_flagsAddOnState;};
	void	SetAddonsState						(u8 st)	{m_flagsAddOnState=st;}//dont use!!! for buy menu only!!!

	virtual void	UpdateAddonsTransform		(bool for_hud = false);
	virtual void    CalcAddonOffset				(Fmatrix base_model_trans, IKinematics & base_model, shared_str bone_name, Fmatrix offset, Fmatrix & dest);
	virtual void    ResetAddonsHudParams		();
	virtual void    UpdateAddonsHudParams		();



	Fmatrix                 fakeAttachTransform;
	IRenderVisual*			fakeViual;
	shared_str				fakeAttachBone;
	shared_str				fakeAttachScopeBone;

	Fmatrix					scopeAttachTransform;
	Fvector					scopeAttachOffset[2]; // pos,rot / world
	IRenderVisual*			scopeVisual;
	shared_str				scopeAttachBone;

	Fmatrix					silencerAttachTransform;
	Fvector					silencerAttachOffset[2]; // pos,rot / world
	IRenderVisual*			silencerVisual;
	shared_str				silencerAttachBone;

	Fmatrix					glauncherAttachTransform;
	Fvector					glauncherAttachOffset[2]; // pos,rot / world

	IRenderVisual*			glauncherVisual;
	IRenderVisual*			grenadeVisual;

	shared_str				glAttachBone;
	shared_str			    glAttachTriggerBone;
	shared_str				glAttachGrenadeBone;

//multy-scope crutch system
//	shared_str GetScopeBoneName{ pSettings->r_string(m_scopes[m_cur_scope], "scope_bone") };

protected:
	//состояние подключенных аддонов
	u8 m_flagsAddOnState;

	//возможность подключения различных аддонов
	ALife::EWeaponAddonStatus	m_eScopeStatus;
	ALife::EWeaponAddonStatus	m_eSilencerStatus;
	ALife::EWeaponAddonStatus	m_eGrenadeLauncherStatus;

	//названия секций подключаемых аддонов
	shared_str		m_sScopeName;
	shared_str		m_sSilencerName;
	shared_str		m_sGrenadeLauncherName;

	//смещение иконов апгрейдов в инвентаре
	int	m_iScopeX, m_iScopeY;
	int	m_iSilencerX, m_iSilencerY;
	int	m_iGrenadeLauncherX, m_iGrenadeLauncherY;

	//рисовать ли иконку аддона вне зависимости от статуса (например для permanent)
	bool m_bScopeForceIcon;
	bool m_bSilencerForceIcon;
	bool m_bGrenadeLauncherForceIcon;

//	bool m_bScopeMultySystem;
///////////////////////////////////////////////////
//	для режима приближения и снайперского прицела
///////////////////////////////////////////////////
protected:

	struct SZoomParams
	{
		bool			m_bZoomEnabled;			//разрешение режима приближения
		bool			m_bHideCrosshairInZoom;
//		bool			m_bZoomDofEnabled;

		bool			m_bIsZoomModeNow;		//когда режим приближения включен
		float			m_fCurrentZoomFactor;	//текущий фактор приближения
		float			m_fZoomRotateTime;		//время приближения
	
		float			m_fIronSightZoomFactor;	//коэффициент увеличения прицеливания
		float			m_fScopeZoomFactor;		//коэффициент увеличения прицела

		float           m_fSecondVPZoomFactor;  //коэффициент увеличения прицела для второго вьюпорта
		float           m_fSecondVPWorldFOV;    //коэффициент увеличения мира во время прицеливания через 3D линзу

		float			m_fZoomRotationFactor;
		
//		Fvector			m_ZoomDof;
//		Fvector4		m_ReloadDof;
		BOOL			m_bUseDynamicZoom;
		shared_str		m_sUseZoomPostprocess;
		shared_str		m_sUseBinocularVision;

		CBinocularsVision*		scopeAliveDetectorVision_;
		CNightVisionEffector*	scopeNightVision_;

		BOOL			reenableNVOnZoomIn_;
		BOOL			reenableActorNVOnZoomOut_;

		BOOL			reenableALDetectorOnZoomIn_;
		BOOL			reenableActorALDetectorOnZoomOut_;

		SZoomParams();

	} m_zoom_params;
	
		float			m_fRTZoomFactor;	 //run-time zoom factor
		float           m_fSVP_RTZoomFactor; //run-time zoom factor for second viewport, only for dynamic scope mode
		CUIWindow*		m_UIScope;

		float			secondVPWorldHudFOVK; // Для тех, кто ленится настроить зум позицию худа
public:

	IC bool					IsZoomEnabled		()	const		{return m_zoom_params.m_bZoomEnabled;}
	virtual	void			ZoomInc				();
	virtual	void			ZoomDec				();
	virtual void			OnZoomIn			();
	virtual void			OnZoomOut			();
	IC		bool			IsZoomed			()	const		{return m_zoom_params.m_bIsZoomModeNow;};
	CUIWindow*				ZoomTexture			();


			bool			ZoomHideCrosshair	()				{return m_zoom_params.m_bHideCrosshairInZoom || ZoomTexture();}

	IC float				GetZoomFactor		() const		{return m_zoom_params.m_fCurrentZoomFactor;}
	IC void					SetZoomFactor		(float f) 		{m_zoom_params.m_fCurrentZoomFactor = f;}

	virtual	float			CurrentZoomFactor	();
	//показывает, что оружие находится в соостоянии поворота для приближенного прицеливания
			bool			IsRotatingToZoom	() const		{	return (m_zoom_params.m_fZoomRotationFactor<1.f);}

	virtual	u8				GetCurrentHudOffsetIdx ();

	//hands runtime offset(Moved from attachable_hud_item, to be able to virtualize it for CWeapon class)
	virtual Fvector&		hands_offset_pos		();
	virtual Fvector&		hands_offset_rot		();

	virtual bool			UseScopeParams			() { return IsScopeAttached(); };

	virtual float				Weight			() const;		
	virtual	u32					Cost			() const;
//gr1ph:
protected:
	virtual void			GetZoomData			(const float scope_factor, float& delta, float& min_zoom_factor);
	void					HandleScopeSwitchings();

public:
    virtual EHandDependence		HandDependence		()	const		{	return eHandDependence;}
			bool				IsSingleHanded		()	const		{	return m_bIsSingleHanded; }

public:
	IC		LPCSTR			strap_bone0			() const {return m_strap_bone0;}
	IC		LPCSTR			strap_bone1			() const {return m_strap_bone1;}
	IC		void			strapped_mode		(bool value) {m_strapped_mode = value;}
	IC		bool			strapped_mode		() const {return m_strapped_mode;}

protected:
	int					m_strap_bone0_id;
	int					m_strap_bone1_id;
	bool					m_strapped_mode_rifle;
	LPCSTR					m_strap_bone0;
	LPCSTR					m_strap_bone1;
	Fmatrix					m_StrapOffset;
	bool					m_strapped_mode;
	bool					m_can_be_strapped;
	bool					m_can_be_strapped_rifle;

	Fmatrix					m_Offset;
	// 0-используется без участия рук, 1-одна рука, 2-две руки
	EHandDependence			eHandDependence;
	bool					m_bIsSingleHanded;

public:
	//загружаемые параметры
	Fvector					vLoadedFirePoint	;
	Fvector					vLoadedFirePoint2	;

	float m_fLR_InertiaFactor; // Фактор горизонтальной инерции худа при движении камеры [-1; +1]
	float m_fUD_InertiaFactor; // Фактор вертикальной инерции худа при движении камеры [-1; +1]

	void AddHUDShootingEffect();
    float m_fLR_MovingFactor; 
    float m_fLR_CameraFactor;
	float m_fLR_ShootingFactor;
	float m_fUD_ShootingFactor; 
	float m_fBACKW_ShootingFactor; 

private:
	firedeps				m_current_firedeps;

protected:
	virtual void			UpdateFireDependencies_internal	();
	virtual void			UpdatePosition			(const Fmatrix& transform);	//.
	virtual void			UpdateXForm				();
	virtual void			UpdateHudAdditonal		(Fmatrix&);
	IC		void			UpdateFireDependencies()			{ if (dwFP_Frame == CurrentFrame()) return; UpdateFireDependencies_internal(); };

	virtual void			LoadFireParams		(LPCSTR section);

	float					aproxEffectiveDistance_;
public:	
	IC		const Fvector&	get_LastFP				()			{ UpdateFireDependencies(); return m_current_firedeps.vLastFP;	}
	IC		const Fvector&	get_LastFP2				()			{ UpdateFireDependencies(); return m_current_firedeps.vLastFP2;	}
	IC		const Fvector&	get_LastFD				()			{ UpdateFireDependencies(); return m_current_firedeps.vLastFD;	}
	IC		const Fvector&	get_LastSP				()			{ UpdateFireDependencies(); return m_current_firedeps.vLastSP;	}

	virtual const Fvector&	get_CurrentFirePoint	()			{ return get_LastFP();				}
	virtual const Fvector&	get_CurrentFirePoint2	()			{ return get_LastFP2();				}
	virtual const Fmatrix&	get_ParticlesXFORM		()			{ UpdateFireDependencies(); return m_current_firedeps.m_FireParticlesXForm;	}
	virtual void			ForceUpdateFireParticles();
	virtual void			debug_draw_firedeps		();

	// For ai and misc. Approximate and is set in weapon and ammo cfgs
	float					GetEffectiveDistance	() { return aproxEffectiveDistance_; };
	void					CalcEffectiveDistance	();

	//////////////////////////////////////////////////////////////////////////
	// Weapon fire
	//////////////////////////////////////////////////////////////////////////
protected:
	virtual void			SetDefaults			();

	//трассирование полета пули
	virtual	void			FireTrace			(const Fvector& P, const Fvector& D);
	virtual float			GetWeaponDeterioration	();

	virtual void			FireStart			();
	virtual void			FireEnd				();// {CShootingObject::FireEnd();}

	virtual void			Fire2Start			();
	virtual void			Fire2End			();
	virtual void			Reload				();
	virtual	void			StopShooting		();
	virtual void			UnloadMagazine		(bool spawn_ammo = true, u32 into_who_id = u32(-1), bool unload_secondary = false);
    

	// обработка визуализации выстрела
	virtual void			OnShot				(){};
	virtual void			AddShotEffector		();
	virtual void			RemoveShotEffector	();
	virtual	void			ClearShotEffector	();
	virtual void			StopShotEffector	();

public:
	float					GetFireDispersion	(bool with_cartridge);
	virtual float			GetFireDispersion	(float cartridge_k);
	virtual	int				ShotsFired			() { return 0; }
	virtual	int				GetCurrentFireMode	() { return 1; }

	//параметы оружия в зависимоти от его состояния исправности
	float					GetConditionDispersionFactor	() const;
	float					GetConditionMisfireProbability	() const;
	virtual	float			GetConditionToShow				() const;

public:
	//отдача при стрельбе 
	CameraRecoil cam_recoil;
	CameraRecoil zoom_cam_recoil;
	
protected:
	//фактор увеличения дисперсии при максимальной изношености 
	//(на сколько процентов увеличится дисперсия)
	float					fireDispersionConditionFactor;
	//вероятность осечки при максимальной изношености

// modified by Peacemaker [17.10.08]
//	float					misfireProbability;
//	float					misfireConditionK;
	float misfireStartCondition;			//изношенность, при которой появляется шанс осечки
	float misfireEndCondition;				//изношеность при которой шанс осечки становится константным
	float misfireStartProbability;			//шанс осечки при изношености больше чем misfireStartCondition
	float misfireEndProbability;			//шанс осечки при изношености больше чем misfireEndCondition
	float conditionDecreasePerShot;			//увеличение изношености при одиночном выстреле
	float conditionDecreasePerQueueShot;	//���������� ����������� ��� �������� ��������

public:
	float GetMisfireStartCondition	() const {return misfireStartCondition;};
	float GetMisfireEndCondition	() const {return misfireEndCondition;};

protected:
	struct SPDM
	{
		float					m_fPDM_disp_base			;
		float					m_fPDM_disp_vel_factor		;
		float					m_fPDM_disp_accel_factor	;
		float					m_fPDM_disp_crouch			;
		float					m_fPDM_disp_crouch_no_acc	;
	};
	SPDM					m_pdm;

	// Коэффициент силы покачивания ствола при прицеливании, умножается на разброс Актора (вычисляемый из PDM)
	float					m_fZoomInertCoef;

protected:
	//для отдачи оружия
	Fvector					m_vRecoilDeltaAngle;

	//для сталкеров, чтоб они знали эффективные границы использования 
	//оружия
	float					m_fMinRadius;
	float					m_fMaxRadius;

//////////////////////////////////////////////////////////////////////////
// партиклы
//////////////////////////////////////////////////////////////////////////

protected:	
	//для второго ствола
			void			StartFlameParticles2();
			void			StopFlameParticles2	();
			void			UpdateFlameParticles2();
protected:
	shared_str					m_sFlameParticles2;
	//объект партиклов для стрельбы из 2-го ствола
	CParticlesObject*		m_pFlameParticles2;

//////////////////////////////////////////////////////////////////////////
// Weapon and ammo
//////////////////////////////////////////////////////////////////////////
protected:
	int						GetAmmoCount_forType(shared_str const& ammo_type) const;
	int						GetAmmoCount		(u8 ammo_type) const;
public:
	IC int					GetAmmoElapsed		()	const		{	return /*int(m_magazine.size())*/iAmmoElapsed;}
	IC int					GetAmmoMagSize		()	const		{ return maxMagazineSize_; }
	int						GetSuitableAmmoTotal(bool use_item_to_spawn = false) const;

	void					SetAmmoElapsed		(int ammo_count);

	virtual void			OnMagazineEmpty		();
			void			SpawnAmmo			(u32 boxCurr = u32(-1), 
													LPCSTR ammoSect = NULL, 
													u32 ParentID = u32(-1));

			bool			SwitchAmmoType		();


	virtual	float			Get_PDM_Base		()	const	{ return m_pdm.m_fPDM_disp_base			; };
	virtual	float			Get_PDM_Vel_F		()	const	{ return m_pdm.m_fPDM_disp_vel_factor		; };
	virtual	float			Get_PDM_Accel_F		()	const	{ return m_pdm.m_fPDM_disp_accel_factor	; };
	virtual	float			Get_PDM_Crouch		()	const	{ return m_pdm.m_fPDM_disp_crouch			; };
	virtual	float			Get_PDM_Crouch_NA	()	const	{ return m_pdm.m_fPDM_disp_crouch_no_acc	; };

	virtual float			GetZoomInertion		()	const	{ return m_fZoomInertCoef; };
	//virtual	float			GetCrosshairInertion()	const	{ return m_crosshair_inertion; };
	//		float			GetFirstBulletDisp	()	const	{ return m_first_bullet_controller.get_fire_dispertion(); };
protected:
	int						iAmmoElapsed;		// ammo in magazine, currently
	int						maxMagazineSize_;	// size (in bullets) of magazine

	//��� �������� � GetSuitableAmmoTotal
	mutable int				m_iAmmoCurrentTotal;
	mutable u32				m_BriefInfo_CalcFrame;	//���� �� ������� ���������� ���-�� ��������
	bool					m_bAmmoWasSpawned;
	//  [10/5/2005]

	virtual bool			IsNecessaryItem	    (const shared_str& item_sect);

	u8						m_changed_ammoType_on_reload; //This ammo aplies once, if ammo type was changed

public:
	xr_vector<shared_str>	m_ammoTypes;

	DEFINE_VECTOR(shared_str, SCOPES_VECTOR, SCOPES_VECTOR_IT);
	SCOPES_VECTOR			m_scopes;
	u8						m_cur_scope;

	CWeaponAmmo*			m_pCurrentAmmo;
	u8						m_ammoType;
	shared_str				m_ammoName;
	bool					m_bHasTracers;
	u8						m_u8TracerColorID;
	// Multitype ammo support
	xr_vector<CCartridge>	m_magazine;
	CCartridge				m_DefaultCartridge;
	float					m_fCurrentCartirdgeDisp;

		bool				unlimited_ammo				();
	IC	bool				can_be_strapped				() const {return m_can_be_strapped;};

	LPCSTR					GetCurrentAmmo_ShortName	();

	void					SetAmmoOnReload(u8 value);

protected:
	u32						m_ef_main_weapon_type;
	u32						m_ef_weapon_type;
	u32						m_ai_weapon_rank;

	float					aiAproximateEffectiveDistanceBase_;

public:
	virtual u32				ef_main_weapon_type	() const;
	virtual u32				ef_weapon_type		() const;
	virtual u32				ai_weapon_rank		() const { return m_ai_weapon_rank; };

protected:
	// This is because when scope is attached we can't ask scope for these params
	// therefore we should hold them by ourself :-((
	float					m_addon_holder_range_modifier;
	float					m_addon_holder_fov_modifier;
public:
	virtual	void			modify_holder_params		(float &range, float &fov) const;
	virtual bool			use_crosshair				()	const {return true;}
			bool			show_crosshair				();
			bool			show_indicators				();
	virtual BOOL			ParentMayHaveAimBullet		();
	virtual BOOL			ParentIsActor				();
	
private:
			bool			install_upgrade_disp		( LPCSTR section, bool test );
			bool			install_upgrade_hit			( LPCSTR section, bool test );
			bool			install_upgrade_addon		( LPCSTR section, bool test );
protected:
	virtual	bool			install_upgrade_ammo_class	( LPCSTR section, bool test );
	virtual bool			install_upgrade_impl		( LPCSTR section, bool test );
			bool			ProcessRpmUpgrade			( LPCSTR section, LPCSTR paramName, float &timeToFireProperty, bool test);
			bool			process_if_exists_deg2rad	( LPCSTR section, LPCSTR name, float& value, bool test );

// Night Vision
public:
	void SwitchNightVision(bool state);
	void SwitchNightVision();
	bool IsScopeNvInstalled() const;
	bool GetNightVisionStatus();

// Alive detector
	
	void SwitchAliveDetector(bool state);
	void SwitchAliveDetector() { SwitchAliveDetector(!m_scope_adetector_enabled); }
	bool IsScopeAliveDetectorInstalled() const;

private:
	bool CanUseScopeNV();
	bool CanUseAliveDetector();

	bool m_scope_adetector_enabled;

private:
	float					m_hit_probability[egdCount];

public:
	const float				&hit_probability			() const;

	virtual void				DumpActiveParams(shared_str const & section_name, CInifile & dst_ini) const;
	virtual shared_str const	GetAnticheatSectionName() const { return SectionName(); };

	Fvector					m_overriden_activation_speed;
	bool					m_activation_speed_is_overriden;
	virtual void			SetActivationSpeedOverride	(Fvector const& speed);
	virtual bool			ActivationSpeedOverriden	(Fvector& dest, bool clear_override);
};

extern bool	m_bDraw_off;
extern bool	m_bHolster_off;
