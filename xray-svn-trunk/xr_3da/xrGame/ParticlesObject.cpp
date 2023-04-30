//----------------------------------------------------
// file: PSObject.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ParticlesObject.h"
#include "../../Include/xrRender/RenderVisual.h"
#include "../../Include/xrRender/ParticleCustom.h"
#include "../render.h"
#include "GamePersistent.h"
#include "level.h"

const Fvector zero_vel		= {0.f,0.f,0.f};

CParticlesObject::CParticlesObject	(LPCSTR p_name, BOOL bAutoRemove, bool destroy_on_game_load) :
	inherited				(destroy_on_game_load)
{
	Init					(p_name,0,bAutoRemove);
}

void CParticlesObject::Init	(LPCSTR p_name, IRender_Sector* S, BOOL bAutoRemove)
{
	m_bDead					= FALSE;
	m_bLooped				= false;
	m_bStopping				= false;
	m_bAutoRemove			= bAutoRemove;
	float time_limit		= 0.0f;

	// create visual
	renderable.visual		= Render->model_CreateParticles(p_name);
	VERIFY					(renderable.visual);
	IParticleCustom* V		= smart_cast<IParticleCustom*>(renderable.visual);  VERIFY(V);
	time_limit				= V->GetTimeLimit();

	if(time_limit > 0.f)
	{
		m_iLifeTime			= iFloor(time_limit*1000.f);
	}
	else
	{
		if(bAutoRemove)
		{
			R_ASSERT3			(!m_bAutoRemove,"Can't set auto-remove flag for looped particle system.",p_name);
		}
		else
		{
			m_iLifeTime = 0;
			m_bLooped = true;
		}
	}


	// spatial
	spatial.s_type			= 0;
	spatial.current_sector	= S;
	
	// sheduled
	shedule.t_min			= 20;
	shedule.t_max			= 50;
	shedule_register		();

	dwLastTime				= EngineTimeU();
}

//----------------------------------------------------
CParticlesObject::~CParticlesObject()
{
	Device.RemoveFromAuxthread5Pool(fastdelegate::FastDelegate0<>(this, &CParticlesObject::PerformAllTheWork_mt)); 
}

void CParticlesObject::UpdateSpatial()
{
	spatial_move();
}

void CParticlesObject::spatial_move_intern()
{
	// spatial	(+ workaround occasional bug inside particle-system)
	vis_data &vis = renderable.visual->getVisData();
	if (_valid(vis.sphere))
	{
		Fvector	P;	float	R;
		renderable.xform.transform_tiny	(P,vis.sphere.P);
		R								= vis.sphere.R;
		if (0==spatial.s_type)	{
			// First 'valid' update - register
			spatial.s_type			= STYPE_RENDERABLE;
			spatial.sphere.set		(P,R);
			spatial_register		();
		} else {
			BOOL	bMove			= FALSE;
			if		(!P.similar(spatial.sphere.P,EPS_L*10.f))		bMove	= TRUE;
			if		(!fsimilar(R,spatial.sphere.R,0.15f))			bMove	= TRUE;
			if		(bMove)			{
				spatial.sphere.set	(P, R);
				inherited::spatial_move_intern();
			}
		}
	}
}

const shared_str CParticlesObject::Name()
{
	IParticleCustom* V	= smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	return (V) ? V->Name() : "";
}

//----------------------------------------------------
void CParticlesObject::Play		(bool bHudMode)
{
	if (m_bCamReady == false) return;

	IParticleCustom* V			= smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	if (bHudMode)
		V->SetHudMode(bHudMode);

	V->Play						();
	dwLastTime					= EngineTimeU()-33ul;

	m_bStopping					= false;
}

void CParticlesObject::PlayStatic(bool bHudMode)
{
	IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	if (bHudMode)
		V->SetHudMode(bHudMode);

	V->Play();
	dwLastTime = EngineTimeU() - 33ul;

	m_bStopping = false;
}

void CParticlesObject::play_at_pos(const Fvector& pos, BOOL xform)
{
	if (m_bCamReady == false) return;

	IParticleCustom* V			= smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	Fmatrix m; m.translate		(pos); 
	V->UpdateParent				(m,zero_vel,xform);
	V->Play						();
	dwLastTime					= EngineTimeU()-33ul;

	m_bStopping					= false;
}

void CParticlesObject::Stop		(BOOL bDefferedStop)
{
	IParticleCustom* V			= smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	V->Stop						(bDefferedStop);
	m_bStopping					= true;
}

float CParticlesObject::shedule_Scale()
{
	return Device.vCameraPosition.distance_to(Position()) / 200.f;
}

void CParticlesObject::ScheduledUpdate(u32 _dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif

	inherited::ScheduledUpdate(_dt);

#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_ParticleObject_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CParticlesObject::Update()
{
	if (m_bDead)
		return;

	if (psDeviceFlags.test(mtParticles))
	{
		fastdelegate::FastDelegate0<> delegate(this, &CParticlesObject::PerformAllTheWork_mt);

		if (::Random.randI(1, 4) == 1)
			Device.AddToAuxThread_Pool(5, delegate);
		else
			Device.AddToAuxThread_Pool(5, delegate);

		UpdateSpatial();
	}
	else
	{
		PerformAllTheWork();
	}
}

void CParticlesObject::PerformAllTheWork()
{
	u32 dt = EngineTimeU() - dwLastTime;

	if (dt)
	{
		IParticleCustom* V		= smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);

		V->OnFrame				(dt);
		dwLastTime				= EngineTimeU();
	}

	UpdateSpatial					();
}

void CParticlesObject::PerformAllTheWork_mt()
{
#ifdef MEASURE_MT
	CTimer measure_mt_update; measure_mt_update.Start();
#endif


	u32 dt = EngineTimeU() - dwLastTime;

	if (dt)
	{
		protectMT_.Enter();

		IParticleCustom* V = smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);

		V->OnFrame(dt);

		dwLastTime = EngineTimeU();

		protectMT_.Leave();
	}


#ifdef MEASURE_MT
	Device.Statistic->mtParticlesTime_ += measure_mt_update.GetElapsed_sec();
#endif
}

void CParticlesObject::SetXFORM			(const Fmatrix& m)
{
	IParticleCustom* V	= smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	V->UpdateParent		(m,zero_vel,TRUE);
	renderable.xform.set(m);
	UpdateSpatial		();
}

void CParticlesObject::UpdateParent		(const Fmatrix& m, const Fvector& vel)
{
	IParticleCustom* V	= smart_cast<IParticleCustom*>(renderable.visual); VERIFY(V);
	V->UpdateParent		(m,vel,FALSE);
	UpdateSpatial		();
}

Fvector& CParticlesObject::Position		()
{
	vis_data &vis = renderable.visual->getVisData();
	return vis.sphere.P;
}

void CParticlesObject::renderable_Render(IRenderBuffer& render_buffer)
{
	protectMT_.Enter();

	render_buffer.renderingMatrix_ = renderable.xform;

	::Render->add_Visual(renderable.visual, render_buffer);

	protectMT_.Leave();
}

bool CParticlesObject::IsAutoRemove			()
{
	if(m_bAutoRemove) return true;
	else return false;
}
void CParticlesObject::SetAutoRemove		(bool auto_remove)
{
	VERIFY(m_bStopping || !IsLooped());
	m_bAutoRemove = auto_remove;
}

//играются ли партиклы, отличается от PSI_Alive, тем что после
//остановки Stop партиклы могут еще доигрывать анимацию IsPlaying = true
bool CParticlesObject::IsPlaying()
{
	IParticleCustom* V	= smart_cast<IParticleCustom*>(renderable.visual); 
	VERIFY(V);
	return !!V->IsPlaying();
}
