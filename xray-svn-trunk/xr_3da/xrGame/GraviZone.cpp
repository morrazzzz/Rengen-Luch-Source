#include "stdafx.h"

#include "gravizone.h"

#include "../../xrphysics/PhysicsShell.h"
#include "entity_alive.h"
#include "phmovementcontrol.h"
#include "PhysicsShellHolder.h"
#include "Level.h"
#include "CharacterPhysicsSupport.h"
#include "Car.h"

#define ALIVE_PULL_FORCE_MULT		(60.f)

CBaseGraviZone ::CBaseGraviZone (void)
{
	m_dwTeleTime = 0;
}

CBaseGraviZone ::~CBaseGraviZone (void)
{
}

void CBaseGraviZone ::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);

	
	m_fThrowInImpulse		= pSettings->r_float(section,	"throw_in_impulse");//800.f;
	m_fThrowInImpulseAlive	= pSettings->r_float(section,	"throw_in_impulse_alive");//800.f;
	m_fThrowInImpulseCar	= READ_IF_EXISTS(pSettings, r_float, section, "throw_in_impulse_car", m_fThrowInImpulseAlive);
	m_fThrowInAtten			= pSettings->r_float(section,	"throw_in_atten");
	m_fThrowInHitKoefAlive	= READ_IF_EXISTS(pSettings, r_float, section, "throw_in_hit_koef_alive", 1.0f);
	m_fThrowInHitKoefCar	= READ_IF_EXISTS(pSettings, r_float, section, "throw_in_hit_koef_car", 1.0f);
	m_fBlowoutRadiusPercent = pSettings->r_float(section,	"blowout_radius_percent");//0.3f;

	m_fTeleHeight			= pSettings->r_float(section,	"tele_height");//1.5f;
    m_dwTimeToTele			= pSettings->r_u32(section,		"time_to_tele");//7000;
	m_dwTelePause			= pSettings->r_u32(section,		"tele_pause");//1000

	if(pSettings->line_exist(section,	"tele_particles_big"))
		m_sTeleParticlesBig = pSettings->r_string(section,	"tele_particles_big");
	else
		m_sTeleParticlesBig = NULL;
	
	if(pSettings->line_exist(section, "tele_particles_small"))
		m_sTeleParticlesSmall = pSettings->r_string(section, "tele_particles_small");
	else
		m_sTeleParticlesSmall = NULL;
}

BOOL CBaseGraviZone ::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	return inherited::SpawnAndImportSOData(data_containing_so);
}

void CBaseGraviZone ::DestroyClientObj()
{
	Telekinesis().deactivate();
	inherited::DestroyClientObj();
}

void CBaseGraviZone ::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	inherited::ScheduledUpdate(dt);
	Telekinesis().schedule_update();


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_VariousAnomalies_ += measure_sc_update.GetElapsed_sec();
#endif
}


bool  CBaseGraviZone ::BlowoutState()
{
	bool result = inherited::BlowoutState();

	UpdateBlowout();
	AffectObjects();

	return result;
}


bool CBaseGraviZone ::IdleState()
{
	bool result = inherited::IdleState();

	m_dwTeleTime += TimeDeltaU();

	if(!result)
	{
		if(m_dwTeleTime> m_dwTimeToTele)
		{
			for(OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it) 
			{
				CPhysicsShellHolder * GO = smart_cast<CPhysicsShellHolder *>( (*it).object );

				if(GO && GO->PPhysicsShell() && Telekinesis().is_active_object(GO))
				{
					Telekinesis().deactivate(GO);
					StopTeleParticles(GO);
				}
			}
		}
		if(m_dwTeleTime> m_dwTimeToTele + m_dwTelePause)
		{
			m_dwTeleTime = 0;

			for(OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it) 
			{
				CPhysicsShellHolder * GO = smart_cast<CPhysicsShellHolder *>( (*it).object );

				if(GO && GO->PPhysicsShell() && !Telekinesis().is_active_object(GO))
				{
					Telekinesis().activate(GO, 0.1f, m_fTeleHeight, m_dwTimeToTele);
					PlayTeleParticles(GO);
				}
			}
		}
	}
	else
		Telekinesis().deactivate();

	return result;
}

bool CBaseGraviZone::CheckAffectField(CPhysicsShellHolder* GO,float dist_to_radius)
{
	return dist_to_radius>BlowoutRadiusPercent(GO);
}

void CBaseGraviZone ::Affect(SZoneObjectInfo* O) 
{
	CPhysicsShellHolder* GO = smart_cast<CPhysicsShellHolder*>(O->object);
	if(!GO) return;


	//////////////////////////////////////////////////////////////////////////
	//	зат€гиваем объет по направлению к центру зоны

	Fvector					throw_in_dir;
	Fvector					zone_center;
	ThrowInCenter			(zone_center);
	Fvector					go_center;
	GO->Center				(go_center);
	throw_in_dir.sub		(zone_center, go_center);

	float dist = throw_in_dir.magnitude();
	float dist_to_radius = dist/Radius();

	if(!fis_zero(dist))
	{
		throw_in_dir.mul(1.f/dist);
	}
	else throw_in_dir.set(0.f,1.f,0.f);

	if( CheckAffectField(GO,dist_to_radius) && true)
	{
		AffectPull(GO,throw_in_dir,dist);
	}
	else
	{
		//////////////////////////////////////////////////////////////////////////
		// выброс аномалии
		
		//если врем€ выброса еще не пришло
		if(m_dwBlowoutExplosionTime<(u32)m_iPreviousStateTime ||
			m_dwBlowoutExplosionTime>=(u32)m_iStateTime)
		{

			AffectPull(GO,throw_in_dir,BlowoutRadiusPercent(GO)*Radius());	
			return;
		}
		AffectThrow(O,GO,throw_in_dir,dist);
			
	}
}

void CBaseGraviZone ::  ThrowInCenter(Fvector& C)
{
	Center(C);
}
void CBaseGraviZone ::	AffectPull(CPhysicsShellHolder* GO,const Fvector& throw_in_dir,float dist)
{
	CEntityAlive* EA = smart_cast<CEntityAlive*>(GO);	
	if(EA && EA->g_Alive())
	{
		AffectPullAlife(EA,throw_in_dir,dist);
	}
	else if(GO && GO->PPhysicsShell())
	{
		AffectPullDead(GO,throw_in_dir,dist);
	}
}

void CBaseGraviZone::AffectPullAlife(CEntityAlive* EA,const Fvector& throw_in_dir,float dist)
{
	float rel_power = RelativePower(dist, Radius());
	float throw_power = m_fThrowInImpulseAlive * GetPullDeltaPower() * ALIVE_PULL_FORCE_MULT * rel_power * rel_power * rel_power * rel_power * rel_power;

	Fvector vel;
	vel.set(throw_in_dir);
	vel.mul(throw_power);
	EA->character_physics_support()->movement()->AddControlVel(vel);
}

void CBaseGraviZone ::	AffectPullDead(CPhysicsShellHolder* GO,const Fvector& throw_in_dir,float dist)
{
	GO->PPhysicsShell()->applyImpulse(throw_in_dir,dist * m_fThrowInImpulse*GO->GetMass()/100.f);
}

void CBaseGraviZone ::	AffectThrow(SZoneObjectInfo* O, CPhysicsShellHolder* GO,const Fvector& throw_in_dir,float dist)
{
	Fvector position_in_bone_space;

	float power = Power(dist, Radius()); //Power(GO->Position().distance_to(zone_center));
	float impulse = m_fHitImpulseScale*power*GO->GetMass();

	if(power > 0.01f) 
	{
		position_in_bone_space.set(0.f,0.f,0.f);
		CreateHit(GO->ID(),ID(),throw_in_dir,power,0,position_in_bone_space,impulse,m_eHitTypeBlowout);
		PlayHitParticles(GO);
	}
}


void CBaseGraviZone ::PlayTeleParticles(CGameObject* pObject)
{
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if(!PP) return;

	shared_str particle_str = NULL;

	//разные партиклы дл€ объектов разного размера
	if(pObject->Radius()<SMALL_OBJECT_RADIUS)
	{
		if(!m_sTeleParticlesSmall) return;
		particle_str = m_sTeleParticlesSmall;
	}
	else
	{
		if(!m_sTeleParticlesBig) return;
		particle_str = m_sTeleParticlesBig;
	}

	PP->StartParticles(particle_str, Fvector().set(0,1,0), ID());
}

void CBaseGraviZone::StopTeleParticles(CGameObject* pObject)
{
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if(!PP) return;
	shared_str particle_str = NULL;

	//разные партиклы дл€ объектов разного размера
	if(pObject->Radius()<SMALL_OBJECT_RADIUS)
	{
		if(!m_sTeleParticlesSmall) return;
		particle_str = m_sTeleParticlesSmall;
	}
	else
	{
		if(!m_sTeleParticlesBig) return;
		particle_str = m_sTeleParticlesBig;
	}

	//остановить партиклы
	PP->StopParticles	(particle_str, BI_NONE, true);
}

void CBaseGraviZone::RemoveLinksToCLObj(CObject* O)
{
	inherited::RemoveLinksToCLObj(O);
	
	Telekinesis().remove_links(O);
}

void CBaseGraviZone::exit_Zone(SZoneObjectInfo& io)
{
	CPhysicsShellHolder * GO = smart_cast<CPhysicsShellHolder*>(io.object );

	if(GO && GO->PPhysicsShell() && Telekinesis().is_active_object(GO))
	{
		Telekinesis().deactivate(GO);
		StopTeleParticles(GO);
	}

	inherited::exit_Zone(io);
}

bool CBaseGraviZone::ShouldIgnoreObject(CGameObject* pObject)
{
	auto pCar = smart_cast<CCar*>(pObject);
	if (pCar) return false;

	return inherited::ShouldIgnoreObject(pObject);
}
