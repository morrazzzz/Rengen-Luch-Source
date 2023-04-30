//----------------------------------------------------
// file: TempObject.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ps_instance.h"
#include "IGame_Level.h"

CPS_Instance::CPS_Instance			(bool destroy_on_game_load)	:
	ISpatial				(g_SpatialSpace),
	m_destroy_on_game_load	(destroy_on_game_load)
{
	g_pGameLevel->Particles.allParticles.push_back(this);

	renderable.pROS_Allowed					= FALSE;

	m_iLifeTime								= int_max;
	m_bAutoRemove							= TRUE;
	m_bDead									= TRUE;

	readyToDestroy							= false;
	alreadyInDestroyQueue					= false;
}

//----------------------------------------------------
CPS_Instance::~CPS_Instance					()
{
	R_ASSERT2(GetCurrentThreadId() == Core.mainThreadID, "Particles must be destructed only within main thread, to avoid mt bugs");
	R_ASSERT2(readyToDestroy, "Particles should be destroyed via DeleteParticleQueue, to avoid mt bugs");

	R_ASSERT(!engineState.test(FRAME_RENDERING));


	xr_vector<CPS_Instance*>::iterator it = std::find(g_pGameLevel->Particles.allParticles.begin(), g_pGameLevel->Particles.allParticles.end(), this);

	R_ASSERT(it != g_pGameLevel->Particles.allParticles.end());

	g_pGameLevel->Particles.allParticles.erase(it);

	shedule_unregister();
}

void CPS_Instance::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	if (renderable.pROS) 
		::Render->ros_destroy(renderable.pROS);	//. particles doesn't need ROS

	ISheduled::ScheduledUpdate(dt);

	m_iLifeTime -= dt;

	// remove???
	if (m_bDead)
		return;

	if (m_bAutoRemove && m_iLifeTime <= 0)
	{
		CPS_Instance* p = this;
		DestroyParticleInstance(p, true);
	}


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_PSInstance_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CPS_Instance::PreDestroy()
{
	m_bDead = TRUE;
	m_iLifeTime = 0;
}
