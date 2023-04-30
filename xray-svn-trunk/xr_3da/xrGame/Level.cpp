#include "pch_script.h"
#include "../environment.h"
#include "../igame_persistent.h"
#include "ParticlesObject.h"
#include "Level.h"
#include "xrServer.h"
#include "net_queue.h"
#include "game_cl_base.h"
#include "hudmanager.h"
#include "ai_space.h"
#include "ShootingObject.h"
#include "player_hud.h"
#include "Level_Bullet_Manager.h"
#include "script_process.h"
#include "script_engine.h"
#include "script_engine_space.h"
#include "infoportion.h"
#include "date_time.h"
#include "space_restriction_manager.h"
#include "seniority_hierarchy_holder.h"
#include "client_spawn_manager.h"
#include "autosave_manager.h"
#include "level_graph.h"
#include "mt_config.h"
#include "phcommander.h"
#include "map_manager.h"
#include "level_sounds.h"
#include "car.h"
#include "trade_parameters.h"
#include "clsid_game.h"

#include "alife_simulator.h"
#include "alife_time_manager.h"

#include "debug_renderer.h"
#include "ai/stalker/ai_stalker.h"

#include "GametaskManager.h"

#ifdef DEBUG
#	include "level_debug.h"
#	include "ai/stalker/ai_stalker.h"
#	include "physicobject.h"
#	include "space_restrictor.h"
#	include "climableobject.h "
#	include "ui_base.h"
#	include "ai_debug.h"

extern Flags32	psAI_Flags;
#endif

extern BOOL	g_bDebugDumpPhysicsStep;
extern CAI_Space *g_ai_space;
extern void draw_wnds_rects();

int			psLUA_GCSTEP		= 10;

CLevel::CLevel():IPureClient(Device.GetTimerGlobal())
{
	Server						= NULL;

	level_game_cl_base			= NULL;
	game_events					= xr_new <NET_Queue_Event>();

	game_configured				= FALSE;
	m_bGameConfigStarted		= FALSE;

	eChangeTrack				= Engine.Event.Handler_Attach	("LEVEL:PlayMusic",this);
	eEnvironment				= Engine.Event.Handler_Attach	("LEVEL:Environment",this);

	eEntitySpawn				= Engine.Event.Handler_Attach	("LEVEL:spawn",this);

	m_pBulletManager			= xr_new <CBulletManager>();
	m_map_manager				= xr_new <CMapManager>();
	m_game_task_manager			= xr_new <CGameTaskManager>();

	m_seniority_hierarchy_holder= xr_new <CSeniorityHierarchyHolder>();

	m_level_sound_manager		= xr_new <CLevelSoundManager>();
	m_space_restriction_manager = xr_new <CSpaceRestrictionManager>();
	m_client_spawn_manager		= xr_new <CClientSpawnManager>();
	m_autosave_manager			= xr_new <CAutosaveManager>();

	#ifdef DRENDER
		m_debug_renderer			= xr_new <CDebugRenderer>();
	#endif

	#ifdef DEBUG
		m_level_debug				= xr_new <CLevelDebug>();
	#endif

	m_ph_commander				= xr_new <CPHCommander>();
	m_ph_commander_scripts		= xr_new <CPHCommander>();

	g_player_hud				= xr_new <player_hud>();
	g_player_hud->load_default();

	pCurrentControlEntity = NULL;

	ambient_sound_next_time		= 0;
	ambient_effect_next_time	= 0;
	ambient_effect_stop_time	= 0;
	ambient_particles			= 0;
}

CLevel::~CLevel()
{
	xr_delete(g_player_hud);

	Msg("- Destroying level");

	Engine.Event.Handler_Detach	(eEntitySpawn,	this);
	Engine.Event.Handler_Detach	(eEnvironment,	this);
	Engine.Event.Handler_Detach	(eChangeTrack,	this);

	if (physics_world())
	{
		destroy_physics_world();
		xr_delete(m_ph_commander_physics_worldstep);
	}

	// Unload sounds
	// unload prefetched sounds
	sound_registry.clear();

	// unload static sounds
	for (u32 i = 0; i < static_Sounds.size(); ++i)
	{
		static_Sounds[i]->destroy();
		xr_delete(static_Sounds[i]);
	}

	static_Sounds.clear			();

	xr_delete					(m_level_sound_manager);
	xr_delete					(m_space_restriction_manager);
	xr_delete					(m_seniority_hierarchy_holder);
	xr_delete					(m_client_spawn_manager);
	xr_delete					(m_autosave_manager);

	#ifdef DRENDER
	xr_delete					(m_debug_renderer);
	#endif


	ai().script_engine().remove_script_process(ScriptEngine::eScriptProcessorLevel);

	xr_delete					(level_game_cl_base);
	xr_delete					(game_events);
	xr_delete					(m_pBulletManager);
	xr_delete					(m_ph_commander);
	xr_delete					(m_ph_commander_scripts);

	ai().unload					();

	#ifdef DEBUG	
	xr_delete(m_level_debug);
	#endif

	xr_delete					(m_map_manager);
	delete_data					(m_game_task_manager);

	// here we clean default trade params
	// because they should be new for each saved/loaded game
	// and I didn't find better place to put this code in
	CTradeParameters::clean();
}

shared_str CLevel::name() const
{
	return (m_name);
}

#pragma note("This can be used, we dont even need to code sound prefetcher")
void CLevel::PrefetchSound(LPCSTR name)
{
	// preprocess sound name
	string_path tmp;
	xr_strcpy(tmp, name);
	xr_strlwr(tmp);

	if (strext(tmp))
		*strext(tmp)=0;

	shared_str snd_name = tmp;
	// find in registry
	SoundRegistryMapIt it = sound_registry.find(snd_name);
	// if find failed - preload sound

	if (it == sound_registry.end())
		sound_registry[snd_name].create(snd_name.c_str(), st_Effect, sg_SourceType);
}

void CLevel::cl_Process_Event(u16 dest, u16 type, NET_Packet& P)
{
	CObject* O = Objects.net_Find(dest);

	if (!O)
	{
	#ifdef DEBUG
		Msg("* WARNING: c_EVENT[%d] to object: can't find object with id [%d]", type, dest);
	#endif
		return;
	}

	CGameObject* GO = smart_cast<CGameObject*>(O);

	if (!GO)
	{
		Msg("! ERROR: c_EVENT[%d] to object: is not gameobject",dest);

		return;
	}

	GO->OnEvent(P, type);

};

void CLevel::ProcessGameEvents()
{
	// Game events
	{
		NET_Packet P;
		u32 svT = timeServer()-NET_Latency;

		while(game_events->available(svT))
		{
			u16 ID,dest,type;

			game_events->get(ID, dest, type, P);

			switch (ID)
			{
			case M_SPAWN:
				{
					u16 dummy16;
					P.r_begin(dummy16);
					cl_Process_Spawn(P);
				}break;
			case M_EVENT:
				{
					cl_Process_Event(dest, type, P);
				}break;
			default:
				{
					R_ASSERT(0);
				}break;
			}			
		}
	}
}

void CLevel::OnFrameBegin()
{
	// commit events from bullet manager from prev-frame
	BulletManager().CommitEvents();

	// Event system
	ClientReceive();
	ProcessGameEvents();

	inherited::OnFrameBegin();
}

void CLevel::OnFrame()
{
#ifdef MEASURE_ON_FRAME
	CTimer measure_on_frame; measure_on_frame.Start();
#endif


	VERIFY(bReady);

	// this needs to go first to be able to send them to aux thread as soon as possible
	Particles.UpdateParticles();

	m_feel_deny.update();

	if (g_mt_config.test(mtMap)) 
		Device.AddToAuxThread_Pool(1, fastdelegate::FastDelegate0<>(m_map_manager, &CMapManager::Update));
	else								
		MapManager().Update();

	if (Device.dwPrecacheFrame == 0)
	{
		//if (g_mt_config.test(mtMap)) 
		//	Device.seqParallel.push_back	(fastdelegate::FastDelegate0<>(m_game_task_manager,&CGameTaskManager::UpdateTasks));
		//else								
		Level().GameTaskManager().UpdateTasks();
	}

	if (g_pGamePersistent->levelEnvironment && (!Device.Paused() || Device.dwPrecacheFrame))
	{
		g_pGamePersistent->Environment().SetEnvTime(GetTimeForEnv());
		g_pGamePersistent->Environment().SetEnvTimeFactor(GetTimeFactorForEnv());

		g_pGamePersistent->Environment().OnFrame();
	}

	if (!Device.Paused())
	{
		// Updates that are spreaded across frames
		Engine.Sheduler.SchedulerUpdate();

		// Per frame update of valuable objects
		Objects.ObjectListUpdate(false, false);
	}

	if (g_hud)
		g_hud->OnFrame();

	// Ambience Sounds
	PlayRandomAmbientSnd();
	// Update weathers ambient effects
	PlayEnvironmentEffects();

	CScriptProcess* levelScript = ai().script_engine().script_process(ScriptEngine::eScriptProcessorLevel);

	if (levelScript)
		levelScript->update();

	m_ph_commander->update();
	m_ph_commander_scripts->update();

	//просчитать полет пуль
	BulletManager().CommitRenderSet();

	// update static sounds
	if (g_mt_config.test(mtLevelSounds)) 
		Device.AddToAuxThread_Pool(1, fastdelegate::FastDelegate0<>(m_level_sound_manager, &CLevelSoundManager::Update));
	else								
		m_level_sound_manager->Update();

	// deffer LUA-GC-STEP
	if (g_mt_config.test(mtLUA_GC))
		Device.AddToAuxThread_Pool(1, fastdelegate::FastDelegate0<>(this, &CLevel::script_gc));
	else
		script_gc();


#ifdef MEASURE_ON_FRAME
	Device.Statistic->onframe_Level_ += measure_on_frame.GetElapsed_ms_f();
#endif
}

void CLevel::OnFrameEnd()
{
	inherited::OnFrameEnd();
}

void CLevel::script_gc()
{
#ifdef MEASURE_MT
	CTimer measure_mt; measure_mt.Start();
#endif


	lua_gc(ai().script_engine().lua(), LUA_GCSTEP, psLUA_GCSTEP);

	
#ifdef MEASURE_MT
	Device.Statistic->mtLUA_GCTime_ += measure_mt.GetElapsed_sec();
#endif
}

#ifdef DEBUG
extern	Flags32	dbg_net_Draw_Flags;
#endif

void CLevel::RenderTracers()
{
	BulletManager().Render();
}

void CLevel::OnRender()
{
	inherited::OnRender();

	if (!level_game_cl_base)
		return;

	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;

	HUD().RenderUI();

	draw_wnds_rects();

	#ifdef DEBUG
	physics_world()->OnRender();

	if (ai().get_level_graph())
		ai().level_graph().render();

	#ifdef DEBUG_PRECISE_PATH
	test_precise_path();
	#endif

	CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(Level().CurrentEntity());

	if (stalker)
		stalker->OnRender();

	if (bDebug)
	{
		for (u32 I = 0; I < Level().Objects.o_count(); I++)
		{
			CObject* _O = Level().Objects.o_get_by_iterator(I);

			CPhysicObject* physic_object = smart_cast<CPhysicObject*>(_O);

			if (physic_object)
				physic_object->OnRender();

			CSpaceRestrictor* space_restrictor = smart_cast<CSpaceRestrictor*>(_O);

			if (space_restrictor)
				space_restrictor->OnRender();

			CClimableObject* climable = smart_cast<CClimableObject*>(_O);

			if (climable)
				climable->OnRender();

			if (dbg_net_Draw_Flags.test(1 << 11)) //draw skeleton
			{
				CGameObject* pGO = smart_cast<CGameObject*>	(_O);

				if (pGO && pGO != Level().CurrentViewEntity() && !pGO->H_Parent())
				{
					if (pGO->Position().distance_to_sqr(Device.vCameraPosition) < 400.0f)
					{
						pGO->dbg_DrawSkeleton();
					}
				}
			};
		}

		ObjectSpace.dbgRender();

		UI().Font().pFontStat->OutSet(170, 630);
		UI().Font().pFontStat->SetHeight(16.0f);
		UI().Font().pFontStat->SetColor(0xffff0000);

		if (Server)
			UI().Font().pFontStat->OutNext("Client Objects:      [%d]", Server->GetEntitiesNum());

		UI().Font().pFontStat->OutNext("Server Objects:      [%d]", Objects.o_count());
		UI().Font().pFontStat->SetHeight(8.0f);
	}

	if (bDebug)
	{
		DBG().draw_object_info();
		DBG().draw_text();
		DBG().draw_level_info();
	}
	#endif

	#ifdef DRENDER
	debug_renderer().render();
	#endif

	#ifdef LOG_PLANNER
	if (psAI_Flags.is(aiVision)) {
		for (u32 I = 0; I < Level().Objects.o_count(); I++) {
			CObject						*object = Objects.o_get_by_iterator(I);
			CAI_Stalker					*stalker = smart_cast<CAI_Stalker*>(object);
			if (!stalker)
				continue;
			stalker->dbg_draw_vision();
		}
	}


	if (psAI_Flags.test(aiDrawVisibilityRays)) {
		for (u32 I = 0; I < Level().Objects.o_count(); I++) {
			CObject						*object = Objects.o_get_by_iterator(I);
			CAI_Stalker					*stalker = smart_cast<CAI_Stalker*>(object);
			if (!stalker)
				continue;

			stalker->dbg_draw_visibility_rays();
		}
	}
	#endif
}

void CLevel::OnEvent(EVENT E, u64 P1, u64)
{
	if (E == eEntitySpawn)
	{
		char Name[128];
		Name[0] = 0;
		sscanf(LPCSTR(P1), "%s", Name);

		Level().g_cl_Spawn(Name, 0xff, M_SPAWN_OBJECT_LOCAL, Fvector().set(0, 0, 0));
	}
	else
		return;
}

void CLevel::net_Save(LPCSTR name) // Game Save
{
	// 1. Create stream
	CMemoryWriter fs;

	// 2. Description
	fs.open_chunk(fsSLS_Description);
	fs.w_stringZ(net_SessionName());
	fs.close_chunk();

	// 3. Server state
	fs.open_chunk(fsSLS_ServerState);
	Server->SLS_Save(fs);
	fs.close_chunk();

	// Save it to file
	fs.save_to(name);
}

void CLevel::SetGameTime(u32 new_hours, u32 new_mins)
{
	u32 year = 1, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
	split_time(GetGameTime(), year, month, day, hours, mins, secs, milisecs);
	u64 new_time = generate_time(year, month, day, new_hours, new_mins, secs, milisecs);

	Times::SetGameTime(new_time);
}


#include "../IGame_Persistent.h"

GlobalFeelTouch::GlobalFeelTouch()
{
}

GlobalFeelTouch::~GlobalFeelTouch()
{
}

struct delete_predicate_by_time : public std::binary_function<Feel::Touch::DenyTouch, DWORD, bool>
{
	bool operator () (Feel::Touch::DenyTouch const & left, DWORD const expire_time) const
	{
		if (left.Expire <= expire_time)
			return true;

		return false;
	};
};

struct objects_ptrs_equal : public std::binary_function<Feel::Touch::DenyTouch, CObject const *, bool>
{
	bool operator() (Feel::Touch::DenyTouch const & left, CObject const * const right) const
	{
		if (left.O == right)
			return true;

		return false;
	}
};

void GlobalFeelTouch::update()
{
	//we ignore P and R arguments, we need just delete evaled denied objects...
	xr_vector<Feel::Touch::DenyTouch>::iterator new_end = 
		std::remove_if(feel_touch_disable.begin(), feel_touch_disable.end(), 
			std::bind2nd(delete_predicate_by_time(), EngineTimeU()));

	feel_touch_disable.erase(new_end, feel_touch_disable.end());
}

bool GlobalFeelTouch::is_object_denied(CObject const * O)
{
	/*Fvector temp_vector;
	feel_touch_update(temp_vector, 0.f);*/
	if (std::find_if(feel_touch_disable.begin(), feel_touch_disable.end(), std::bind2nd(objects_ptrs_equal(), O)) == feel_touch_disable.end())
	{
		return false;
	}

	return true;
}

void CLevel::OnAlifeSimulatorUnLoaded()
{
	MapManager().ResetStorage();
	GameTaskManager().ResetStorage();
}

void CLevel::OnAlifeSimulatorLoaded()
{
	MapManager().ResetStorage();
	GameTaskManager().ResetStorage();
}

void CLevel::ReloadEnvironment()
{
	DestroyEnvironment();

	Msg("---Environment destroyed");
	Msg("---Start to destroy configs");

	CInifile** s = (CInifile**)(&pSettings);

	xr_delete(*s);
	xr_delete(pGameIni);

	Msg("---Start to rescan configs");

	FS.get_path("$game_config$")->m_Flags.set(FS_Path::flNeedRescan, TRUE);
	FS.get_path("$game_scripts$")->m_Flags.set(FS_Path::flNeedRescan, TRUE);
	FS.rescan_pathes();

	Msg("---Start to create configs");

	string_path fname;

	FS.update_path(fname, "$game_config$", "system.ltx");

	Msg("---Updated path to system.ltx is %s", fname);

	pSettings = xr_new <CInifile>(fname, TRUE);
	CHECK_OR_EXIT(0 != pSettings->section_count(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	FS.update_path(fname, "$game_config$", "game.ltx");

	pGameIni = xr_new <CInifile>(fname, TRUE);

	CHECK_OR_EXIT(0 != pGameIni->section_count(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	Msg("---Create environment");

	CreateEnvironment();

	Msg("---Call level_weathers.restart_weather_manager");

	luabind::functor<void> lua_function;

	string256 fn;
	xr_strcpy(fn, "level_weathers.restart_weather_manager");

	R_ASSERT2(ai().script_engine().functor<void>(fn, lua_function), make_string("Can't find function %s", fn));

	lua_function();

	Msg("---Done");
}

/*
CZoneList* CLevel::create_hud_zones_list()
{
	hud_zones_list = xr_new<CZoneList>();
	hud_zones_list->clear();
	return hud_zones_list;
}
*/
