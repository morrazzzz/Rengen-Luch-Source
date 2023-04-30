#include "stdafx.h"

#include "Weapon.h"
#include "ParticlesObject.h"
#include "player_hud.h"
#include "entity_alive.h"
#include "gamepersistent.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"

#include "actor.h"
#include "actoreffector.h"
#include "level.h"

#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "clsid_game.h"
#include "object_broker.h"
#include "weaponBinocularsVision.h"
#include "ui/UIWindow.h"
#include "ui/UIXmlInit.h"

#include "debug_renderer.h"

#include "CustomOutfit.h"

#include "ai\trader\ai_trader.h"
#include "inventory_item_impl.h"

bool m_bDraw_off = true;
bool m_bHolster_off = true;

extern CUIXml*	pWpnScopeXml;

CWeapon::SZoomParams::SZoomParams()
{
	m_bZoomEnabled = true;
	m_bHideCrosshairInZoom = true;
	m_bIsZoomModeNow = false;
	m_fCurrentZoomFactor = 50.f;
	m_fZoomRotateTime = 0.15;
	m_fIronSightZoomFactor = 50.f;
	m_fScopeZoomFactor = 50.f;
	m_fZoomRotationFactor = 0.f;
	m_fSecondVPZoomFactor = 0.0f;
	m_fSecondVPWorldFOV = 100.0f;
	m_bUseDynamicZoom = FALSE;
	scopeAliveDetectorVision_ = nullptr;
	scopeNightVision_ = nullptr;
}

CWeapon::CWeapon()
{
	SetState				(eHidden);
	SetNextState			(eHidden);
	m_sub_state				= eSubstateReloadBegin;
	m_bTriStateReload		= false;
	SetDefaults				();

	m_Offset.identity		();
	m_StrapOffset.identity	();

	m_iAmmoCurrentTotal		= 0;
	m_BriefInfo_CalcFrame	= 0;

	iAmmoElapsed			= -1;
	maxMagazineSize_		= -1;
	m_ammoType				= 0;
	m_ammoName				= NULL;

	eHandDependence			= hdNone;

	m_zoom_params.m_fCurrentZoomFactor			= camFov;
	m_zoom_params.m_fZoomRotationFactor			= 0.f;
	m_zoom_params.scopeAliveDetectorVision_		= NULL;
	m_zoom_params.scopeNightVision_				= NULL;

	m_zoom_params.reenableNVOnZoomIn_			= false;
	m_zoom_params.reenableActorNVOnZoomOut_		= false;

	m_zoom_params.reenableALDetectorOnZoomIn_		= false;
	m_zoom_params.reenableActorALDetectorOnZoomOut_ = false;

	m_scope_adetector_enabled = false;


	m_pCurrentAmmo			= NULL;

	m_pFlameParticles2		= NULL;
	m_sFlameParticles2		= NULL;


	m_fCurrentCartirdgeDisp = 1.f;

	m_strap_bone0			= 0;
	m_strap_bone1			= 0;
	m_strap_bone0_id			= -1;
	m_strap_bone1_id			= -1;
	m_StrapOffset.identity	();
	m_strapped_mode			= false;
	m_strapped_mode_rifle		= false;
	m_can_be_strapped_rifle		= false;
	m_can_be_strapped		= false;
	m_ef_main_weapon_type	= u32(-1);
	m_ef_weapon_type		= u32(-1);
	m_UIScope				= NULL;

	m_fLR_MovingFactor = 0.f;
	m_fLR_CameraFactor = 0.f;

	m_fLR_ShootingFactor = 0.f;
	m_fUD_ShootingFactor = 0.f;
	m_fBACKW_ShootingFactor = 0.f;

	SetAmmoOnReload(u8(-1));
	m_cur_scope				= 0;

	m_fRTZoomFactor			= 0.f;
	m_fSVP_RTZoomFactor		= 0.f;
	m_ai_weapon_rank		= u32(-1);

	aproxEffectiveDistance_ = 50.f;

	aiAproximateEffectiveDistanceBase_ = 50.f;

	secondVPWorldHudFOVK = 1.0f;

	scopeVisual = nullptr;
	silencerVisual = nullptr;
	glauncherVisual = nullptr;
	fakeViual = nullptr;
	grenadeVisual = nullptr;

	scopeAttachOffset[0].set(0, 0, 0);
	scopeAttachOffset[1].set(0, 0, 0);

	silencerAttachOffset[0].set(0, 0, 0);
	silencerAttachOffset[1].set(0, 0, 0);

	glauncherAttachOffset[0].set(0, 0, 0);
	glauncherAttachOffset[1].set(0, 0, 0);

	m_activation_speed_is_overriden = false;
}

CWeapon::~CWeapon()
{
	xr_delete	(m_UIScope);

	xr_delete	(m_zoom_params.scopeNightVision_);
	xr_delete	(m_zoom_params.scopeAliveDetectorVision_);
}

void CWeapon::Hit					(SHit* pHDS)
{
	inherited::Hit(pHDS);
}



void CWeapon::UpdateXForm	()
{
	if (CurrentFrame() == dwXF_Frame)
		return;

	dwXF_Frame = CurrentFrame();

	if (!GetHUDmode())
		UpdateAddonsTransform();

	if (!H_Parent())
		return;

	// Get access to entity and its visual
	CEntityAlive*			E = smart_cast<CEntityAlive*>(H_Parent());
	CAI_Trader*				T = smart_cast<CAI_Trader*>(H_Parent());

	if (T) return;

	if (!E) {
		return;
	}

	const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
	if (!parent || parent->use_simplified_visual())
		return;

	if (!m_can_be_strapped_rifle)
	{
		if (parent->attached(this))
			return;
	}

	IKinematics*			V = smart_cast<IKinematics*>	(E->Visual());
	VERIFY					(V);

	// Get matrices
	int						boneL = -1, boneR = -1, boneR2 = -1;

	if ((m_strap_bone0_id == -1 || m_strap_bone1_id == -1) && m_can_be_strapped_rifle)
	{
		m_strap_bone0_id = V->LL_BoneID(m_strap_bone0);
		m_strap_bone1_id = V->LL_BoneID(m_strap_bone1);
	}

	u32 activeSlot = parent->inventory().GetActiveSlot();
	if (activeSlot != CurrSlot() && m_can_be_strapped_rifle && parent->inventory().IsInSlot(this))
	{
		boneR				= m_strap_bone0_id;
		boneR2				= m_strap_bone1_id;
		boneL				= boneR;

		if (!m_strapped_mode_rifle) m_strapped_mode_rifle = true;
	} else {
		E->g_WeaponBones		(boneL,boneR,boneR2);

		if (m_strapped_mode_rifle) m_strapped_mode_rifle = false;
	}

	if (boneR == -1) 		return;

	if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
		boneL				= boneR2;

	if (boneL == -1) 		return;

	V->CalculateBones(need_actual);

	Fmatrix& mL				= V->LL_GetTransform(u16(boneL));
	Fmatrix& mR				= V->LL_GetTransform(u16(boneR));
	// Calculate
	Fmatrix					mRes;
	Fvector					R,D,N;
	D.sub					(mL.c,mR.c);	

	if(fis_zero(D.magnitude())) {
		mRes.set			(E->XFORM());
		mRes.c.set			(mR.c);
	}
	else {		
		D.normalize			();
		R.crossproduct		(mR.j,D);

			N.crossproduct	(D,R);			
			N.normalize();

			mRes.set		(R,N,D,mR.c);
			mRes.mulA_43	(E->XFORM());
		}

	UpdatePosition			(mRes);

}

void CWeapon::UpdateFireDependencies_internal()
{
	if (CurrentFrame() != dwFP_Frame)
	{
		dwFP_Frame = CurrentFrame();

		UpdateXForm			();

		if ( GetHUDmode() )
		{
			HudItemData()->setup_firedeps		(m_current_firedeps);
			VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
		} else 
		{
			// 3rd person or no parent
			Fmatrix& parent			= XFORM();
			Fvector& fp				= vLoadedFirePoint;
			Fvector& fp2			= vLoadedFirePoint2;
			Fvector& sp				= vLoadedShellPoint;

			parent.transform_tiny	(m_current_firedeps.vLastFP,fp);
			parent.transform_tiny	(m_current_firedeps.vLastFP2,fp2);
			parent.transform_tiny	(m_current_firedeps.vLastSP,sp);
			
			m_current_firedeps.vLastFD.set	(0.f,0.f,1.f);
			parent.transform_dir	(m_current_firedeps.vLastFD);

			m_current_firedeps.m_FireParticlesXForm.set(parent);
			VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
		}
	}
}

void CWeapon::ForceUpdateFireParticles()
{
	if ( !GetHUDmode() )
	{//update particlesXFORM real bullet direction

		if (!H_Parent())		return;

		Fvector					p, d; 
		smart_cast<CEntity*>(H_Parent())->g_fireParams	(this, p,d);

		Fmatrix						_pxf;
		_pxf.k						= d;
		_pxf.i.crossproduct			(Fvector().set(0.0f,1.0f,0.0f),	_pxf.k);
		_pxf.j.crossproduct			(_pxf.k,		_pxf.i);
		_pxf.c						= XFORM().c;
		
		m_current_firedeps.m_FireParticlesXForm.set	(_pxf);
	}

}

void CWeapon::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);
	CShootingObject::LoadCfg(section);
	
	if(pSettings->line_exist(section, "flame_particles_2"))
		m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");

	// load ammo classes
	m_ammoTypes.clear	(); 
	LPCSTR				S = pSettings->r_string(section,"ammo_class");
	if (S && S[0]) 
	{
		string128		_ammoItem;
		int				count		= _GetItemCount	(S);
		for (int it=0; it<count; ++it)	
		{
			_GetItem				(S,it,_ammoItem);
			m_ammoTypes.push_back	(_ammoItem);
		}
		m_ammoName = pSettings->r_string(*m_ammoTypes[0],"inv_name_short");
	}
	else
		m_ammoName = 0;

	iAmmoElapsed		= pSettings->r_s32		(section,"ammo_elapsed"		);
	maxMagazineSize_	= pSettings->r_s32		(section,"ammo_mag_size"	);
	
	////////////////////////////////////////////////////
	// дисперсия стрельбы

	//подбрасывание камеры во время отдачи
	u8 rm = READ_IF_EXISTS( pSettings, r_u8, section, "cam_return", 1 );
	cam_recoil.camReturnMode = (rm == 1);
	
	rm = READ_IF_EXISTS( pSettings, r_u8, section, "cam_return_stop", 0 );
	cam_recoil.camStopReturn = (rm == 1);

	float temp_f = 0.0f;
	temp_f					= pSettings->r_float( section,"cam_relax_speed" );
	cam_recoil.camRelaxSpeed = _abs(deg2rad(temp_f));
	VERIFY(!fis_zero(cam_recoil.camRelaxSpeed));
	if (fis_zero(cam_recoil.camRelaxSpeed))
	{
		cam_recoil.camRelaxSpeed = EPS_L;
	}

	cam_recoil.camRelaxSpeed_AI = cam_recoil.camRelaxSpeed;
	if ( pSettings->line_exist( section, "cam_relax_speed_ai" ) )
	{
		temp_f						= pSettings->r_float( section, "cam_relax_speed_ai" );
		cam_recoil.camRelaxSpeed_AI = _abs(deg2rad(temp_f));
		VERIFY(!fis_zero(cam_recoil.camRelaxSpeed_AI));
		if (fis_zero(cam_recoil.camRelaxSpeed_AI))
		{
			cam_recoil.camRelaxSpeed_AI = EPS_L;
		}
	}
	temp_f						= pSettings->r_float( section, "cam_max_angle" );
	cam_recoil.camMaxAngleVert = _abs(deg2rad(temp_f));
	VERIFY(!fis_zero(cam_recoil.camMaxAngleVert));
	if (fis_zero(cam_recoil.camMaxAngleVert))
	{
		cam_recoil.camMaxAngleVert = EPS;
	}
	
	temp_f						= pSettings->r_float( section, "cam_max_angle_horz" );
	cam_recoil.camMaxAngleHorz = _abs(deg2rad(temp_f));
	VERIFY(!fis_zero(cam_recoil.camMaxAngleHorz));
	if (fis_zero(cam_recoil.camMaxAngleHorz))
	{
		cam_recoil.camMaxAngleHorz = EPS;
	}
	
	temp_f						= pSettings->r_float( section, "cam_step_angle_horz" );
	cam_recoil.camStepAngleHorz = deg2rad(temp_f);
	
	cam_recoil.camDispersionFrac = _abs(READ_IF_EXISTS(pSettings, r_float, section, "cam_dispersion_frac", 0.7f));

	//подбрасывание камеры во время отдачи в режиме zoom ==> ironsight or scope
	//zoom_cam_recoil.Clone( cam_recoil ); ==== нельзя !!!!!!!!!!
	zoom_cam_recoil.camRelaxSpeed = cam_recoil.camRelaxSpeed;
	zoom_cam_recoil.camRelaxSpeed_AI = cam_recoil.camRelaxSpeed_AI;
	zoom_cam_recoil.camDispersionFrac = cam_recoil.camDispersionFrac;
	zoom_cam_recoil.camMaxAngleVert = cam_recoil.camMaxAngleVert;
	zoom_cam_recoil.camMaxAngleHorz = cam_recoil.camMaxAngleHorz;
	zoom_cam_recoil.camStepAngleHorz = cam_recoil.camStepAngleHorz;

	zoom_cam_recoil.camReturnMode = cam_recoil.camReturnMode;
	zoom_cam_recoil.camStopReturn = cam_recoil.camStopReturn;

	
	if ( pSettings->line_exist( section, "zoom_cam_relax_speed" ) )
	{
		zoom_cam_recoil.camRelaxSpeed = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed")));
		VERIFY(!fis_zero(zoom_cam_recoil.camRelaxSpeed));
		if (fis_zero(zoom_cam_recoil.camRelaxSpeed))
		{
			zoom_cam_recoil.camRelaxSpeed = EPS_L;
		}
	}
	if ( pSettings->line_exist( section, "zoom_cam_relax_speed_ai" ) )
	{
		zoom_cam_recoil.camRelaxSpeed_AI = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed_ai")));
		VERIFY(!fis_zero(zoom_cam_recoil.camRelaxSpeed_AI));
		if (fis_zero(zoom_cam_recoil.camRelaxSpeed_AI))
		{
			zoom_cam_recoil.camRelaxSpeed_AI = EPS_L;
		}
	}
	if ( pSettings->line_exist( section, "zoom_cam_max_angle" ) )
	{
		zoom_cam_recoil.camMaxAngleVert = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle")));
		VERIFY(!fis_zero(zoom_cam_recoil.camMaxAngleVert));
		if (fis_zero(zoom_cam_recoil.camMaxAngleVert))
		{
			zoom_cam_recoil.camMaxAngleVert = EPS;
		}
	}
	if ( pSettings->line_exist( section, "zoom_cam_max_angle_horz" ) )
	{
		zoom_cam_recoil.camMaxAngleHorz = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle_horz")));
		VERIFY(!fis_zero(zoom_cam_recoil.camMaxAngleHorz));
		if (fis_zero(zoom_cam_recoil.camMaxAngleHorz))
		{
			zoom_cam_recoil.camMaxAngleHorz = EPS;
		}
	}
	if ( pSettings->line_exist( section, "zoom_cam_step_angle_horz" ) )	{
		zoom_cam_recoil.camStepAngleHorz = deg2rad(pSettings->r_float(section, "zoom_cam_step_angle_horz"));
	}
	if ( pSettings->line_exist( section, "zoom_cam_dispersion_frac" ) )	{
		zoom_cam_recoil.camDispersionFrac = _abs(pSettings->r_float(section, "zoom_cam_dispersion_frac"));
	}

	m_pdm.m_fPDM_disp_base			= pSettings->r_float( section, "PDM_disp_base"			);
	m_pdm.m_fPDM_disp_vel_factor	= pSettings->r_float( section, "PDM_disp_vel_factor"	);
	m_pdm.m_fPDM_disp_accel_factor	= pSettings->r_float( section, "PDM_disp_accel_factor"	);
	m_pdm.m_fPDM_disp_crouch		= READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch", 1.0f); //pSettings->r_float( section, "PDM_disp_crouch"		);
	m_pdm.m_fPDM_disp_crouch_no_acc = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch_no_acc", 1.0f); //pSettings->r_float( section, "PDM_disp_crouch_no_acc" );
	//  [8/2/2005]

	m_fZoomInertCoef				= READ_IF_EXISTS(pSettings, r_float, section, "zoom_inertion_factor", 1.0f);

	fireDispersionConditionFactor = pSettings->r_float(section,"fire_dispersion_condition_factor"); 
// modified by Peacemaker [17.10.08]
//	misfireProbability			  = pSettings->r_float(section,"misfire_probability"); 
//	misfireConditionK			  = READ_IF_EXISTS(pSettings, r_float, section, "misfire_condition_k",	1.0f);
	misfireStartCondition			= pSettings->r_float(section, "misfire_start_condition");
	misfireEndCondition				= READ_IF_EXISTS(pSettings, r_float, section, "misfire_end_condition", 0.f);
	misfireStartProbability			= READ_IF_EXISTS(pSettings, r_float, section, "misfire_start_prob", 0.f);
	misfireEndProbability			= pSettings->r_float(section, "misfire_end_prob");
	conditionDecreasePerShot		= pSettings->r_float(section,"condition_shot_dec");  
	conditionDecreasePerQueueShot	= READ_IF_EXISTS(pSettings, r_float, section, "condition_queue_shot_dec", conditionDecreasePerShot); 


	vLoadedFirePoint	= pSettings->r_fvector3		(section,"fire_point"		);
	
	if(pSettings->line_exist(section,"fire_point2")) 
		vLoadedFirePoint2= pSettings->r_fvector3	(section,"fire_point2");
	else 
		vLoadedFirePoint2= vLoadedFirePoint;

	// hands
	eHandDependence		= EHandDependence(pSettings->r_s32(section,"hand_dependence"));
	m_bIsSingleHanded	= true;
	if (pSettings->line_exist(section, "single_handed"))
		m_bIsSingleHanded	= !!pSettings->r_bool(section, "single_handed");
	// 
	m_fMinRadius		= pSettings->r_float		(section,"min_radius");
	m_fMaxRadius		= pSettings->r_float		(section,"max_radius");


	// информация о возможных апгрейдах и их визуализации в инвентаре
	m_eScopeStatus			 = (ALife::EWeaponAddonStatus)pSettings->r_s32(section,"scope_status");
	m_eSilencerStatus		 = (ALife::EWeaponAddonStatus)pSettings->r_s32(section,"silencer_status");
	m_eGrenadeLauncherStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section,"grenade_launcher_status");

	m_zoom_params.m_bZoomEnabled		= !!pSettings->r_bool(section,"zoom_enabled");
	m_zoom_params.m_fZoomRotateTime		= pSettings->r_float(section,"zoom_rotate_time");
	m_zoom_params.m_fIronSightZoomFactor	= READ_IF_EXISTS(pSettings, r_float, section, "ironsight_zoom_factor", 50.0f);

	m_bScopeForceIcon			= !!READ_IF_EXISTS(pSettings, r_bool, section, "scope_force_icon", FALSE);
	m_bSilencerForceIcon		= !!READ_IF_EXISTS(pSettings, r_bool, section, "silencer_force_icon", FALSE);
	m_bGrenadeLauncherForceIcon = !!READ_IF_EXISTS(pSettings, r_bool, section, "grenade_launcher_force_icon", FALSE);

	secondVPWorldHudFOVK		= READ_IF_EXISTS(pSettings, r_float, section, "svp_world_hud_fov_k", 1.f);

//	m_bScopeMultySystem = !!READ_IF_EXISTS(pSettings, r_bool, section, "scope_multy_system", FALSE);

	if (pSettings->line_exist(section, "scopes_sect"))		
	{
		LPCSTR str = pSettings->r_string(section, "scopes_sect");
		for(int i = 0, count = _GetItemCount(str); i < count; ++i )	
		{
			string128						scope_section;
			_GetItem						(str, i, scope_section);
			m_scopes.push_back				(scope_section);
		}
	}
	else if (pSettings->line_exist(section, "scope_name"))
	{
		m_scopes.push_back(section);
	}

	if (m_eScopeStatus == ALife::eAddonPermanent)
	{
		shared_str scope_tex_name			= pSettings->r_string(section, "scope_texture");
		m_zoom_params.m_fScopeZoomFactor	= pSettings->r_float(section, "scope_zoom_factor");

		m_zoom_params.m_bUseDynamicZoom			= READ_IF_EXISTS(pSettings, r_bool, section, "scope_dynamic_zoom", FALSE);

		m_zoom_params.m_sUseZoomPostprocess		= READ_IF_EXISTS(pSettings, r_string, section, "scope_nightvision", 0);
		m_zoom_params.m_sUseBinocularVision		= READ_IF_EXISTS(pSettings, r_string, section, "scope_alive_detector", 0);

		m_zoom_params.m_fSecondVPZoomFactor		= READ_IF_EXISTS(pSettings, r_float, section, "svp_zoom_factor", 0.0f);
		m_zoom_params.m_fSecondVPWorldFOV		= READ_IF_EXISTS(pSettings, r_float, section, "svp_world_fov", 100.0f);

		scopeAttachBone							= READ_IF_EXISTS(pSettings, r_string, section, "scope_attach_bone", "wpn_scope");
		fakeAttachScopeBone						= READ_IF_EXISTS(pSettings, r_string, section, "fake_skeleton_bone", "wpn_body");

		m_UIScope				= xr_new <CUIWindow>();
		if(!pWpnScopeXml)
		{
			pWpnScopeXml			= xr_new <CUIXml>();
			pWpnScopeXml->Load		(CONFIG_PATH, UI_PATH, "scopes.xml");
		}
		CUIXmlInit::InitWindow	(*pWpnScopeXml, scope_tex_name.c_str(), 0, m_UIScope);

	}

	m_sSilencerName = READ_IF_EXISTS(pSettings, r_string, section, "silencer_name", "");
	m_iSilencerX = READ_IF_EXISTS(pSettings, r_s32, section, "silencer_x", 0);
	m_iSilencerY = READ_IF_EXISTS(pSettings, r_s32, section, "silencer_y", 0);
	if (m_eSilencerStatus == ALife::eAddonAttachable || m_bSilencerForceIcon)
	{
		R_ASSERT(m_sSilencerName.size() > 0);
	}
	
	m_sGrenadeLauncherName = READ_IF_EXISTS(pSettings, r_string, section, "grenade_launcher_name", "");
	m_iGrenadeLauncherX = READ_IF_EXISTS(pSettings, r_s32, section,"grenade_launcher_x", 0);
	m_iGrenadeLauncherY = READ_IF_EXISTS(pSettings, r_s32, section,"grenade_launcher_y", 0);
	if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable || m_bGrenadeLauncherForceIcon)
	{
		R_ASSERT(m_sGrenadeLauncherName.size() > 0);
	}

	InitAddons();

	//////////////////////////////////////
	if(pSettings->line_exist(section,"auto_spawn_ammo"))
		m_bAutoSpawnAmmo = pSettings->r_bool(section,"auto_spawn_ammo");
	else
		m_bAutoSpawnAmmo = TRUE;
	//////////////////////////////////////


	m_zoom_params.m_bHideCrosshairInZoom		= true;

	if(pSettings->line_exist(hud_sect, "zoom_hide_crosshair"))
		m_zoom_params.m_bHideCrosshairInZoom = !!pSettings->r_bool(hud_sect, "zoom_hide_crosshair");	

	Fvector			def_dof;
	def_dof.set		(-1,-1,-1);
//	m_zoom_params.m_ZoomDof		= READ_IF_EXISTS(pSettings, r_fvector3, section, "zoom_dof", Fvector().set(-1,-1,-1));
//	m_zoom_params.m_bZoomDofEnabled	= !def_dof.similar(m_zoom_params.m_ZoomDof);

	m_bHasTracers			= !!READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
	m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", u8(-1));

	string256						temp;
	for (int i=egdNovice; i<egdCount; ++i) 
	{
		strconcat					(sizeof(temp),temp,"hit_probability_",get_token_name(difficulty_type_token,i));
		m_hit_probability[i]		= READ_IF_EXISTS(pSettings,r_float,section,temp,1.f);
	}

	m_ai_weapon_rank = READ_IF_EXISTS(pSettings, r_u32, section, "weapon_rank", (u32)-1);
	if (m_ai_weapon_rank == (u32)-1)
	{
		Msg("weapon_rank not set for weapon %s! Using default = 0.", section);
		m_ai_weapon_rank = 0;
	}

	if ((m_eSilencerStatus == ALife::eAddonPermanent
		|| m_eGrenadeLauncherStatus == ALife::eAddonPermanent
		|| m_eScopeStatus == ALife::eAddonPermanent))
	{
		InitAddons();
	}

	aiAproximateEffectiveDistanceBase_ = READ_IF_EXISTS(pSettings, r_float, section, "ai_effective_dist_base", 50.f);
}
 
void CWeapon::LoadFireParams		(LPCSTR section)
{
	cam_recoil.camDispersion = deg2rad( pSettings->r_float( section,"cam_dispersion" ) ); 
	cam_recoil.camDispersionInc = 0.0f;

	if ( pSettings->line_exist( section, "cam_dispersion_inc" ) )	{
		cam_recoil.camDispersionInc = deg2rad( pSettings->r_float( section, "cam_dispersion_inc" ) ); 
	}
	
	zoom_cam_recoil.camDispersion		= cam_recoil.camDispersion;
	zoom_cam_recoil.camDispersionInc	= cam_recoil.camDispersionInc;

	if ( pSettings->line_exist( section, "zoom_cam_dispersion" ) )	{
		zoom_cam_recoil.camDispersion		= deg2rad( pSettings->r_float( section, "zoom_cam_dispersion" ) ); 
	}
	if ( pSettings->line_exist( section, "zoom_cam_dispersion_inc" ) )	{
		zoom_cam_recoil.camDispersionInc	= deg2rad( pSettings->r_float( section, "zoom_cam_dispersion_inc" ) ); 
	}

	CShootingObject::LoadFireParams(section);
};

BOOL CWeapon::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	BOOL bResult					= inherited::SpawnAndImportSOData(data_containing_so);
	CSE_Abstract					*e	= (CSE_Abstract*)(data_containing_so);
	CSE_ALifeItemWeapon			    *se_weapon = smart_cast<CSE_ALifeItemWeapon*>(e);

	//iAmmoCurrent					= E->a_current;
	iAmmoElapsed					= se_weapon->a_elapsed;
	m_flagsAddOnState				= se_weapon->m_addon_flags.get();
	m_cur_scope						= se_weapon->m_cur_scope;
	SetState						(se_weapon->wpn_state);
	SetNextState					(se_weapon->wpn_state);

	R_ASSERT2(m_cur_scope <= m_scopes.size(), make_string("m_scopes.size = %u, m_cur_scope = %u", m_scopes.size(), m_cur_scope));

	ImportSOMagazine(se_weapon, m_magazine);

	UpdateAddonsVisibility();
	InitAddons();

	m_bAmmoWasSpawned		= false;

	return bResult;
}

void CWeapon::ImportSOMagazine(CSE_ALifeItemWeapon* se_weapon, xr_vector<CCartridge>& mag_to_pack)
{
	// load data exported to server in ::ExportDataToServer
	mag_to_pack.clear();

	if (!se_weapon->m_MagazinePacked.size()) // if first load - spawn what ever is specified in spawn file
	{
		m_ammoType = se_weapon->ammo_type;

		if (m_ammoType >= m_ammoTypes.size())
		{
			Msg("!m_ammoType index %u is out of ammo indexes (should be less than %d); Weapon section is [%s]. Probably default ammo upgrade is the reason", m_ammoType, m_ammoTypes.size(), SectionNameStr());
			m_ammoType = 0;
		}

		m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);

		while (mag_to_pack.size() < iAmmoElapsed)
		{
			CCartridge l_cartridge = m_DefaultCartridge;
			l_cartridge.m_LocalAmmoType = m_ammoType;
			mag_to_pack.push_back(l_cartridge);
		}
	}
	else
	{
		R_ASSERT(magazinePacked.size() % 2 == 0);

		for (u32 i = 0; i < se_weapon->m_MagazinePacked.size(); i += 2)
		{
			u8 ammo_type = se_weapon->m_MagazinePacked[i];
			u16 ammo_cnt = se_weapon->m_MagazinePacked[i + 1];

			if (ammo_type >= m_ammoTypes.size())
			{
				Msg("!m_ammoType index %u is out of ammo indexes (should be less than %d); Weapon section is [%s]. Probably default ammo upgrade is the reason", ammo_type, m_ammoTypes.size(), SectionNameStr());
				ammo_type = 0;
			}

			m_DefaultCartridge.Load(m_ammoTypes[ammo_type].c_str(), ammo_type);

			for (u16 j = 0; j < ammo_cnt; j++)
			{
				CCartridge l_cartridge = m_DefaultCartridge;

				l_cartridge.m_LocalAmmoType = ammo_type;
				mag_to_pack.push_back(l_cartridge);
			}
		}
	}

	if (mag_to_pack.size())
	{
		m_fCurrentCartirdgeDisp = m_DefaultCartridge.param_s.kDisp;
		m_ammoType = mag_to_pack.back().m_LocalAmmoType;
	}

	R_ASSERT((u32)iAmmoElapsed == mag_to_pack.size());
}

void CWeapon::DestroyClientObj()
{
	inherited::DestroyClientObj();

	//удалить объекты партиклов
	StopFlameParticles	();
	StopFlameParticles2	();
	StopLight			();
	Light_Destroy		();

	while (m_magazine.size()) m_magazine.pop_back();
}

BOOL CWeapon::IsUpdating()
{	
	bool bIsActiveItem = m_pCurrentInventory && m_pCurrentInventory->ActiveItem()==this;
	return bIsActiveItem || bWorking || IsPending() || getVisible();
}

void CWeapon::ExportDataToServer(NET_Packet& P)
{
	inherited::ExportDataToServer(P);

	u8 need_upd				= IsUpdating() ? 1 : 0;
	P.w_u8					(need_upd);
	P.w_u16					(u16(iAmmoElapsed));
	P.w_u8					(m_cur_scope);
	P.w_u8					(m_flagsAddOnState);
	P.w_u8					(m_ammoType);

	PackAndExportMagazine(P, m_magazine);

	u8 state = (GetState() == eMisfire || GetState() == eMagEmpty) ? (u8)GetState() : (u8)eHidden; // save only same states
	P.w_u8					(state);
	P.w_u8					((u8)IsZoomed());
}

void CWeapon::PackAndExportMagazine(NET_Packet& P, xr_vector<CCartridge>& mag_to_pack)
{
	magazinePacked.clear();

	u8 temp_ammo_type = u8(-1);
	u16 cnt = 0;
	// Write like this: type 1, cnt 5; type 2, cnt 12; type 1, cnt 8; etc. Suports unordered ammo queue (ex AA BBBB AAAA BB...)
	for (u32 i = 0; i < mag_to_pack.size(); i++)
	{
		CCartridge& l_cartridge = *(mag_to_pack.begin() + i);

		if (temp_ammo_type != l_cartridge.m_LocalAmmoType) // Note: first itteration - we write first ammo queue type
		{
			if (i != 0) // if first ammo - dont write cnt yet
				magazinePacked.push_back(cnt); // Ammo queue type changed - save its cnt

			magazinePacked.push_back(l_cartridge.m_LocalAmmoType); // Write new ammo type queue

			temp_ammo_type = l_cartridge.m_LocalAmmoType;
			cnt = 0;
		}

		cnt++;

		if (i == mag_to_pack.size() - 1) // finish writing - write the cnt of last ammo type
			magazinePacked.push_back(cnt);
	}

	VERIFY(magazinePacked.size() % 2 == 0);

	P.w_u16(magazinePacked.size());

	for (u32 i = 0; i < magazinePacked.size(); i += 2)
	{
		P.w_u8((u8)magazinePacked[i]); // write queue type
		P.w_u16(magazinePacked[i + 1]); // write queue cnt
	}
}

void CWeapon::RemoveLinksToCLObj(CObject* object)
{
	inherited::RemoveLinksToCLObj(object);
	if (m_zoom_params.scopeAliveDetectorVision_)
		m_zoom_params.scopeAliveDetectorVision_->remove_links(object);
}

void CWeapon::save(NET_Packet &output_packet)
{
	inherited::save	(output_packet);

	save_data		(m_zoom_params.m_bIsZoomModeNow,output_packet);
	//save_data		(m_bRememberActorNVisnStatus,	output_packet);
	save_data		(m_fRTZoomFactor, output_packet);
	save_data		(m_fSVP_RTZoomFactor, output_packet);

	// keep scope enchantment devices "On" and save it, if player did not disable them before zoom out
	save_data		(m_zoom_params.reenableNVOnZoomIn_, output_packet);
	save_data		(m_zoom_params.reenableALDetectorOnZoomIn_, output_packet);
}

void CWeapon::load(IReader &input_packet)
{
	inherited::load	(input_packet);

	load_data		(m_zoom_params.m_bIsZoomModeNow,input_packet);

	if (m_zoom_params.m_bIsZoomModeNow)	
		OnZoomIn();
	else			
		OnZoomOut();

	//load_data		(m_bRememberActorNVisnStatus,	input_packet);
	load_data(m_fRTZoomFactor, input_packet);
	load_data(m_fSVP_RTZoomFactor, input_packet);

	load_data(m_zoom_params.reenableNVOnZoomIn_, input_packet);
	load_data(m_zoom_params.reenableALDetectorOnZoomIn_, input_packet);

	UpdateAddonsVisibility();
}


void CWeapon::OnEvent(NET_Packet& P, u16 type) 
{
	inherited::OnEvent(P,type);
};

void CWeapon::ScheduledUpdate(u32 dT)
{
	// Queue shrink
//	u32	dwTimeCL		= Level().timeServer()-NET_Latency;
//	while ((NET.size()>2) && (NET[1].dwTimeStamp<dwTimeCL)) NET.pop_front();	

	// Inherited
	inherited::ScheduledUpdate(dT);
}

void CWeapon::BeforeDetachFromParent(bool just_before_destroy)
{
	RemoveShotEffector			();

	inherited::BeforeDetachFromParent(just_before_destroy);

	FireEnd						();
	SetPending					(FALSE);
	SwitchState					(eHidden);

	m_strapped_mode				= false;
	m_strapped_mode_rifle			= false;
	m_zoom_params.m_bIsZoomModeNow	= false;
	UpdateXForm					();

}

void CWeapon::AfterDetachFromParent()
{
	inherited::AfterDetachFromParent();
	Light_Destroy				();
	UpdateAddonsVisibility		();
};

void CWeapon::AfterAttachToParent()
{
	inherited::AfterAttachToParent();

	UpdateAddonsVisibility		();
};

void CWeapon::OnActiveItem ()
{
	UpdateAddonsVisibility();
	if (m_bDraw_off == true) {
		SwitchState(eShowing);
	}
	m_BriefInfo_CalcFrame = 0;
	inherited::OnActiveItem		();
	//если мы занружаемся и оружие было в руках
//	SetState					(eIdle);
//	SetNextState				(eIdle);
}

void CWeapon::OnHiddenItem ()
{

	if (m_bHolster_off == true) {
		SwitchState(eHiding);
	}

	OnZoomOut();
	inherited::OnHiddenItem		();

	SetAmmoOnReload(u8(-1));
	
	m_BriefInfo_CalcFrame = 0;
}

void CWeapon::SendHiddenItem()
{
	if (!CHudItem::object().getDestroy() && m_pCurrentInventory)
	{
		SetPending(TRUE);

		if (m_changed_ammoType_on_reload == u8(-1))
			SetAmmoOnReload(u8(-1));
		else
			SetAmmoOnReload(m_changed_ammoType_on_reload);

		OnStateSwitch(eHiding);
	}
}

void CWeapon::BeforeAttachToParent()
{
	inherited::BeforeAttachToParent();

	OnZoomOut					();
	SetAmmoOnReload(u8(-1));
}

bool CWeapon::AllowBore()
{
	return true;
}

extern int hud_adj_mode;

void CWeapon::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif
	

	inherited::UpdateCL();
	UpdateHUDAddonsVisibility();
	//подсветка от выстрела

	UpdateLight();

	//нарисовать партиклы
	UpdateFlameParticles();
	UpdateFlameParticles2();
	
	if( (GetNextState()==GetState()) && H_Parent()==Level().CurrentEntity())
	{
		CActor* pActor = smart_cast<CActor*>(H_Parent());

		if(pActor && this == pActor->inventory().ActiveItem())
		{
			CEntity::SEntityState st;
			pActor->g_State(st);

			if (!st.bSprint && hud_adj_mode == 0 &&	GetState() == eIdle && (EngineTimeU() - m_dw_curr_substate_time > 20000) && !IsZoomed() && g_player_hud->attached_item(1) == NULL)
			{
				if(AllowBore())
					SwitchState(eBore);

				ResetSubStateTime();
			}
		}
	}

	if (m_zoom_params.scopeAliveDetectorVision_)
		m_zoom_params.scopeAliveDetectorVision_->Update();

	CActor* pA = smart_cast<CActor*>(H_Parent());

	if (!pA)
	{
		#ifdef MEASURE_UPDATES
		Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
		#endif

		return;
	}

	HandleScopeSwitchings();
	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CWeapon::HandleScopeSwitchings()
{
	if ((m_zoom_params.reenableNVOnZoomIn_ || m_zoom_params.reenableALDetectorOnZoomIn_) && CanUseScopeNV())
	{
		if (m_zoom_params.reenableNVOnZoomIn_)
		{
			if (m_zoom_params.m_sUseZoomPostprocess.size() && IsScopeAttached())
			{
				if (!m_zoom_params.scopeNightVision_)
					m_zoom_params.scopeNightVision_ = xr_new<CNightVisionEffector>(*m_zoom_params.m_sUseZoomPostprocess);

				BOOL use_shader = READ_IF_EXISTS(pSettings, r_bool, *m_zoom_params.m_sUseZoomPostprocess, "nv_use_shader_instead_off_ppe", FALSE);

				if (use_shader)
				{
					LPCSTR shader_sect = pSettings->r_string(*m_zoom_params.m_sUseZoomPostprocess, "nightvision_sect_shader");
					m_zoom_params.scopeNightVision_->StartShader(shader_sect, Actor(), false); // no turn on sound
				}
				else
					m_zoom_params.scopeNightVision_->StartPPE(m_zoom_params.m_sUseZoomPostprocess, Actor(), false); // no turn on sound
			}

			m_zoom_params.reenableNVOnZoomIn_ = false;
		}

		if (m_zoom_params.reenableALDetectorOnZoomIn_)
		{
			m_scope_adetector_enabled = true;

			if (!m_zoom_params.scopeAliveDetectorVision_)
					m_zoom_params.scopeAliveDetectorVision_ = xr_new <CBinocularsVision>(m_zoom_params.m_sUseBinocularVision);

			m_zoom_params.reenableALDetectorOnZoomIn_ = false;
		}
	}

	if ((m_zoom_params.reenableActorNVOnZoomOut_ || m_zoom_params.reenableActorALDetectorOnZoomOut_) && !IsZoomed() && IsRotatingToZoom())
	{
		if (m_zoom_params.reenableActorNVOnZoomOut_)
		{
			if (Actor()->GetActorNVHandler())
				Actor()->GetActorNVHandler()->SwitchNightVision(true, false);

			m_zoom_params.reenableActorNVOnZoomOut_ = false;
		}

		if (m_zoom_params.reenableActorALDetectorOnZoomOut_)
		{
			Actor()->SwitchActorAliveDetector(true);

			m_zoom_params.reenableActorALDetectorOnZoomOut_ = false;
		}
	}
}

void CWeapon::SwitchNightVision() 
{ 
	if (m_zoom_params.scopeNightVision_)
		SwitchNightVision(!m_zoom_params.scopeNightVision_->IsActive());
}

void CWeapon::SwitchNightVision(bool state)
{
	if ((state && !CanUseScopeNV()) ||
		!m_zoom_params.scopeNightVision_ ||
		!m_zoom_params.scopeNightVision_->HasSounds())	// 'HasSounds' detects if it's NV -- true then -- or a contrast effect -- false. 
	{
		return;
	}

	int result = -1;

	bool is_active_now = m_zoom_params.scopeNightVision_->IsActive();

	if (state == true && !is_active_now)
	{
		BOOL use_shader = READ_IF_EXISTS(pSettings, r_bool, *m_zoom_params.m_sUseZoomPostprocess, "nv_use_shader_instead_off_ppe", FALSE);

		if (use_shader)
		{
			LPCSTR shader_sect = pSettings->r_string(*m_zoom_params.m_sUseZoomPostprocess, "nightvision_sect_shader");
			m_zoom_params.scopeNightVision_->StartShader(shader_sect, Actor());
		}
		else
			m_zoom_params.scopeNightVision_->StartPPE(m_zoom_params.m_sUseZoomPostprocess, Actor());

		result = 1;
	}
	else
	{
		if (is_active_now)
		{
			m_zoom_params.scopeNightVision_->StopPPE(100000.0f);
			m_zoom_params.scopeNightVision_->StopShader();

			result = 0;
		}
	}

	R_ASSERT(result != -1);
}

bool CWeapon::GetNightVisionStatus()
{
	bool is_active_now = m_zoom_params.scopeNightVision_ ? m_zoom_params.scopeNightVision_->IsActive() : false;

	return is_active_now;
}

bool CWeapon::need_renderable()
{
	return !(IsZoomed() && ZoomTexture() && !IsRotatingToZoom());
}

bool CWeapon::CanUseScopeNV()
{
	return IsZoomed() && !IsRotatingToZoom();
}

bool CWeapon::IsScopeNvInstalled() const
{
	return IsScopeAttached() && m_zoom_params.m_sUseZoomPostprocess.size() &&
		   !xr_strcmp(m_zoom_params.m_sUseZoomPostprocess, "scope_nightvision");
}

void CWeapon::SwitchAliveDetector(bool state)
{
	if (!m_zoom_params.m_sUseBinocularVision.size() || !IsScopeAttached() ||
		(state && !CanUseAliveDetector()))
	{
		return;
	}

	m_scope_adetector_enabled = state;

	if (m_scope_adetector_enabled)
	{
		if (!m_zoom_params.scopeAliveDetectorVision_)
			m_zoom_params.scopeAliveDetectorVision_ = xr_new <CBinocularsVision>(m_zoom_params.m_sUseBinocularVision);
	}
	else
	{
		xr_delete(m_zoom_params.scopeAliveDetectorVision_);
	}
}

bool CWeapon::IsScopeAliveDetectorInstalled() const
{
	return IsScopeAttached() && m_zoom_params.m_sUseBinocularVision.size();
}

bool CWeapon::CanUseAliveDetector()
{
	return !need_renderable();
}

void CWeapon::signal_HideComplete()
{
	if(H_Parent())
		setVisible(FALSE);

	SetPending				(FALSE);
}

void CWeapon::SetDefaults()
{
	bWorking2			= false;
	SetPending			(FALSE);

	m_flags.set			(FUsingCondition, TRUE);
	bMisfire			= false;
	m_flagsAddOnState	= 0;
	m_zoom_params.m_bIsZoomModeNow	= false;
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
	Position().set		(trans.c);
	if (m_strapped_mode || m_strapped_mode_rifle)
		XFORM().mul			(trans,m_StrapOffset);
	else
		XFORM().mul			(trans,m_Offset);
	VERIFY				(!fis_zero(DET(renderable.xform)));
}


bool CWeapon::Action(u16 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;

	
	switch(cmd) 
	{
		case kWPN_FIRE: 
			{
				//если оружие чем-то занято, то ничего не делать
				{				
					if(IsPending())		
						return				false;

					if(flags&CMD_START) 
						FireStart			();
					else 
						FireEnd				();
				};

			} 
			return true;
		case kWPN_NEXT: 
			{
				if(IsPending()) 
				{
					return false;
				}
									
				if(flags&CMD_START) 
				{
					SwitchAmmoType();
				}
			} 
            return true;

		case kWPN_ZOOM:
			if(IsZoomEnabled())
			{
				if(flags&CMD_START && !IsPending())
					OnZoomIn();
				else if(IsZoomed())
					OnZoomOut();
				return true;
			}else 
				return false;

		case kWPN_ZOOM_INC:
		case kWPN_ZOOM_DEC:
			if(IsZoomEnabled() && IsZoomed())
			{
				if(cmd==kWPN_ZOOM_INC)
					ZoomInc();
				else
					ZoomDec();
				return true;
			}else
				return false;
	}
	return false;
}

bool CWeapon::SwitchAmmoType() 
{
	u8 l_newType = m_ammoType;
	bool b1, b2;
	do
	{
		l_newType = (l_newType + 1) % m_ammoTypes.size();
		b1 = l_newType != m_ammoType;
		b2 = unlimited_ammo() ? false : (!m_pCurrentInventory->GetAmmoFromInv(*m_ammoTypes[l_newType]));
	} while (b1 && b2);

	if (l_newType != m_ammoType)
	{
		SetAmmoOnReload(l_newType);
		/*						m_ammoType = l_newType;
								m_pAmmo = NULL;
								if (unlimited_ammo())
								{
									m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));
								};
		*/
		UnloadMagazine();
		Reload();
	}

	return true;
}

void CWeapon::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, u32 ParentID) 
{
	if(!m_ammoTypes.size())			return;
	m_bAmmoWasSpawned				= true;
	
	int l_type						= 0;
	l_type							%= m_ammoTypes.size();

	if(!ammoSect) ammoSect			= *m_ammoTypes[l_type]; 
	
	++l_type; 
	l_type							%= m_ammoTypes.size();

	CSE_Abstract *D					= F_entity_Create(ammoSect);

	{	
		CSE_ALifeItemAmmo *l_pA		= smart_cast<CSE_ALifeItemAmmo*>(D);
		R_ASSERT					(l_pA);
		l_pA->m_boxSize				= (u16)pSettings->r_s32(ammoSect, "box_size");
		D->s_name					= ammoSect;
		D->set_name_replace			("");
		D->s_RP						= 0xff;
		D->ID						= 0xffff;
		if (ParentID == 0xffffffff)	
			D->ID_Parent			= (u16)H_Parent()->ID();
		else
			D->ID_Parent			= (u16)ParentID;

		D->ID_Phantom				= 0xffff;
		D->s_flags.assign			(M_SPAWN_OBJECT_LOCAL);
		D->RespawnTime				= 0;
		l_pA->m_tNodeID				= ai_location().level_vertex_id();

		if(boxCurr == 0xffffffff) 	
			boxCurr					= l_pA->m_boxSize;

		while(boxCurr) 
		{
			l_pA->a_elapsed			= (u16)(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);
			NET_Packet				P;
			D->Spawn_Write			(P, TRUE);
			Level().Send			(P,net_flags(TRUE));

			if(boxCurr > l_pA->m_boxSize) 
				boxCurr				-= l_pA->m_boxSize;
			else 
				boxCurr				= 0;
		}
	};
	F_entity_Destroy				(D);
}

int CWeapon::GetSuitableAmmoTotal(bool use_item_to_spawn) const
{
	int l_count = iAmmoElapsed;
	if(!m_pCurrentInventory) return l_count;

	//чтоб не делать лишних пересчетов
	if(m_pCurrentInventory->ModifyFrame()<= m_BriefInfo_CalcFrame)
		return l_count + m_iAmmoCurrentTotal;

	m_BriefInfo_CalcFrame = CurrentFrame();
	m_iAmmoCurrentTotal = 0;

	for ( u8 i = 0; i < u8(m_ammoTypes.size()); ++i ) 
	{
		LPCSTR l_ammoType = *m_ammoTypes[i];

		for (TIItemContainer::iterator l_it = m_pCurrentInventory->belt_.begin(); m_pCurrentInventory->belt_.end() != l_it; ++l_it)
		{
			CWeaponAmmo *l_pAmmo = smart_cast<CWeaponAmmo*>(*l_it);

			if(l_pAmmo && !xr_strcmp(l_pAmmo->SectionName(), l_ammoType)) 
			{
				m_iAmmoCurrentTotal = m_iAmmoCurrentTotal + l_pAmmo->m_boxCurr;
			}
		}

		if (g_actor->m_inventory != m_pCurrentInventory)
		{
			for (TIItemContainer::iterator l_it = m_pCurrentInventory->ruck_.begin(); m_pCurrentInventory->ruck_.end() != l_it; ++l_it)
			{
				CWeaponAmmo *l_pAmmo = smart_cast<CWeaponAmmo*>(*l_it);
				if(l_pAmmo && !xr_strcmp(l_pAmmo->SectionName(), l_ammoType)) 
				{
					m_iAmmoCurrentTotal = m_iAmmoCurrentTotal + l_pAmmo->m_boxCurr;
				}
			}
		}

		if (!use_item_to_spawn)
			continue;

		if (!inventory_owner().item_to_spawn())
			continue;

		m_iAmmoCurrentTotal += inventory_owner().ammo_in_box_to_spawn();
	}
	return l_count + m_iAmmoCurrentTotal;
}

int CWeapon::GetAmmoCount(u8 ammo_type) const
{
	VERIFY(m_pCurrentInventory);
	R_ASSERT(ammo_type < m_ammoTypes.size());

	return GetAmmoCount_forType(m_ammoTypes[ammo_type]);
}

int CWeapon::GetAmmoCount_forType(shared_str const& ammo_type) const
{
	int res = 0;

	TIItemContainer::iterator itb = m_pCurrentInventory->belt_.begin();
	TIItemContainer::iterator ite = m_pCurrentInventory->belt_.end();
	for (; itb != ite; ++itb)
	{
		CWeaponAmmo*	pAmmo = smart_cast<CWeaponAmmo*>(*itb);
		if (pAmmo && (pAmmo->SectionName() == ammo_type))
		{
			res += pAmmo->m_boxCurr;
		}
	}

	itb = m_pCurrentInventory->ruck_.begin();
	ite = m_pCurrentInventory->ruck_.end();
	for (; itb != ite; ++itb)
	{
		CWeaponAmmo*	pAmmo = smart_cast<CWeaponAmmo*>(*itb);
		if (pAmmo && (pAmmo->SectionName() == ammo_type))
		{
			res += pAmmo->m_boxCurr;
		}
	}
	return res;
}

float CWeapon::GetConditionMisfireProbability() const
{
// modified by Peacemaker [17.10.08]
//	if(GetCondition() > 0.95f) 
//		return 0.0f;
	if(GetCondition() > misfireStartCondition) 
		return 0.0f;
	if(GetCondition() < misfireEndCondition) 
		return misfireEndProbability;
//	float mis = misfireProbability+powf(1.f-GetCondition(), 3.f)*misfireConditionK;
	float mis = misfireStartProbability + (
		(misfireStartCondition - GetCondition()) *				// condition goes from 1.f to 0.f
		(misfireEndProbability - misfireStartProbability) /		// probability goes from 0.f to 1.f
		((misfireStartCondition == misfireEndCondition) ?		// !!!say "No" to devision by zero
			misfireStartCondition : 
			(misfireStartCondition - misfireEndCondition))
										  );
	clamp(mis,0.0f,0.99f);
	return mis;
}

BOOL CWeapon::CheckForMisfire	()
{
	float rnd = ::Random.randF(0.f,1.f);
	float mp = GetConditionMisfireProbability();
	if(rnd < mp)
	{
		FireEnd();

		bMisfire = true;
		SwitchState(eMisfire);		
		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CWeapon::IsMisfire() const
{	
	return bMisfire;
}

void CWeapon::Reload()
{
	OnZoomOut();

	if (m_pCurrentInventory == Actor()->m_inventory)
	{
		Actor()->BreakSprint();
		Actor()->trySprintCounter_ = 0;
	}
}

void CWeapon::UnloadMagazine(bool spawn_ammo, u32 into_who_id, bool unload_secondary)
{

}

bool CWeapon::IsGrenadeLauncherAttached() const
{
	return (ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)) || 
		ALife::eAddonPermanent == m_eGrenadeLauncherStatus;
}

bool CWeapon::IsScopeAttached() const
{
	return (ALife::eAddonAttachable == m_eScopeStatus &&
			0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope)) || 
		ALife::eAddonPermanent == m_eScopeStatus;

}

bool CWeapon::IsSilencerAttached() const
{
	return (ALife::eAddonAttachable == m_eSilencerStatus &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer)) || 
		ALife::eAddonPermanent == m_eSilencerStatus;
}

bool CWeapon::GrenadeLauncherAttachable()
{
	return (ALife::eAddonAttachable == m_eGrenadeLauncherStatus);
}
bool CWeapon::ScopeAttachable()
{
	return (ALife::eAddonAttachable == m_eScopeStatus);
}
bool CWeapon::SilencerAttachable()
{
	return (ALife::eAddonAttachable == m_eSilencerStatus);
}

void CWeapon::UpdateHUDAddonsVisibility()
{
	if(!GetHUDmode())
		return;
}

void CWeapon::UpdateAddonsVisibility()
{
	if (GetHUDmode())
		return;

	IKinematics* pWeaponVisual = smart_cast<IKinematics*>(Visual()); R_ASSERT(pWeaponVisual);

	UpdateHUDAddonsVisibility();	

	// Mortan: fake mesh with custom bones without animations, hehehe boys
	if (!fakeViual)
	{
		LPCSTR fake_visual_path = READ_IF_EXISTS(pSettings, r_string, SectionName(), "custom_skeleton", "");

		if (xr_strcmp(fake_visual_path, "") == 0)
			fakeViual = nullptr;
		else 
			fakeViual = ::Render->model_Create(fake_visual_path);
	}

	if(ScopeAttachable())
	{	
		if(IsScopeAttached())
		{
			if (!scopeVisual)
			{
				LPCSTR scope_visual_path = READ_IF_EXISTS(pSettings, r_string,GetScopeName(), "scope_visual", "dynamics\\weapons\\wpn_upgrade\\wpn_scope");

				scopeVisual = ::Render->model_Create(scope_visual_path);
			}
		}
		else
		{
			if (scopeVisual)
			{
				::Render->model_Delete(scopeVisual);
				scopeVisual = nullptr;
			}
		}
	}

	if(m_eScopeStatus == ALife::eAddonDisabled)
	{
		if (scopeVisual)
		{
			::Render->model_Delete(scopeVisual);
			scopeVisual = nullptr;
		}
	}

	if(SilencerAttachable())
	{
		if(IsSilencerAttached())
		{
			if (!silencerVisual)
			{
				LPCSTR silencer_visual_path = READ_IF_EXISTS(pSettings, r_string, GetSilencerName(), "silencer_visual", "dynamics\\weapons\\wpn_upgrade\\wpn_silencer");

				silencerVisual = ::Render->model_Create(silencer_visual_path);
			}
		}
		else
		{
			if (silencerVisual)
			{
				::Render->model_Delete(silencerVisual);
				silencerVisual = nullptr;
			}
		}
	}

	if(m_eSilencerStatus == ALife::eAddonDisabled)
	{
		if (silencerVisual)
		{
			::Render->model_Delete(silencerVisual);
			silencerVisual = nullptr;
		}
	}

	if(GrenadeLauncherAttachable())
	{
		if(IsGrenadeLauncherAttached())
		{
			if (!glauncherVisual)
			{
				LPCSTR gl_visual_path = READ_IF_EXISTS(pSettings, r_string, GetGrenadeLauncherName(), "gl_visual", "dynamics\\weapons\\wpn_upgrade\\wpn_grenade_launcher");

				glauncherVisual = ::Render->model_Create(gl_visual_path);
			}
		}
		else
		{
			if (glauncherVisual)
			{
				::Render->model_Delete(glauncherVisual);
				glauncherVisual = nullptr;
			}
		}
	}

	if(m_eGrenadeLauncherStatus== ALife::eAddonDisabled)
	{
		if (glauncherVisual)
		{
			::Render->model_Delete(glauncherVisual);
			glauncherVisual = nullptr;
		}
	}
}

Fmatrix GetAddonsParams(Fvector pos, Fvector rot)
{
	Fmatrix buffer;
	Fvector ypr = rot;
	ypr.mul(PI / 180.f);
	buffer.setHPB(ypr.x, ypr.y, ypr.z);
	buffer.translate_over(pos);
	return buffer;
}

extern int hud_adj_addon_idx;
extern shared_str hud_section_name;

void CWeapon::UpdateAddonsHudParams()
{
	if (HudItemData())
	{
		bNeedUpdateAddons = false;

		bool is_16x9 = UI().is_widescreen();
		string64	_prefix;
		xr_sprintf(_prefix, "%s", is_16x9 ? "_16x9" : "");
		string128	val_name;

		ResetAddonsHudParams();

		if (ScopeAttachable() && IsScopeAttached())
		{
			HudItemData()->m_measures.m_custom_addon_offset[0][0] = READ_IF_EXISTS(pSettings, r_fvector3, m_scopes[m_cur_scope], "hud_attach_pos", Fvector3().set(0, 0, 0));
			HudItemData()->m_measures.m_custom_addon_offset[0][1] = READ_IF_EXISTS(pSettings, r_fvector3, m_scopes[m_cur_scope], "hud_attach_rot", Fvector3().set(0, 0, 0));

			strconcat(sizeof(val_name), val_name, "aim_scope_hud_offset_pos", _prefix);
			HudItemData()->m_measures.m_hands_offset_scope[0][1]  = READ_IF_EXISTS(pSettings, r_fvector3,  m_scopes[m_cur_scope], val_name, HudItemData()->m_measures.m_hands_offset_ironsight[0][1]);
			strconcat(sizeof(val_name), val_name, "aim_scope_hud_offset_rot", _prefix);
			HudItemData()->m_measures.m_hands_offset_scope[1][1]  = READ_IF_EXISTS(pSettings, r_fvector3,  m_scopes[m_cur_scope], val_name, HudItemData()->m_measures.m_hands_offset_ironsight[1][1]);

		}

		if (SilencerAttachable() && IsSilencerAttached())
		{
			HudItemData()->m_measures.m_custom_addon_offset[1][0] = READ_IF_EXISTS(pSettings, r_fvector3, m_section_id, "sil_hud_attach_pos", Fvector3().set(0, 0, 0));
			HudItemData()->m_measures.m_custom_addon_offset[1][1] = READ_IF_EXISTS(pSettings, r_fvector3, m_section_id, "sil_hud_attach_rot", Fvector3().set(0, 0, 0));
		}

		if (GrenadeLauncherAttachable() && IsGrenadeLauncherAttached())
		{
			HudItemData()->m_measures.m_custom_addon_offset[2][0] = READ_IF_EXISTS(pSettings, r_fvector3, m_section_id, "gl_hud_attach_pos", Fvector3().set(0, 0, 0));
			HudItemData()->m_measures.m_custom_addon_offset[2][1] = READ_IF_EXISTS(pSettings, r_fvector3, m_section_id, "gl_hud_attach_rot", Fvector3().set(0, 0, 0));
		}
	}
}

void CWeapon::ResetAddonsHudParams()
{
	for (u8 i = 0; i < 3; i++)
	{
		HudItemData()->m_measures.m_custom_addon_offset[i][0].set(0, 0, 0);
		HudItemData()->m_measures.m_custom_addon_offset[i][1].set(0, 0, 0);
	}
}

void CWeapon::CalcAddonOffset(Fmatrix base_model_trans, IKinematics & base_model, shared_str bone_name, Fmatrix offset, Fmatrix & dest)
{
	u16 bone_id = base_model.LL_BoneID(bone_name);
	Fmatrix bone_trans = base_model.LL_GetTransform(bone_id);
	Fmatrix temp; temp.mul(base_model_trans, bone_trans);
	dest.mul(temp, offset);
}

void CWeapon::UpdateAddonsTransform(bool for_hud)
{
	Fmatrix base_model_trans = for_hud ? HudItemData()->m_item_transform : XFORM();
	IKinematics* base_model = for_hud ? HudItemData()->m_model : smart_cast<IKinematics*>(Visual());
	u8 idx = for_hud ? 0 : 1;
	bool hud_adj = hud_adj_mode == 10 || hud_adj_mode == 11;

	hud_section_name = NULL;

	if (for_hud && bNeedUpdateAddons) UpdateAddonsHudParams();

	if (ScopeAttachable() && IsScopeAttached())
	{
		Fmatrix offset = for_hud ? HudItemData()->m_measures.GetAddonsParams(0) : GetAddonsParams(scopeAttachOffset[0], scopeAttachOffset[1]);
		CalcAddonOffset(base_model_trans, *base_model, scopeAttachBone, offset, scopeAttachTransform);
		if (hud_adj && hud_adj_addon_idx == 0) hud_section_name._set(m_scopes[m_cur_scope]);
	}

	if (SilencerAttachable() && IsSilencerAttached())
	{
		Fmatrix offset = for_hud ? HudItemData()->m_measures.GetAddonsParams(1) : GetAddonsParams(silencerAttachOffset[0], silencerAttachOffset[1]);
		CalcAddonOffset(base_model_trans, *base_model, silencerAttachBone, offset, silencerAttachTransform);
		if (hud_adj && hud_adj_addon_idx == 1) hud_section_name._set(m_section_id);
	}

	if (GrenadeLauncherAttachable() && IsGrenadeLauncherAttached())
	{
		Fmatrix offset = for_hud ? HudItemData()->m_measures.GetAddonsParams(2) : GetAddonsParams(glauncherAttachOffset[0], glauncherAttachOffset[1]);
		CalcAddonOffset(base_model_trans, *base_model, glAttachBone, offset, glauncherAttachTransform);
		if (hud_adj && hud_adj_addon_idx == 2) hud_section_name._set(m_section_id);
	}
}

void CWeapon::InitAddons()
{
}

float CWeapon::CurrentZoomFactor()
{
	if(IsSecondVPZoomPresent())
		return IsScopeAttached() ? m_zoom_params.m_fSecondVPWorldFOV : m_zoom_params.m_fIronSightZoomFactor;
	else 
		return IsScopeAttached() ? m_zoom_params.m_fScopeZoomFactor : m_zoom_params.m_fIronSightZoomFactor;
};

void GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor);

void CWeapon::OnZoomIn()
{
	m_zoom_params.m_bIsZoomModeNow = true;

	if (IsScopeAttached() && m_zoom_params.m_bUseDynamicZoom)
	{
		if (IsSecondVPZoomPresent())
		{
			if (m_fSVP_RTZoomFactor == 0.f)
				ZoomDynamicMod(true, true);

			SetZoomFactor(m_zoom_params.m_fSecondVPWorldFOV);
		}
		else
		{
			if (m_fRTZoomFactor == 0.f)
			{// For a removable scope, if m_fRTZoomFactor is not set, take factor from m_zoom_params.m_fScopeZoomFactor
				m_fRTZoomFactor = m_zoom_params.m_fScopeZoomFactor;
			}
			SetZoomFactor(m_fRTZoomFactor);
		}
	}
	else
		m_zoom_params.m_fCurrentZoomFactor	= CurrentZoomFactor();

	EnableHudInertion					(FALSE);

	//if(m_zoom_params.m_bZoomDofEnabled && !IsScopeAttached())
	//	GamePersistent().SetEffectorDOF	(m_zoom_params.m_ZoomDof);

	if(GetHUDmode())
		GamePersistent().SetPickableEffectorDOF(true);

	CActor* pA = smart_cast<CActor*>(H_Parent());
	if (!pA)
		return;

	// disable actor's NV and AD while zooming with scope

	if (IsScopeAttached())
	{
		if (pA->GetActorNightVisionStatus())
		{
			pA->GetActorNVHandler()->SwitchNightVision(false, false);

			m_zoom_params.reenableActorNVOnZoomOut_ = true;
		}

		if (pA->GetActorAliveDetectorState())
		{
			pA->SwitchActorAliveDetector();

			m_zoom_params.reenableActorALDetectorOnZoomOut_ = true;
		}

		if (m_zoom_params.m_sUseZoomPostprocess.size())
			m_zoom_params.scopeNightVision_ = xr_new<CNightVisionEffector>(*m_zoom_params.m_sUseZoomPostprocess);
	}
}

void CWeapon::OnZoomOut()
{
	m_zoom_params.m_bIsZoomModeNow		= false;
	m_fRTZoomFactor = GetZoomFactor();//store current
	m_zoom_params.m_fCurrentZoomFactor	= camFov;
	EnableHudInertion					(TRUE);

// 	GamePersistent().RestoreEffectorDOF	();

	if(GetHUDmode())
		GamePersistent().SetPickableEffectorDOF(false);

	//ResetSubStateTime					();

	if (m_zoom_params.scopeAliveDetectorVision_)
	{
		m_zoom_params.reenableALDetectorOnZoomIn_ = true;
	}

	xr_delete(m_zoom_params.scopeAliveDetectorVision_);

	if (m_zoom_params.scopeNightVision_)
	{
		m_zoom_params.reenableNVOnZoomIn_ = m_zoom_params.scopeNightVision_->IsActive();

		m_zoom_params.scopeNightVision_->StopPPE(100000.0f, false);
		m_zoom_params.scopeNightVision_->StopShader(false);
	}

	xr_delete(m_zoom_params.scopeNightVision_);
}

CUIWindow* CWeapon::ZoomTexture()
{
	if (UseScopeTexture() && !hud_adj_mode)
		return m_UIScope;
	else
		return NULL;
}

void CWeapon::SwitchState(u32 S)
{
	SetNextState		(S);	// Very-very important line of code!!! :)

	if (!CHudItem::object().getDestroy()/* && (S!=NEXT_STATE)*/ 
		&& m_pCurrentInventory)	
	{
		if (m_changed_ammoType_on_reload == u8(-1))
			SetAmmoOnReload(u8(-1));
		else
			SetAmmoOnReload(m_changed_ammoType_on_reload);

		OnStateSwitch(S);
	}

	// Calc Approx effective dist using weapon and current ammo values
	CalcEffectiveDistance();
}

void CWeapon::OnMagazineEmpty	()
{
	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}


void CWeapon::reinit			()
{
	CShootingObject::reinit		();
	CHudItemObject::reinit			();
}

void CWeapon::reload			(LPCSTR section)
{
	CShootingObject::reload		(section);
	CHudItemObject::reload			(section);
	
	m_can_be_strapped			= true;
	m_can_be_strapped_rifle			= (BaseSlot() == RIFLE_SLOT || BaseSlot() == RIFLE_2_SLOT);
	m_strapped_mode				= false;
	m_strapped_mode_rifle			= false;
	
	if (pSettings->line_exist(section,"strap_bone0"))
	{
		m_strap_bone0			= pSettings->r_string(section,"strap_bone0");
	} else {
		m_can_be_strapped		= false;
		m_can_be_strapped_rifle		= false;
	}
	
	if (pSettings->line_exist(section,"strap_bone1"))
	{
		m_strap_bone1			= pSettings->r_string(section,"strap_bone1");
	} else {
		m_can_be_strapped		= false;
		m_can_be_strapped_rifle		= false;
	}

	if (m_eScopeStatus == ALife::eAddonAttachable) {
		m_addon_holder_range_modifier = READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "holder_range_modifier", m_holder_range_modifier);
		m_addon_holder_fov_modifier = READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "holder_fov_modifier", m_holder_fov_modifier);
	}
	else {
		m_addon_holder_range_modifier	= m_holder_range_modifier;
		m_addon_holder_fov_modifier		= m_holder_fov_modifier;
	}


	{
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"position");
		ypr					= pSettings->r_fvector3		(section,"orientation");
		ypr.mul				(PI/180.f);

		m_Offset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_Offset.translate_over	(pos);
	}

	m_StrapOffset			= m_Offset;
	if (pSettings->line_exist(section,"strap_position") && pSettings->line_exist(section,"strap_orientation")) {
		Fvector				pos,ypr;
		pos					= pSettings->r_fvector3		(section,"strap_position");
		ypr					= pSettings->r_fvector3		(section,"strap_orientation");
		ypr.mul				(PI/180.f);

		m_StrapOffset.setHPB			(ypr.x,ypr.y,ypr.z);
		m_StrapOffset.translate_over	(pos);
	} else {
		m_can_be_strapped		= false;
		m_can_be_strapped_rifle		= false;
	}

	m_ef_main_weapon_type	= READ_IF_EXISTS(pSettings,r_u32,section,"ef_main_weapon_type",u32(-1));
	m_ef_weapon_type		= READ_IF_EXISTS(pSettings,r_u32,section,"ef_weapon_type",u32(-1));
}

void CWeapon::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CWeapon::activate_physic_shell()
{
	UpdateXForm();
	CPhysicsShellHolder::activate_physic_shell();
}

void CWeapon::setup_physic_shell()
{
	CPhysicsShellHolder::setup_physic_shell();
}

bool CWeapon::can_kill	() const
{
	if (GetSuitableAmmoTotal(true) || m_ammoTypes.empty())
		return				(true);

	return					(false);
}

CInventoryItem *CWeapon::can_kill	(CInventory *inventory) const
{
	if (GetAmmoElapsed() || m_ammoTypes.empty())
		return				(const_cast<CWeapon*>(this));

	TIItemContainer::iterator I = inventory->allContainer_.begin();
	TIItemContainer::iterator E = inventory->allContainer_.end();
	for ( ; I != E; ++I) {
		CInventoryItem	*inventory_item = smart_cast<CInventoryItem*>(*I);
		if (!inventory_item)
			continue;
		
		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(),m_ammoTypes.end(),inventory_item->object().SectionName());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}
//праркенен
const CInventoryItem *CWeapon::can_kill	(const xr_vector<const CGameObject*> &items) const
{
	if (m_ammoTypes.empty())
		return				(this);

	xr_vector<const CGameObject*>::const_iterator I = items.begin();
	xr_vector<const CGameObject*>::const_iterator E = items.end();
	for ( ; I != E; ++I) {
		const CInventoryItem	*inventory_item = smart_cast<const CInventoryItem*>(*I);
		if (!inventory_item)
			continue;

		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(),m_ammoTypes.end(),inventory_item->object().SectionName());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

bool CWeapon::ready_to_kill	() const
{
	return					(
		!IsMisfire() && 
		((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2)) && 
		GetAmmoElapsed()
	);
}

void CWeapon::UpdateHudAdditonal(Fmatrix& trans)
{
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (!pActor)        return;

	attachable_hud_item* hi = HudItemData();
	R_ASSERT(hi);

	u8 idx = GetCurrentHudOffsetIdx();

	float fAvgTimeDelta = TimeDelta();

	if ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f))
	{
		Fvector                     curr_offs, curr_rot;

		if (IsScopeAttached())
		{
			curr_offs = hi->m_measures.m_hands_offset_scope[0][idx];//pos,aim
			curr_rot = hi->m_measures.m_hands_offset_scope[1][idx];//rot,aim
		}
		else
		{
			curr_offs = hi->m_measures.m_hands_offset_ironsight[0][idx];//pos,aim
			curr_rot = hi->m_measures.m_hands_offset_ironsight[1][idx];//rot,aim
		}

		curr_offs.mul(m_zoom_params.m_fZoomRotationFactor);
		curr_rot.mul(m_zoom_params.m_fZoomRotationFactor);

		Fmatrix                     hud_rotation;
		hud_rotation.identity();
		hud_rotation.rotateX(curr_rot.x);

		Fmatrix                     hud_rotation_y;
		hud_rotation_y.identity();
		hud_rotation_y.rotateY(curr_rot.y);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation_y.identity();
		hud_rotation_y.rotateZ(curr_rot.z);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation.translate_over(curr_offs);
		trans.mulB_43(hud_rotation);

		if (pActor->IsZoomAimingMode())
			m_zoom_params.m_fZoomRotationFactor += TimeDelta() / m_zoom_params.m_fZoomRotateTime;
		else
			m_zoom_params.m_fZoomRotationFactor -= TimeDelta() / m_zoom_params.m_fZoomRotateTime;

		clamp(m_zoom_params.m_fZoomRotationFactor, 0.f, 1.f);
	}

	float fShootingReturnSpeedMod = angle_lerp(
			hi->m_measures.m_shooting_params.m_ret_speed,
			hi->m_measures.m_shooting_params.m_ret_speed_aim,
			m_zoom_params.m_fZoomRotationFactor);

	float fShootingBackwOffset = angle_lerp(
			hi->m_measures.m_shooting_params.m_shot_offset_BACKW.x,
			hi->m_measures.m_shooting_params.m_shot_offset_BACKW.y,
			m_zoom_params.m_fZoomRotationFactor);

	Fvector4 vShOffsets; // x = L, y = R, z = U, w = D
	vShOffsets.x = angle_lerp(
			hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD.x,
			hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD_aim.x,
			m_zoom_params.m_fZoomRotationFactor);
	vShOffsets.y = angle_lerp(
			hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD.y,
			hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD_aim.y,
			m_zoom_params.m_fZoomRotationFactor);
	vShOffsets.z = angle_lerp(
			hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD.z,
			hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD_aim.z,
			m_zoom_params.m_fZoomRotationFactor);
	vShOffsets.w = angle_lerp(
			hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD.w,
			hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD_aim.w,
			m_zoom_params.m_fZoomRotationFactor);

	m_fLR_ShootingFactor *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);
	m_fUD_ShootingFactor *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);
	m_fBACKW_ShootingFactor *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);

	{
		float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
		if (m_fLR_ShootingFactor < 0.0f)
		{
			m_fLR_ShootingFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fLR_ShootingFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fLR_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fLR_ShootingFactor, 0.0f, 1.0f);
		}
	}

	{
		float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
		if (m_fUD_ShootingFactor < 0.0f)
		{
			m_fUD_ShootingFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fUD_ShootingFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fUD_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fUD_ShootingFactor, 0.0f, 1.0f);
		}
	}

	{
		float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
		m_fBACKW_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
		clamp(m_fBACKW_ShootingFactor, 0.0f, 1.0f);
	}

	{
		float fLR_lim = (m_fLR_ShootingFactor < 0.0f ? vShOffsets.x : vShOffsets.y);
		float fUD_lim = (m_fUD_ShootingFactor < 0.0f ? vShOffsets.z : vShOffsets.w);

		Fvector curr_offs;
		curr_offs = {fLR_lim * m_fLR_ShootingFactor, fUD_lim * -1.f * m_fUD_ShootingFactor, -1.f * fShootingBackwOffset * m_fBACKW_ShootingFactor};

		Fmatrix hud_rotation;
		hud_rotation.identity();
		hud_rotation.translate_over(curr_offs);
		trans.mulB_43(hud_rotation);
	}

	float fStrafeMaxTime = hi->m_measures.m_strafe_offset[2][idx].y; 
	if (fStrafeMaxTime <= EPS)
		fStrafeMaxTime = 0.01f;

	float fStepPerUpd = fAvgTimeDelta / fStrafeMaxTime; 

	float fCamReturnSpeedMod = 1.5f; 


	float fCamLimitNoAim = 0.5f;		 //
	float fYMag = pActor->fFPCamYawMagnitude;

	if (fYMag != 0.0f)
	{ //-->     Y
		m_fLR_CameraFactor -= (fYMag * 0.005f);

		float fCamLimitBlend = 1.0f - ((1.0f - fCamLimitNoAim) * (1.0f - m_zoom_params.m_fZoomRotationFactor));
		clamp(m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
	}
	else
	{ //-->    -
		if (m_fLR_CameraFactor < 0.0f)
		{
			m_fLR_CameraFactor += fStepPerUpd * fCamReturnSpeedMod;
			clamp(m_fLR_CameraFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fLR_CameraFactor -= fStepPerUpd * fCamReturnSpeedMod;
			clamp(m_fLR_CameraFactor, 0.0f, 1.0f);
		}
	}

	// Добавляем боковой наклон от ходьбы вбок
	float fChangeDirSpeedMod = 3; // Восколько быстро меняем направление направление наклона, если оно в другую сторону от текущего

	u32 iMovingState = pActor->MovingState();
	if ((iMovingState & mcLStrafe) != 0)
	{ // Движемся влево
		float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{ // Движемся вправо
		float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		m_fLR_MovingFactor += fVal;
	}
	else
	{ // Двигаемся в любом другом направлении
		if (m_fLR_MovingFactor < 0.0f)
		{
			m_fLR_MovingFactor += fStepPerUpd;
			clamp(m_fLR_MovingFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fLR_MovingFactor -= fStepPerUpd;
			clamp(m_fLR_MovingFactor, 0.0f, 1.0f);
		}
	}

	clamp(m_fLR_MovingFactor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

											// Вычисляем и нормализируем итоговый фактор наклона
	float fLR_Factor = m_fLR_MovingFactor + m_fLR_CameraFactor;
	clamp(fLR_Factor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

	// Производим наклон ствола для нормального режима и аима
	if (idx == 0 && hi->m_measures.m_strafe_offset[2][0].x)
	{
		Fvector curr_offs, curr_rot;

		// Смещение позиции худа в стрейфе
		curr_offs = hi->m_measures.m_strafe_offset[0][0]; //pos
		curr_offs.mul(fLR_Factor);                   // Умножаем на фактор стрейфа

															 // Поворот худа в стрейфе
		curr_rot = hi->m_measures.m_strafe_offset[1][0]; //rot
		curr_rot.mul(-PI / 180.f);                          // Преобразуем углы в радианы
		curr_rot.mul(fLR_Factor);                   // Умножаем на фактор стрейфа

		curr_offs.mul(1.f - m_zoom_params.m_fZoomRotationFactor);
		curr_rot.mul(1.f - m_zoom_params.m_fZoomRotationFactor);

		Fmatrix hud_rotation;
		Fmatrix hud_rotation_y;

		hud_rotation.identity();
		hud_rotation.rotateX(curr_rot.x);

		hud_rotation_y.identity();
		hud_rotation_y.rotateY(curr_rot.y);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation_y.identity();
		hud_rotation_y.rotateZ(curr_rot.z);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation.translate_over(curr_offs);
		trans.mulB_43(hud_rotation);
	}
}


void CWeapon::AddHUDShootingEffect()
{
	if (IsHidden() || ParentIsActor() == false)
		return;


	m_fBACKW_ShootingFactor = 1.0f;


	float fPowerMin = 0.0f;
	attachable_hud_item *hi = HudItemData();
	if (hi != nullptr)
	{
		fPowerMin = clampr(hi->m_measures.m_shooting_params.m_min_LRUD_power, 0.0f, 0.99f);
	}

	float fPowerRnd = 1.0f - fPowerMin;

	m_fLR_ShootingFactor = ::Random.randF(-fPowerRnd, fPowerRnd);
	m_fLR_ShootingFactor += (m_fLR_ShootingFactor >= 0.0f ? fPowerMin : -fPowerMin);

	m_fUD_ShootingFactor = ::Random.randF(-fPowerRnd, fPowerRnd);
	m_fUD_ShootingFactor += (m_fUD_ShootingFactor >= 0.0f ? fPowerMin : -fPowerMin);
}

void	CWeapon::SetAmmoElapsed	(int ammo_count)
{
	iAmmoElapsed				= ammo_count;

	u32 uAmmo					= u32(iAmmoElapsed);

	if (uAmmo != m_magazine.size())
	{
		if (uAmmo > m_magazine.size())
		{
			CCartridge			l_cartridge; 
			l_cartridge.Load	(*m_ammoTypes[m_ammoType], u8(m_ammoType));
			while (uAmmo > m_magazine.size())
				m_magazine.push_back(l_cartridge);
		}
		else
		{
			while (uAmmo < m_magazine.size())
				m_magazine.pop_back();
		};
	};
}

u32	CWeapon::ef_main_weapon_type	() const
{
	VERIFY	(m_ef_main_weapon_type != u32(-1));
	return	(m_ef_main_weapon_type);
}

u32	CWeapon::ef_weapon_type	() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

bool CWeapon::IsNecessaryItem	    (const shared_str& item_sect)
{
	return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end() );
}

void CWeapon::modify_holder_params		(float &range, float &fov) const
{
	if (!IsScopeAttached()) {
		inherited::modify_holder_params	(range,fov);
		return;
	}
	range	*= m_addon_holder_range_modifier;
	fov		*= m_addon_holder_fov_modifier;
}

bool CWeapon::render_item_ui_query()
{
	bool b_is_active_item = (m_pCurrentInventory->ActiveItem()==this);
	bool res = b_is_active_item && IsZoomed() && ZoomHideCrosshair() && ZoomTexture() && !IsRotatingToZoom();
	return res;
}

void CWeapon::render_item_ui()
{
	if (m_zoom_params.scopeAliveDetectorVision_)
		m_zoom_params.scopeAliveDetectorVision_->Draw();

	ZoomTexture()->Update	();
	ZoomTexture()->Draw		();
}

bool CWeapon::unlimited_ammo() 
{ 
	return psActorFlags.test(AF_UNLIMITEDAMMO) && 
			m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited); 

};

LPCSTR	CWeapon::GetCurrentAmmo_ShortName	()
{
	if (m_magazine.empty()) return ("");
	CCartridge &l_cartridge = m_magazine.back();
	return *(l_cartridge.m_InvShortName);
}

float CWeapon::Weight() const
{
	float res = CInventoryItemObject::Weight();
	if(IsGrenadeLauncherAttached()&&GetGrenadeLauncherName().size()){
		res += pSettings->r_float(GetGrenadeLauncherName(),"inv_weight");
	}
	if(IsScopeAttached()&&m_scopes.size()){
		res += pSettings->r_float(GetScopeName(),"inv_weight");
	}
	if(IsSilencerAttached()&&GetSilencerName().size()){
		res += pSettings->r_float(GetSilencerName(),"inv_weight");
	}
	
	if(iAmmoElapsed)
	{
		float w		= pSettings->r_float(m_ammoTypes[m_ammoType].c_str(),"inv_weight");
		float bs	= pSettings->r_float(m_ammoTypes[m_ammoType].c_str(),"box_size");

		res			+= w*(iAmmoElapsed/bs);
	}
	return res;
}
void CWeapon::Hide		()
{

	if (m_bHolster_off == true) {
		SwitchState(eHiding);
	}

	OnZoomOut();
}

void CWeapon::Show		()
{
	if (m_bDraw_off == true) {
		SwitchState(eShowing);
	}
}

bool CWeapon::show_crosshair()
{
	return !IsPending() && ( !IsZoomed() || !ZoomHideCrosshair() );
}

bool CWeapon::show_indicators()
{
	return ! ( IsZoomed() && ZoomTexture() );
}

float CWeapon::GetConditionToShow	() const
{
	return	(GetCondition());//powf(GetCondition(),4.0f));
}

BOOL CWeapon::ParentMayHaveAimBullet	()
{
	CObject* O=H_Parent();
	CEntityAlive* EA=smart_cast<CEntityAlive*>(O);
	return EA->cast_actor()!=0;
}

BOOL CWeapon::ParentIsActor	()
{
	CObject* O			= H_Parent();
	if (!O)
		return FALSE;

	CEntityAlive* EA	= smart_cast<CEntityAlive*>(O);
	if (!EA)
		return FALSE;

	return EA->cast_actor()!=0;
}

void CWeapon::debug_draw_firedeps()
{
#ifdef DEBUG
	if(hud_adj_mode==5||hud_adj_mode==6||hud_adj_mode==7)
	{
		CDebugRenderer			&render = Level().debug_renderer();

		if(hud_adj_mode==5)
			render.draw_aabb(get_LastFP(),	0.005f,0.005f,0.005f,D3DCOLOR_XRGB(255,0,0));

		if(hud_adj_mode==6)
			render.draw_aabb(get_LastFP2(),	0.005f,0.005f,0.005f,D3DCOLOR_XRGB(0,0,255));

		if(hud_adj_mode==7)
			render.draw_aabb(get_LastSP(),		0.005f,0.005f,0.005f,D3DCOLOR_XRGB(0,255,0));
	}
#endif // DEBUG
}

const float &CWeapon::hit_probability	() const
{
	VERIFY					((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster)); 
	return					(m_hit_probability[g_SingleGameDifficulty]);
}

// gr1ph: added zoom in/out

void CWeapon::GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor)
{
	float def_fov = camFov;
	float min_zoom_k = 0.3f;
	float zoom_step_count = 3.0f;
	float delta_factor_total = def_fov - scope_factor;
	min_zoom_factor = def_fov - delta_factor_total * min_zoom_k;
	delta = (delta_factor_total * (1 - min_zoom_k)) / zoom_step_count;
}

u8 CWeapon::GetCurrentHudOffsetIdx()
{
	CActor* pActor	= smart_cast<CActor*>(H_Parent());
	if(!pActor)		return 0;
	
	bool b_aiming		= 	((IsZoomed() && m_zoom_params.m_fZoomRotationFactor<=1.f) ||
							(!IsZoomed() && m_zoom_params.m_fZoomRotationFactor>0.f));

	if(!b_aiming)
		return		0;
	else
		return		1;
}

void CWeapon::renderable_Render(IRenderBuffer& render_buffer)
{
	protectWeaponRender_.Enter();
#pragma todo("MT Warning: better to move this call to main thread. May be into updateCL?")
	UpdateXForm();

	//нарисовать подсветку
#pragma todo("MT Warning: better to move this call to main thread. May be into updateCL?")
	RenderLight();

	protectWeaponRender_.Leave();

	inherited::renderable_Render(render_buffer);
}

void CWeapon::render_hud_mode(IKinematics* hud_model, IRenderBuffer& render_buffer)
{
#pragma todo("MT Warning: better to move this call to main thread. May be into updateCL?")
	RenderLight();
}

bool CWeapon::MovingAnimAllowedNow()
{
	return !IsZoomed();
}

bool CWeapon::IsHudModeNow()
{
	return (HudItemData()!=NULL);
}

void CWeapon::ZoomDec()
{
	ZoomDynamicMod(false, false);
}

void CWeapon::ZoomInc()
{
	ZoomDynamicMod(true, false);
}

u32 CWeapon::Cost() const
{
	u32 res = CInventoryItem::Cost();
	if(IsGrenadeLauncherAttached()&&GetGrenadeLauncherName().size()){
		res += pSettings->r_u32(GetGrenadeLauncherName(),"cost");
	}
	if(IsScopeAttached()&&m_scopes.size()){
		res += pSettings->r_u32(GetScopeName(),"cost");
	}
	if(IsSilencerAttached()&&GetSilencerName().size()){
		res += pSettings->r_u32(GetSilencerName(),"cost");
	}
	
	if(iAmmoElapsed)
	{
		float w		= pSettings->r_float(m_ammoTypes[m_ammoType].c_str(),"cost");
		float bs	= pSettings->r_float(m_ammoTypes[m_ammoType].c_str(),"box_size");

		res			+= iFloor(w*(iAmmoElapsed/bs));
	}
	return res;
}

void CWeapon::SetAmmoOnReload(u8 value){
	m_changed_ammoType_on_reload = value;
	R_ASSERT2(m_changed_ammoType_on_reload < m_ammoTypes.size() || m_changed_ammoType_on_reload == u8(-1), make_string("Engine Bug: Reaload Ammo Type is not within Weapon Ammo and not less the Size: Value = [%u], Size = [%u], Name = [%s]", m_changed_ammoType_on_reload, m_ammoTypes.size(), Name() ? Name() : "null"));
};

extern ENGINE_API float		hudFovRuntime_K;

// ON / OFF for +SecondVP+
// Called only for active weapon in hands
void CWeapon::UpdateSecondVP(bool bCond_4)
{
	bool bIsActiveItem = m_pCurrentInventory && m_pCurrentInventory->ActiveItem() == this;
	R_ASSERT(ParentIsActor() && bIsActiveItem); // This function should be called only for weapons in actor's hands.
	
	CActor* pActor = smart_cast<CActor*>(H_Parent());

	bool bCond_1 = GetInZoomNow() > 0.05f;

	bool bCond_2 = IsSecondVPZoomPresent();

	bool bCond_3 = pActor->cam_Active() == pActor->cam_FirstEye();

	Device.m_SecondViewport.SetSVPActive(bCond_1 && bCond_2 && bCond_3 && bCond_4);

	if (Device.m_SecondViewport.IsSVPActive())
	{
		hudFovRuntime_K = (hudFovRuntime_K - (3.5f * TimeDelta()));
		clamp(hudFovRuntime_K, secondVPWorldHudFOVK, 2.f);
	}
	else
	{
		hudFovRuntime_K = (hudFovRuntime_K + (3.5f * TimeDelta()));
		clamp(hudFovRuntime_K, secondVPWorldHudFOVK, 1.f);
	}
}

float CWeapon::GetSecondVPFov() const
{
	if (!IsSecondVPZoomPresent())
		return 0.0f;

	if (m_zoom_params.m_bUseDynamicZoom)
		return m_fSVP_RTZoomFactor  * (0.75f); // 0.75f like as in CActor::currentFOV()

	return GetSecondVPZoomFactor() * (0.75f);
}

bool CWeapon::UseScopeTexture()
{
	if (IsSecondVPZoomPresent()) return false;

	return true;
}

float CWeapon::GetInZoomNow() const 
{ 
	if (!psActorFlags.test(AF_USE_3D_SCOPES))
		return 0.f;

	return m_zoom_params.m_fZoomRotationFactor;
}

void CWeapon::ZoomDynamicMod(bool bIncrement, bool bForceLimit)
{
	if (!IsScopeAttached())					return;
	if (!m_zoom_params.m_bUseDynamicZoom)	return;

	bool bHas3dZoom = IsSecondVPZoomPresent();
	float delta, min_zoom_factor, max_zoom_factor;
	max_zoom_factor = bHas3dZoom ? GetSecondVPZoomFactor() : m_zoom_params.m_fScopeZoomFactor;
	GetZoomData(max_zoom_factor, delta, min_zoom_factor);

	if (bForceLimit)
	{
		if(bHas3dZoom) m_fSVP_RTZoomFactor = (bIncrement ? max_zoom_factor : min_zoom_factor);
		else SetZoomFactor(bIncrement ? max_zoom_factor : min_zoom_factor);
	}
	else
	{
		float f = bHas3dZoom ? m_fSVP_RTZoomFactor : GetZoomFactor();
		f -= (delta * (bIncrement ? 1.f : -1.f));
		clamp(f, max_zoom_factor, min_zoom_factor);

		if (bHas3dZoom) m_fSVP_RTZoomFactor = f;
		else SetZoomFactor(f);
	}
}

Fvector& CWeapon::hands_offset_pos()
{
	attachable_hud_item* hi = HudItemData();
	R_ASSERT(hi);

	u8 idx = GetCurrentHudOffsetIdx();

	if (IsScopeAttached())
	{
		return hi->m_measures.m_hands_offset_scope[0][idx];
	}
	else
	{
		return hi->m_measures.m_hands_offset_ironsight[0][idx];
	}
}

Fvector& CWeapon::hands_offset_rot()
{
	attachable_hud_item* hi = HudItemData();
	R_ASSERT(hi);

	u8 idx = GetCurrentHudOffsetIdx();

	if (IsScopeAttached())
	{
		return hi->m_measures.m_hands_offset_scope[1][idx];
	}
	else
	{
		return hi->m_measures.m_hands_offset_ironsight[1][idx];
	}
}

bool CWeapon::ActivationSpeedOverriden(Fvector& dest, bool clear_override)
{
	if (m_activation_speed_is_overriden)
	{
		if (clear_override)
		{
			m_activation_speed_is_overriden = false;
		}

		dest = m_overriden_activation_speed;
		return true;
	}

	return false;
}

void CWeapon::SetActivationSpeedOverride(Fvector const& speed)
{
	m_overriden_activation_speed = speed;
	m_activation_speed_is_overriden = true;
}