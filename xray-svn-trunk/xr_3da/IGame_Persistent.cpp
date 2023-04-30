#include "stdafx.h"
#pragma hdrstop

#include "IGame_Persistent.h"

#include "environment.h"
#include "x_ray.h"
#include "IGame_Level.h"
#include "XR_IOConsole.h"
#include "Render.h"

#include "CustomHUD.h"

#include "CommonFlags.h"


ENGINE_API IGame_Persistent* g_pGamePersistent = nullptr;

extern BOOL mt_texture_prefetching;

IGame_Persistent::IGame_Persistent()
{
	RDEVICE.seqAppStart.Add			(this);
	RDEVICE.seqAppEnd.Add			(this);
	RDEVICE.seqFrame.Add			(this, REG_PRIORITY_HIGH + 100);
	RDEVICE.seqAppActivate.Add		(this);
	RDEVICE.seqAppDeactivate.Add	(this);
	RDEVICE.seqRender.Add			(this);

	m_pMainMenu						= NULL;

	weaponSVPShaderParams			= xr_new <WeaponSVPParamsShExport>(); //--#SM+#--

#ifndef _EDITOR
	levelEnvironment				= xr_new <CEnvironment>();

	if (g_uCommonFlags.test(CF_Prefetch_UI))
	{
		Msg("*Start prefetching UI textures");

		Device.m_pRender->RenderPrefetchUITextures();
	}
#endif

	render_scene					= true;

	if(strstr(Core.Params, "-noprefetch"))
		psDeviceFlags.set(rsPrefObjects, FALSE);
}

IGame_Persistent::~IGame_Persistent()
{
	RDEVICE.seqFrame.Remove			(this);
	RDEVICE.seqAppStart.Remove		(this);
	RDEVICE.seqAppEnd.Remove		(this);
	RDEVICE.seqAppActivate.Remove	(this);
	RDEVICE.seqAppDeactivate.Remove	(this);
	RDEVICE.seqRender.Remove		(this);

	if (g_uCommonFlags.test(CF_Prefetch_UI))
		ReportUITxrsForPrefetching();

	xr_delete(weaponSVPShaderParams);
	xr_delete(levelEnvironment);
}

void IGame_Persistent::OnAppActivate()
{
}

void IGame_Persistent::OnAppDeactivate()
{
}

void IGame_Persistent::OnAppStart()
{
#ifndef _EDITOR
	Environment().load();
#endif    
}

void IGame_Persistent::OnAppEnd()
{
#ifndef _EDITOR
	Environment().unload();
#endif    
	OnGameEnd();

#ifndef _EDITOR
	DEL_INSTANCE(g_hud);
#endif    
}


void IGame_Persistent::PreStart(LPCSTR op)
{
	string256						prev_type;
	params							new_game_params;

	xr_strcpy						(prev_type, m_game_params.m_game_type);

	new_game_params.parse_cmd_line	(op);

	// change game type
	if (0 != xr_strcmp(prev_type,new_game_params.m_game_type))
	{
		OnGameEnd();
	}
}

void IGame_Persistent::Start(LPCSTR op)
{
	string256						prev_type;
	xr_strcpy						(prev_type, m_game_params.m_game_type);
	m_game_params.parse_cmd_line	(op);

	// change game type
	if ((0 != xr_strcmp(prev_type, m_game_params.m_game_type))) 
	{
		if (*m_game_params.m_game_type)
			OnGameStart();

#ifndef _EDITOR
		if(g_hud)
			DEL_INSTANCE			(g_hud);
#endif            
	}

	if(g_pGameLevel)
		VERIFY(g_pGameLevel->Particles.particlesToDelete.empty());
}

void IGame_Persistent::Disconnect()
{
#ifndef _EDITOR
	if(g_hud)
		DEL_INSTANCE(g_hud);
#endif
}

void IGame_Persistent::OnGameStart()
{
#ifndef _EDITOR

	loading_save_timer.Start();
	loading_save_timer_started = true;

	pApp->LoadPhaseBegin(psDeviceFlags.test(rsPrefObjects) ? LEVEL_LOAD_AND_PREFETCH_PHASES : LEVEL_LOAD_PHASES);

	g_loading_events.push_back(LOADING_EVENT(this, &IGame_Persistent::Prefetching_stage));

#endif
}

IC bool IGame_Persistent::SceneRenderingBlocked()
{
	if (!render_scene || (m_pMainMenu && m_pMainMenu->CanSkipSceneRendering()))
	{
		return true;
	}

	return false;
}

bool IGame_Persistent::Prefetching_stage()
{
#ifndef __BORLANDC__

	if (!psDeviceFlags.test(rsPrefObjects))
		return true;

	prefetching_in_progress = true;

	LoadTitle("st_prefetching_objects", "Prefetching Objects and Models");

	// prefetch game objects & models
	float p_time = 1000.f*Device.GetTimerGlobal()->GetElapsed_sec();
	u64 before_memory = Device.Statistic->GetTotalRAMConsumption();

	Log("# Loading objects");
	ObjectPool.prefetch();
	Log("# Loading models");
	Render->models_Prefetch();

	s64 memory_used = (s64)Device.Statistic->GetTotalRAMConsumption() - (s64)before_memory;

	p_time = 1000.f*Device.GetTimerGlobal()->GetElapsed_sec() - p_time;
	Msg("* [prefetch_objects_and_models] Time:[%d ms]", iFloor(p_time));
	Msg("* [prefetch_objects_and_models] Memory Aquired:[%u K]", memory_used / 1024);

	Log("# Prefetching Textures");
	Device.m_pRender->ResourcesDeferredUpload(mt_texture_prefetching);

	prefetching_in_progress = false;

#endif

	return true;
}


void IGame_Persistent::OnGameEnd()
{
#ifndef _EDITOR
	ObjectPool.clear();
	Render->models_Clear(TRUE);
#endif
}

void IGame_Persistent::OnFrame()
{
#ifdef MEASURE_ON_FRAME
	CTimer measure_on_frame; measure_on_frame.Start();
#endif



#ifdef MEASURE_ON_FRAME
	Device.Statistic->onframe_IGamePersistent_ += measure_on_frame.GetElapsed_ms_f();
#endif
}


void IGame_Persistent::OnRender()
{
#ifndef _EDITOR

	if (!g_pGameLevel || !g_pGameLevel->bReady || SceneRenderingBlocked())
	{
		Device.auxThread_4_Allowed_.Set();
	}

#endif
}

void IGame_Persistent::OnAssetsChanged()
{
#ifndef _EDITOR
	Device.m_pRender->OnAssetsChanged(); //Resources->m_textures_description.Load();
#endif    
}

void IGame_Persistent::ReportUITxrsForPrefetching()
{
	if (SuggestedForPrefetchingUI.size() > 0)
	{
		Msg("- These UI textures are suggested to be prefetched since they caused stutterings when some UI window was loading");
		Msg("- Add this list to prefetch_ui_textures.ltx (wisely)");

		for (u32 i = 0; i < SuggestedForPrefetchingUI.size(); i++)
		{
			Msg("%s", SuggestedForPrefetchingUI[i].c_str());
		}
	}
}
