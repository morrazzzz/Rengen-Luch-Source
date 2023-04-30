#include "stdafx.h"
#include "ZoneCampfire.h"
#include "ParticlesObject.h"
#include "GamePersistent.h"
#include "../LightAnimLibrary.h"

CZoneCampfire::CZoneCampfire()
:m_pDisabledParticles(NULL),m_pEnablingParticles(NULL),m_turned_on(true),m_turn_time(0)
{
//.	g_zone = this;
}

CZoneCampfire::~CZoneCampfire()
{
	DestroyParticleInstance	(m_pDisabledParticles);
	DestroyParticleInstance	(m_pEnablingParticles);
	m_disabled_sound.destroy	();
}

void CZoneCampfire::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);
}

void CZoneCampfire::GoEnabledState()
{
	inherited::GoEnabledState();
	
	if(m_pDisabledParticles)
	{
		m_pDisabledParticles->Stop	(FALSE);
		DestroyParticleInstance	(m_pDisabledParticles);
	}

	m_disabled_sound.stop		();
	m_disabled_sound.destroy	();

	LPCSTR str						= pSettings->r_string(SectionName(),"enabling_particles");
	m_pEnablingParticles			= CParticlesObject::Create(str,FALSE);
	m_pEnablingParticles->UpdateParent(XFORM(),zero_vel);
	m_pEnablingParticles->Play		(false);
}

void CZoneCampfire::GoDisabledState()
{
	inherited::GoDisabledState		();

	R_ASSERT						(NULL==m_pDisabledParticles);
	LPCSTR str						= pSettings->r_string(SectionName(),"disabled_particles");
	m_pDisabledParticles			= CParticlesObject::Create(str,FALSE);
	m_pDisabledParticles->UpdateParent	(XFORM(),zero_vel);
	m_pDisabledParticles->Play			(false);
	
	
	str = pSettings->r_string		(SectionName(),"disabled_sound");
	m_disabled_sound.create			(str, st_Effect,sg_SourceType);
	m_disabled_sound.play_at_pos	(0, Position(), true);
}

#define OVL_TIME 3000
void CZoneCampfire::turn_on_script()
{
	if( psDeviceFlags.test(rsR2|rsR3|rsR4) )
	{
		m_turn_time				= EngineTimeU()+OVL_TIME;
		m_turned_on				= true;
		GoEnabledState			();
	}
}

void CZoneCampfire::turn_off_script()
{
	if( psDeviceFlags.test(rsR2|rsR3|rsR4) )
	{
		m_turn_time				= EngineTimeU()+OVL_TIME;
		m_turned_on				= false;
		GoDisabledState			();
	}
}

bool CZoneCampfire::is_on()
{
	return m_turned_on;
}

void CZoneCampfire::ScheduledUpdate(u32	dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif
	

	if (!IsEnabled() && m_turn_time)
	{
		UpdateWorkload(dt);
	}

	if(m_pIdleParticles)
	{
		Fvector vel;
		vel.mul(GamePersistent().Environment().wind_blast_direction,GamePersistent().Environment().wind_strength_factor);
		m_pIdleParticles->UpdateParent(XFORM(),vel);
	}

	inherited::ScheduledUpdate(dt);


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_VariousAnomalies_ += measure_sc_update.GetElapsed_sec();
#endif
}


void CZoneCampfire::PlayIdleParticles(bool bIdleLight)
{
	if(m_turn_time==0 || m_turn_time-EngineTimeU()<(OVL_TIME-2000))
	{
		inherited::PlayIdleParticles(bIdleLight);
		if(m_pEnablingParticles)
		{
			m_pEnablingParticles->Stop	(FALSE);
			DestroyParticleInstance	(m_pEnablingParticles);
		}
	}
}

void CZoneCampfire::StopIdleParticles(bool bIdleLight)
{
	if(m_turn_time==0 || m_turn_time-EngineTimeU()<(OVL_TIME-500))
		inherited::StopIdleParticles(bIdleLight);
}

BOOL CZoneCampfire::AlwaysInUpdateList()
{
	if(m_turn_time)
		return TRUE;
	else
		return inherited::AlwaysInUpdateList();
}

void CZoneCampfire::UpdateWorkload(u32 dt)
{
	inherited::UpdateWorkload(dt);
	if(m_turn_time>EngineTimeU())
	{
		float k = float(m_turn_time-EngineTimeU())/float(OVL_TIME);

		if(m_turned_on)
		{
			k = 1.0f-k;
			PlayIdleParticles	(true);
			StartIdleLight		();
		}else
		{
			StopIdleParticles	(false);
		}

		if(m_pIdleLight && m_pIdleLight->get_active())
		{
			VERIFY(m_pIdleLAnim);
			int frame = 0;

			u32 clr = m_pIdleLAnim->CalculateBGR(EngineTime(), frame);

			Fcolor		fclr;
			fclr.set	(	((float)color_get_B(clr)/255.f)*k,
							((float)color_get_G(clr)/255.f)*k,
							((float)color_get_R(clr)/255.f)*k,
							1.f);
			
			float range = m_fIdleLightRange + 0.25f*::Random.randF(-1.f,1.f);
			range	*= k;

			m_pIdleLight->set_range	(range);
			m_pIdleLight->set_color	(fclr);
		}


	}else
	if(m_turn_time)
	{
		m_turn_time = 0;
		if(m_turned_on)
		{
			PlayIdleParticles(true);
		}else
		{
			StopIdleParticles(true);
		}
	}
}