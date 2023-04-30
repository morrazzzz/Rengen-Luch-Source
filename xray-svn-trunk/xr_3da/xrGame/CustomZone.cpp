#include "stdafx.h"
#include "../xr_ioconsole.h"
#include "customzone.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "xrserver_objects_alife_monsters.h"
#include "../LightAnimLibrary.h"
#include "level.h"
#include "game_cl_base.h"
#include "../igame_persistent.h"
#include "../xr_collide_form.h"
#include "artifact.h"
#include "ai_object_location.h"
#include "../Include/xrRender/Kinematics.h"
#include "zone_effector.h"
#include "breakableobject.h"
#include "GamePersistent.h"
#include "GameConstants.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"

#define WIND_RADIUS (4*Radius())	//расстояние до актера, когда появляется ветер 
#define FASTMODE_DISTANCE (50.f)	//distance to camera from sphere, when zone switches to fast update sequence

extern BOOL	gameObjectLightShadows_;

CCustomZone::CCustomZone(void) 
{
	m_zone_flags.zero			();

	m_fMaxPower					= 100.f;
	m_fAttenuation				= 1.f;
	m_fEffectiveRadius			= 1.0f;
	m_zone_flags.set			(eZoneIsActive, FALSE);
	m_eHitTypeBlowout			= ALife::eHitTypeWound;
	m_pIdleParticles			= NULL;
	m_pLight					= NULL;
	m_pIdleLight				= NULL;
	m_pIdleLAnim				= NULL;
	

	m_StateTime.resize(eZoneStateMax);
	for(int i=0; i<eZoneStateMax; i++)
		m_StateTime[i] = 0;


	m_dwAffectFrameNum			= 0;
	m_fArtefactSpawnProbability = 0.f;
	m_fThrowOutPower			= 0.f;
	m_fArtefactSpawnHeight		= 0.f;
	SpawnArtefactWhenBlewOut	= false;
	m_fBlowoutWindPowerMax = m_fStoreWindPower = 0.f;
	m_fDistanceToCurEntity		= flt_max;
	m_ef_weapon_type			= u32(-1);
	m_owner_id					= u32(-1);

	m_actor_effector			= NULL;
	m_zone_flags.set			(eIdleObjectParticlesDontStop, FALSE);
	m_zone_flags.set			(eBlowoutWindActive, FALSE);
	m_zone_flags.set			(eFastMode, TRUE);

	deathSpawnReenableDelay_	= 0;
	nextDeathSpawnArtOKTime_	= 0;
	m_eZoneState				= eZoneStateIdle;
}

CCustomZone::~CCustomZone(void) 
{	
	m_idle_sound.destroy		();
	m_accum_sound.destroy		();
	m_awaking_sound.destroy		();
	m_blowout_sound.destroy		();
	m_hit_sound.destroy			();
	m_entrance_sound.destroy	();
	m_ArtefactBornSound.destroy	();
	xr_delete					(m_actor_effector);
}

void CCustomZone::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);

	m_iDisableHitTime		= pSettings->r_s32(section,				"disable_time");	
	m_iDisableHitTimeSmall	= pSettings->r_s32(section,				"disable_time_small");	
	m_iDisableIdleTime		= pSettings->r_s32(section,				"disable_idle_time");	
	m_fHitImpulseScale		= pSettings->r_float(section,			"hit_impulse_scale");
	m_fEffectiveRadius		= pSettings->r_float(section,			"effective_radius");
	m_fMaxPower				= READ_IF_EXISTS(pSettings, r_float, section, "hit_power", 1.f);

	m_eHitTypeBlowout		= ALife::g_tfString2HitType(pSettings->r_string(section, "hit_type"));

	m_zone_flags.set(eIgnoreNonAlive,	pSettings->r_bool(section,	"ignore_nonalive"));
	m_zone_flags.set(eIgnoreSmall,		pSettings->r_bool(section,	"ignore_small"));
	m_zone_flags.set(eIgnoreArtefact,	pSettings->r_bool(section,	"ignore_artefacts"));
	m_zone_flags.set(eVisibleByDetector,pSettings->r_bool(section,	"visible_by_detector"));
	



	//загрузить времена для зоны
	m_StateTime[eZoneStateIdle]			= -1;
	m_StateTime[eZoneStateAwaking]		= pSettings->r_s32(section, "awaking_time");
	m_StateTime[eZoneStateBlowout]		= pSettings->r_s32(section, "blowout_time");
	m_StateTime[eZoneStateAccumulate]	= pSettings->r_s32(section, "accamulate_time");
	
//////////////////////////////////////////////////////////////////////////
	ISpatial*		self				=	smart_cast<ISpatial*> (this);
	if (self)		self->spatial.s_type |=	(STYPE_COLLIDEABLE|STYPE_SHAPE);
//////////////////////////////////////////////////////////////////////////

	LPCSTR sound_str = NULL;
	
	if(pSettings->line_exist(section,"idle_sound")) 
	{
		sound_str = pSettings->r_string(section,"idle_sound");
		m_idle_sound.create(sound_str, st_Effect,sg_SourceType);
	}
	
	if(pSettings->line_exist(section,"accum_sound")) 
	{
		sound_str = pSettings->r_string(section,"accum_sound");
		m_accum_sound.create(sound_str, st_Effect,sg_SourceType);
	}
	if(pSettings->line_exist(section,"awake_sound")) 
	{
		sound_str = pSettings->r_string(section,"awake_sound");
		m_awaking_sound.create(sound_str, st_Effect,sg_SourceType);
	}
	
	if(pSettings->line_exist(section,"blowout_sound")) 
	{
		sound_str = pSettings->r_string(section,"blowout_sound");
		m_blowout_sound.create(sound_str, st_Effect,sg_SourceType);
	}
	
	
	if(pSettings->line_exist(section,"hit_sound")) 
	{
		sound_str = pSettings->r_string(section,"hit_sound");
		m_hit_sound.create(sound_str, st_Effect,sg_SourceType);
	}

	if(pSettings->line_exist(section,"entrance_sound")) 
	{
		sound_str = pSettings->r_string(section,"entrance_sound");
		m_entrance_sound.create(sound_str, st_Effect,sg_SourceType);
	}


	if(pSettings->line_exist(section,"idle_particles")) 
		m_sIdleParticles	= pSettings->r_string(section,"idle_particles");
	if(pSettings->line_exist(section,"blowout_particles")) 
		m_sBlowoutParticles = pSettings->r_string(section,"blowout_particles");

	m_bBlowoutOnce = FALSE;
	if (pSettings->line_exist(section, "blowout_once"))
		m_bBlowoutOnce		= pSettings->r_bool(section,"blowout_once");

	if(pSettings->line_exist(section,"accum_particles")) 
		m_sAccumParticles = pSettings->r_string(section,"accum_particles");

	if(pSettings->line_exist(section,"awake_particles")) 
		m_sAwakingParticles = pSettings->r_string(section,"awake_particles");
	

	if(pSettings->line_exist(section,"entrance_small_particles")) 
		m_sEntranceParticlesSmall = pSettings->r_string(section,"entrance_small_particles");
	if(pSettings->line_exist(section,"entrance_big_particles")) 
		m_sEntranceParticlesBig = pSettings->r_string(section,"entrance_big_particles");

	if(pSettings->line_exist(section,"hit_small_particles")) 
		m_sHitParticlesSmall = pSettings->r_string(section,"hit_small_particles");
	if(pSettings->line_exist(section,"hit_big_particles")) 
		m_sHitParticlesBig = pSettings->r_string(section,"hit_big_particles");

	if(pSettings->line_exist(section,"idle_small_particles")) 
		m_sIdleObjectParticlesBig = pSettings->r_string(section,"idle_big_particles");
	
	if(pSettings->line_exist(section,"idle_big_particles")) 
		m_sIdleObjectParticlesSmall = pSettings->r_string(section,"idle_small_particles");
	
	if(pSettings->line_exist(section,"idle_particles_dont_stop"))
		m_zone_flags.set(eIdleObjectParticlesDontStop, pSettings->r_bool(section,"idle_particles_dont_stop")); 

	if(pSettings->line_exist(section,"postprocess")) 
	{
		m_actor_effector				= xr_new<CZoneEffector>();
		m_actor_effector->LoadCfg		(pSettings->r_string(section,"postprocess"));
	};


	if(pSettings->line_exist(section,"bolt_entrance_particles")) 
	{
		m_sBoltEntranceParticles	= pSettings->r_string(section, "bolt_entrance_particles");
		m_zone_flags.set			(eBoltEntranceParticles, (m_sBoltEntranceParticles.size()!=0));
	}

	if(pSettings->line_exist(section,"blowout_particles_time")) 
	{
		m_dwBlowoutParticlesTime = pSettings->r_u32(section,"blowout_particles_time");
		if (s32(m_dwBlowoutParticlesTime)>m_StateTime[eZoneStateBlowout])	{
			m_dwBlowoutParticlesTime=m_StateTime[eZoneStateBlowout];
			Msg("! ERROR: invalid 'blowout_particles_time' in '%s'",section);
		}
	}
	else
		m_dwBlowoutParticlesTime = 0;

	if(pSettings->line_exist(section,"blowout_light_time")) 
	{
		m_dwBlowoutLightTime = pSettings->r_u32(section,"blowout_light_time");
		if (s32(m_dwBlowoutLightTime)>m_StateTime[eZoneStateBlowout])	{
			m_dwBlowoutLightTime=m_StateTime[eZoneStateBlowout];
			Msg("! ERROR: invalid 'blowout_light_time' in '%s'",section);
		}
	}
	else
		m_dwBlowoutLightTime = 0;

	if(pSettings->line_exist(section,"blowout_sound_time")) 
	{
		m_dwBlowoutSoundTime = pSettings->r_u32(section,"blowout_sound_time");
		if (s32(m_dwBlowoutSoundTime)>m_StateTime[eZoneStateBlowout])	{
			m_dwBlowoutSoundTime=m_StateTime[eZoneStateBlowout];
			Msg("! ERROR: invalid 'blowout_sound_time' in '%s'",section);
		}
	}
	else
		m_dwBlowoutSoundTime = 0;

	if(pSettings->line_exist(section,"blowout_explosion_time"))	{
		m_dwBlowoutExplosionTime = pSettings->r_u32(section,"blowout_explosion_time"); 
		if (s32(m_dwBlowoutExplosionTime)>m_StateTime[eZoneStateBlowout])	{
			m_dwBlowoutExplosionTime=m_StateTime[eZoneStateBlowout];
			Msg("! ERROR: invalid 'blowout_explosion_time' in '%s'",section);
		}
	}
	else
		m_dwBlowoutExplosionTime = 0;

	m_zone_flags.set(eBlowoutWind,  pSettings->r_bool(section,"blowout_wind"));
	if( m_zone_flags.test(eBlowoutWind) ){
		m_dwBlowoutWindTimeStart = pSettings->r_u32(section,"blowout_wind_time_start"); 
		m_dwBlowoutWindTimePeak = pSettings->r_u32(section,"blowout_wind_time_peak"); 
		m_dwBlowoutWindTimeEnd = pSettings->r_u32(section,"blowout_wind_time_end"); 
		R_ASSERT(m_dwBlowoutWindTimeStart < m_dwBlowoutWindTimePeak);
		R_ASSERT(m_dwBlowoutWindTimePeak < m_dwBlowoutWindTimeEnd);

		if((s32)m_dwBlowoutWindTimeEnd < m_StateTime[eZoneStateBlowout]){
			m_dwBlowoutWindTimeEnd =u32( m_StateTime[eZoneStateBlowout]-1);
			Msg("! ERROR: invalid 'blowout_wind_time_end' in '%s'",section);
		}

		
		m_fBlowoutWindPowerMax = pSettings->r_float(section,"blowout_wind_power");
	}

	//загрузить параметры световой вспышки от взрыва
	m_zone_flags.set(eBlowoutLight, pSettings->r_bool (section, "blowout_light"));

	if(m_zone_flags.test(eBlowoutLight) ){
		sscanf(pSettings->r_string(section,"light_color"), "%f,%f,%f", &m_LightColor.r, &m_LightColor.g, &m_LightColor.b);
		m_fLightRange			= pSettings->r_float(section,"light_range");
		m_fLightTime			= pSettings->r_float(section,"light_time");
		m_fLightTimeLeft		= 0;

		m_fLightHeight		= pSettings->r_float(section,"light_height");
	}

	//загрузить параметры idle подсветки
	m_zone_flags.set(eIdleLight,	pSettings->r_bool (section, "idle_light"));
	if( m_zone_flags.test(eIdleLight) )
	{
		m_fIdleLightRange = pSettings->r_float(section,"idle_light_range");
		m_fIdleLightRangeDelta = pSettings->r_float(section,"idle_light_range_delta");
		LPCSTR light_anim = pSettings->r_string(section,"idle_light_anim");
		m_pIdleLAnim	 = LALib.FindItem(light_anim);
		m_fIdleLightHeight = pSettings->r_float(section,"idle_light_height");

		//volumetric
		m_zone_flags.set(eIdleLightVolumetric, READ_IF_EXISTS(pSettings, r_bool, section, "light_volumetric", false));

		volumetric_distance = READ_IF_EXISTS(pSettings, r_float, section, "volumetric_distance", 0.80f);
		volumetric_intensity = READ_IF_EXISTS(pSettings, r_float, section, "volumetric_intensity", 0.40f);
		volumetric_quality = READ_IF_EXISTS(pSettings, r_float, section, "volumetric_quality", 1.0f);
	}


	//пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ
	m_zone_flags.set(eSpawnBlowoutArtefacts,	pSettings->r_bool(section,"spawn_blowout_artefacts"));
	if(m_zone_flags.test(eSpawnBlowoutArtefacts) )
	{
		//Msg("spawn_blowout_artefacts, %s", section);

		m_fArtefactSpawnProbability		= READ_IF_EXISTS(pSettings, r_float, section,	"artefact_spawn_probability", 0.0f);
		m_iArtSpawnCicles				= READ_IF_EXISTS(pSettings, r_u8, section,		"artefact_spawn_cicles",2);
		//m_fArtefactSpawnProbability = 1.0;
		//m_fArtefactSpawnProbability	= pSettings->r_float (section,"artefact_spawn_probability");
		if(pSettings->line_exist(section,"artefact_spawn_particles")) 
			m_sArtefactSpawnParticles = pSettings->r_string(section,"artefact_spawn_particles");
		else
			m_sArtefactSpawnParticles = NULL;

		if(pSettings->line_exist(section,"artefact_born_sound"))
		{
			sound_str = pSettings->r_string(section,"artefact_born_sound");
			m_ArtefactBornSound.create(sound_str, st_Effect,sg_SourceType);
		}

		deathSpawnReenableDelay_		= READ_IF_EXISTS(pSettings, r_u16, section, "death_art_spawn_delay", 1);

		m_fThrowOutPower				= READ_IF_EXISTS(pSettings, r_float, section, "throw_out_power", 0.0f);
		//m_fThrowOutPower = pSettings->r_float (section,			"throw_out_power");
		m_fArtefactSpawnHeight			= READ_IF_EXISTS(pSettings, r_float, section, "artefact_spawn_height", 0.0f);
		//m_fArtefactSpawnHeight = pSettings->r_float (section,	"artefact_spawn_height");

	//	LPCSTR						l_caParameters = pSettings->r_string(section, "artefacts");
		LPCSTR l_caParameters			= READ_IF_EXISTS(pSettings, r_string, section, "artefacts", "");
		u16 m_wItemCount			= (u16)_GetItemCount(l_caParameters);
		R_ASSERT2					(!(m_wItemCount & 1),"Invalid number of parameters in string 'artefacts' in the 'system.ltx'!");
		m_wItemCount				>>= 1;

		m_ArtefactSpawn.clear();
		string512 l_caBuffer;

		float total_probability = 0.f;

		m_ArtefactSpawn.resize(m_wItemCount);
		for (u16 i=0; i<m_wItemCount; ++i) 
		{
			ARTEFACT_SPAWN& artefact_spawn = m_ArtefactSpawn[i];
			artefact_spawn.section = _GetItem(l_caParameters,i << 1,l_caBuffer);
			artefact_spawn.probability = (float)atof(_GetItem(l_caParameters,(i << 1) | 1,l_caBuffer));
			total_probability += artefact_spawn.probability;
		}

		if (total_probability == 0.f) total_probability = 1.0;
		R_ASSERT3(!fis_zero(total_probability), "The probability of artefact spawn is zero!",*ObjectName());
		//пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ
		for(u16 i = 0; i < m_ArtefactSpawn.size(); ++i)
		{
			m_ArtefactSpawn[i].probability = m_ArtefactSpawn[i].probability/total_probability;
		}
		//Msg("%s anomaly,m_fArtefactSpawnProbability %f,m_sArtefactSpawnParticles %s,sound_str %s,m_fThrowOutPower %f,m_fArtefactSpawnHeight %f,m_ArtefactSpawn %u", section, m_fArtefactSpawnProbability, m_sArtefactSpawnParticles, sound_str, m_fThrowOutPower, m_fArtefactSpawnHeight, m_ArtefactSpawn.size());
	}
	else{ //Msg("spawn_blowout_artefacts == no, %s", section); 
	}
	bool use = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_secondary_hit", false);
	m_zone_flags.set(eUseSecondaryHit, use);
	if(use)
		m_fSecondaryHitPower	= pSettings->r_float(section,"secondary_hit_power");

	m_ef_anomaly_type			= pSettings->r_u32(section,"ef_anomaly_type");
	m_ef_weapon_type			= pSettings->r_u32(section,"ef_weapon_type");
	
	m_zone_flags.set			(eAffectPickDOF, pSettings->r_bool (section, "pick_dof_effector"));
}

BOOL CCustomZone::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	if (!inherited::SpawnAndImportSOData(data_containing_so))
		return					(FALSE);

	CSE_Abstract				*e = (CSE_Abstract*)(data_containing_so);
	CSE_ALifeCustomZone			*Z = smart_cast<CSE_ALifeCustomZone*>(e);
	VERIFY						(Z);
	
	m_fMaxPower					= pSettings->r_float(SectionName(),"max_start_power");
	m_fAttenuation				= pSettings->r_float(SectionName(),"attenuation");
	m_owner_id					= Z->m_owner_id;
	if(m_owner_id != u32(-1))
		m_ttl					= EngineTimeU() + 40000;// 40 sec
	else
		m_ttl					= u32(-1);

	m_TimeToDisable				= Z->m_disabled_time*1000;
	m_TimeToEnable				= Z->m_enabled_time*1000;
	m_TimeShift					= Z->m_start_time_shift*1000;
	m_StartTime					= EngineTimeU();
	m_zone_flags.set			(eUseOnOffTime,	(m_TimeToDisable!=0)&&(m_TimeToEnable!=0) );

	//добавить источники света

	if ( m_zone_flags.test(eIdleLight))
	{
		m_pIdleLight = ::Render->light_create();
		
		m_pIdleLight->set_shadow(true);
	}
	else
		m_pIdleLight = NULL;

	if (m_zone_flags.test(eIdleLightVolumetric))
	{
		m_pIdleLight->set_volumetric(true);
		m_pIdleLight->set_volumetric_distance(volumetric_distance);
		m_pIdleLight->set_volumetric_intensity(volumetric_intensity);
		m_pIdleLight->set_volumetric_quality(volumetric_quality);
	}

	if (m_zone_flags.test(eBlowoutLight)) 
	{
		m_pLight = ::Render->light_create();
		
		m_pLight->set_shadow(gameObjectLightShadows_ == TRUE ? true : false);
	}else
		m_pLight = NULL;

	setEnabled					(TRUE);

	PlayIdleParticles			();

	m_iPreviousStateTime		= m_iStateTime = 0;

	m_dwLastTimeMoved			= EngineTimeU();
	m_vPrevPos.set				(Position());


	if(spawn_ini() && spawn_ini()->line_exist("fast_mode","always_fast"))
	{
		m_zone_flags.set(eAlwaysFastmode, spawn_ini()->r_bool("fast_mode","always_fast"));
	}
	return						(TRUE);
}

void CCustomZone::DestroyClientObj() 
{
	StopIdleParticles		();

	inherited::DestroyClientObj();

	StopWind				();

	m_pLight.destroy		();
	m_pIdleLight.destroy	();

	DestroyParticleInstance(m_pIdleParticles);

	if(m_actor_effector)			
		m_actor_effector->Stop		(); 

	OBJECT_INFO_VEC_IT i = m_ObjectInfoMap.begin();
	OBJECT_INFO_VEC_IT e = m_ObjectInfoMap.end();
	for(;e!=i;++i)
		exit_Zone(*i);
	m_ObjectInfoMap.clear();	
}

bool CCustomZone::IdleState()
{
	UpdateOnOffState	();

	return false;
}

bool CCustomZone::AwakingState()
{
	if(m_iStateTime>=m_StateTime[eZoneStateAwaking])
	{
		SwitchZoneState(eZoneStateBlowout);
		return true;
	}
	return false;
}

bool CCustomZone::BlowoutState()
{
	if(m_iStateTime>=m_StateTime[eZoneStateBlowout])
	{
		SwitchZoneState(eZoneStateAccumulate);
		if (m_bBlowoutOnce){
			ZoneDisable();
		}
		
		return true;
	}
	return false;
}
bool CCustomZone::AccumulateState()
{
	if(m_iStateTime>=m_StateTime[eZoneStateAccumulate])
	{
		if(m_zone_flags.test(eZoneIsActive) )
			SwitchZoneState(eZoneStateBlowout);
		else
			SwitchZoneState(eZoneStateIdle);

		return true;
	}
	return false;
}
void CCustomZone::UpdateWorkload	(u32 dt)
{
	m_iPreviousStateTime	= m_iStateTime;
	m_iStateTime			+= (int)dt;

	if (!IsEnabled())		{
		if (m_actor_effector)
			m_actor_effector->Stop();
		return;
	};

	UpdateIdleLight			();

	switch(m_eZoneState)
	{
	case eZoneStateIdle:
		IdleState();
		break;
	case eZoneStateAwaking:
		AwakingState();
		break;
	case eZoneStateBlowout:
		BlowoutState();
		break;
	case eZoneStateAccumulate:
		AccumulateState();
		break;
	case eZoneStateDisabled:
		break;
	default: NODEFAULT;
	}

	if (Level().CurrentEntity()) 
	{
		Fvector P			= Device.vCameraPosition;
		P.y					-= 0.9f;
		float radius		= 1.0f;
		CalcDistanceTo		(P, m_fDistanceToCurEntity, radius);

		if (m_actor_effector)
		{
			m_actor_effector->Update(m_fDistanceToCurEntity, radius, m_eHitTypeBlowout);
		}
	}

	if(m_pLight && m_pLight->get_active())
		UpdateBlowoutLight	();

	if(m_zone_flags.test(eUseSecondaryHit) && m_eZoneState!=eZoneStateIdle && m_eZoneState!=eZoneStateDisabled)
	{
		UpdateSecondaryHit();
	}
}

// called only in "fast-mode"
void CCustomZone::UpdateCL() 
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif
	

	inherited::UpdateCL();

	if (m_zone_flags.test(eFastMode))
		UpdateWorkload(TimeDeltaU());	

	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_CustomZone_ += measure_updatecl.GetElapsed_sec();
#endif
}

// called as usual
void CCustomZone::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	m_zone_flags.set(eZoneIsActive, FALSE);

	if (IsEnabled())
	{
		const Fsphere& s		= CFORM()->getSphere();
		Fvector					P;
		XFORM().transform_tiny	(P,s.P);

		// update
		feel_touch_update		(P,s.R);

		//пройтись по всем объектам в зоне
		//и проверить их состояние
		for(OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin(); 
			m_ObjectInfoMap.end() != it; ++it) 
		{
			CGameObject* pObject		= (*it).object;
			if (!pObject)				continue;
			SZoneObjectInfo& info		= (*it);

			info.dw_time_in_zone += dt;

			if((!info.small_object && m_iDisableHitTime != -1 && (int)info.dw_time_in_zone > m_iDisableHitTime) ||
				(info.small_object && m_iDisableHitTimeSmall != -1 && (int)info.dw_time_in_zone > m_iDisableHitTimeSmall))
			{
				if (ShouldIgnoreObject(pObject))
				{
					info.zone_ignore = true;
				}
			}
			if(m_iDisableIdleTime != -1 && (int)info.dw_time_in_zone > m_iDisableIdleTime)
			{
				if (ShouldIgnoreObject(pObject))
				{
					StopObjectIdleParticles( pObject );
				}
			}

			//если есть хотя бы один не дисабленый объект, то
			//зона считается активной
			if(info.zone_ignore == false) 
				m_zone_flags.set(eZoneIsActive,TRUE);
		}

		if(eZoneStateIdle ==  m_eZoneState)
			CheckForAwaking();

		inherited::ScheduledUpdate(dt);

		// check "fast-mode" border
		float	cam_distance	= Device.vCameraPosition.distance_to(P)-s.R;
		
		if (cam_distance>FASTMODE_DISTANCE && !m_zone_flags.test(eAlwaysFastmode) )	
			o_switch_2_slow	();
		else									
			o_switch_2_fast	();

		if (!m_zone_flags.test(eFastMode))
			UpdateWorkload	(dt);

	};

	UpdateOnOffState	();

	
#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_CustomZone_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CCustomZone::CheckForAwaking()
{
	if(m_zone_flags.test(eZoneIsActive) && eZoneStateIdle ==  m_eZoneState)
		SwitchZoneState(eZoneStateAwaking);
}

void CCustomZone::feel_touch_new	(CObject* O) 
{
//	if(smart_cast<CActor*>(O) && O == Level().CurrentEntity())
//					m_pLocalActor	= smart_cast<CActor*>(O);

	CGameObject*	pGameObject		= smart_cast<CGameObject*>(O);
	CEntityAlive*	pEntityAlive	= smart_cast<CEntityAlive*>(pGameObject);
	CArtefact*		pArtefact		= smart_cast<CArtefact*>(pGameObject);
	
	SZoneObjectInfo object_info		;
	object_info.object = pGameObject;

	if(pEntityAlive && pEntityAlive->g_Alive())
		object_info.nonalive_object = false;
	else
		object_info.nonalive_object = true;

	if(pGameObject->Radius()<SMALL_OBJECT_RADIUS)
		object_info.small_object = true;
	else
		object_info.small_object = false;

	if((object_info.small_object && m_zone_flags.test(eIgnoreSmall)) ||
		(object_info.nonalive_object && m_zone_flags.test(eIgnoreNonAlive)) || 
		(pArtefact && m_zone_flags.test(eIgnoreArtefact)))
		object_info.zone_ignore = true;
	else
		object_info.zone_ignore = false;
	enter_Zone(object_info);
	m_ObjectInfoMap.push_back(object_info);
	
	if (IsEnabled())
	{
		PlayEntranceParticles(pGameObject);
		PlayObjectIdleParticles(pGameObject);
	}
};

void CCustomZone::feel_touch_delete(CObject* O) 
{
	CGameObject* pGameObject =smart_cast<CGameObject*>(O);
	if(!pGameObject->getDestroy())
	{
		StopObjectIdleParticles(pGameObject);
	}

	OBJECT_INFO_VEC_IT it = std::find(m_ObjectInfoMap.begin(),m_ObjectInfoMap.end(),pGameObject);
	if(it!=m_ObjectInfoMap.end())
	{	
		exit_Zone(*it);
		m_ObjectInfoMap.erase(it);
	}
}

BOOL CCustomZone::feel_touch_contact(CObject* O) 
{
	if (smart_cast<CCustomZone*>(O))				return FALSE;
	if (smart_cast<CBreakableObject*>(O))			return FALSE;
	if (0==smart_cast<IKinematics*>(O->Visual()))	return FALSE;

	if (O->ID() == ID())
		return		(FALSE);

	CGameObject *object = smart_cast<CGameObject*>(O);
    if (!object || !object->IsVisibleForZones())
		return		(FALSE);

	if (!((CCF_Shape*)CFORM())->Contact(O))
		return		(FALSE);

	return			(object->feel_touch_on_contact(this));
}


float CCustomZone::RelativePower(float dist, float nearest_shape_radius)
{
	float radius = effective_radius(nearest_shape_radius);
	float power = (radius<dist) ? 0 : (1.f - m_fAttenuation*(dist/radius)*(dist/radius));
	return (power<0.0f) ? 0.0f : power;
}

float CCustomZone::effective_radius(float nearest_shape_radius)
{
	return /*Radius()*/nearest_shape_radius*m_fEffectiveRadius;
}

float CCustomZone::Power(float dist, float nearest_shape_radius) 
{
	return  m_fMaxPower * RelativePower(dist, nearest_shape_radius);
}

void CCustomZone::PlayIdleParticles(bool bIdleLight)
{
	m_idle_sound.play_at_pos(0, Position(), true);

	if(*m_sIdleParticles)
	{
		if (!m_pIdleParticles)
		{
			m_pIdleParticles = CParticlesObject::Create(m_sIdleParticles.c_str(),FALSE);
			m_pIdleParticles->UpdateParent(XFORM(),zero_vel);
		
			m_pIdleParticles->UpdateParent(XFORM(),zero_vel);
			m_pIdleParticles->Play(false);
		}
	}
	if(bIdleLight)
		StartIdleLight	();
}

void CCustomZone::StopIdleParticles(bool bIdleLight)
{
	m_idle_sound.stop();

	if(m_pIdleParticles)
	{
		m_pIdleParticles->Stop(FALSE);
		DestroyParticleInstance(m_pIdleParticles);
	}

	if(bIdleLight)
		StopIdleLight();
}


void  CCustomZone::StartIdleLight	()
{
	if(m_pIdleLight)
	{
		m_pIdleLight->set_range(m_fIdleLightRange);
		Fvector pos = Position();
		pos.y += m_fIdleLightHeight;
		m_pIdleLight->set_position(pos);
		m_pIdleLight->set_active(true);
	}
}
void  CCustomZone::StopIdleLight	()
{
	if(m_pIdleLight)
		m_pIdleLight->set_active(false);
}

void CCustomZone::UpdateIdleLight	()
{
	if(!m_pIdleLight || !m_pIdleLight->get_active())
		return;


	VERIFY(m_pIdleLAnim);

	int frame = 0;
	u32 clr					= m_pIdleLAnim->CalculateBGR(EngineTime(),frame); // возвращает в формате BGR
	Fcolor					fclr;
	fclr.set				((float)color_get_B(clr)/255.f,(float)color_get_G(clr)/255.f,(float)color_get_R(clr)/255.f,1.f);
	
	float range = m_fIdleLightRange + m_fIdleLightRangeDelta*::Random.randF(-1.f,1.f);
	m_pIdleLight->set_range	(range);
	m_pIdleLight->set_color	(fclr);

	Fvector pos		= Position();
	pos.y			+= m_fIdleLightHeight;
	m_pIdleLight->set_position(pos);
}


void CCustomZone::PlayBlowoutParticles()
{
	if(!m_sBlowoutParticles) return;

	CParticlesObject* pParticles;
	pParticles	= CParticlesObject::Create(*m_sBlowoutParticles,TRUE);
	pParticles->UpdateParent(XFORM(),zero_vel);
	pParticles->Play(false);
}

void CCustomZone::PlayHitParticles(CGameObject* pObject)
{
	m_hit_sound.play_at_pos(0, pObject->Position());

	shared_str particle_str = NULL;

	if(pObject->Radius()<SMALL_OBJECT_RADIUS)
	{
		if(!m_sHitParticlesSmall) return;
		particle_str = m_sHitParticlesSmall;
	}
	else
	{
		if(!m_sHitParticlesBig) return;
		particle_str = m_sHitParticlesBig;
	}

	if( particle_str.size() )
	{
		CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
		if (PP){
			u16 play_bone = PP->GetRandomBone(); 
			if (play_bone!=BI_NONE)
				PP->StartParticles	(particle_str,play_bone,Fvector().set(0,1,0), ID());
		}
	}
}
#include "bolt.h"
void CCustomZone::PlayEntranceParticles(CGameObject* pObject)
{
	m_entrance_sound.play_at_pos		(0, pObject->Position());

	LPCSTR particle_str				= NULL;

	if(pObject->Radius()<SMALL_OBJECT_RADIUS)
	{
		if(!m_sEntranceParticlesSmall) 
			return;

		particle_str = m_sEntranceParticlesSmall.c_str();
	}
	else
	{
		if(!m_sEntranceParticlesBig) 
			return;

		particle_str				= m_sEntranceParticlesBig.c_str();
	}

	Fvector							vel;
	CPhysicsShellHolder* shell_holder=smart_cast<CPhysicsShellHolder*>(pObject);
	if(shell_holder)
		shell_holder->PHGetLinearVell(vel);
	else 
		vel.set						(0,0,0);
	
	//выбрать случайную косточку на объекте
	CParticlesPlayer* PP			= smart_cast<CParticlesPlayer*>(pObject);
	if (PP)
	{
		u16 play_bone				= PP->GetRandomBone(); 
		
		if (play_bone!=BI_NONE)
		{
			CParticlesObject* pParticles = CParticlesObject::Create(particle_str, TRUE);
			Fmatrix					xform;
			Fvector					dir;
			if(fis_zero				(vel.magnitude()))
				dir.set				(0,1,0);
			else
			{
				dir.set				(vel);
				dir.normalize		();
			}

			PP->MakeXFORM			(pObject,play_bone,dir,Fvector().set(0,0,0),xform);
			pParticles->UpdateParent(xform, vel);
			pParticles->Play		(false);
		}
	}
	if(m_zone_flags.test(eBoltEntranceParticles) && smart_cast<CBolt*>(pObject))
		PlayBoltEntranceParticles();
}

void CCustomZone::PlayBoltEntranceParticles()
{

	CCF_Shape* Sh		= (CCF_Shape*)CFORM();
	const Fmatrix& XF	= XFORM();
	Fmatrix				PXF;
	xr_vector<CCF_Shape::shape_def>& Shapes = Sh->Shapes();
	Fvector				sP0, sP1, vel;

	CParticlesObject* pParticles = NULL;

	xr_vector<CCF_Shape::shape_def>::iterator it = Shapes.begin();
	xr_vector<CCF_Shape::shape_def>::iterator it_e = Shapes.end();
	
	for(;it!=it_e;++it)
	{
		CCF_Shape::shape_def& s = *it;
		switch (s.type)
		{
		case 0: // sphere
			{
			sP0						= s.data.sphere.P;
			XF.transform_tiny		(sP0);

			
			
			float ki				= 10.0f * s.data.sphere.R;
			float c					= 2.0f * s.data.sphere.R;

			float quant_h			= (PI_MUL_2/float(ki))*c;
			float quant_p			= (PI_DIV_2/float(ki));

			for(float i=0; i<ki; ++i)
			{
				vel.setHP				(	::Random.randF(quant_h/2.0f, quant_h)*i, 
											::Random.randF(quant_p/2.0f, quant_p)*i
										);

				vel.mul					(s.data.sphere.R);

				sP1.add					(sP0, vel);

				PXF.identity			();
				PXF.k.normalize			(vel);
				Fvector::generate_orthonormal_basis(PXF.k, PXF.j, PXF.i);

				PXF.c					= sP1;

				pParticles				= CParticlesObject::Create(m_sBoltEntranceParticles.c_str(), TRUE);
				pParticles->UpdateParent(PXF,vel);
				pParticles->Play		(false);
			}
			}break;
		case 1: // box
			break;
		}
	}

}

void CCustomZone::PlayBulletParticles(Fvector& pos)
{
	m_entrance_sound.play_at_pos(0, pos);

	if(!m_sEntranceParticlesSmall) return;
	
	CParticlesObject* pParticles;
	pParticles = CParticlesObject::Create(*m_sEntranceParticlesSmall,TRUE);
	
	Fmatrix M;
	M = XFORM();
	M.c.set(pos);

	pParticles->UpdateParent(M,zero_vel);
	pParticles->Play(false);
}

void CCustomZone::PlayObjectIdleParticles(CGameObject* pObject)
{
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if(!PP) return;

	shared_str particle_str = NULL;

	//разные партиклы для объектов разного размера
	if(pObject->Radius()<SMALL_OBJECT_RADIUS)
	{
		if(!m_sIdleObjectParticlesSmall) return;
		particle_str = m_sIdleObjectParticlesSmall;
	}
	else
	{
		if(!m_sIdleObjectParticlesBig) return;
		particle_str = m_sIdleObjectParticlesBig;
	}

	
	//запустить партиклы на объекте
	//. new
	PP->StopParticles (particle_str, BI_NONE, true);

	PP->StartParticles (particle_str, Fvector().set(0,1,0), ID());
	if (!IsEnabled())
		PP->StopParticles	(particle_str, BI_NONE, true);
}

void CCustomZone::StopObjectIdleParticles(CGameObject* pObject)
{
	if (m_zone_flags.test(eIdleObjectParticlesDontStop) && !pObject->cast_actor())
		return;

	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if(!PP) return;


	OBJECT_INFO_VEC_IT it	= std::find(m_ObjectInfoMap.begin(),m_ObjectInfoMap.end(),pObject);
	if(m_ObjectInfoMap.end() == it) return;
	
	
	shared_str particle_str = NULL;
	//разные партиклы для объектов разного размера
	if(pObject->Radius()<SMALL_OBJECT_RADIUS)
	{
		if(!m_sIdleObjectParticlesSmall) return;
		particle_str = m_sIdleObjectParticlesSmall;
	}
	else
	{
		if(!m_sIdleObjectParticlesBig) return;
		particle_str = m_sIdleObjectParticlesBig;
	}

	PP->StopParticles	(particle_str, BI_NONE, true);
}

void	CCustomZone::Hit					(SHit* pHDS)
{
	Fmatrix M;
	M.identity();
	M.translate_over	(pHDS->p_in_bone_space);
	M.mulA_43			(XFORM());
	PlayBulletParticles	(M.c);	
}

void CCustomZone::StartBlowoutLight		()
{
	if(!m_pLight || m_fLightTime<=0.f) return;
	
	m_fLightTimeLeft = EngineTime() + m_fLightTime*1000.0f;

	m_pLight->set_color(m_LightColor.r, m_LightColor.g, m_LightColor.b);
	m_pLight->set_range(m_fLightRange);
	
	Fvector pos = Position();
	pos.y		+= m_fLightHeight;
	m_pLight->set_position(pos);
	m_pLight->set_active(true);

}

void  CCustomZone::StopBlowoutLight		()
{
	m_fLightTimeLeft = 0.f;
	m_pLight->set_active(false);
}

void CCustomZone::UpdateBlowoutLight	()
{
	if(m_fLightTimeLeft > EngineTime())
	{
		float time_k	= m_fLightTimeLeft - EngineTime();

//		m_fLightTimeLeft -= TimeDelta();
		clamp(time_k, 0.0f, m_fLightTime*1000.0f);

		float scale		= time_k/(m_fLightTime*1000.0f);
		scale			= powf(scale+EPS_L, 0.15f);
		float r			= m_fLightRange*scale;
		VERIFY(_valid(r));
		m_pLight->set_color(m_LightColor.r*scale, 
							m_LightColor.g*scale, 
							m_LightColor.b*scale);
		m_pLight->set_range(r);

		Fvector pos			= Position();
		pos.y				+= m_fLightHeight;
		m_pLight->set_position(pos);
	}
	else
		StopBlowoutLight ();
}

void CCustomZone::AffectObjects()
{
	if(m_dwAffectFrameNum == CurrentFrame())	
		return;

	m_dwAffectFrameNum	= CurrentFrame();

	if(Device.dwPrecacheFrame)					
		return;


	OBJECT_INFO_VEC_IT it;
	for(it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it) 
	{
		if (!(*it).object->getDestroy()){
			Affect(&(*it));


			CEntityAlive* EA = smart_cast<CEntityAlive*>((*it).object);
			if (EA)
			{
				//Msg("%s affects %s", Name(), (*it).object->Name());

				if (EA->g_Alive()){
					// пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅпїЅпїЅ, пїЅпїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅ
					bool found = false;
					for (u32 i = 0; i < affectedgameobjects.size(); ++i)
					{
						if (EA == affectedgameobjects[i])
						{
							//Msg("%s: %s is in list alredy", Name(), (*it).object->Name());
							found = true;
						}
					}
					if (!found){
						//Msg("%s puts %s in list", Name(), (*it).object->Name());
						affectedgameobjects.push_back(EA);
					}
				}
				else if (!EA->g_Alive()){
					// пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅ, пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅ
					bool found = false;
					for (u32 i = 0; i < affectedgameobjects.size(); ++i)
					{
						if (EA == affectedgameobjects[i])
						{
							//Msg("%s -> %s is in list so lets clear list and let af spawn", Name(), (*it).object->Name());
							found = true;
						}
					}
					if (found){
						//Msg("%s -> %s let af to spawn", Name(), (*it).object->Name());
						SpawnArtefactWhenBlewOut = true;
						affectedgameobjects.clear();
					}
				}
			}
		}
	}
}

void CCustomZone::UpdateBlowout()
{
	if(m_dwBlowoutParticlesTime>=(u32)m_iPreviousStateTime && 
		m_dwBlowoutParticlesTime<(u32)m_iStateTime)
		PlayBlowoutParticles();

	if(m_dwBlowoutLightTime>=(u32)m_iPreviousStateTime && 
		m_dwBlowoutLightTime<(u32)m_iStateTime)
		StartBlowoutLight ();

	if(m_dwBlowoutSoundTime>=(u32)m_iPreviousStateTime && 
		m_dwBlowoutSoundTime<(u32)m_iStateTime)
		m_blowout_sound.play_at_pos	(0, Position());

	if(m_zone_flags.test(eBlowoutWind) && m_dwBlowoutWindTimeStart>=(u32)m_iPreviousStateTime && 
		m_dwBlowoutWindTimeStart<(u32)m_iStateTime)
		StartWind();

	UpdateWind();


	if(m_dwBlowoutExplosionTime>=(u32)m_iPreviousStateTime && 
		m_dwBlowoutExplosionTime<(u32)m_iStateTime)
	{
		AffectObjects();
		BornArtefact();
	}
}

void  CCustomZone::OnMove()
{
	if(m_dwLastTimeMoved == 0)
	{
		m_dwLastTimeMoved = EngineTimeU();
		m_vPrevPos.set(Position());
	}
	else
	{
		float time_delta	= float(EngineTimeU() - m_dwLastTimeMoved)/1000.f;
		m_dwLastTimeMoved	= EngineTimeU();

		Fvector				vel;
			
		if(fis_zero(time_delta))
			vel = zero_vel;
		else
		{
			vel.sub(Position(), m_vPrevPos);
			vel.div(time_delta);
		}

		if (m_pIdleParticles)
			m_pIdleParticles->UpdateParent(XFORM(), vel);

		if(m_pLight && m_pLight->get_active())
			m_pLight->set_position(Position());

		if(m_pIdleLight && m_pIdleLight->get_active())
			m_pIdleLight->set_position(Position());
     }
}

void	CCustomZone::OnEvent (NET_Packet& P, u16 type)
{	
	switch (type)
	{
		case GE_OWNERSHIP_TAKE : 
			{
				u16 id;
                P.r_u16(id);
				OnOwnershipTake(id);
				break;
			} 
         case GE_OWNERSHIP_REJECT : 
			 {
				 u16 id;
                 P.r_u16			(id);
                 CArtefact *artefact = smart_cast<CArtefact*>(Level().Objects.net_Find(id)); 
				 if(artefact)
				 {
					 bool			just_before_destroy = !P.r_eof() && P.r_u8();
					artefact->H_SetParent(NULL,just_before_destroy);
					if (!just_before_destroy)
						ThrowOutArtefact(artefact);
				 }
                 break;
			}
	}
	inherited::OnEvent(P, type);
};
void CCustomZone::OnOwnershipTake(u16 id)
{
	CGameObject* GO  = smart_cast<CGameObject*>(Level().Objects.net_Find(id));  VERIFY(GO);
	if(!smart_cast<CArtefact*>(GO))
	{
		Msg("zone_name[%s] object_name[%s]",ObjectName().c_str(), GO->ObjectName().c_str() );
	}
	CArtefact *artefact = smart_cast<CArtefact*>(Level().Objects.net_Find(id));  VERIFY(artefact);
	artefact->H_SetParent(this);
	
	artefact->setVisible(FALSE);
	artefact->setEnabled(FALSE);

	NET_Packet						P;
	u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
	P.w_u16(id);
	u_EventSend(P);

	//m_SpawnedArtefacts.push_back(artefact);
}

void CCustomZone::OnStateSwitch	(EZoneState new_state)
{
	if (new_state==eZoneStateDisabled)
		Disable();
	else
		Enable();

	if(new_state==eZoneStateAccumulate)
		PlayAccumParticles();

	if(new_state==eZoneStateAwaking)
		PlayAwakingParticles();

	m_eZoneState			= new_state;
	m_iPreviousStateTime	= m_iStateTime = 0;
};

void CCustomZone::SwitchZoneState(EZoneState new_state)
{
	// !!! Just single entry for given state !!!
	OnStateSwitch(new_state);

	m_iPreviousStateTime = m_iStateTime = 0;
}

bool CCustomZone::Enable()
{
	if (IsEnabled()) return false;

	o_switch_2_fast();

	for(OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin(); 
		m_ObjectInfoMap.end() != it; ++it) 
	{
		CGameObject* pObject = (*it).object;
		if (!pObject) continue;
		PlayEntranceParticles(pObject);
		PlayObjectIdleParticles(pObject);
	}
	PlayIdleParticles	();
	return				true;
};

bool CCustomZone::Disable()
{
	if (!IsEnabled()) return false;
	o_switch_2_slow();

	for(OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end()!=it; ++it) 
	{
		CGameObject* pObject = (*it).object;
		if (!pObject) 
			continue;

		StopObjectIdleParticles(pObject);
	}
	StopIdleParticles	();
	if (m_actor_effector)
		m_actor_effector->Stop();

	return false;
};

void CCustomZone::ZoneEnable()
{
	SwitchZoneState(eZoneStateIdle);
};

void CCustomZone::ZoneDisable()
{
	SwitchZoneState(eZoneStateDisabled);
}

bool CCustomZone::ShouldIgnoreObject(CGameObject* pObject)
{
	auto pEntityAlive = smart_cast<CEntityAlive*>(pObject);
	return !pEntityAlive || !pEntityAlive->g_Alive();
}

void CCustomZone::SpawnArtefact()
{
	//пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ
	//пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ
	float rnd = ::Random.randF(.0f,1.f-EPS_L);
	float prob_threshold = 0.f;

	std::size_t i=0;
	for(; i<m_ArtefactSpawn.size(); i++)
	{
		prob_threshold += m_ArtefactSpawn[i].probability;
		if(rnd<prob_threshold) break;
	}
	R_ASSERT(i<m_ArtefactSpawn.size());

#ifdef DEBUG
	Msg("%s Spawning %s", Name(), m_ArtefactSpawn[i].section.c_str());
#endif

	Fvector pos;
	Center(pos);

	Level().spawn_item(*m_ArtefactSpawn[i].section, pos, ai_location().level_vertex_id(), ID());
}


void CCustomZone::BornArtefact()
{
	if (!m_zone_flags.test(eSpawnBlowoutArtefacts) || m_ArtefactSpawn.empty())
		return;
	//Msg("%s BornArtefact(), %f", Name(), m_fArtefactSpawnProbability);

	if (Alife()->GetTotalSpawnedAnomArts() > GameConstants::GetMaxAnomalySpawnedArtefacts())
		return;

	CSE_ALifeDynamicObject* server_obj = Alife()->objects().object(ID());

	R_ASSERT(server_obj);

	CSE_ALifeCustomZone* server_anom = smart_cast<CSE_ALifeCustomZone*>(server_obj);

	R_ASSERT(server_anom);

	if (server_anom->suroundingArtsID_.size() > server_anom->maxSurroundingArtefacts_)
		return;

	if (nextDeathSpawnArtOKTime_ > GetGameTimeInHours())
	{
#ifdef DEBUG
		Msg("^ Death spawn delay");
#endif
		return;
	}

	for (int i = 0; i < m_iArtSpawnCicles; ++i){//tatarinrafa:Lets add an oportunity of spawning several arts
		if (::Random.randF(0.f, 1.f) < m_fArtefactSpawnProbability) {

			//Msg("%s BornArtefact(), Probability went through", Name());
			if (SpawnArtefactWhenBlewOut == true){
			//	Msg("SpawnArtefact");
				SpawnArtefact();
			}
		}
	}
	SpawnArtefactWhenBlewOut = false;
}

void CCustomZone::ThrowOutArtefact(CArtefact* pArtefact)
{
	Fvector pos;
	pos.set(Position());
	pos.y += m_fArtefactSpawnHeight;
	pArtefact->XFORM().c.set(pos);

	if(*m_sArtefactSpawnParticles)
	{
		CParticlesObject* pParticles;
		pParticles = CParticlesObject::Create(*m_sArtefactSpawnParticles,TRUE);
		pParticles->UpdateParent(pArtefact->XFORM(),zero_vel);
		pParticles->Play(false);
	}

	m_ArtefactBornSound.play_at_pos(0, pos);

	pArtefact->ChangePosition(pos);

	Fvector dir;
	dir.random_dir();
	dir.normalize();
	pArtefact->m_pPhysicsShell->applyImpulse (dir, m_fThrowOutPower);

	CSE_ALifeDynamicObject* owner = Alife()->objects().object(ID());

	R_ASSERT(owner);

	CSE_ALifeCustomZone* owner_anomaly = smart_cast<CSE_ALifeCustomZone*>(owner);

	R_ASSERT(owner_anomaly);

	pArtefact->owningAnomalyID_ = owner_anomaly->ID;
	owner_anomaly->suroundingArtsID_.push_back(pArtefact->ID());

#ifdef DEBUG
	Msg("Death spawned server art %u server anom id %u", pArtefact->ID(), owner_anomaly->ID);
#endif

	Alife()->totalSpawnedAnomalyArts_++;

	nextDeathSpawnArtOKTime_ = GetGameTimeInHours() + deathSpawnReenableDelay_;
}

void CCustomZone::StartWind()
{
	if(m_fDistanceToCurEntity>WIND_RADIUS) return;

	m_zone_flags.set(eBlowoutWindActive, TRUE);

	m_fStoreWindPower = g_pGamePersistent->Environment().wind_strength_factor;
	clamp(g_pGamePersistent->Environment().wind_strength_factor, 0.f, 1.f);
}

void CCustomZone::StopWind()
{
	if(!m_zone_flags.test(eBlowoutWindActive)) return;
	m_zone_flags.set(eBlowoutWindActive, FALSE);
	g_pGamePersistent->Environment().wind_strength_factor = m_fStoreWindPower;
}

void CCustomZone::UpdateWind()
{
	if(!m_zone_flags.test(eBlowoutWindActive)) return;

	if(m_fDistanceToCurEntity>WIND_RADIUS || m_dwBlowoutWindTimeEnd<(u32)m_iStateTime)
	{
		StopWind();
		return;
	}

	if(m_dwBlowoutWindTimePeak > (u32)m_iStateTime)
	{
		g_pGamePersistent->Environment().wind_strength_factor = m_fBlowoutWindPowerMax + ( m_fStoreWindPower - m_fBlowoutWindPowerMax)*
								float(m_dwBlowoutWindTimePeak - (u32)m_iStateTime)/
								float(m_dwBlowoutWindTimePeak - m_dwBlowoutWindTimeStart);
		clamp(g_pGamePersistent->Environment().wind_strength_factor, 0.f, 1.f);
	}
	else
	{
		g_pGamePersistent->Environment().wind_strength_factor = m_fBlowoutWindPowerMax + (m_fStoreWindPower - m_fBlowoutWindPowerMax)*
			float((u32)m_iStateTime - m_dwBlowoutWindTimePeak)/
			float(m_dwBlowoutWindTimeEnd - m_dwBlowoutWindTimePeak);
		clamp(g_pGamePersistent->Environment().wind_strength_factor, 0.f, 1.f);
	}
}

u32	CCustomZone::ef_anomaly_type() const
{
	return	(m_ef_anomaly_type);
}

u32	CCustomZone::ef_weapon_type() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

void CCustomZone::CreateHit(u16 id_to,
	u16 id_from,
	const Fvector& hit_dir,
	float hit_power,
	s16 bone_id,
	const Fvector& pos_in_bone,
	float hit_impulse,
	ALife::EHitType hit_type)
{

	if (m_owner_id != u32(-1))
		id_from = (u16)m_owner_id;

	NET_Packet			l_P;
	Fvector hdir = hit_dir;
	SHit Hit = SHit(hit_power, hdir, this, bone_id, pos_in_bone, hit_impulse, hit_type, 0.0f, false);
	Hit.GenHeader(GE_HIT, id_to);
	Hit.whoID = id_from;
	Hit.weaponID = this->ID();
	Hit.Write_Packet(l_P);
	u_EventSend(l_P);
}

void CCustomZone::RemoveLinksToCLObj(CObject* O)
{
	CGameObject* GO				= smart_cast<CGameObject*>(O);
	OBJECT_INFO_VEC_IT it		= std::find(m_ObjectInfoMap.begin(),m_ObjectInfoMap.end(), GO);
	if(it!=m_ObjectInfoMap.end())
	{
		exit_Zone				(*it);
		m_ObjectInfoMap.erase	(it);
	}
	if(GO->ID()==m_owner_id)	m_owner_id = u32(-1);

	if(m_actor_effector && m_actor_effector->m_pActor && m_actor_effector->m_pActor->ID() == GO->ID())
		m_actor_effector->Stop();

	inherited::RemoveLinksToCLObj(O);
}

void CCustomZone::enter_Zone(SZoneObjectInfo& io)
{
	if(m_zone_flags.test(eAffectPickDOF) && Level().CurrentEntity())
	{
		if(io.object->ID()==Level().CurrentEntity()->ID())
			GamePersistent().SetPickableEffectorDOF(true);
	}
}

void CCustomZone::exit_Zone	(SZoneObjectInfo& io)
{
	StopObjectIdleParticles(io.object);

	if(m_zone_flags.test(eAffectPickDOF) && Level().CurrentEntity())
	{
		if(io.object->ID()==Level().CurrentEntity()->ID())
			GamePersistent().SetPickableEffectorDOF(false);
	}
}

void CCustomZone::PlayAccumParticles()
{
	if(m_sAccumParticles.size())
	{
		CParticlesObject* pParticles;
		pParticles	= CParticlesObject::Create(*m_sAccumParticles,TRUE);
		pParticles->UpdateParent(XFORM(),zero_vel);
		pParticles->Play(false);
	}

	if(m_accum_sound._handle())
		m_accum_sound.play_at_pos	(0, Position());
}

void CCustomZone::PlayAwakingParticles()
{
	if(m_sAwakingParticles.size())
	{
		CParticlesObject* pParticles;
		pParticles	= CParticlesObject::Create(*m_sAwakingParticles,TRUE);
		pParticles->UpdateParent(XFORM(),zero_vel);
		pParticles->Play(false);
	}

	if(m_awaking_sound._handle())
		m_awaking_sound.play_at_pos	(0, Position());
}

void CCustomZone::UpdateOnOffState()
{
	if(!m_zone_flags.test(eUseOnOffTime)) return;
	
	bool dest_state;
	u32 t = (EngineTimeU()-m_StartTime+m_TimeShift) % (m_TimeToEnable+m_TimeToDisable);
	if	(t < m_TimeToEnable) 
	{
		dest_state=true;
	}else
	if(t >=(m_TimeToEnable+m_TimeToDisable) ) 
	{
		dest_state=true;
	}else
	{
		dest_state=false;
		VERIFY(t<(m_TimeToEnable+m_TimeToDisable));
	}

	if( (eZoneStateDisabled==m_eZoneState) && dest_state)
	{
		GoEnabledState		();
	}
	else if( (eZoneStateIdle==m_eZoneState) && !dest_state)
	{
		GoDisabledState			();
	}
}

void CCustomZone::GoDisabledState()
{
	//switch to disable	
	OnStateSwitch(eZoneStateDisabled);

	OBJECT_INFO_VEC_IT it		= m_ObjectInfoMap.begin();
	OBJECT_INFO_VEC_IT it_e		= m_ObjectInfoMap.end();

	for(;it!=it_e;++it)
		exit_Zone(*it);
	
	m_ObjectInfoMap.clear		();
	feel_touch.clear			();
}

void CCustomZone::GoEnabledState()
{
	//switch to idle	
	OnStateSwitch(eZoneStateIdle);
}

BOOL CCustomZone::feel_touch_on_contact	(CObject *O)
{
	if ((spatial.s_type | STYPE_VISIBLEFORAI) != spatial.s_type)
		return			(FALSE);

	return				(inherited::feel_touch_on_contact(O));
}

BOOL CCustomZone::AlwaysInUpdateList()
{
	bool b_idle = ZoneState()==eZoneStateIdle || ZoneState()==eZoneStateDisabled;

 	if(!b_idle || (m_zone_flags.test(eAlwaysFastmode) && IsEnabled()) )
		return TRUE;
 	else
 		return inherited::AlwaysInUpdateList();
}

void CCustomZone::CalcDistanceTo(const Fvector& P, float& dist, float& radius)
{
	R_ASSERT			(CFORM()->Type()==cftShape);
	CCF_Shape* Sh		= (CCF_Shape*)CFORM();

	dist				= P.distance_to(Position());
	float sr			= CFORM()->getSphere().R;
	//quick test
	if(Sh->Shapes().size()==1)
	{
		radius			= sr;
		return;
	}
/*
	//2nd quick test
	Fvector				SC;
	float				dist2;
	XF.transform_tiny	(SC,CFORM()->getSphere().P);
	dist2				= P.distance_to(SC);
	if(dist2>sr)
	{
		radius		= sr;
		return;
	}
*/
	//full test
	const Fmatrix& XF	= XFORM();
	xr_vector<CCF_Shape::shape_def>& Shapes = Sh->Shapes();
	CCF_Shape::shape_def* nearest_s = NULL;
	float nearest = flt_max;


	Fvector sP;

	xr_vector<CCF_Shape::shape_def>::iterator it = Shapes.begin();
	xr_vector<CCF_Shape::shape_def>::iterator it_e = Shapes.end();
	for(;it!=it_e;++it)
	{
		CCF_Shape::shape_def& s = *it;
		float d = 0.0f;
		switch (s.type)
		{
		case 0: // sphere
			sP = s.data.sphere.P;
			break;
		case 1: // box
			sP = s.data.box.c;
			break;
		}

		XF.transform_tiny(sP);
		d = P.distance_to(sP);
		if(d<nearest)
		{
			nearest		= d;
			nearest_s	= &s;
		}
	}
	R_ASSERT(nearest_s);
	
	dist	= nearest;

	if(nearest_s->type==0)
		radius	= nearest_s->data.sphere.R;
	else
	{
		float r1 = nearest_s->data.box.i.magnitude();
		float r2 = nearest_s->data.box.j.magnitude();
		float r3 = nearest_s->data.box.k.magnitude();
		radius = _max(r1,r2);
		radius = _max(radius,r3);
	}

}

// Lain: added Start/Stop idle light calls
void CCustomZone::o_switch_2_fast				()
{
	if (m_zone_flags.test(eFastMode))		return	;
	m_zone_flags.set(eFastMode, TRUE);
	StartIdleLight();
	processing_activate			();
}

void CCustomZone::o_switch_2_slow				()
{
	if (!m_zone_flags.test(eFastMode))	return	;
	m_zone_flags.set(eFastMode, FALSE);
	if ( !light_in_slow_mode() )
	{
		StopIdleLight();
	}
	processing_deactivate		();
}

void CCustomZone::save							(NET_Packet &output_packet)
{
	inherited::save			(output_packet);
	output_packet.w_u8		(static_cast<u8>(m_eZoneState));
	output_packet.w_u32		(nextDeathSpawnArtOKTime_); // this parametr is ok to save only for online presence, i guess
}

void CCustomZone::load							(IReader &input_packet)
{
	inherited::load			(input_packet);	

	CCustomZone::EZoneState temp = static_cast<CCustomZone::EZoneState>(input_packet.r_u8());

	if (temp == eZoneStateDisabled)
		m_eZoneState = eZoneStateDisabled;
	else
		m_eZoneState = eZoneStateIdle;

	nextDeathSpawnArtOKTime_ = input_packet.r_u32();
}