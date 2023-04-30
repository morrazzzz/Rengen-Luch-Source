#include "stdafx.h"
#include "artifact.h"
#include "../../xrphysics/PhysicsShell.h"
#include "PhysicsShellHolder.h"
#include "../Include/xrRender/Kinematics.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "inventory.h"
#include "level.h"
#include "ai_object_location.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "../../xrphysics/iphworld.h"
#include "restriction_space.h"
#include "artefact_activation.h"

#define	FASTMODE_DISTANCE (50.f)	//distance to camera from sphere, when zone switches to fast update sequence

extern BOOL	gameObjectLightShadows_;

CArtefact::CArtefact(void) 
{
	shedule.t_min				= 20;
	shedule.t_max				= 50;
	m_sParticlesName			= NULL;
	m_pTrailLight				= NULL;
	m_activationObj				= NULL;
	custom_detect_sound;

	owningAnomalyID_			= NEW_INVALID_OBJECT_ID;
}

CArtefact::~CArtefact(void) 
{
	if (custom_detect_sound_string)custom_detect_sound.destroy();
}

void CArtefact::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg			(section);


	if (pSettings->line_exist(section, "particles"))
		m_sParticlesName	= pSettings->r_string(section, "particles");

	m_bLightsEnabled		= !!pSettings->r_bool(section, "lights_enabled");
	detect_radius_koef		= READ_IF_EXISTS(pSettings,r_float,section,"detect_radius_koef",1.0f);

	custom_detect_sound_string = READ_IF_EXISTS(pSettings, r_string, section, "custom_detect_sound", NULL);

	if (custom_detect_sound_string)
	{
		custom_detect_sound.create(pSettings->r_string(section, "custom_detect_sound"), st_Effect, SOUND_TYPE_ITEM);
	}

	if(m_bLightsEnabled)
	{
		sscanf(pSettings->r_string(section,"trail_light_color"), "%f,%f,%f", 
			&m_TrailLightColor.r, &m_TrailLightColor.g, &m_TrailLightColor.b);
		m_fTrailLightRange	= pSettings->r_float(section,"trail_light_range");
	}


	{
		m_fHealthRestoreSpeed = pSettings->r_float		(section,"health_restore_speed"		);
		m_fRadiationRestoreSpeed = pSettings->r_float	(section,"radiation_restore_speed"	);
		m_fSatietyRestoreSpeed = pSettings->r_float		(section,"satiety_restore_speed"	);
		m_fPowerRestoreSpeed = pSettings->r_float		(section,"power_restore_speed"		);
		m_fBleedingRestoreSpeed = pSettings->r_float	(section,"bleeding_restore_speed"	);
		m_fPsyhealthRestoreSpeed = READ_IF_EXISTS		(pSettings, r_float, section, "psy_health_restore_speed", 0.0f);
		if(pSettings->section_exist(/**SectionName(), */pSettings->r_string(section,"hit_absorbation_sect")))
			m_ArtefactHitImmunities.LoadImmunities(pSettings->r_string(section,"hit_absorbation_sect"),pSettings);
	}
	m_bCanSpawnZone = !!pSettings->line_exist("artefact_spawn_zones", section);

	//tatarinrafa added additional_inventory_weight to artefacts
	m_additional_weight			= READ_IF_EXISTS(pSettings, r_float, section, "additional_inventory_weight", 0.0f);

	//tatarinrafa: added additional jump speed sprint speed walk speed
	m_additional_jump_speed		= READ_IF_EXISTS(pSettings, r_float, section, "additional_jump_speed", 0.0f);
	m_additional_run_coef		= READ_IF_EXISTS(pSettings, r_float, section, "additional_run_coef", 0.0f);
	m_additional_sprint_koef	= READ_IF_EXISTS(pSettings, r_float, section, "additional_sprint_koef", 0.0f);
}

BOOL CArtefact::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	BOOL result = inherited::SpawnAndImportSOData(data_containing_so);

	CSE_ALifeItemArtefact* server_art = smart_cast<CSE_ALifeItemArtefact*>(data_containing_so);

	if (server_art)
		owningAnomalyID_ = server_art->serverOwningAnomalyID_;

	if (*m_sParticlesName) 
	{
		Fvector dir;
		dir.set(0, 1, 0);

		CParticlesPlayer::StartParticles(m_sParticlesName, dir, ID(), -1, false);
	}

	VERIFY(m_pTrailLight == NULL);
	m_pTrailLight = ::Render->light_create();
	m_pTrailLight->set_shadow(gameObjectLightShadows_ == TRUE ? true : false);
	m_pTrailLight->set_use_static_occ(false);

	StartLights();
	/////////////////////////////////////////
	m_CarringBoneID = u16(-1);
	/////////////////////////////////////////
	IKinematicsAnimated	*K			= smart_cast<IKinematicsAnimated*>(Visual());
	if(K)
		K->PlayCycle("idle");
	
	o_fastmode					= FALSE	;		// start initially with fast-mode enabled
	o_render_frame				= 0		;
	SetState					(eHidden);

	return result;	
}

void CArtefact::ExportDataToServer(NET_Packet& P)
{
	inherited::ExportDataToServer(P);

	P.w_u32(owningAnomalyID_);
}

void CArtefact::DestroyClientObj() 
{
/*
	if (*m_sParticlesName) 
		CParticlesPlayer::StopParticles(m_sParticlesName, BI_NONE, true);
*/
	inherited::DestroyClientObj();

	StopLights					();
	m_pTrailLight.destroy		();
	CPHUpdateObject::Deactivate	();
	xr_delete					(m_activationObj);
}

void CArtefact::AfterAttachToParent() 
{
	inherited::AfterAttachToParent();

	StopLights();

	if (*m_sParticlesName) 
	{	
		CParticlesPlayer::StopParticles(m_sParticlesName, BI_NONE, true);
	}
}

void CArtefact::BeforeDetachFromParent(bool just_before_destroy) 
{
	VERIFY(!physics_world()->Processing());
	inherited::BeforeDetachFromParent(just_before_destroy);

	StartLights();
	if (*m_sParticlesName) 
	{
		Fvector dir;
		dir.set(0,1,0);
		CParticlesPlayer::StartParticles(m_sParticlesName,dir,ID(),-1, false);
	}
}

// called only in "fast-mode"
void CArtefact::UpdateCL() 
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	inherited::UpdateCL();
	
	if (o_fastmode || m_activationObj)
		UpdateWorkload(TimeDeltaU());

	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CArtefact::UpdateWorkload(u32 dt) 
{
	VERIFY(!physics_world()->Processing());
	// particles - velocity
	Fvector vel = {0, 0, 0};

	if (H_Parent()) 
	{
		CPhysicsShellHolder* pPhysicsShellHolder = smart_cast<CPhysicsShellHolder*>(H_Parent());

		if(pPhysicsShellHolder)
			pPhysicsShellHolder->PHGetLinearVell(vel);
	}

	CParticlesPlayer::SetParentVel(vel);

	// 
	UpdateLights();

	if(m_activationObj)
	{
		CPHUpdateObject::Activate();
		m_activationObj->UpdateActivation();

		return;
	}

	// custom-logic
	UpdateCLChild();
}

void CArtefact::ScheduledUpdate(u32 dt) 
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif
	

	inherited::ScheduledUpdate(dt);

	//////////////////////////////////////////////////////////////////////////
	// check "fast-mode" border
	if (H_Parent())
		o_switch_2_slow();
	else
	{
		Fvector	center;
		Center(center);

		BOOL rendering = (CurrentFrame() == o_render_frame);

		float cam_distance = Device.vCameraPosition.distance_to(center) - Radius();
		if (rendering || (cam_distance < FASTMODE_DISTANCE))
			o_switch_2_fast	();
		else
			o_switch_2_slow	();
	}

	if (!o_fastmode)
		UpdateWorkload(dt);


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_VariousItems_ += measure_sc_update.GetElapsed_sec();
#endif
}


void CArtefact::create_physic_shell	()
{
	///create_box2sphere_physic_shell	();
	m_pPhysicsShell=P_build_Shell(this,false);
	m_pPhysicsShell->Deactivate();
}

void CArtefact::StartLights()
{
	VERIFY(!physics_world()->Processing());
	if(!m_bLightsEnabled) return;

	//включить световую подсветку от двигателя
	m_pTrailLight->set_color(m_TrailLightColor.r, 
		m_TrailLightColor.g, 
		m_TrailLightColor.b);

	m_pTrailLight->set_range(m_fTrailLightRange);
	m_pTrailLight->set_position(Position()); 
	m_pTrailLight->set_active(true);
}

void CArtefact::StopLights()
{
	VERIFY(!physics_world()->Processing());
	if(!m_bLightsEnabled) return;
	m_pTrailLight->set_active(false);
}

void CArtefact::UpdateLights()
{
	VERIFY(!physics_world()->Processing());
	if(!m_bLightsEnabled || !m_pTrailLight->get_active()) return;
	m_pTrailLight->set_position(Position());
}

void CArtefact::ActivateArtefact	()
{
	VERIFY(m_bCanSpawnZone);
	VERIFY( H_Parent() );
	m_activationObj = xr_new <SArtefactActivation>(this,H_Parent()->ID());
	m_activationObj->Start();

}

void CArtefact::PhDataUpdate	(dReal step)
{
	if(m_activationObj)
		m_activationObj->PhDataUpdate			(step);
}

bool CArtefact::CanTake() const
{
	if(!inherited::CanTake())return false;
	return (m_activationObj==NULL);
}

void CArtefact::Hide()
{
	SwitchState(eHiding);
}

void CArtefact::Show()
{
	SwitchState(eShowing);
}
#include "inventoryOwner.h"
#include "Entity_alive.h"
void CArtefact::UpdateXForm()
{
	if (CurrentFrame() != dwXF_Frame)
	{
		dwXF_Frame = CurrentFrame();

		if (0==H_Parent())	return;

		// Get access to entity and its visual
		CEntityAlive*		E		= smart_cast<CEntityAlive*>(H_Parent());
        
		if(!E)				return	;

		const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
		if (parent && parent->use_simplified_visual())
			return;

		VERIFY				(E);
		IKinematics*		V		= smart_cast<IKinematics*>	(E->Visual());
		VERIFY				(V);

		// Get matrices
		int					boneL = -1, boneR = -1, boneR2 = -1;
		E->g_WeaponBones	(boneL,boneR,boneR2);
		if (boneR == -1 || boneR2 == -1) return;

		boneL = boneR2;

		V->NeedToCalcBones();

		Fmatrix& mL			= V->LL_GetTransform(u16(boneL));
		Fmatrix& mR			= V->LL_GetTransform(u16(boneR));

		// Calculate
		Fmatrix				mRes;
		Fvector				R,D,N;
		D.sub				(mL.c,mR.c);	D.normalize_safe();
		R.crossproduct		(mR.j,D);		R.normalize_safe();
		N.crossproduct		(D,R);			N.normalize_safe();
		mRes.set			(R,N,D,mR.c);
		mRes.mulA_43		(E->XFORM());
//		UpdatePosition		(mRes);
		XFORM().mul			(mRes,offset());
	}
}
#include "xr_level_controller.h"
bool CArtefact::Action(u16 cmd, u32 flags) 
{
	switch (cmd)
	{
	case kWPN_FIRE:
		{
			if (flags&CMD_START && m_bCanSpawnZone){
				SwitchState(eActivating);
				return true;
			}
			if (flags&CMD_STOP && m_bCanSpawnZone && GetState()==eActivating)
			{
				SwitchState(eIdle);
				return true;
			}
		}break;
	default:
		break;
	}
	return inherited::Action(cmd,flags);
}

void CArtefact::onMovementChanged	(ACTOR_DEFS::EMoveCommand cmd)
{
	if( (cmd == ACTOR_DEFS::mcSprint)&&(GetState()==eIdle)  )
		PlayAnimIdle		();
}

void CArtefact::OnStateSwitch		(u32 S)
{
	inherited::OnStateSwitch	(S);
	switch(S){
	case eShowing:
		{
			PlayHUDMotion("anm_show", FALSE, this, S);
		}break;
	case eHiding:
		{
			PlayHUDMotion("anm_hide", FALSE, this, S);
		}break;
	case eActivating:
		{
			PlayHUDMotion("anm_activate", FALSE, this, S);
		}break;
	case eIdle:
		{
			PlayAnimIdle();
		}break;
	};
}

void CArtefact::PlayAnimIdle()
{
	PlayHUDMotion("anm_idle", FALSE, NULL, eIdle);
}

void CArtefact::OnAnimationEnd		(u32 state)
{
	inherited::OnAnimationEnd(state);
	switch (state)
	{
	case eHiding:
		{
			SwitchState(eHidden);
//.			if(m_pCurrentInventory->GetNextActiveSlot()!=NO_ACTIVE_SLOT)
//.				m_pCurrentInventory->Activate(m_pCurrentInventory->GetPrevActiveSlot());
		}break;
	case eShowing:
		{
			SwitchState(eIdle);
		}break;
	case eActivating:
		{
			SwitchState		(eHiding);

			ActivateArtefact();
		}break;
	};
}

void CArtefact::MoveTo(Fvector const&  position)
{
	if (!PPhysicsShell())
		return;

	Fmatrix	M = XFORM();
	M.translate(position);
	ForceTransform(M);
}