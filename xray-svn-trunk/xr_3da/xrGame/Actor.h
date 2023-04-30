#pragma once

#include "../feel_touch.h"
#include "../feel_sound.h"
#include "../iinputreceiver.h"
#include "../../Include/xrRender/KinematicsAnimated.h"
#include "actorflags.h"
#include "actordefs.h"
#include "entity_alive.h"
#include "PHMovementControl.h"
#include "../../xrphysics/PhysicsShell.h"
#include "InventoryOwner.h"
#include "PhraseDialogManager.h"
#include "torch.h"
#include "NightVisionActor.h"
#include "step_manager.h"
#include "ActorState.h"
#include "ui_defs.h"


using namespace ACTOR_DEFS;

class CInfoPortion;
struct GAME_NEWS_DATA;
class CActorCondition;
class COutfitBase;
class CCustomOutfit;
class CKnownContactsRegistryWrapper;
class CEncyclopediaRegistryWrapper;
class CGameTaskRegistryWrapper;
class CGameNewsRegistryWrapper;
class CCharacterPhysicsSupport;
class CActorCameraManager;
// refs
class ENGINE_API CCameraBase;
class ENGINE_API CBoneInstance;
class ENGINE_API CBlend;
class CWeaponList;
class CEffectorBobbing;
class CHolderCustom;
class CUsableScriptObject;

struct SShootingEffector;
struct SSleepEffector;
class  CSleepEffectorPP;
class CInventoryBox;
//class  CActorEffector;

class	CHudItem;
class   CArtefact;

class CCar;

struct SActorMotions;
struct SActorVehicleAnims;
class  CActorCondition;
class SndShockEffector;
class CScriptCameraDirection;
class CActorFollowerMngr;

struct CameraRecoil;
class CCameraShotEffector;
class CActorInputHandler;

class CActorMemory;
class CActorStatisticMgr;

class CLocationManager;
class CTorch;
class CNightVisionActor;

struct UsableObject
{
	CGameObject*	gameObject;
	float			useDelay;

	UsableObject()
	{
		gameObject = nullptr;
		useDelay = 0.f;
	}
};

class	CActor: 
	public CEntityAlive, 
	public IInputReceiver,
	public Feel::Touch,
	public CInventoryOwner,
	public CPhraseDialogManager,
	public CStepManager,
	public Feel::Sound

{
	friend class CActorCondition;
private:
	typedef CEntityAlive	inherited;

public:
										CActor				();
	virtual								~CActor				();

public:
	virtual BOOL						AlwaysInUpdateList			()						{ return TRUE; }

	virtual CAttachmentOwner*			cast_attachment_owner		()						{return this;}
	virtual CInventoryOwner*			cast_inventory_owner		()						{return this;}
	virtual CActor*						cast_actor					()						{return this;}
	virtual CGameObject*				cast_game_object			()						{return this;}
	virtual IInputReceiver*				cast_input_receiver			()						{return this;}
	virtual	CCharacterPhysicsSupport*	character_physics_support	()						{return m_pPhysics_support;}
	virtual	CCharacterPhysicsSupport*	character_physics_support	() const				{return m_pPhysics_support;}
	virtual CPHDestroyable*				ph_destroyable				()						;
			CHolderCustom*				Holder						()						{return m_holder;}
public:

	virtual void						LoadCfg				(LPCSTR section);

	virtual BOOL						SpawnAndImportSOData( CSE_Abstract* data_containing_so);
	virtual void						ExportDataToServer	( NET_Packet& P);				// export to server
	virtual void						DestroyClientObj	();
	virtual BOOL						NeedDataExport		();//	{ return getSVU() | getLocal(); };		// relevant for export to server
	virtual	void						RemoveLinksToCLObj	( CObject* O );					//

	//object serialization
	virtual void						save				(NET_Packet &output_packet);
	virtual void						load				(IReader &input_packet);
	virtual void						net_Save			(NET_Packet& P);
	virtual	BOOL						net_SaveRelevant	();

	virtual void xr_stdcall				on_requested_spawn  (CObject *object);

	void _stdcall						AfterLoad();
	
	virtual void						ScheduledUpdate		(u32 T); 
	virtual void						UpdateCL			();
	
	virtual void						OnEvent				(NET_Packet& P, u16 type);

	// Render
	virtual void						renderable_Render			(IRenderBuffer& render_buffer);
	virtual BOOL						renderable_ShadowGenerate	();
	
	virtual	void						feel_sound_new				(CObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position, float power);
	virtual	Feel::Sound*				dcast_FeelSound				()	{ return this;	}
			float						m_snd_noise;
#ifdef DEBUG
			virtual void						OnRender();
#endif


public:

	virtual bool OnReceiveInfo		(shared_str info_id) const;
	virtual void OnDisableInfo		(shared_str info_id) const;

	virtual void NewPdaContact		(CInventoryOwner*);
	virtual void LostPdaContact		(CInventoryOwner*);

	void		DumpTasks();

protected:
	virtual void AddEncyclopediaArticle	(const CInfoPortion* info_portion) const;
	virtual void AddGameTask			(const CInfoPortion* info_portion) const;
	virtual void AddMapLocation			(const CInfoPortion* info_portion) const;

protected:
struct SDefNewsMsg{
		GAME_NEWS_DATA*	news_data;
		u32				time;
		bool operator < (const SDefNewsMsg& other) const {return time>other.time;}
	};
	xr_vector<SDefNewsMsg> m_defferedMessages;
	void UpdateDefferedMessages();	

public:	
	void			AddGameNews_deffered	 (GAME_NEWS_DATA& news_data, u32 delay);
	virtual void	AddGameNews				 (GAME_NEWS_DATA& news_data);
protected:
	CActorStatisticMgr*				m_statistic_manager;
public:
	virtual void StartTalk			(CInventoryOwner* talk_partner);
	virtual	void RunTalkDialog		(CInventoryOwner* talk_partner, bool* custom_break_bool = nullptr);

	CActorStatisticMgr&				StatisticMgr()	{return *m_statistic_manager;}
	CEncyclopediaRegistryWrapper	*encyclopedia_registry;
	CGameNewsRegistryWrapper		*game_news_registry;
	CCharacterPhysicsSupport		*m_pPhysics_support;

	virtual LPCSTR	Name () const {return CInventoryOwner::Name();}

public:
	//PhraseDialogManager
	virtual void ReceivePhrase				(DIALOG_SHARED_PTR& phrase_dialog);
	virtual void UpdateAvailableDialogs		(CPhraseDialogManager* partner);
	virtual void TryToTalk					();
			bool OnDialogSoundHandlerStart	(CInventoryOwner *inv_owner, LPCSTR phrase);
			bool OnDialogSoundHandlerStop	(CInventoryOwner *inv_owner);


	virtual void reinit			();
	virtual void reload			(LPCSTR section);
	virtual bool use_bolts		() const;

			void OnItemTake		(CInventoryItem *inventory_item, bool duringSpawn) override;
	
	virtual void OnItemRuck		(CInventoryItem *inventory_item, EItemPlace previous_place);
	virtual void OnItemBelt		(CInventoryItem *inventory_item, EItemPlace previous_place);
	
	virtual void OnItemDrop		(CInventoryItem *inventory_item);
	virtual void OnItemDropUpdate ();

	virtual void						Die				(CObject* who);
	virtual	void						Hit				(SHit* pHDS);
	virtual	void						PHHit			(SHit &H);
	virtual void						HitSignal		(float P, Fvector &vLocalDir,	CObject* who, s16 element);
			void						HitSector		(CObject* who, CObject* weapon);
			void						HitMark			(float P, Fvector dir, CObject* who_object, s16 element, Fvector position_in_bone_space, float impulse,  ALife::EHitType hit_type);
			
			void						Feel_Grenade_Update(float rad);

			float						GetRawHitDamage	(float hit_power, ALife::EHitType hit_type);

			virtual	bool				use_default_throw_force();
			virtual	float				missile_throw_force();

			virtual bool				unlimited_ammo();

	virtual float						GetMass				() ;
	virtual float						Radius				() const;
	virtual void						g_PerformDrop		();

public:

	//свойства артефактов
	virtual void		UpdateArtefactsOnBeltAndOutfit	();
	virtual float		HitArtefactsOnBelt		(float hit_power, ALife::EHitType hit_type);
	virtual float		HitBoosters				(float hit_power, ALife::EHitType hit_type);

	// названия секций предметов из быстрых слотов
	string32 quickUseSlotsContents_[4];

protected:
	//звук тяжелого дыхания
	ref_sound			m_HeavyBreathSnd;
	ref_sound			m_BloodSnd;
	ref_sound			m_DangerSnd;

protected:
	//Sleep params
	//время когда актера надо разбудить
	ALife::_TIME_ID			m_dwWakeUpTime;
	float					m_fOldTimeFactor;
	float					m_fOldOnlineRadius;
	float					m_fSleepTimeFactor;

protected:
	// Death
	float					m_hit_slowmo;
	float					m_hit_probability;
	s8						m_block_sprint_counter;

	// media
	SndShockEffector*		m_sndShockEffector;
	CScriptCameraDirection*	m_ScriptCameraDirection;
	xr_vector<ref_sound>	sndHit[ALife::eHitTypeMax];
	ref_sound				sndDie[SND_DIE_COUNT];


	float					m_fLandingTime;
	float					m_fJumpTime;
	float					m_fFallTime;
	float					m_fCamHeightFactor;
	float					m_fImmunityCoef;
	float					m_fDispersionCoef;

	// Dropping
	BOOL					b_DropActivated;
	float					f_DropPower;

	//random seed для Zoom mode
	s32						m_ZoomRndSeed;
	//random seed для Weapon Effector Shot
	s32						m_ShotRndSeed;

	bool					m_bOutBorder;
	//сохраняет счетчик объектов в feel_touch, для которых необходимо обновлять размер колижена с актером 
	u32						m_feel_touch_characters;
	//разрешения на удаление трупа актера 
	//после того как контролирующий его игрок зареспавнился заново. 
	//устанавливается в game
private:
	void					SwitchOutBorder(bool new_border_state);
public:
	bool					m_bAllowDeathRemove;

	void					SetZoomRndSeed			(s32 Seed = 0);
	s32						GetZoomRndSeed			()	{ return m_ZoomRndSeed;	};

	void					SetShotRndSeed			(s32 Seed = 0);
	s32						GetShotRndSeed			()	{ return m_ShotRndSeed;	};

public:
	void					detach_Vehicle			();
	void					steer_Vehicle			(float angle);
	void					attach_Vehicle			(CHolderCustom* vehicle);

	virtual bool			can_attach				(const CInventoryItem *inventory_item) const;
	void					cam_Set					(EActorCameras style);
	bool					use_Vehicle				(CHolderCustom* object);
	bool					use_MountedWeapon		(CHolderCustom* object);
protected:
	CHolderCustom*			m_holder;
	u16						m_holderID;
	bool					use_Holder				(CHolderCustom* holder);
	void					ActorPrepareUse			();
	void					ActorUse				();
	UsableObject			GetUseObject			();

protected:
	BOOL					m_bAnimTorsoPlayed;
	static void				AnimTorsoPlayCallBack(CBlend* B);

	// Rotation
	SRotation				r_torso;
	float					r_torso_tgt_roll;
	//положение торса без воздействия эффекта отдачи оружия
	SRotation				unaffected_r_torso;

	//ориентация модели
	float					r_model_yaw_dest;
	float					r_model_yaw;			// orientation of model
	float					r_model_yaw_delta;		// effect on multiple "strafe"+"something"


public:
	SActorMotions*			m_anims;
	SActorVehicleAnims*		m_vehicle_anims;

	CBlend*					m_current_legs_blend;
	CBlend*					m_current_torso_blend;
	CBlend*					m_current_jump_blend;
	MotionID				m_current_legs;
	MotionID				m_current_torso;
	MotionID				m_current_head;

	bool					b_saveAllowed;

	// callback на анимации модели актера
	void					SetCallbacks		();
	void					ResetCallbacks		();
	static void	__stdcall	Spin0Callback		(CBoneInstance*);
	static void	__stdcall	Spin1Callback		(CBoneInstance*);
	static void	__stdcall	ShoulderCallback	(CBoneInstance*);
	static void	__stdcall	HeadCallback		(CBoneInstance*);
	static void	__stdcall	VehicleHeadCallback	(CBoneInstance*);

	virtual const SRotation	Orientation			()	const	{ return r_torso; };
	SRotation				&Orientation		()			 { return r_torso; };

	void					g_SetAnimation		(u32 mstate_rl);
	void					g_SetSprintAnimation(u32 mstate_rl,MotionID &head,MotionID &torso,MotionID &legs);

public:
	virtual void			OnHUDDraw			(CCustomHUD* hud, IRenderBuffer& render_buffer);
			BOOL			HUDview				( )const ;

	//visiblity 
	virtual	float			GetEyeFovValue()	const	{ return 90.f; }
	virtual	float			GetEyeRangeValue()	const	{ return 500.f; }

	
	//////////////////////////////////////////////////////////////////////////
	// Cameras and effectors
	//////////////////////////////////////////////////////////////////////////
public:
	CActorCameraManager&	Cameras				() 	{VERIFY(m_pActorEffector); return *m_pActorEffector;}
	IC CCameraBase*			cam_Active			()	{return cameras[cam_active];}
	IC CCameraBase*			cam_FirstEye		()	{return cameras[eacFirstEye];}

protected:
	void					cam_Update				(float dt, float fFOV);
	void					cam_Lookout				( const Fmatrix &xform, float camera_height );
	void					camUpdateLadder		(float dt);
	void					cam_SetLadder			();
	void					cam_UnsetLadder			();
	float					currentFOV				();

	// Cameras
	CCameraBase*			cameras[eacMaxCam];
	EActorCameras			cam_active;
	float					fPrevCamPos;
	Fvector					vPrevCamDir;
	float					fCurAVelocity;
	CEffectorBobbing*		pCamBobbing;

	void					LoadSleepEffector		(LPCSTR section);
	SSleepEffector*			m_pSleepEffector;
	CSleepEffectorPP*		m_pSleepEffectorPP;

	//менеджер эффекторов, есть у каждого актрера
	CActorCameraManager*	m_pActorEffector;
	static float			f_Ladder_cam_limit;

	float					maxHudWetness_;
	float					wetnessAccmBase_;
	float					wetnessDecreaseF_;

public:
	virtual void			feel_touch_new				(CObject* O);
	virtual void			feel_touch_delete			(CObject* O);
	virtual BOOL			feel_touch_contact			(CObject* O);
	virtual BOOL			feel_touch_on_contact		(CObject* O);

	CGameObject*			ObjectWeLookingAt			() {return m_pObjectWeLookingAt;}
	CInventoryOwner*		PersonWeLookingAt			() {return m_pPersonWeLookingAt;}
	LPCSTR					GetDefaultActionForObject	() {return *m_sDefaultObjAction;}
//--#SM+#--
		float fFPCamYawMagnitude; //--#SM+#--
		float fFPCamPitchMagnitude; //--#SM+#--
protected:
	CUsableScriptObject*	m_pUsableObject;
	// Person we're looking at
	CInventoryOwner*		m_pPersonWeLookingAt;
	CHolderCustom*			m_pHolderWeLookingAt;
	CGameObject*			m_pObjectWeLookingAt;
	CInventoryBox*			m_pInvBoxWeLookingAt;

	// Tip for action for object we're looking at
	shared_str				m_sDefaultObjAction;
	shared_str				m_sCharacterUseAction;
	shared_str				m_sDeadCharacterUseAction;
	shared_str				m_sDeadCharacterUseOrDragAction;
	shared_str				m_sDeadCharacterDontUseAction;
	shared_str				m_sCarCharacterUseAction;
	shared_str				m_sInventoryItemUseAction;
	shared_str				m_sInventoryBoxUseAction;
	shared_str				m_sTurretCharacterUseAction;

	//расстояние подсветки предметов
	float					m_fPickupInfoRadius;

	//расстояние (в метрах) на котором актер чувствует гранату (любую)
	float					m_fFeelGrenadeRadius;
	float					m_fFeelGrenadeTime; 	//время гранаты (сек) после которого актер чувствует гранату

	void					NearItemsUpdate		();
	void					PickupItemsUpdate	();
	void					PickupInfoDraw		(CObject* object);

public:
	//////////////////////////////////////////////////////////////////////////
	// Motions (передвижения актрера)
	//////////////////////////////////////////////////////////////////////////

	void					g_cl_CheckControls		(u32 mstate_wf, Fvector &vControlAccel, float &Jump, float dt);
	void					g_cl_ValidateMState		(float dt, u32 mstate_wf);
	void					g_cl_Orientate			(u32 mstate_rl, float dt);
	void					g_sv_Orientate			(u32 mstate_rl, float dt);
	void					g_Orientate				(u32 mstate_rl, float dt);
	bool					g_LadderOrient			() ;

	bool					CanAccelerate			();
	bool					CanJump					();
	bool					CanMove					();
	float					CameraHeight			();
	bool					CanSprint				();
	bool					CanRun					();
	void					StopAnyMove				();
	void					BreakSprint				();



	// Alex ADD: for smooth crouch fix
	float					CurrentHeight;

	bool					inventoryLimitsMoving_;

	bool					AnyAction				()	{return (mstate_real & mcAnyAction) != 0;};
	bool					AnyMove					()	{return (mstate_real & mcAnyMove) != 0;};

	bool					is_jump					();	
	// Max Weight when you can walk
	float					MaxWalkWeight			() const;
	virtual float			MaxCarryWeight			() const;

	// Max Weight bonus from stuff like Outfit, Boosters, Artifacts, etc.
	float					GetAdditionalWeight		() const override;
	// Max Weight bonus from Outfit
	float					GetOutfitWeightBonus	() const override;
	u32						MovingState				() const {return mstate_real;}

	// For activating sprint when reloading
	u8						trySprintCounter_;

	Fvector					GetMovementSpeed		() { return NET_SavedAccel; };

protected:
	u32						mstate_wishful;
	u32						mstate_old;
	u32						mstate_real;

	BOOL					m_bJumpKeyPressed;

	float					m_fWalkAccel;
	float					m_fJumpSpeed;
	float					m_fRunFactor;
	float					m_fRunBackFactor;
	float					m_fWalkBackFactor;
	float					m_fCrouchFactor;
	float					m_fClimbFactor;
	float					m_fSprintFactor;

	float					m_fWalk_StrafeFactor;
	float					m_fRun_StrafeFactor;
	float					m_fPriceFactor;
	float					m_fZoomInertCoef;

	//for additional jump speed sprint speed walk speed
	float					m_fRunFactorAdditional;
	float					m_fSprintFactorAdditional;

	//Для ограничение настакивания кучи артов дающих эти бонусы
	float					m_fRunFactorAdditionalLimit;
	float					m_fSprintFactorAdditionalLimit;
	float					m_fJumpFactorAdditionalLimit;

public:
	virtual void			IR_OnMouseMove			(int x, int y);
	virtual void			IR_OnKeyboardPress		(int dik);
	virtual void			IR_OnKeyboardRelease	(int dik);
	virtual void			IR_OnKeyboardHold		(int dik);
	virtual void			IR_OnMouseWheel			(int direction);
	virtual	float			GetLookFactor			();

	//////////////////////////////////////////////////////////////////////////
	// Weapon fire control (оружие актрера)
	//////////////////////////////////////////////////////////////////////////
public:
	virtual void						g_WeaponBones		(int &L, int &R1, int &R2);
	virtual void						g_fireParams		(const CHudItem* pHudItem, Fvector& P, Fvector& D);
	virtual bool						g_stateFire			() {return ! ((mstate_wishful & mcLookout));};
	virtual BOOL						g_State				(SEntityState& state) const;
	virtual	float						GetWeaponAccuracy	() const;
			bool						IsZoomAimingMode	() const {return m_bZoomAimingMode;}
protected:
	//если актер целится в прицел
	void								SetZoomAimingMode(bool val) { m_bZoomAimingMode = val; }
	bool								m_bZoomAimingMode;

	//настройки аккуратности стрельбы
	//базовая дисперсия (когда игрок стоит на месте)
	float								m_fDispBase;
	float								m_fDispAim;
	//коэффициенты на сколько процентов увеличится базовая дисперсия
	//учитывает скорость актера 
	float								m_fDispVelFactor;
	//если актер бежит
	float								m_fDispAccelFactor;
	//если актер сидит
	float								m_fDispCrouchFactor;
	//crouch+no acceleration
	float								m_fDispCrouchNoAccelFactor;
	//смещение firepoint относительно default firepoint для бросания болтов и гранат
	Fvector								m_vMissileOffset;

protected:
	//косточки используемые при стрельбе
	int									m_r_hand;
	int									m_l_finger1;
    int									m_r_finger2;
	int									m_head;
	int									m_eye_left;
	int									m_eye_right;

	int									m_l_clavicle;
	int									m_r_clavicle;
	int									m_spine2;
	int									m_spine1;
	int									m_spine;
	int									m_neck;


protected:
	xr_deque<net_update>	NET;
	Fvector					NET_SavedAccel;
	net_update				NET_Last;

	//---------------------------------------------
	
////////////////////////////////////////////////////////////////////////////
virtual	bool				can_validate_position_on_spawn	(){return false;}
	///////////////////////////////////////////////////////
	// апдайт с данными физики
	xr_deque<net_update_A>	NET_A;
	
	//---------------------------------------------	
	/// spline coeff /////////////////////
	float			SCoeff[3][4];			//коэффициэнты для сплайна Бизье
	float			HCoeff[3][4];			//коэффициэнты для сплайна Эрмита
	Fvector			IPosS, IPosH, IPosL;	//положение актера после интерполяции Бизье, Эрмита, линейной

#ifdef DEBUG
	DEF_DEQUE(VIS_POSITION, Fvector);

	VIS_POSITION	LastPosS;
	VIS_POSITION	LastPosH;
	VIS_POSITION	LastPosL;
#endif

	SPHNetState				LastState;
	SPHNetState				RecalculatedState;
	SPHNetState				PredictedState;
	
	InterpData				IStart;
	InterpData				IRec;
	InterpData				IEnd;

	//---------------------------------------------
	DEF_DEQUE				(PH_STATES, SPHNetState);
	PH_STATES				m_States;
	u16						m_u16NumBones;
#ifdef DEBUG
	//---------------------------------------------
	virtual void			OnRender_Network();
	//---------------------------------------------
#endif
	//////////////////////////////////////////////////////////////////////////
	// Actor physics
	//////////////////////////////////////////////////////////////////////////
public:
			void			g_Physics		(Fvector& accel, float jump, float dt);
	virtual void			ForceTransform	(const Fmatrix &m);
			void			SetPhPosition	(const Fmatrix& pos);

	virtual void			MoveActor		(Fvector NewPos, Fvector NewDir);

	virtual void			SpawnAmmoForWeapon		(CInventoryItem *pIItem);
	virtual void			RemoveAmmoForWeapon		(CInventoryItem *pIItem);
	virtual	void			spawn_supplies			();
	virtual bool			human_being				() const
	{
		return				(true);
	}

	virtual	shared_str		GetDefaultVisualOutfit	() const	{return m_DefaultVisualOutfit;};
	virtual	void			SetDefaultVisualOutfit	(shared_str DefaultOutfit) {m_DefaultVisualOutfit = DefaultOutfit;};

	//Functions for actor legs and actor shadows 			#+# SkyLoader
	virtual	shared_str		GetDefaultVisualOutfit_legs	() const	{return m_DefaultVisualOutfit_legs;};
	virtual	void			SetDefaultVisualOutfit_legs	(shared_str DefaultOutfit) {m_DefaultVisualOutfit_legs = DefaultOutfit;};
	virtual	void			SetDrawLegs	(bool DrawLegs) {m_bDrawLegs = DrawLegs;};
	virtual bool			DrawLegs	() const {return m_bDrawLegs;}
	virtual	void			SetActorShadows	(bool ActorShadows) {m_bActorShadows = ActorShadows;};
	virtual bool			IsActorShadowsOn	() const {return m_bActorShadows;}
	virtual bool			IsFirstEye	() const {return (m_bFirstEye);}
	virtual bool			IsLookAt	() const {return (eacLookAt==cam_active);}
	virtual EActorCameras	ActiveCameraType() const { return cam_active; }
	virtual bool			CanBeDrawLegs	() const {return (m_bCanBeDrawLegs);}
	//

	virtual u16				HolderID 	() const {return m_holderID;}

	virtual void			UpdateAnimation			() 	{ g_SetAnimation(mstate_real); };

	virtual void			ChangeVisual			( shared_str NewVisual );
	virtual void			OnChangeVisual			();

	void					UpdateHeavyBreath();

	// Calc hud wetness for raindrops shader
	void					ComputeHudWetness();
	void ComputeCondition();

	//////////////////////////////////////////////////////////////////////////
	// Controlled Routines
	//////////////////////////////////////////////////////////////////////////

			void			set_input_external_handler			(CActorInputHandler *handler);
			bool			input_external_handler_installed	() const {return (m_input_external_handler != 0);}
			
			IC		void			lock_accel_for(u32 time){ m_time_lock_accel = EngineTimeU() + time; }

private:	
	CActorInputHandler		*m_input_external_handler;
	u32						m_time_lock_accel;

	bool					needActivateNV_; // for after loading
	bool					needActivateCrouchState_; // for after loading

	/////////////////////////////////////////
	// DEBUG INFO
protected:
		shared_str				m_DefaultVisualOutfit;
		shared_str				m_DefaultVisualOutfit_legs;
		bool					m_bDrawLegs;
		bool					m_bFirstEye;
		bool					m_bCanBeDrawLegs;
		bool					m_bActorShadows;

		LPCSTR					invincibility_fire_shield_3rd;
		LPCSTR					invincibility_fire_shield_1st;
		shared_str				m_sHeadShotParticle;
		u32						last_hit_frame;

		Fvector					m_AutoPickUp_AABB;
		Fvector					m_AutoPickUp_AABB_Offset;
public:
		void					SetWeaponHideState				(u32 State, bool bSet, bool need_unholster = true);
		virtual CCustomOutfit*	GetOutfit() const;

private://IPhysicsShellHolder
		virtual	void _stdcall	HideAllWeapons(bool v) { SetWeaponHideState(INV_STATE_BLOCK_ALL, v); }
public:
	void						SetCantRunState(bool bSet);
private:
	CActorCondition				*m_entity_condition;

protected:
	virtual	CEntityConditionSimple	*create_entity_condition	(CEntityConditionSimple* ec);

public:
	IC		CActorCondition		&conditions					() const;
	virtual DLL_Pure			*_construct					();
	virtual bool				natural_weapon				() const {return false;}
	virtual bool				natural_detector			() const {return false;}
	virtual bool				use_center_to_aim			() const;

protected:
	u16							m_iLastHitterID;
	u16							m_iLastHittingWeaponID;
	s16							m_s16LastHittedElement;
	Fvector						m_vLastHitDir;
	Fvector						m_vLastHitPos;
	float						m_fLastHealth;
	bool						m_bWasHitted;
	bool						m_bWasBackStabbed;


	bool						m_secondRifleSlotAllowed;

public:
	virtual void				SetHitInfo						(CObject* who, CObject* weapon, s16 element, Fvector Pos, Fvector Dir);

	virtual	void				OnHitHealthLoss					(float NewHealth);	
	virtual	void				OnCriticalHitHealthLoss			();
	virtual	void				OnCriticalWoundHealthLoss		();
	virtual void				OnCriticalRadiationHealthLoss	();

	virtual	bool				InventoryAllowSprint			();
	virtual void				OnNextWeaponSlot				();
	virtual void				OnPrevWeaponSlot				();
			bool				CanPutInSlot					(PIItem item, u32 slot) override;

public:
	
	virtual	void				on_weapon_shot_start			(CWeapon *weapon);
	virtual	void				on_weapon_shot_update			();
	virtual	void				on_weapon_shot_stop				();
	virtual	void				on_weapon_shot_remove			(CWeapon *weapon);
	virtual	void				on_weapon_hide					(CWeapon *weapon);
			Fvector				weapon_recoil_delta_angle		();
			Fvector				weapon_recoil_last_delta		();
protected:
	virtual	void				update_camera					(CCameraShotEffector* effector);
	//step manager
	virtual bool				is_on_ground					();

private:
	CActorMemory				*m_memory;

public:
			void				SetActorVisibility				(u16 who, float value);
	IC		CActorMemory		&memory							() const {VERIFY(m_memory); return(*m_memory); };

	void						OnDifficultyChanged				();

	IC float					HitProbability					() {return m_hit_probability;}
	virtual	CVisualMemoryManager*visual_memory					() const;

	virtual	BOOL				BonePassBullet					(int boneID);
	virtual	void				On_B_NotCurrentEntity			();

private:
	collide::rq_results			RQR;
	BOOL						CanPickItem						(const CFrustum& frustum, const Fvector& from, CObject* item);
	xr_vector<ISpatial*>		ISpatialResult;

	CLocationManager			*m_location_manager;

	xr_vector<CInventoryItem*>	pickUpItems;

public:
	IC		const CLocationManager	&locations					() const
	{
		VERIFY						(m_location_manager);
		return						(*m_location_manager);
	}
private:
	ALife::_OBJECT_ID	m_holder_id;

public:
	virtual bool				register_schedule				() const {return false;}
// lost alpha start
			

			bool				GetActorNightVisionStatus		() const { return m_nv_handler && m_nv_handler->GetNightVisionStatus(); }
		//	bool				GetNightVisionStatus			() const;
			void				SwitchNightVision				();				// switches any available NV device 
			void				SwitchNightVision				(bool val);		// switches any available NV device to value
			void				SwitchDeviceNightVision			();				// switches any NV device, exept weapon's.
			void				SwitchDeviceNightVision			(bool val);		// switches any NV device to value, exept weapon's.

			bool				GetActorAliveDetectorState		() const;
			void				SwitchAliveDetector				();
			void				SwitchActorAliveDetector		();
			void				SwitchActorAliveDetector		(bool state);

			CCar*				GetAttachedCar					();
			void				RechargeTorchBattery			();
			CTorch				*GetCurrentTorch				();
			bool				IsLimping						();
			bool				IsReloadingWeapon				();
			bool				UsingTurret						();
			u16					GetTurretTemp					();
			void				SetDirectionSlowly				(Fvector pos, float time);
			void				SetIconState					(EActorState state, bool show);
			bool				GetIconState					(EActorState state) { return !!m_icons_state.is(1 << state); }
			Flags32&			GetIconsState					()					{ return m_icons_state; }

			float				SetWalkAccel					(float new_value);	
			void				SetImmunityCoeff				(float value)   { m_fImmunityCoef = value;}
			void				SetCamDispersionCoeff			(float value)   { m_fDispersionCoef = value; }
			void				SetZoomInertCoeff				(float value)   { m_fZoomInertCoef = value; }
			void				SetSprintFactor					(float value)   { m_fSprintFactor = value; }
			void				SetPriceFactor					(float value)   { m_fPriceFactor = value; }
			float				GetPriceFactor					()				{ return m_fPriceFactor; }

			CNightVisionActor*	GetActorNVHandler() { return m_nv_handler; }
private:
	CTorch*						m_current_torch;
	CNightVisionActor*			m_nv_handler;
	COutfitBase*				m_alive_detector_device;

	Flags32						m_icons_state;

public:
	bool						needUseKeyRelease;
	float						timeUseAccum;
	CGameObject*				usageTarget;

	bool						pickUpLongInProgress;

	UsableObject				currentUsable;

	virtual	bool				is_ai_obstacle					() const;

public:
			void			DisableHitMarks(bool disable)		{m_disabled_hitmarks = disable;};
			bool			DisableHitMarks()					{return m_disabled_hitmarks;};

			void			set_inventory_disabled (bool is_disabled) { m_inventory_disabled = is_disabled; }
			bool			inventory_disabled () const { return m_inventory_disabled; }
private:
			void			set_state_box(u32	mstate);
private:
	bool					m_disabled_hitmarks;
	bool					m_inventory_disabled;

DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CActor)
#undef script_type_list
#define script_type_list save_type_list(CActor)

extern bool		isActorAccelerated			(u32 mstate, bool ZoomMode);

IC	CActorCondition	&CActor::conditions	() const{ VERIFY(m_entity_condition); return(*m_entity_condition);}

extern CActor*		g_actor;
CActor*				Actor();
extern const float	s_fFallTime;
