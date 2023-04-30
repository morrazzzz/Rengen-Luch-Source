#include "pch_script.h"
#include "ActorFlags.h"
#include "hudmanager.h"
#include "alife_space.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "CameraLook.h"
#include "car.h"
#include "effectorfall.h"
#include "EffectorBobbing.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "CustomOutfit.h"
#include "actorcondition.h"
#include "UIGameCustom.h"
#include "Actor.h"
#include "actoranimdefs.h"
#include "ai_sounds.h"
#include "inventory.h"
#include "../../xrphysics/matrix_utils.h"
#include "level.h"
#include "string_table.h"
#include "alife_registry_wrappers.h"
#include "artifact.h"
#include "CharacterPhysicsSupport.h"
#include "material_manager.h"
#include "../../xrphysics/IColisiondamageInfo.h"
#include "GameTaskManager.h"
#include "Script_Game_Object.h"
#include "script_callback_ex.h"
#include "InventoryBox.h"
#include "mounted_turret.h"
#include "player_hud.h"
#include "ai\monsters\ai_monster_utils.h"
#include "..\igame_persistent.h"
#include "Grenade.h"
#include "actorflags.h"

#define ARTEFACTS_UPDATE_TIME 0.100f

Flags32 psActorFlags = {0};

extern ENGINE_API float		fActor_Lum;
extern ENGINE_API BOOL		showActorLuminocity_;

extern u32					crosshairAnimationType;
extern bool					g_bAutoClearCrouch;
extern u32					g_bAutoApplySprint;

extern ENGINE_API float camFov;

float NET_Jump				= 0;

extern float cammera_into_collision_shift;

void CActor::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif
	
	UpdateInventoryOwner(TimeDeltaU()); // cop merge, was in sc update

	if (m_feel_touch_characters > 0)
	{
		for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
		{
			CPhysicsShellHolder* sh = smart_cast<CPhysicsShellHolder*>(*it);

			if (sh && sh->character_physics_support())
			{
				sh->character_physics_support()->movement()->UpdateObjectBox(character_physics_support()->movement()->PHCharacter());
			}
		}
	}

	if (m_holder)
		m_holder->UpdateEx(currentFOV());

	m_snd_noise -= 0.3f * TimeDelta();

	VERIFY2(_valid(renderable.xform), CObject::ObjectName().c_str());

	inherited::UpdateCL();

	VERIFY2(_valid(renderable.xform), CObject::ObjectName().c_str());

	m_pPhysics_support->in_UpdateCL();

	VERIFY2(_valid(renderable.xform), CObject::ObjectName().c_str());

	SetZoomAimingMode(false);

	CWeapon* pWeapon = smart_cast<CWeapon*>(inventory().ActiveItem());

	cam_Update(float(TimeDeltaU()) / 1000.0f, currentFOV());

	if (Level().CurrentEntity() && this->ID() == Level().CurrentEntity()->ID())
	{
		CurrentGameUI()->ShowCrosshair(CB_WEAPON, true);
		CurrentGameUI()->ShowGameIndicators(IB_WEAPON, true);
	}

	if (pWeapon)
	{
		if (pWeapon->IsZoomed())
		{
			CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>(Cameras().GetCamEffector(eCEZoom));
			if (S)
			{
				S->SetParams(GetWeaponAccuracy() * pWeapon->GetZoomInertion() * m_fZoomInertCoef);
			}

			m_bZoomAimingMode = true;
		}

		if (Level().CurrentEntity() && this->ID() == Level().CurrentEntity()->ID())
		{
			float fire_disp_full = pWeapon->GetFireDispersion(true) + GetWeaponAccuracy();

			if (!Device.m_SecondViewport.IsSVPActive())
				HUD().SetCrosshairDisp(fire_disp_full, 0.02f);

			if (crosshairAnimationType != 0)
				HUD().ShowCrosshair(pWeapon->use_crosshair());
			else
				HUD().ShowCrosshair(false);

			if (eacLookAt == cam_active)
			{
				CurrentGameUI()->ShowCrosshair(CB_WEAPON, true);
				CurrentGameUI()->ShowGameIndicators(IB_WEAPON, true);
			}
			else
			{
				CurrentGameUI()->ShowCrosshair(CB_WEAPON, pWeapon->show_crosshair());
				CurrentGameUI()->ShowGameIndicators(IB_WEAPON, pWeapon->show_indicators());

				// Обновляем двойной рендер от оружия [Update SecondVP with weapon data]
				// XXX: To all: maybe after disable VP we can disable this part?
				pWeapon->UpdateSecondVP(); //--#SM+#-- +SecondVP+

				// Обновляем информацию об оружии в шейдерах
				g_pGamePersistent->weaponSVPShaderParams->svpParams.x = pWeapon->GetInZoomNow(); //--#SM+#--
				g_pGamePersistent->weaponSVPShaderParams->svpParams.y = pWeapon->GetSecondVPFov(); //--#SM+#--
			}
		}

	}
	else if (m_holder && smart_cast<CMountedTurret*>(m_holder))
	{
		HUD().SetCrosshairDisp(0.f);
		HUD().ShowCrosshair(true);

		CurrentGameUI()->ShowCrosshair(CB_WEAPON, true);
		CurrentGameUI()->ShowGameIndicators(IB_WEAPON, true);
	}
	else if (Level().CurrentEntity() && this->ID() == Level().CurrentEntity()->ID())
	{
		HUD().SetCrosshairDisp(0.f);
		HUD().ShowCrosshair(false);

		// Очищаем информацию об оружии в шейдерах
		g_pGamePersistent->weaponSVPShaderParams->svpParams.set(0.f, 0.f, 0.f, 0.f); //--#SM+#--

		// Отключаем второй вьюпорт [Turn off SecondVP]
		Device.m_SecondViewport.SetSVPActive(false); //--#SM+#-- +SecondVP+
	}

	UpdateDefferedMessages();

	if (g_Alive())
		CStepManager::update(this == Level().CurrentViewEntity());

	spatial.s_type |= STYPE_REACTTOSOUND;

	if (m_sndShockEffector)
	{
		if (this == Level().CurrentViewEntity())
		{
			m_sndShockEffector->Update();

			if (!m_sndShockEffector->InWork() || !g_Alive())
				xr_delete(m_sndShockEffector);
		}
		else
			xr_delete(m_sndShockEffector);
	}

	if (m_ScriptCameraDirection)
	{
		if (this == Level().CurrentViewEntity())
		{
			m_ScriptCameraDirection->Update();

			if (!m_ScriptCameraDirection->InWork() || !g_Alive())
				xr_delete(m_ScriptCameraDirection);
		}
		else
			xr_delete(m_ScriptCameraDirection);
	}

	CTorch *flashlight = GetCurrentTorch();
	if (flashlight)
		flashlight->UpdateBattery();

	if (m_nv_handler)
		m_nv_handler->UpdateCL();

	if (m_alive_detector_device &&
		m_alive_detector_device != GetCurrentOutfit() &&
		m_alive_detector_device != inventory().ItemFromSlot(HELMET_SLOT))
	{
		m_alive_detector_device->SwitchAliveDetector(false);
		m_alive_detector_device = nullptr;
	}

	Fmatrix	trans;

	if (cam_Active() == cam_FirstEye())
		Cameras().hud_camera_Matrix(trans);
	else
		Cameras().camera_Matrix(trans);

	if (IsFocused())
		g_player_hud->update(trans);

	if (showActorLuminocity_)
		fActor_Lum = ROS()->get_ai_luminocity();
	

#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_Actor_ += measure_updatecl.GetElapsed_sec();
#endif
}


void CActor::ScheduledUpdate(u32 DT)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	if (IsFocused())
	{
		BOOL bHudView = HUDview();
		if (bHudView)
		{
			CInventoryItem* pInvItem = inventory().ActiveItem();
			if (pInvItem)
			{
				CHudItem* pHudItem = smart_cast<CHudItem*>(pInvItem);
				if (pHudItem)
				{
					if (pHudItem->IsHidden())
					{
						g_player_hud->detach_item(pHudItem);
					}
					else
					{
						g_player_hud->attach_item(pHudItem);
					}
				}
			}
			else
			{
				g_player_hud->detach_item_idx(0);
				//Msg("---No active item in inventory(), item 0 detached.");
			}
		}
		else
		{
			g_player_hud->detach_all_items();
			//Msg("---No hud view found, all items detached.");
		}

	}

	if (m_holder || !getEnabled() || !Ready())
	{
		m_sDefaultObjAction = NULL;
		inherited::ScheduledUpdate(DT);

		return;
	}


	clamp(DT, 0u, 100u);
	float dt = float(DT) / 1000.f;

	// Check controls, create accel, prelimitary setup "mstate_real"

	if (Level().CurrentControlEntity() == this)
		//------------------------------------------------
	{
		g_cl_CheckControls(mstate_wishful, NET_SavedAccel, NET_Jump, dt);
		{

		}

		g_cl_Orientate(mstate_real, dt);
		g_Orientate(mstate_real, dt);

		g_Physics(NET_SavedAccel, NET_Jump, dt);

		g_cl_ValidateMState(dt, mstate_wishful);
		g_SetAnimation(mstate_real);

		// Check for game-contacts
		Fvector C; float R;
		//m_PhysicMovementControl->GetBoundingSphere	(C,R);

		Center(C);
		R = Radius();

		feel_touch_update(C, R);

		Feel_Grenade_Update(m_fFeelGrenadeRadius);

		// Dropping
		if (b_DropActivated)
		{
			f_DropPower += dt * 0.1f;

			clamp(f_DropPower, 0.f, 1.f);
		}
		else
		{
			f_DropPower = 0.f;
		}

		mstate_wishful &= ~mcAccel;
		mstate_wishful &= ~mcLStrafe;
		mstate_wishful &= ~mcRStrafe;
		mstate_wishful &= ~mcLLookout;
		mstate_wishful &= ~mcRLookout;
		mstate_wishful &= ~mcFwd;
		mstate_wishful &= ~mcBack;

		if (g_bAutoClearCrouch)
		{
			mstate_wishful &= ~mcCrouch;

			if (g_bAutoApplySprint > 0)
			{
				g_bAutoApplySprint += 1;
			}
		}

		if (g_bAutoApplySprint == 10)//применит бег на 10й кадр
		{
			mstate_wishful |= mcSprint;
			g_bAutoApplySprint = 0;
		}

	}

	NET_Jump = 0;

	inherited::ScheduledUpdate(DT);

	//эффектор включаемый при ходьбе
	if (psActorFlags.test(AF_HEAD_BOBBING))
	{
		if (!pCamBobbing)
		{
			pCamBobbing = xr_new <CEffectorBobbing>();

			Cameras().AddCamEffector(pCamBobbing);
		}

		pCamBobbing->SetState(mstate_real, conditions().IsLimping(), IsZoomAimingMode());
	}
	else if (pCamBobbing)
	{
		Cameras().RemoveCamEffector(eCEBobbing);

		pCamBobbing = nullptr;
	}

	//звук тяжелого дыхания при низком хп, уталости и хромании
	UpdateHeavyBreath();

	float bs = conditions().GetZoneDanger();
	if (bs > 0.1f)
	{
		Fvector snd_pos;
		snd_pos.set(0, ACTOR_HEIGHT, 0);

		if (!m_DangerSnd._feedback())
			m_DangerSnd.play_at_pos(this, snd_pos, sm_Looped | sm_2D);
		else
			m_DangerSnd.set_position(snd_pos);

		float v = bs + 0.25f;

		m_DangerSnd.set_volume(v);
	}
	else
	{
		if (m_DangerSnd._feedback())
			m_DangerSnd.stop();
	}

	if (!g_Alive() && m_DangerSnd._feedback())
		m_DangerSnd.stop();

	ComputeHudWetness();

	ComputeCondition();

	PickupItemsUpdate();

	if (((BOOL)m_bActorShadows == psActorFlags.test(AF_ACTOR_BODY)) && g_Alive() && !m_holder)
	{
		if (m_bActorShadows)
			SetDefaultVisualOutfit_legs(pSettings->r_string(*SectionName(), "default_outfit_legs"));
		else
			SetDefaultVisualOutfit_legs(GetDefaultVisualOutfit());

		m_bActorShadows = (psActorFlags.test(AF_ACTOR_BODY)) ? false : true;

		if (eacFirstEye == cam_active) //reset visual
		{
			cam_Set(eacLookAt);
			cam_Set(eacFirstEye);
		}
	}

	//если в режиме HUD, то сама модель актера не рисуется
	if (!character_physics_support()->IsRemoved())
		if (m_bDrawLegs && ((!psDeviceFlags.test(rsR2) && !psDeviceFlags.test(rsR3) && !psDeviceFlags.test(rsR4) && !m_bActorShadows) || ((psDeviceFlags.test(rsR2) || psDeviceFlags.test(rsR3) || psDeviceFlags.test(rsR4)) && m_bActorShadows)))
			setVisible(TRUE);
		else
			setVisible(!HUDview());

	//что актер видит перед собой
	collide::rq_result& RQ = HUD().GetCurrentRayQuery();

	float dist_to_obj = RQ.range;

	if (RQ.O && eacFirstEye != cam_active)
		dist_to_obj = get_bone_position(this, "bip01_spine").distance_to((smart_cast<CGameObject*>(RQ.O))->Position());

	if (!input_external_handler_installed() && RQ.O && RQ.O->getVisible() && dist_to_obj < inventory().GetTakeDist())
	{
		m_pObjectWeLookingAt = smart_cast<CGameObject*>(RQ.O);

		CGameObject* game_object = smart_cast<CGameObject*>(RQ.O);

		m_pUsableObject = smart_cast<CUsableScriptObject*>(game_object);
		m_pInvBoxWeLookingAt = smart_cast<CInventoryBox*>(game_object);
		m_pPersonWeLookingAt = smart_cast<CInventoryOwner*>(game_object);
		m_pHolderWeLookingAt = smart_cast<CHolderCustom*>(game_object);

		CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(game_object);

		if (m_pUsableObject && m_pUsableObject->tip_text())
		{
			m_sDefaultObjAction = CStringTable().translate(m_pUsableObject->tip_text());
		}
		else
		{
			if (m_pPersonWeLookingAt && pEntityAlive && pEntityAlive->g_Alive() && m_pPersonWeLookingAt->IsTalkEnabled())
				m_sDefaultObjAction = m_sCharacterUseAction;

			else if (pEntityAlive && !pEntityAlive->g_Alive())
			{
				bool b_allow_drag = !!pSettings->line_exist("ph_capture_visuals", pEntityAlive->VisualName());

				if (m_pPersonWeLookingAt && !m_pPersonWeLookingAt->inventory().CanBeDragged())
					b_allow_drag = false;

				if (b_allow_drag)
					m_sDefaultObjAction = m_sDeadCharacterUseOrDragAction;
				else if (!m_pPersonWeLookingAt->deadbody_closed_status())
					m_sDefaultObjAction = m_sDeadCharacterUseAction;

			}
			else if (m_pHolderWeLookingAt)
			{
				if (smart_cast<CCar*>(m_pHolderWeLookingAt))
					m_sDefaultObjAction = m_sCarCharacterUseAction;
				else
					m_sDefaultObjAction = m_sTurretCharacterUseAction;
			}

			else if (inventory().m_pTarget && inventory().m_pTarget->CanTake())
				m_sDefaultObjAction = m_sInventoryItemUseAction;
			else
				m_sDefaultObjAction = NULL;
		}
	}
	else 
	{
		m_pPersonWeLookingAt	= NULL;
		m_sDefaultObjAction		= NULL;
		m_pUsableObject			= NULL;
		m_pObjectWeLookingAt	= NULL;
		m_pHolderWeLookingAt	= NULL;
		m_pInvBoxWeLookingAt	= NULL;
	}

	//для свойст артефактов, находящихся на поясе
	UpdateArtefactsOnBeltAndOutfit();
	m_pPhysics_support->in_shedule_Update(DT);

	
#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_Actor_ += measure_sc_update.GetElapsed_sec();
#endif
};


void CActor::UpdateHeavyBreath()
{
	if (this == Level().CurrentControlEntity())
	{
		if ((conditions().health() <= 0.15f || conditions().GetPower() < 0.35f) && g_Alive())
		{
			if (conditions().health() <= 0.15f)
			{
				if (conditions().health() <= 0.03f)
					m_HeavyBreathSnd.set_frequency(1.f);
				else if (conditions().health() <= 0.05f)
					m_HeavyBreathSnd.set_frequency(0.8f);
				else if (conditions().health() <= 0.12f)
					m_HeavyBreathSnd.set_frequency(0.6f);
				else if (conditions().health() <= 0.15f)
					m_HeavyBreathSnd.set_frequency(0.5f);
			}
			else if (conditions().GetPower() <= 0.35f)
			{
				if (conditions().GetPower() <= 0.10f)
					m_HeavyBreathSnd.set_frequency(1.f);
				else if (conditions().GetPower() <= 0.15f)
					m_HeavyBreathSnd.set_frequency(0.8f);
				else if (conditions().GetPower() <= 0.25f)
					m_HeavyBreathSnd.set_frequency(0.6f);
				else if (conditions().GetPower() <= 0.35f)
					m_HeavyBreathSnd.set_frequency(0.5f);
			}

			if (!m_HeavyBreathSnd._feedback())
				m_HeavyBreathSnd.play_at_pos(this, Fvector().set(0, ACTOR_HEIGHT, 0), sm_Looped | sm_2D);
			else
				m_HeavyBreathSnd.set_position(Fvector().set(0, ACTOR_HEIGHT, 0));
		}
		else if (m_HeavyBreathSnd._feedback())
			m_HeavyBreathSnd.stop();

		float bs = conditions().BleedingSpeed();

		if (bs > 0.6f)
		{
			if (conditions().health() < 0.2f)
				m_BloodSnd.set_frequency(0.5f);
			else if (conditions().health() <= 0.3f)
				m_BloodSnd.set_frequency(0.6f);
			else if (conditions().health() <= 0.4f)
				m_BloodSnd.set_frequency(0.7f);
			else if (conditions().health() <= 0.5f)
				m_BloodSnd.set_frequency(0.8f);
			else if (conditions().health() <= 0.8f)
				m_BloodSnd.set_frequency(0.9f);
			else
				m_BloodSnd.set_frequency(1);

			Fvector snd_pos;

			snd_pos.set(0, ACTOR_HEIGHT, 0);

			if (!m_BloodSnd._feedback())
				m_BloodSnd.play_at_pos(this, snd_pos, sm_Looped | sm_2D);
			else
				m_BloodSnd.set_position(snd_pos);

			float v = bs + 0.25f;

			m_BloodSnd.set_volume(v);
		}
		else
		{
			if (m_BloodSnd._feedback())
				m_BloodSnd.stop();
		}

		if (!g_Alive() && m_BloodSnd._feedback())
			m_BloodSnd.stop();
	}
}


void CActor::UpdateArtefactsOnBeltAndOutfit()
{
	static float update_time = 0;

	float f_update_time = 0;

	if(update_time < ARTEFACTS_UPDATE_TIME)
	{
		update_time += conditions().fdelta_time();

		return;
	}
	else
	{
		f_update_time = update_time;
		update_time = 0.0f;
	}

	//tatarinrafa: added additional jump speed sprint speed walk speed
	float run_koef_additional		= 0.0f;
	float sprint_koef_additional	= 0.0f;
	float jump_koef_additional		= 0.0f;

	for (TIItemContainer::iterator it = inventory().artefactBelt_.begin();
		inventory().artefactBelt_.end() != it; ++it)
	{
		CArtefact*	artefact = smart_cast<CArtefact*>(*it);
		if(artefact)
		{
			conditions().AddBleeding			(artefact->m_fBleedingRestoreSpeed*f_update_time);
			conditions().ChangeHealth			(artefact->m_fHealthRestoreSpeed*f_update_time);
			conditions().ChangePower			(artefact->m_fPowerRestoreSpeed*f_update_time);
			conditions().ChangeSatiety			(artefact->m_fSatietyRestoreSpeed*f_update_time);
			conditions().AddRadiation			(artefact->m_fRadiationRestoreSpeed*f_update_time);
			conditions().AddPsyHealth			(artefact->m_fPsyhealthRestoreSpeed*f_update_time);

			//сложим бонусы скорости от артифактов на поясе
			run_koef_additional		 += artefact->m_additional_run_coef;
			sprint_koef_additional	 += artefact->m_additional_sprint_koef;
			jump_koef_additional	 += artefact->m_additional_jump_speed;
		}
	}

	//проверим не превысили ли лимит указаный в актор лтх. только для артов. для костюмов не проверяем
	if (run_koef_additional > m_fRunFactorAdditionalLimit)
		run_koef_additional = m_fRunFactorAdditionalLimit;

	if (sprint_koef_additional >m_fSprintFactorAdditionalLimit)
		sprint_koef_additional = m_fSprintFactorAdditionalLimit;

	if (jump_koef_additional >m_fJumpFactorAdditionalLimit)
		jump_koef_additional = m_fJumpFactorAdditionalLimit;
	
	CCustomOutfit* outfit = GetOutfit();

	if (outfit)
	{
		conditions().ChangeBleeding	(outfit->GetBleedingRestoreSpeed()  * f_update_time);
		conditions().ChangeHealth	(outfit->GetHealthRestoreSpeed()    * f_update_time);
		conditions().ChangePower	(outfit->GetPowerRestoreSpeed()     * f_update_time);
		conditions().ChangeSatiety	(outfit->GetSatietyRestoreSpeed()   * f_update_time);
		conditions().ChangeRadiation(outfit->GetRadiationRestoreSpeed() * f_update_time);

		//добавим бонусы от костюма
		run_koef_additional		+= outfit->m_additional_run_coef;
		sprint_koef_additional	+= outfit->m_additional_sprint_koef;
		jump_koef_additional	+= outfit->m_additional_jump_speed;
	}

	m_fSprintFactorAdditional = sprint_koef_additional;
	m_fRunFactorAdditional = run_koef_additional;
	character_physics_support()->movement()->SetJumpUpVelocity(m_fJumpSpeed + jump_koef_additional);//для прыжка немного подругому
}

void CActor::PHHit(SHit &H)
{
	m_pPhysics_support->in_Hit(H, false);
}

struct playing_pred
{
	IC	bool operator() (ref_sound &s)
	{
		return (NULL != s._feedback());
	}
};

void CActor::Hit(SHit* pHDS)
{
	bool b_initiated = pHDS->aim_bullet; // physics strike by poltergeist

	pHDS->aim_bullet = false;

	SHit& HDS = *pHDS;
	if (HDS.hit_type < ALife::eHitTypeBurn || HDS.hit_type >= ALife::eHitTypeMax)
	{
		string256	err;
		xr_sprintf(err, "Unknown/unregistered hit type [%d]", HDS.hit_type);
		R_ASSERT2(0, err);

	}
#ifdef DEBUG
	if (ph_dbg_draw_mask.test(phDbgCharacterControl)) {
		DBG_OpenCashedDraw();
		Fvector to; to.add(Position(), Fvector().mul(HDS.dir, HDS.phys_impulse()));
		DBG_DrawLine(Position(), to, D3DCOLOR_XRGB(124, 124, 0));
		DBG_ClosedCashedDraw(500);
	}
#endif

	bool bPlaySound = true;

	if (!g_Alive())
		bPlaySound = false;

	if (!sndHit[HDS.hit_type].empty() && conditions().PlayHitSound(pHDS))
	{
		ref_sound& S = sndHit[HDS.hit_type][Random.randI(sndHit[HDS.hit_type].size())];
		bool b_snd_hit_playing = sndHit[HDS.hit_type].end() != std::find_if(sndHit[HDS.hit_type].begin(), sndHit[HDS.hit_type].end(), playing_pred());

		if (ALife::eHitTypeExplosion == HDS.hit_type)
		{
			if (this == Level().CurrentControlEntity())
			{
				S.set_volume(10.0f);
				if (!m_sndShockEffector) {
					m_sndShockEffector = xr_new<SndShockEffector>();
					m_sndShockEffector->Start(this, float(S.get_length_sec()*1000.0f), HDS.damage());
				}
			}
			else
				bPlaySound = false;
		}
		if (bPlaySound && !b_snd_hit_playing)
		{
			Fvector point = Position();
			point.y += CameraHeight();
			S.play_at_pos(this, point);
		}
	}

	//slow actor, only when he gets hit
	m_hit_slowmo = conditions().HitSlowmo(pHDS);

	//---------------------------------------------------------------
	if ((Level().CurrentViewEntity() == this) && (HDS.hit_type == ALife::eHitTypeFireWound))
	{
		CObject* pLastHitter = Level().Objects.net_Find(m_iLastHitterID);
		CObject* pLastHittingWeapon = Level().Objects.net_Find(m_iLastHittingWeaponID);
		HitSector(pLastHitter, pLastHittingWeapon);
	}

	if ((mstate_real&mcSprint) && Level().CurrentControlEntity() == this && conditions().DisableSprint(pHDS))
	{
		bool const is_special_burn_hit_2_self = (pHDS->who == this) && (pHDS->boneID == BI_NONE) &&
			((pHDS->hit_type == ALife::eHitTypeBurn) || (pHDS->hit_type == ALife::eHitTypeLightBurn));
		if (!is_special_burn_hit_2_self)
		{
			mstate_wishful &= ~mcSprint;
		}
	}
	if (!m_disabled_hitmarks)
	{
		bool b_fireWound = (pHDS->hit_type == ALife::eHitTypeFireWound || pHDS->hit_type == ALife::eHitTypeWound_2);
		b_initiated = b_initiated && (pHDS->hit_type == ALife::eHitTypeStrike);

		if (b_fireWound || b_initiated)
			HitMark(HDS.damage(), HDS.dir, HDS.who, HDS.bone(), HDS.p_in_bone_space, HDS.impulse, HDS.hit_type);
	}

	float hit_power = HitArtefactsOnBelt(HDS.damage(), HDS.hit_type);

	hit_power = HitBoosters(hit_power, HDS.hit_type);

	hit_power *= m_fImmunityCoef;

	if (GodMode())
	{
		HDS.power = 0.0f;
		inherited::Hit(&HDS);
		return;
	}
	else
	{
		HDS.power = hit_power;
		HDS.add_wound = true;
		inherited::Hit(&HDS);
	}
}

void CActor::HitMark(float P, Fvector dir, CObject* who_object, s16 element, Fvector position_in_bone_space, float impulse, ALife::EHitType hit_type)
{
	// hit marker
	if ((hit_type == ALife::eHitTypeFireWound || hit_type == ALife::eHitTypeWound_2) && g_Alive() && (Level().CurrentEntity() == this))
	{
		HUD().HitMarked(0, P, dir);

		CEffectorCam* ce = Cameras().GetCamEffector((ECamEffectorType)effFireHit);
		if (!ce)
		{
			int id = -1;
			Fvector	cam_pos, cam_dir, cam_norm;

			cam_Active()->Get(cam_pos, cam_dir, cam_norm);
			cam_dir.normalize_safe();
			dir.normalize_safe();

			float ang_diff = angle_difference(cam_dir.getH(), dir.getH());
			Fvector						cp;
			cp.crossproduct(cam_dir, dir);
			bool bUp = (cp.y>0.0f);

			Fvector cross;
			cross.crossproduct(cam_dir, dir);
			VERIFY(ang_diff >= 0.0f && ang_diff <= PI);

			float _s1 = PI_DIV_8;
			float _s2 = _s1 + PI_DIV_4;
			float _s3 = _s2 + PI_DIV_4;
			float _s4 = _s3 + PI_DIV_4;

			if (ang_diff <= _s1)
				id = 2;
			else if (ang_diff > _s1 && ang_diff <= _s2)
				id = (bUp) ? 5 : 7;
			else if (ang_diff>_s2 && ang_diff <= _s3)
				id = (bUp) ? 3 : 1;
			else if (ang_diff>_s3 && ang_diff <= _s4)
				id = (bUp) ? 4 : 6;
			else if (ang_diff>_s4)
				id = 0;
			else
				VERIFY(0);

			string64 sect_name;
			xr_sprintf(sect_name, "effector_fire_hit_%d", id);

			AddEffector(this, effFireHit, sect_name, P / 100.0f);
		}
	}

	if (who_object && !fis_zero(P))
		callback(GameObject::eHit)(
			lua_game_object(), 
			P,
			dir,
			smart_cast<const CGameObject*>(who_object)->lua_game_object(),
			element
		);

}

void CActor::HitSignal(float perc, Fvector& vLocalDir, CObject* who, s16 element)
{
	if (g_Alive()) 
	{	
		// check damage bone
		Fvector D;
		XFORM().transform_dir(D,vLocalDir);

		float yaw, pitch;
		D.getHP(yaw,pitch);

		IKinematics *K = smart_cast<IKinematics*>(Visual());
		IKinematicsAnimated *KA = smart_cast<IKinematicsAnimated*>(Visual());

		VERIFY(K && KA);

		MotionID motion_ID = m_anims->m_normal.m_damage[iFloor(K->LL_GetBoneInstance(element).get_param(1) + (angle_difference(r_model_yaw + r_model_yaw_delta,yaw) <= PI_DIV_2 ? 0 : 1))];
		float power_factor = perc/100.f; clamp(power_factor,0.f,1.f);

		VERIFY(motion_ID.valid());

		KA->PlayFX(motion_ID,power_factor);
	}
}

float CActor::HitArtefactsOnBelt(float hit_power, ALife::EHitType hit_type)
{
	float res_hit_power_k = 1.0f;
	float _af_count = 0.0f;

	for (TIItemContainer::iterator it = inventory().artefactBelt_.begin(); inventory().artefactBelt_.end() != it; ++it)
	{
		CArtefact*	artefact = smart_cast<CArtefact*>(*it);
		if (artefact)
		{
			res_hit_power_k += artefact->m_ArtefactHitImmunities.AffectHit(1.0f, hit_type);
			_af_count += 1.0f;
		}
	}

	res_hit_power_k -= _af_count;

	return res_hit_power_k * hit_power;
}

float CActor::HitBoosters(float hit_power, ALife::EHitType hit_type)
{
	float res_hit_power_k = 1.0f;
	float _af_count = 0.0f;

	for (u16 i = 0; i < conditions().Eat_Effects.size(); i++)
	{
		if (conditions().Eat_Effects[i].BoosterParam.EffectIsBooster && (conditions().Eat_Effects[i].DurationExpiration >= EngineTime()) && (conditions().Eat_Effects[i].UseTimeExpiration < EngineTime()))
		{
			res_hit_power_k += conditions().Eat_Effects[i].BoosterParam.BoosterHitImmunities.AffectHit(1.0f, hit_type);
			_af_count += 1.0f;
		}
	}

	res_hit_power_k -= _af_count;

	return res_hit_power_k * hit_power;
}


void CActor::OnItemTake(CInventoryItem* inventory_item, bool duringSpawn)
{
	CInventoryOwner::OnItemTake(inventory_item, duringSpawn);
}

void CActor::OnItemDrop(CInventoryItem* inventory_item)
{
	//change of actor hud and visual if there is no outfit in slot and droped item.type is outfit
	PIItem	outfit = this->inventory().ItemFromSlot(OUTFIT_SLOT);
	CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>	(inventory_item);

	if (!outfit && pOutfit)
	{
		if (IsFirstEye())
		{
			shared_str DefVisual = GetDefaultVisualOutfit_legs();
			if (DefVisual.size())
			{
				ChangeVisual(DefVisual);
			}
		}
		else {
			shared_str DefVisual = GetDefaultVisualOutfit();
			if (DefVisual.size())
			{
				ChangeVisual(DefVisual);
			}
		}

		if (this == Level().CurrentViewEntity())
			g_player_hud->load_default();
	}


	CWeapon* weapon = smart_cast<CWeapon*>(inventory_item);
	if (weapon && weapon->m_eItemPlace == eItemPlaceSlot)
	{
		weapon->OnZoomOut();
	}

	CInventoryOwner::OnItemDrop(inventory_item);
}

void CActor::OnItemDropUpdate()
{
	CInventoryOwner::OnItemDropUpdate();

	TIItemContainer::iterator I = inventory().allContainer_.begin();
	TIItemContainer::iterator E = inventory().allContainer_.end();
	
	for ( ; I != E; ++I)
		if(!(*I)->IsInvalid() && !attached(*I))
			attach(*I);
}

void CActor::OnItemRuck(CInventoryItem* inventory_item, EItemPlace previous_place)
{
	CInventoryOwner::OnItemRuck(inventory_item, previous_place);
}

void CActor::OnItemBelt(CInventoryItem* inventory_item, EItemPlace previous_place)
{
	CInventoryOwner::OnItemBelt(inventory_item, previous_place);
}

void start_tutorial(LPCSTR name);

void CActor::Die(CObject* who)
{
	inherited::Die(who);

	u16 I = inventory().FirstSlot();
	u16 E = inventory().LastSlot();

	for (; I <= E; ++I)
	{
		PIItem item_in_slot = inventory().ItemFromSlot(I);
		if (I == inventory().GetActiveSlot())
		{
			CGrenade* grenade = smart_cast<CGrenade*>(item_in_slot);
			if (grenade)
				grenade->DropGrenade();
			else
				item_in_slot->SetDropManual(TRUE);

			continue;
		}
		else
		{
			CCustomOutfit* pOutfit = smart_cast<CCustomOutfit* > (item_in_slot);

			if (pOutfit)
				continue;
		};

		if (item_in_slot)
			inventory().MoveToRuck(item_in_slot);
	};

	///!!! чистка пояса
	TIItemContainer &l_blist = inventory().belt_;

	while (!l_blist.empty())
		inventory().MoveToRuck(l_blist.front());

	if (psActorFlags.test(AF_FST_PSN_DEATH))
	{
		cam_Set(eacFirstEye);
		m_bActorShadows = true;
	}
	else
		cam_Set(eacFreeLook);

	mstate_wishful	&=		~mcAnyMove;
	mstate_real		&=		~mcAnyMove;

	::Sound->play_at_pos(sndDie[Random.randI(SND_DIE_COUNT)],this,Position());

	m_HeavyBreathSnd.stop();
	m_BloodSnd.stop();
	m_DangerSnd.stop();
	
	start_tutorial("game_over");

	CurrentGameUI()->HideShownDialogs();

	xr_delete				(m_sndShockEffector);
	xr_delete				(m_ScriptCameraDirection);

	if (m_nv_handler)
	{
		m_nv_handler->SwitchNightVision(false);
		xr_delete(m_nv_handler);
	}
}

void CActor::SwitchOutBorder(bool new_border_state)
{
	if(new_border_state)
	{
		callback(GameObject::eExitLevelBorder)(lua_game_object());
	}
	else 
	{
		callback(GameObject::eEnterLevelBorder)(lua_game_object());
	}

	m_bOutBorder=new_border_state;
}

void CActor::g_Physics(Fvector& _accel, float jump, float dt)
{
	// Correct accel
	Fvector accel;

	accel.set(_accel);
	m_hit_slowmo -= dt;
	if (m_hit_slowmo < 0)
		m_hit_slowmo = 0.f;

	accel.mul(1.f - m_hit_slowmo);

	if (g_Alive())
	{
		if (mstate_real&mcClimb&&!cameras[eacFirstEye]->bClampYaw)
			accel.set(0.f, 0.f, 0.f);

		character_physics_support()->movement()->Calculate(accel, cameras[cam_active]->vDirection, 0, jump, dt, false);
		bool new_border_state = character_physics_support()->movement()->isOutBorder();

		if (m_bOutBorder != new_border_state && Level().CurrentControlEntity() == this)
		{
			SwitchOutBorder(new_border_state);
		}

		character_physics_support()->movement()->GetPosition(Position());
		character_physics_support()->movement()->bSleep = false;
	}

	if (g_Alive())
	{
		if (character_physics_support()->movement()->gcontact_Was)
			Cameras().AddCamEffector(xr_new <CEffectorFall>(character_physics_support()->movement()->gcontact_Power));

		if (!fis_zero(character_physics_support()->movement()->gcontact_HealthLost))
		{
			VERIFY(character_physics_support());
			VERIFY(character_physics_support()->movement());

			ICollisionDamageInfo* di = character_physics_support()->movement()->CollisionDamageInfo();

			VERIFY(di);

			bool b_hit_initiated = di->GetAndResetInitiated();
			Fvector hdir;
			di->HitDir(hdir);

			SetHitInfo(this, NULL, 0, Fvector().set(0, 0, 0), hdir);

			if (Level().CurrentControlEntity() == this)
			{
				SHit HDS = SHit(character_physics_support()->movement()->gcontact_HealthLost, hdir, di->DamageInitiator(), character_physics_support()->movement()->ContactBone(), di->HitPos(), 0.f, di->HitType(), 0.0f, b_hit_initiated);

				NET_Packet	l_P;
				HDS.GenHeader(GE_HIT, ID());
				HDS.whoID = di->DamageInitiator()->ID();
				HDS.weaponID = di->DamageInitiator()->ID();
				HDS.Write_Packet(l_P);

				u_EventSend(l_P);
			}
		}
	}
}

void CActor::renderable_Render(IRenderBuffer& render_buffer)
{
	VERIFY(_valid(XFORM()));

	if(devfloat1)
		inherited::renderable_Render(render_buffer);

	if (!HUDview() || (m_bActorShadows && m_bFirstEye))
	{
		CInventoryOwner::renderable_Render(render_buffer);
	}
}

BOOL CActor::renderable_ShadowGenerate()
{
	if (m_holder || (!m_bActorShadows && m_bFirstEye))
		return FALSE;

	return inherited::renderable_ShadowGenerate();
}

void CActor::OnHUDDraw(CCustomHUD* hud, IRenderBuffer& render_buffer)
{
	if (IsFocused())
	{
		g_player_hud->render_hud(render_buffer);
	}
}