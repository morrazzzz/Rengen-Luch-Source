#include "pch_script.h"
#include "Actor.h"
#include "CameraLook.h"
#include "CameraFirstEye.h"
#include "inventory.h"
#include "string_table.h"
#include "CharacterPhysicsSupport.h"
#include "material_manager.h"
#include "actor_memory.h"
#include "location_manager.h"
#include "SleepEffector.h"
#include "Car.h"
#include "actoranimdefs.h"
#include "ActorEffector.h"
#include "alife_registry_wrappers.h"
#include "ActorCondition.h"
#include "..\igame_persistent.h"

extern float cammera_into_collision_shift;

CActor::CActor() : CEntityAlive()
{
	encyclopedia_registry	= xr_new <CEncyclopediaRegistryWrapper>();
	game_news_registry		= xr_new <CGameNewsRegistryWrapper>();
	// Cameras
	cameras[eacFirstEye]	= xr_new <CCameraFirstEye>(this, CCameraBase::flPositionRigid);
	cameras[eacFirstEye]->LoadCfg("actor_firsteye_cam");

	psActorFlags.set(AF_PSP, TRUE);

	cameras[eacLookAt]		= xr_new <CCameraLook2>(this);
	cameras[eacLookAt]->LoadCfg("actor_look_cam");

	cameras[eacFreeLook]	= xr_new <CCameraLook>(this);
	cameras[eacFreeLook]->LoadCfg("actor_free_cam");

	cam_active				= eacFirstEye;
	fPrevCamPos				= 0.0f;
	vPrevCamDir.set			(0.f, 0.f, 1.f);

	fFPCamYawMagnitude = 0.0f; //--#SM+#--
	fFPCamPitchMagnitude = 0.0f; //--#SM+#--
		// Alex ADD: for smooth crouch fix
	CurrentHeight = 0.f;

	fCurAVelocity			= 0.0f;

	// эффекторы
	pCamBobbing				= 0;
	m_pSleepEffector		= NULL;
	m_pSleepEffectorPP		= NULL;

	r_torso.yaw				= 0;
	r_torso.pitch			= 0;
	r_torso.roll			= 0;
	r_torso_tgt_roll		= 0;
	r_model_yaw				= 0;
	r_model_yaw_delta		= 0;
	r_model_yaw_dest		= 0;

	b_DropActivated			= 0;
	f_DropPower				= 0.f;

	m_fRunFactor			= 2.f;
	m_fCrouchFactor			= 0.2f;
	m_fClimbFactor			= 1.f;
	m_fCamHeightFactor		= 0.87f;

	m_fRunFactorAdditional		= 0.f;
	m_fSprintFactorAdditional	= 0.f;

	m_fSprintFactor			= 4.f;
	m_fImmunityCoef			= 1.f;
	m_fDispersionCoef		= 1.f;
	m_fPriceFactor			= 1.f;
	m_fZoomInertCoef		= 1.f;

	m_fFallTime				= s_fFallTime;
	m_bAnimTorsoPlayed		= false;
	b_saveAllowed			= true;

	m_fFeelGrenadeRadius	=	10.0f;
	m_fFeelGrenadeTime      =	1.0f;

	m_pPhysicsShell			= NULL;
	m_holder				= NULL;
	m_holderID				= u16(-1);

	inventory().SetBeltUseful(true);

	m_pPersonWeLookingAt	= NULL;
	m_pHolderWeLookingAt	= NULL;
	m_pObjectWeLookingAt	= NULL;

	m_pActorEffector		= NULL;

	SetZoomAimingMode		(false);

	m_sDefaultObjAction		= NULL;

	m_icons_state.zero		 ();
	m_pUsableObject			= NULL;

	m_anims					= xr_new <SActorMotions>();
	m_vehicle_anims			= xr_new <SActorVehicleAnims>();
	m_entity_condition		= NULL;
	m_iLastHitterID			= u16(-1);
	m_iLastHittingWeaponID	= u16(-1);
	m_statistic_manager		= NULL;

	m_memory				= xr_new <CActorMemory>(this);
	m_bOutBorder			= false;
	m_hit_probability		= 1.f;
	m_feel_touch_characters = 0;
	m_location_manager		= xr_new <CLocationManager>(this);

	m_current_torch			= nullptr;
	m_nv_handler			= nullptr;
	m_alive_detector_device = nullptr;

	inventory().SetCurrentDetector(NULL);

	m_secondRifleSlotAllowed = false;

	xr_strcpy(quickUseSlotsContents_[0], "");
	xr_strcpy(quickUseSlotsContents_[1], "");
	xr_strcpy(quickUseSlotsContents_[2], "");
	xr_strcpy(quickUseSlotsContents_[3], "");

	trySprintCounter_		= 0;

	maxHudWetness_			= 0.7f;
	wetnessAccmBase_		= 0.01f;
	wetnessDecreaseF_		= 0.023f;

	needActivateNV_			= false;
	inventoryLimitsMoving_	= false;

	needUseKeyRelease		= false;
	timeUseAccum			= 0.f;
	usageTarget				= nullptr;

	pickUpLongInProgress	= false;

	m_block_sprint_counter	= 0;

	m_disabled_hitmarks		= false;
	m_inventory_disabled	= false;
}


CActor::~CActor()
{
	xr_delete				(m_nv_handler);

	xr_delete				(m_location_manager);
	xr_delete				(m_memory);
	xr_delete				(encyclopedia_registry);
	xr_delete				(game_news_registry);

	for (int i = 0; i < eacMaxCam; ++i)
		xr_delete(cameras[i]);

	m_HeavyBreathSnd.destroy();
	m_BloodSnd.destroy();
	m_DangerSnd.destroy();

	xr_delete				(m_pActorEffector);
	xr_delete				(m_pSleepEffector);
	xr_delete				(m_pPhysics_support);
	xr_delete				(m_anims);
	xr_delete				(m_vehicle_anims);
}

void CActor::reinit	()
{
	character_physics_support()->movement()->CreateCharacter();
	character_physics_support()->movement()->SetPhysicsRefObject(this);

	CEntityAlive::reinit();
	CInventoryOwner::reinit();

	character_physics_support()->in_Init();
	material().reinit();

	m_pUsableObject								= NULL;

	memory().reinit();
	
	set_input_external_handler(0);

	m_time_lock_accel = 0;
}

void CActor::reload(LPCSTR section)
{
	CEntityAlive::reload		(section);
	CInventoryOwner::reload		(section);
	material().reload			(section);
	CStepManager::reload		(section);

	memory().reload				(section);
	m_location_manager->reload	(section);
}

void set_box(LPCSTR section, CPHMovementControl &mc, u32 box_num)
{
	Fbox	bb; Fvector	vBOX_center, vBOX_size;
	// m_PhysicMovementControl: BOX
	string64 buff, buff1;
	strconcat(sizeof(buff), buff, "ph_box", itoa(box_num, buff1, 10), "_center");
	vBOX_center = pSettings->r_fvector3(section, buff);
	strconcat(sizeof(buff), buff, "ph_box", itoa(box_num, buff1, 10), "_size");
	vBOX_size = pSettings->r_fvector3(section, buff);
	vBOX_size.y += cammera_into_collision_shift / 2.f;
	bb.set(vBOX_center, vBOX_center); bb.grow(vBOX_size);
	mc.SetBox(box_num, bb);
}

void CActor::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg				(section);
	material().LoadCfg				(section);
	CInventoryOwner::LoadCfg		(section);
	m_location_manager->LoadCfg	(section);

	OnDifficultyChanged();

	ISpatial* self = smart_cast<ISpatial*>(this);

	if (self)
	{
		self->spatial.s_type |=	STYPE_VISIBLEFORAI;
		self->spatial.s_type &= ~STYPE_REACTTOSOUND;
	}

	// m_PhysicMovementControl: General
	Fbox bb; Fvector vBOX_center, vBOX_size;

	// m_PhysicMovementControl: BOX
	vBOX_center = pSettings->r_fvector3	(section, "ph_box2_center");
	vBOX_size	= pSettings->r_fvector3	(section, "ph_box2_size");
	bb.set(vBOX_center,vBOX_center);
	bb.grow(vBOX_size);

	character_physics_support()->movement()->SetBox(2, bb);

	// m_PhysicMovementControl: BOX
	vBOX_center = pSettings->r_fvector3	(section, "ph_box1_center");
	vBOX_size	= pSettings->r_fvector3	(section, "ph_box1_size");

	bb.set	(vBOX_center,vBOX_center);
	bb.grow(vBOX_size);
	character_physics_support()->movement()->SetBox(1, bb);

	// m_PhysicMovementControl: BOX
	vBOX_center= pSettings->r_fvector3	(section, "ph_box0_center");
	vBOX_size	= pSettings->r_fvector3	(section, "ph_box0_size");

	bb.set	(vBOX_center,vBOX_center);
	bb.grow(vBOX_size);

	character_physics_support()->movement()->SetBox(0,bb);

	// m_PhysicMovementControl: Crash speed and mass
	float	cs_min		= pSettings->r_float(section, "ph_crash_speed_min");
	float	cs_max		= pSettings->r_float(section, "ph_crash_speed_max");
	float	mass		= pSettings->r_float(section, "ph_mass");

	character_physics_support()->movement()->SetCrashSpeeds(cs_min,cs_max);
	character_physics_support()->movement()->SetMass(mass);

	if(pSettings->line_exist(section,"stalker_restrictor_radius"))
		character_physics_support()->movement()->SetActorRestrictorRadius(rtStalker, pSettings->r_float(section, "stalker_restrictor_radius"));
	if(pSettings->line_exist(section,"stalker_small_restrictor_radius"))
		character_physics_support()->movement()->SetActorRestrictorRadius(rtStalkerSmall, pSettings->r_float(section, "stalker_small_restrictor_radius"));
	if(pSettings->line_exist(section,"medium_monster_restrictor_radius"))
		character_physics_support()->movement()->SetActorRestrictorRadius(rtMonsterMedium, pSettings->r_float(section, "medium_monster_restrictor_radius"));

	character_physics_support()->movement()->LoadCfg(section);

	set_box(section, *character_physics_support()->movement(), 2);
	set_box(section, *character_physics_support()->movement(), 1);
	set_box(section, *character_physics_support()->movement(), 0);

	m_fWalkAccel				= pSettings->r_float(section, "walk_accel");	
	m_fJumpSpeed				= pSettings->r_float(section, "jump_speed");
	m_fRunFactor				= pSettings->r_float(section, "run_coef");
	m_fRunBackFactor			= pSettings->r_float(section, "run_back_coef");
	m_fWalkBackFactor			= pSettings->r_float(section, "walk_back_coef");
	m_fCrouchFactor				= pSettings->r_float(section, "crouch_coef");
	m_fClimbFactor				= pSettings->r_float(section, "climb_coef");
	m_fSprintFactor				= pSettings->r_float(section, "sprint_koef");

	m_fWalk_StrafeFactor		= READ_IF_EXISTS(pSettings, r_float, section, "walk_strafe_coef", 1.0f);
	m_fRun_StrafeFactor			= READ_IF_EXISTS(pSettings, r_float, section, "run_strafe_coef", 1.0f);

	m_fRunFactorAdditionalLimit		= READ_IF_EXISTS(pSettings, r_float, section, "artifact_run_speed_limit",		m_fRunFactor);
	m_fSprintFactorAdditionalLimit	= READ_IF_EXISTS(pSettings, r_float, section, "artifact_sprint_speed_limit",	m_fSprintFactor);
	m_fJumpFactorAdditionalLimit	= READ_IF_EXISTS(pSettings, r_float, section, "artifact_jump_speed_limit",		m_fJumpSpeed);

	character_physics_support()->movement()->SetJumpUpVelocity(m_fJumpSpeed);

	m_fCamHeightFactor = pSettings->r_float(section, "camera_height_factor");
	character_physics_support()->movement()->SetJumpUpVelocity(m_fJumpSpeed);
	float AirControlParam = pSettings->r_float(section, "air_control_param");
	character_physics_support()->movement()->SetAirControlParam(AirControlParam);

	m_fPickupInfoRadius	= pSettings->r_float(section, "pickup_info_radius");
	m_fSleepTimeFactor	= pSettings->r_float(section, "sleep_time_factor");

	m_fFeelGrenadeRadius = pSettings->r_float(section, "feel_grenade_radius");
	m_fFeelGrenadeTime = pSettings->r_float(section, "feel_grenade_time");
	m_fFeelGrenadeTime *= 1000.0f;

	character_physics_support()->in_Load(section);
	
	//загрузить параметры эффектора
	LoadSleepEffector("sleep_effector");

	//загрузить параметры смещения firepoint
	m_vMissileOffset = pSettings->r_fvector3(section,"missile_throw_offset");

	LPCSTR hit_snd_sect = pSettings->r_string(section,"hit_sounds");

	for (int hit_type = 0; hit_type<(int)ALife::eHitTypeMax; ++hit_type)
	{
		LPCSTR hit_name = ALife::g_cafHitType2String((ALife::EHitType)hit_type);

		LPCSTR hit_snds = READ_IF_EXISTS(pSettings, r_string, hit_snd_sect, hit_name, "");

		int cnt = _GetItemCount(hit_snds);
		string128 tmp;

		VERIFY(cnt != 0);

		for(int i = 0; i < cnt; ++i)
		{
			sndHit[hit_type].push_back		(ref_sound());
			sndHit[hit_type].back().create	(_GetItem(hit_snds,i,tmp),st_Effect,sg_SourceType);
		}

		char buf[256];

		::Sound->create(sndDie[0], strconcat(sizeof(buf), buf, *ObjectName(), "\\die0"), st_Effect, SOUND_TYPE_MONSTER_DYING);
		::Sound->create(sndDie[1], strconcat(sizeof(buf), buf, *ObjectName(), "\\die1"), st_Effect, SOUND_TYPE_MONSTER_DYING);
		::Sound->create(sndDie[2], strconcat(sizeof(buf), buf, *ObjectName(), "\\die2"), st_Effect, SOUND_TYPE_MONSTER_DYING);
		::Sound->create(sndDie[3], strconcat(sizeof(buf), buf, *ObjectName(), "\\die3"), st_Effect, SOUND_TYPE_MONSTER_DYING);

		m_HeavyBreathSnd.create(pSettings->r_string(section,"heavy_breath_snd"), st_Effect,SOUND_TYPE_MONSTER_INJURING);
		m_BloodSnd.create(pSettings->r_string(section,"heavy_blood_snd"), st_Effect,SOUND_TYPE_MONSTER_INJURING);
		m_DangerSnd.create(pSettings->r_string(section, "heavy_danger_snd"), st_Effect, SOUND_TYPE_MONSTER_INJURING);
	}

	//--#SM+#-- Сбрасываем режим рендеринга в дефолтный [reset some render flags]
	// TODO: check this, maybe not needed
	//g_pGamePersistent->m_pGShaderConstants->m_blender_mode.set(0.f, 0.f, 0.f, 0.f);

	cam_Set(eacFirstEye);

	// sheduler
	shedule.t_min				= shedule.t_max = 1;

	// настройки дисперсии стрельбы
	m_fDispBase					= pSettings->r_float		(section, "disp_base");
	m_fDispBase					= deg2rad(m_fDispBase);

	m_fDispAim					= pSettings->r_float		(section, "disp_aim");
	m_fDispAim					= deg2rad(m_fDispAim);

	m_fDispVelFactor			= pSettings->r_float		(section, "disp_vel_factor");
	m_fDispAccelFactor			= pSettings->r_float		(section, "disp_accel_factor");
	m_fDispCrouchFactor			= pSettings->r_float		(section, "disp_crouch_factor");
	m_fDispCrouchNoAccelFactor	= pSettings->r_float		(section, "disp_crouch_no_acc_factor");

	m_bActorShadows = (psActorFlags.test(AF_ACTOR_BODY)) ? false : true;

	LPCSTR default_outfit = READ_IF_EXISTS(pSettings, r_string, section, "default_outfit", 0);

	SetDefaultVisualOutfit(default_outfit);

	LPCSTR default_outfit_legs = pSettings->r_string(section, "default_outfit_legs");

	if (m_bActorShadows)
		SetDefaultVisualOutfit_legs(default_outfit);
	else
		SetDefaultVisualOutfit_legs(default_outfit_legs);

	m_bCanBeDrawLegs = true;

	if (CanBeDrawLegs())
		m_bDrawLegs = true;
	else
		m_bDrawLegs = false;

	invincibility_fire_shield_1st	= READ_IF_EXISTS(pSettings, r_string, section, "Invincibility_Shield_1st", 0);
	invincibility_fire_shield_3rd	= READ_IF_EXISTS(pSettings, r_string, section, "Invincibility_Shield_3rd", 0);
	m_AutoPickUp_AABB				= READ_IF_EXISTS(pSettings, r_fvector3, section, "AutoPickUp_AABB", Fvector().set(0.02f, 0.02f, 0.02f));
	m_AutoPickUp_AABB_Offset		= READ_IF_EXISTS(pSettings, r_fvector3, section, "AutoPickUp_AABB_offs", Fvector().set(0, 0, 0));

	CStringTable string_table;
	m_sCharacterUseAction			= "character_use";
	m_sDeadCharacterUseAction		= "dead_character_use";
	m_sDeadCharacterUseOrDragAction = "dead_character_use_or_drag";
	m_sDeadCharacterDontUseAction	= "dead_character_dont_use";
	m_sCarCharacterUseAction		= "car_character_use";
	m_sInventoryItemUseAction		= "inventory_item_use";
	m_sInventoryBoxUseAction		= "inventory_box_use";
	m_sTurretCharacterUseAction		= "turret_character_use";
	//---------------------------------------------------------------------
	m_sHeadShotParticle				= READ_IF_EXISTS(pSettings, r_string, section, "HeadShotParticle", 0);
	m_secondRifleSlotAllowed		= READ_IF_EXISTS(pSettings, r_bool, section, "allow_second_rifle_slot", false) != 0;

	// Enable luminocity calculation for AI visual memory manager
	ROS()->needed_for_ai = true;

	maxHudWetness_					= READ_IF_EXISTS(pSettings, r_float, section, "hud_wetness_max", 0.7f);
	wetnessAccmBase_				= READ_IF_EXISTS(pSettings, r_float, section, "hud_wetness_accm_base", 0.023f);
	wetnessDecreaseF_				= READ_IF_EXISTS(pSettings, r_float, section, "hud_wetness_decreese_base", 0.01f);

	// Alex ADD: for smooth crouch fix
	CurrentHeight = CameraHeight();
}

CEntityConditionSimple* CActor::create_entity_condition(CEntityConditionSimple* ec)
{
	if (!ec)
		m_entity_condition = xr_new <CActorCondition>(this);
	else
		m_entity_condition = smart_cast<CActorCondition*>(ec);

	return		(inherited::create_entity_condition(m_entity_condition));
}

DLL_Pure* CActor::_construct()
{
	m_pPhysics_support = xr_new <CCharacterPhysicsSupport>(CCharacterPhysicsSupport::etActor, this);

	CEntityAlive::_construct();
	CInventoryOwner::_construct();
	CStepManager::_construct();

	return(this);
}