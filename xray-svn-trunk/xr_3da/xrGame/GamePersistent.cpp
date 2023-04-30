#include "pch_script.h"
#include "gamepersistent.h"
#include "../fmesh.h"
#include "../xr_ioconsole.h"
#include "../gamemtllib.h"
#include "../../Include/xrRender/Kinematics.h"
#include "MainMenu.h"
#include "level.h"
#include "ParticlesObject.h"
#include "actor.h"
#include "stalker_animation_data_storage.h"
#include "stalker_velocity_holder.h"
#include "alife_simulator.h"
#include "HUDManager.h"
#include "ui/UIPdaWnd.h"

#include "ActorEffector.h"
#include "script_engine.h"
#include "ui/UITextureMaster.h"

#include "GameConstants.h"

#ifndef MASTER_GOLD
#	include "custommonster.h"
#endif

#include "ai_debug.h"
#include "../x_ray.h"

#ifdef DEBUG_MEMORY_MANAGER
	static	void *	ode_alloc	(size_t size)								{ return Memory.mem_alloc(size,"ODE");			}
	static	void *	ode_realloc	(void *ptr, size_t oldsize, size_t newsize)	{ return Memory.mem_realloc(ptr,newsize,"ODE");	}
	static	void	ode_free	(void *ptr, size_t size)					{ return xr_free(ptr);							}
#else
	static	void *	ode_alloc	(size_t size)								{ return xr_malloc(size);			}
	static	void *	ode_realloc	(void *ptr, size_t oldsize, size_t newsize)	{ return xr_realloc(ptr,newsize);	}
	static	void	ode_free	(void *ptr, size_t size)					{ return xr_free(ptr);				}
#endif

static float diff_far	= 70.0f;
static float diff_near	= -70.0f;

bool m_bCamReady = false;

CGamePersistent::CGamePersistent()
{
	m_bPickableDOF				= false;

	m_pUI_core					= NULL;
	m_pMainMenu					= NULL;
	m_intro						= NULL;

	m_intro_event.bind			(this, &CGamePersistent::start_logo_intro);

	dSetAllocHandler			(ode_alloc);
	dSetReallocHandler			(ode_realloc);
	dSetFreeHandler				(ode_free);

	eQuickLoad					= Engine.Event.Handler_Attach("Game:QuickLoad",this);

	Fvector3* DofValue		= Console->GetFVectorPtr("r2_dof");

	SetBaseDof				(*DofValue);
}

CGamePersistent::~CGamePersistent()
{	
	Device.seqFrame.Remove(this);
	Engine.Event.Handler_Detach(eQuickLoad, this);
}

void CGamePersistent::RegisterModel(IRenderVisual* V)
{
	// Check types
	switch (V->getType())
	{
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:{
		u16 def_idx = GMLib.GetMaterialIdx("default_object");

		R_ASSERT2(GMLib.GetMaterialByIdx(def_idx)->Flags.is(SGameMtl::flDynamic), "'default_object' - must be dynamic");

		IKinematics* K = smart_cast<IKinematics*>(V); VERIFY(K);

		int cnt = K->LL_BoneCount();

		for (u16 k = 0; k<cnt; k++)
		{
			CBoneData& bd = K->LL_GetData(k);

			if (*(bd.game_mtl_name))
			{
				bd.game_mtl_idx = GMLib.GetMaterialIdx(*bd.game_mtl_name);

				R_ASSERT2(GMLib.GetMaterialByIdx(bd.game_mtl_idx)->Flags.is(SGameMtl::flDynamic), "Required dynamic game material");
			}
			else
			{
				bd.game_mtl_idx = def_idx;
			}
		}
	}break;
	}
}

extern void clean_game_globals	();
extern void init_game_globals	();

void CGamePersistent::OnAppStart()
{
	// load game materials
	GMLib.Load();
	init_game_globals();

	__super::OnAppStart();

	m_pUI_core = xr_new <ui_core>();
	m_pMainMenu = xr_new <CMainMenu>();
}


void CGamePersistent::OnAppEnd()
{
	if (m_pMainMenu->IsActive())
		m_pMainMenu->Activate(false);

	xr_delete(m_pMainMenu);
	xr_delete(m_pUI_core);

	__super::OnAppEnd();

	clean_game_globals();

	GMLib.Unload();

}

void CGamePersistent::Start(LPCSTR op)
{
	__super::Start(op);
}

void CGamePersistent::Disconnect()
{
	__super::Disconnect();

	// stop all played emitters
	::Sound->stop_emitters();
}

#include "xr_level_controller.h"

void CGamePersistent::OnGameStart()
{
	__super::OnGameStart();
	
	diff_far	= pSettings->r_float("zone_pick_dof", "far"); 
	diff_near	= pSettings->r_float("zone_pick_dof", "near");

	GameConstants::LoadConstants();
}

void CGamePersistent::OnGameEnd	()
{
	__super::OnGameEnd();

	xr_delete(g_stalker_animation_data_storage);
	xr_delete(g_stalker_velocity_holder);
}

#include "UI/UIGameTutorial.h"

extern int ps_intro;

void CGamePersistent::start_logo_intro()
{
	if (!ps_intro)
	{
		m_intro_event = 0;

		if (!xr_strlen(m_game_params.m_new_or_load))
		{
			Console->Show();
			Console->Execute("main_menu on");
		}

		return;
	}

	if (Device.dwPrecacheFrame == 0)
	{
		m_intro_event.bind(this, &CGamePersistent::update_logo_intro);

		if (!xr_strlen(m_game_params.m_game_or_spawn) && !g_pGameLevel)
		{
			if (m_intro)
				return;

			m_intro = xr_new <CUISequencer>();

			m_intro->Start("intro_logo");

			Console->Hide();
		}
	}
}

void CGamePersistent::update_logo_intro()
{
	if(m_intro && (false == m_intro->IsActive()))
	{
		m_intro_event = 0;
		xr_delete(m_intro);
		Console->Execute("main_menu on");
	}
	else if (!m_intro)
	{
		m_intro_event = 0;
	}
}

extern int g_keypress_on_start;

void CGamePersistent::game_loaded()
{
	if(Device.dwPrecacheFrame <= 2)
	{
		if(	g_pGameLevel && g_pGameLevel->bReady && g_keypress_on_start && load_screen_renderer.b_need_user_input)
		{
			pApp->ClearTitle();

			if (m_intro)
				return;

			m_intro = xr_new <CUISequencer>();
			m_intro->Start("game_loaded");
			m_intro->m_on_destroy_event.bind(this, &CGamePersistent::update_game_loaded);

			Msg("# Game is loaded: Waiting for user input");
		}

		m_intro_event = 0;
	}
}

void CGamePersistent::update_game_loaded()
{
	xr_delete(m_intro);

	Msg("# Processing after load events");

	for (u32 i = 0; i < afterGameLoadedStuff_.size(); i++)
		afterGameLoadedStuff_[i]();

	afterGameLoadedStuff_.clear();

	start_game_intro();
}

#include "ai_space.h"
#include "alife_spawn_registry.h"

void CGamePersistent::start_game_intro()
{
	if (g_pGameLevel && g_pGameLevel->bReady && Device.dwPrecacheFrame <= 2)
	{
		m_intro_event.bind(this, &CGamePersistent::update_game_intro);

		LPCSTR spawn_name = *ai().alife().spawns().get_spawn_name();
		bool load_spawn = (0==stricmp(m_game_params.m_new_or_load,"load") && 0==xr_strcmp(m_game_params.m_game_or_spawn, spawn_name));	//skyloader: flag if load save and (save == spawn_name), for example, all.sav
		
		if (0==stricmp(m_game_params.m_new_or_load,"new") || load_spawn)
		{
			if (NULL != m_intro)
				return;

			m_intro = xr_new <CUISequencer>();

			m_intro->Start("intro_game");

			if (!load_spawn)
				Msg("intro_start intro_game");
			else
				m_intro->Stop(); //<= skyloader: call functions from sequencer (call the first scene in sid bunker)
		}
			
	}
}

#include "script_engine.h"

void synchronization_callback()
{
	string256 fn;
	luabind::functor<void> callback;

	xr_strcpy(fn, pSettings->r_string("lost_alpha_cfg", "on_synchronization_done"));

	R_ASSERT(ai().script_engine().functor<void>(fn, callback));

	callback();
}

void CGamePersistent::update_game_intro()
{
	if(m_intro && (false==m_intro->IsActive()))
	{
		xr_delete(m_intro);
		m_intro_event = 0;
		synchronization_callback();
	}
	else if(!m_intro)
	{
		m_intro_event = 0;
		synchronization_callback();
	}
}

#include "holder_custom.h"

extern CUISequencer * g_tutorial;
extern CUISequencer * g_tutorial2;

void CGamePersistent::OnFrame()
{
#ifdef MEASURE_ON_FRAME
	CTimer measure_on_frame; measure_on_frame.Start();
#endif


	inherited::OnFrame();

	TutorialAndIntroHandling();

	if(!m_pMainMenu->IsActive())
		m_pMainMenu->DestroyInternal(false);

	if (CurrentFrame() % 200 == 0)
		CUITextureMaster::FreeCachedShaders();

	if(!g_pGameLevel || !g_pGameLevel->bReady)
		return;

	SetActiveCamera();

	UpdateDof();


#ifdef MEASURE_ON_FRAME
	Device.Statistic->onframe_GamePersistant_ += measure_on_frame.GetElapsed_ms_f();
#endif
}

#include "game_sv_base.h"
#include "xrServer.h"
#include "UIGameCustom.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIPdaWnd.h"

void CGamePersistent::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if(E == eQuickLoad)
	{
		loading_save_timer.Start();
		loading_save_timer_started = true;

		pApp->LoadPhaseBegin(QUICK_LOAD_PHASES);

		if (Device.Paused())
			Device.Pause(FALSE, TRUE, TRUE, "eQuickLoad");

		CUIGameCustom* ui_game_custom = NULL;

		if ((ui_game_custom = CurrentGameUI()) != NULL)
		{
			ui_game_custom->HideShownDialogs();
			ui_game_custom->UIMainIngameWnd->reset_ui();

			xr_delete(ui_game_custom->m_PdaMenu);

			ui_game_custom->m_PdaMenu = xr_new <CUIPdaWnd>();
		}
		
		if(g_tutorial)
			g_tutorial->Stop();

		if(g_tutorial2)
			g_tutorial2->Stop();
		
		LPSTR saved_name = (LPSTR)(P1);

		Level().remove_objects();

		R_ASSERT(Level().Server->Server_game_sv_base);

		Level().Server->Server_game_sv_base->restart_simulator(saved_name);

		xr_free(saved_name);

		pApp->LoadPhaseEnd();

		return;
	}
}

float CGamePersistent::MtlTransparent(u32 mtl_idx)
{
	return GMLib.GetMaterialByIdx((u16)mtl_idx)->fVisTransparencyFactor;
}

static BOOL bRestorePause	= FALSE;
static BOOL bEntryFlag		= TRUE;

void CGamePersistent::OnAppActivate()
{
	bool bIsMP = false;

	bIsMP &= !Device.Paused();

	if(!bIsMP)
	{
		Device.Pause(FALSE, !bRestorePause, TRUE, "CGP::OnAppActivate");
	}else
	{
		Device.Pause(FALSE, TRUE, TRUE, "CGP::OnAppActivate MP");
	}

	bEntryFlag = TRUE;
}

void CGamePersistent::OnAppDeactivate()
{
	if(!bEntryFlag) return;

	bool bIsMP = false;

	bRestorePause = FALSE;

	if (!bIsMP)
	{
		bRestorePause = Device.Paused();
		Device.Pause(TRUE, TRUE, TRUE, "CGP::OnAppDeactivate");
	}
	else
	{
		Device.Pause(TRUE, FALSE, TRUE, "CGP::OnAppDeactivate MP");
	}

	bEntryFlag = FALSE;
}

bool CGamePersistent::OnRenderPPUI_query()
{
	return MainMenu()->OnRenderPPUI_query();
}

extern void draw_wnds_rects();
void CGamePersistent::OnRenderPPUI_main()
{
	// always
	MainMenu()->OnRenderPPUI_main();
	draw_wnds_rects();
}

void CGamePersistent::OnRenderPPUI_PP()
{
	MainMenu()->OnRenderPPUI_PP();
}

#include "string_table.h"

void CGamePersistent::LoadTitle(LPCSTR ui_msg, LPCSTR log_msg, bool change_tip, shared_str map_name)
{
	string512 buff;
	xr_sprintf(buff, "%s...", CStringTable().translate(ui_msg).c_str());

	if (change_tip)
	{
		string512 buff2;

		u8						tip_num;
		luabind::functor<u8>	m_functor;

		R_ASSERT(ai().script_engine().functor("loadscreen.get_tip_number", m_functor));
		tip_num = m_functor(map_name.c_str());

		xr_sprintf(buff2, "%s:%d", CStringTable().translate("ls_tip_number").c_str(), tip_num);
		shared_str tmp = buff2;

		xr_sprintf(buff2, "ls_tip_%d", tip_num);

		pApp->LoadTitleInt(buff, tmp.c_str(), CStringTable().translate(buff2).c_str(), log_msg);
	}
	else
		pApp->LoadTitleInt(buff, "keep", "", log_msg);
}

bool CGamePersistent::CanBePaused()
{
	return true;
}

void CGamePersistent::SetPickableEffectorDOF(bool bSet)
{
	m_bPickableDOF = bSet;

	if(!bSet)
		RestoreEffectorDOF();
}

void CGamePersistent::GetCurrentDof(Fvector3& dof)
{
	dof = m_dof[1];
}

void CGamePersistent::SetBaseDof(const Fvector3& dof)
{
	if (!m_bPickableDOF)
		m_dof[0] = m_dof[1] = m_dof[2] = m_dof[3] = dof;
	else
		m_dof[3] = dof;
}

void CGamePersistent::SetEffectorDOF(const Fvector& needed_dof)
{
	if(m_bPickableDOF)
		return;

	m_dof[0] = needed_dof;
	m_dof[2] = m_dof[1]; //current
}

void CGamePersistent::RestoreEffectorDOF()
{
	SetEffectorDOF(m_dof[3]);
}

#include "UIGameSP.h"

void CGamePersistent::UpdateDof()
{
	if(m_bPickableDOF)
	{
		Fvector pick_dof;

		pick_dof.y	= HUD().GetCurrentRayQuery().range;
		pick_dof.x	= pick_dof.y+diff_near;
		pick_dof.z	= pick_dof.y+diff_far;

		m_dof[0]	= pick_dof;
		m_dof[2]	= m_dof[1]; //current
	}

	if(m_dof[1].similar(m_dof[0]))
		return;

	float td = TimeDelta();
	Fvector diff;
	diff.sub(m_dof[0], m_dof[2]);
	diff.mul(td / 0.2f); //0.2 sec
	m_dof[1].add(diff);

	(m_dof[0].x<m_dof[2].x) ? clamp(m_dof[1].x, m_dof[0].x, m_dof[2].x) : clamp(m_dof[1].x, m_dof[2].x, m_dof[0].x);
	(m_dof[0].y<m_dof[2].y) ? clamp(m_dof[1].y, m_dof[0].y, m_dof[2].y) : clamp(m_dof[1].y, m_dof[2].y, m_dof[0].y);
	(m_dof[0].z<m_dof[2].z) ? clamp(m_dof[1].z, m_dof[0].z, m_dof[2].z) : clamp(m_dof[1].z, m_dof[2].z, m_dof[0].z);
}

void CGamePersistent::SelectWeatherCycle()
{
	Msg("Runing scripted weather cycle selection");

	luabind::functor<void> lua_func;

	R_ASSERT2(ai().script_engine().functor("level_weathers.select_weather_cycle", lua_func), "Can't find level_weathers.select_weather_cycle");

	lua_func();
}

bool CGamePersistent::IsWeatherScriptLoaded()
{
	luabind::functor<bool> lua_bool;

	R_ASSERT2(ai().script_engine().functor("level_weathers.is_weather_script_ready", lua_bool), "Can't find level_weathers.is_weather_script_ready");

	bool result = lua_bool();

	return result;
}

void CGamePersistent::SetActiveCamera()
{
	if (Device.Paused())
	{
#ifndef MASTER_GOLD
		if (Level().CurrentViewEntity())
		{
			if (!g_actor || (g_actor->ID() != Level().CurrentViewEntity()->ID()))
			{
				CCustomMonster* custom_monster = smart_cast<CCustomMonster*>(Level().CurrentViewEntity());

				if (custom_monster) // can be spectator in multiplayer
					custom_monster->UpdateCamera();

				if (m_bCamReady == false)
					m_bCamReady = true;
			}
			else
			{
				CCameraBase* C = NULL;

				if (g_actor)
				{
					if (!Actor()->Holder())
						C = Actor()->cam_Active();
					else
						C = Actor()->Holder()->Camera();

					Actor()->Cameras().UpdateFromCamera(C);
					Actor()->Cameras().ApplyDevice(VIEWPORT_NEAR);

					if (m_bCamReady == false)
						m_bCamReady = true;
				}
			}
		}
#else
		if (g_actor)
		{
			CCameraBase* C = NULL;

			if (!Actor()->Holder())
				C = Actor()->cam_Active();
			else
				C = Actor()->Holder()->Camera();

			Actor()->Cameras().UpdateFromCamera(C);
			Actor()->Cameras().ApplyDevice(VIEWPORT_NEAR);

			if (m_bCamReady == false)
				m_bCamReady = true;
		}
#endif
	}
}

void CGamePersistent::TutorialAndIntroHandling()
{
	if (Device.dwPrecacheFrame == 5 && m_intro_event.empty())
	{
		m_intro_event.bind(this, &CGamePersistent::game_loaded);
	}

	if (g_tutorial2)
	{
		g_tutorial2->Destroy();

		xr_delete(g_tutorial2);
	}

	if (g_tutorial && !g_tutorial->IsActive())
		xr_delete(g_tutorial);

	if (!m_intro_event.empty())
		m_intro_event();

	if (Device.dwPrecacheFrame == 0 && !m_intro && m_intro_event.empty())
		load_screen_renderer.stop();
}

#include "ui\uimainingamewnd.h"
#pragma todo ("Cop merge A: Implement these callbacks")
void CGamePersistent::OnSectorChanged(int sector)
{
	//if (CurrentGameUI())
	//	CurrentGameUI()->UIMainIngameWnd->OnSectorChanged(sector);
}

void CGamePersistent::OnAssetsChanged()
{
	IGame_Persistent::OnAssetsChanged();
	CStringTable().rescan();
}