#include "pch_script.h"
#include "../xr_ioconsole.h"
#include "../xr_ioc_cmd.h"
#include "../customhud.h"
#include "../fdemorecord.h"
#include "xrMessages.h"
#include "level.h"
#include "script_debugger.h"
#include "ai_debug.h"
#include "alife_simulator.h"
#include "hit.h"
#include "actor.h"
#include "ActorFlags.h"
#include "customzone.h"
#include "script_engine.h"
#include "script_engine_space.h"
#include "script_process.h"
#include "ui/UIMainIngameWnd.h"
#include "../../xrphysics/iphworld.h"
#include "string_table.h"
#include "ai_space.h"
#include "ai/monsters/BaseMonster/base_monster.h"
#include "date_time.h"
#include "mt_config.h"
#include "MainMenu.h"
#include "saved_game_wrapper.h"
#include "level_graph.h"
#include "cameralook.h"
#include "WeaponMagazined.h"
#include "UIGameSP.h"
#include "ui/UIWeatherEditor.h"
#include "hudmanager.h"
#include "../x_ray.h"
#include "../CommonFlags.h"
#include "../../xrphysics/console_vars.h"

#include "inventory_upgrade_manager.h"
#include "ui/UIInventoryWnd.h"
#include "ui/UITalkWnd.h"
#include "ui/UIUpgradeWnd.h"
#include "character_hit_animations_params.h"
#include "../../xrPhysics/PhysicsCommon.h"
#include "../../xrPhysics/console_vars.h"

#ifdef DEBUG
#	include "PHDebug.h"
#	include "ui/UIDebugFonts.h" 
#	include "game_graph.h"
#	include "level_debug.h"
#endif


//----------------------------------------Variables----------------------------------------


//---Development
extern float		timeFactorGame_;

string_path			g_last_saved_game;

extern float		g_bullet_time_factor;

extern int			hud_adj_mode;
extern int			hud_adj_item_idx;
extern int			hud_adj_addon_idx;
extern float		hud_adj_delta_pos;
extern float		hud_adj_delta_rot;

bool				freeCamIsOn_ = false;
BOOL				gameObjectLightShadows_ = true;

//---Development cheats


//---Features
int					ps_intro = 0;

Flags32				g_mt_config = { mtLevelPath | mtDetailPath | mtMap | mtObjectHandler | mtSoundPlayer | mtAiVision | mtAIMisc | mtBullets | mtLUA_GC | mtLevelSounds | mtALife | mtIKinematics | mtLoadNPCSounds };

BOOL				useInverseKinematics_ = 1; // CPU Intensive, asspecialy, if not using mt_kinematics flag ON

extern ENGINE_API u32 particlesCollision_;
extern ENGINE_API float particlesCollisionDistance_;

xr_token particles_collision_token[] = {
	{ "off", 0 },
	{ "only_with_settings", 1 },
	{ "all", 2 },
	{ 0, 0 }
};


extern float g_smart_cover_factor;
extern float g_smart_cover_animation_speed_factor;


//---Gameplay
extern ESingleGameDifficulty g_SingleGameDifficulty;
shared_str			g_language;
extern u32			crosshairAnimationType = 0;
xr_token			qcrosshair_type_token[] =
{
	{ "st_opt_cross_0", 0 },
	{ "st_opt_cross_1", 1 },
	{ "st_opt_cross_2", 2 },
	{ "st_opt_cross_3", 3 },
	{ "st_opt_cross_4", 4 },
	{ 0, 0 }
};

Flags32 psHoldZoom = { TRUE };

xr_vector<xr_token>			qhud_type_token;

int					quick_save_counter = 0;
int					max_quick_saves = 16;

int					sndMaxShotSounds_ = 25;

float				g_aim_predict_time = 0.44f;

extern float		air_resistance_epsilon;
int					g_keypress_on_start = 1;

//---Debug
extern int			psLUA_GCSTEP;
extern void			show_smart_cast_stats();
extern void			clear_smart_cast_stats();
extern void			release_smart_cast_stats();

extern BOOL			g_bShowHitSectors;
extern BOOL			g_show_wnd_rect2;

extern ENGINE_API BOOL		showActorLuminocity_;

extern XRPHYSICS_API float	camera_collision_character_skin_depth;
extern XRPHYSICS_API float	camera_collision_character_shift_z;

int					g_upgrades_log = 0;

extern float		g_ai_aim_min_speed;
extern float		g_ai_aim_min_angle;
extern float		g_ai_aim_max_angle;

extern BOOL			g_ai_use_old_vision;

#ifdef DEBUG
extern BOOL			g_ShowAnimationInfo		;

Flags32				dbg_net_Draw_Flags		= {0};

BOOL				g_bDebugNode			= FALSE;
u32					g_dwDebugNodeSource		= 0;
u32					g_dwDebugNodeDest		= 0;
extern BOOL			g_bDrawBulletHit;

float				debug_on_frame_gather_stats_frequency	= 0.f;

extern LPSTR		dbg_stalker_death_anim;
extern BOOL			b_death_anim_velocity;

// cop
extern BOOL			death_anim_debug;
extern BOOL			dbg_imotion_draw_velocity;
extern BOOL			dbg_imotion_collide_debug;
extern BOOL			dbg_imotion_draw_skeleton;
extern BOOL			g_ai_dbg_sight;
extern BOOL			g_ai_aim_use_smooth_aim;
extern BOOL			g_debug_doors;
extern BOOL			dbg_moving_bones_snd_player;
extern Flags32		dbg_track_obj_flags;
extern float		dbg_imotion_draw_velocity_scale;

extern float		dbg_text_height_scale;
extern Flags32		dbg_track_obj_flags;
extern BOOL			dbg_draw_ragdoll_spawn;
extern BOOL			debug_step_info;
extern BOOL			debug_step_info_load;
extern BOOL			debug_character_material_load;

extern XRPHYSICS_API BOOL dbg_draw_camera_collision;

extern BOOL			dbg_draw_animation_movement_controller;
extern BOOL			dbg_draw_character_bones;
extern BOOL			dbg_draw_character_physics;
extern BOOL			dbg_draw_character_binds;
extern BOOL			dbg_draw_character_physics_pones;
extern BOOL			dbg_draw_doors;

extern hit_animation_global_params ghit_anims_params;

#endif
int				g_AI_inactive_time = 0;


//----------------------------------------Special Classes----------------------------------------


//---Development

extern void lua_debug_print(LPCSTR str);
class CCC_LogPrint : public IConsole_Command
{
	public:
		CCC_LogPrint(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
		virtual void Execute(LPCSTR args)
		{
			string256 text;
			text[0] = 0;
			xr_strcpy(text, args);

			lua_debug_print(text);
		}
};

class CCC_SetGameTime : public IConsole_Command {
public:
	CCC_SetGameTime(LPCSTR N) : IConsole_Command(N) {};
	virtual void	Execute(LPCSTR args)
	{
		u32 hours = 0, mins = 0;

		sscanf(args, "%d:%d", &hours, &mins);
		Level().SetGameTime(hours, mins);
	}
};

class CCC_TimeFactor : public IConsole_Command {
public:
	CCC_TimeFactor(LPCSTR N) : IConsole_Command(N) {};
	virtual void	Execute(LPCSTR args)
	{
		float time_factor = (float)atof(args);

		clamp(time_factor, EPS, 1000.f);

		SetETimeFactor(time_factor);

		Msg("- Time factor set to %f", time_factor);
	}
	virtual void	Status(TStatus &S)
	{
		xr_sprintf(S, "%f", ETimeFactor());
	}

	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "[0.001 - 1000.0]");
	}
};

class CCC_TimeFactorGame : public IConsole_Command
{
public:
	CCC_TimeFactorGame(LPCSTR N) : IConsole_Command(N) {};

	virtual void	Execute(LPCSTR args)
	{
		float time_factor_game = (float)atof(args);
		clamp(time_factor_game, 0.0001f, 1000.f);

		Msg("- Game Time factor is set to %f", time_factor_game);

		if (IsAlifeInited())
			SetGameTimeFactor(time_factor_game);
	}

	virtual void	Status(TStatus &S)
	{
		if (IsAlifeInited())
			xr_sprintf(S, "%f", GetGameTimeFactor());
		else
			xr_sprintf(S, "0");	
	}

	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "[0.001 - 1000.0]");
	}
};

struct CCC_StartTimeSingle : public IConsole_Command {
	CCC_StartTimeSingle(LPCSTR N) : IConsole_Command(N) {};
	virtual void Execute(LPCSTR args)
	{
		u32 year = 1, month = 1, day = 1, hours = 0, mins = 0, secs = 0, milisecs = 0;
		sscanf(args, "%d.%d.%d %d:%d:%d.%d", &year, &month, &day, &hours, &mins, &secs, &milisecs);
		year = _max(year, 1);
		month = _max(month, 1);
		day = _max(day, 1);
		u64 start_game_time = generate_time(year, month, day, hours, mins, secs, milisecs);

		if (!IsAlifeInited())
			return;

		SetGameTime(start_game_time);
	}

	virtual void Status(TStatus& S)
	{
		u32 year = 1, month = 1, day = 1, hours = 0, mins = 0, secs = 0, milisecs = 0;

		if (IsAlifeInited())
			split_time(GetGameTime(), year, month, day, hours, mins, secs, milisecs);

		xr_sprintf(S, "%d.%d.%d %d:%d:%d.%d", year, month, day, hours, mins, secs, milisecs);
	}
};

class CCC_DynamicWeather : public IConsole_Command
{
public:
	CCC_DynamicWeather(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	}
	virtual void Execute(LPCSTR args)
	{
		string256 str;
		str[0] = 0;
		sscanf(args, "%s", str);

		if (!xr_strcmp(str, ""))
		{
			Msg("---Manually sets weather type; Types are specified in environment.ltx->[weathers] section");
			Msg("^ Currently lerping from [%s] to [%s]", g_pGamePersistent->Environment().Current[0]->m_identifier.c_str(), g_pGamePersistent->Environment().Current[1]->m_identifier.c_str());

			return;
		}
		if (MainMenu()->IsActive() || !g_pGameLevel)
			return;
		luabind::functor<void> lua_func;
		if (xr_strcmp(str, "")){
			R_ASSERT2(ai().script_engine().functor("level_weathers.set_weather_manualy", lua_func), "Can't find level_weathers.set_weather_manualy");
			lua_func(str);
		}
	};
};

class CCC_DynamicWeatherSetNext : public IConsole_Command
{
public:
	CCC_DynamicWeatherSetNext(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	}
	virtual void Execute(LPCSTR args)
	{
		string256 str;
		str[0] = 0;
		sscanf(args, "%s", str);
		if (!xr_strcmp(str, "")){
			Msg("---Manually sets next weather, which will be chosen when engine will be selecting next weather; Types are specified in environment.ltx->[weathers] section");
			Msg("^ Currently lerping from [%s] to [%s]", g_pGamePersistent->Environment().Current[0]->m_identifier.c_str(), g_pGamePersistent->Environment().Current[1]->m_identifier.c_str());

			return;
		}

		if (MainMenu()->IsActive() || !g_pGameLevel)
			return;
		luabind::functor<void> lua_func;
		if (xr_strcmp(str, "")){
			R_ASSERT2(ai().script_engine().functor("level_weathers.set_next_weather_manualy", lua_func), "Can't find level_weathers.set_next_weather_manualy");
			lua_func(str);
		}
	};
};

class CCC_DynamicWeatherTune : public IConsole_Command
{
public:
	CCC_DynamicWeatherTune(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	}
	virtual void Execute(LPCSTR args)
	{
		string256 str;
		str[0] = 0;
		sscanf(args, "%s", str);

		if (!xr_strcmp(str, "1"))
		{
			Msg("---Starts the WEditor UI window to help dynamicly tune weather configs");
			Msg("---To properly configure weather section, set time to desired weather_section->time");
			Msg("---Do not configure weather properties at the time that is way after the desired weather time section");
			Msg("---WEditor will automaticly reset the game time to the time at its start up at each change to weather properties");
			Msg("---Do not go back and forth in time due to WE mechanics");
			Msg("---Keep in mind: Engine gets the weathers values for current game time by average from prev. wsection to next wsection");
			Msg("---So if weather params in prev. wsection and next wsection are a lot different, that will affect the whole time betwean");
			Msg("---prev. and next weather sections times");
			Msg("---WEditor will print all properties to log file at its closing. Then you need to manually insert all those properties to ltx file");
			Msg("---Not for static render weathers");

			Msg("^ Currently lerping from [%s] to [%s]", g_pGamePersistent->Environment().Current[0]->m_identifier.c_str(), g_pGamePersistent->Environment().Current[1]->m_identifier.c_str());

			return;
		}
		else
		{
			if (MainMenu()->IsActive() || !g_pGameLevel)
				return;

			CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

			if (!pGameSP)
				return;

			if (!pGameSP->m_WeatherEditor->IsShown())
			{
				pGameSP->m_WeatherEditor->ShowDialog(true);

				GetDayTimeHour(pGameSP->m_WeatherEditor->HoursAtStartUp);

				GetDayTimeMinute(pGameSP->m_WeatherEditor->MinsAtStartUp);

				pGameSP->m_WeatherEditor->InitVars();
				pGameSP->m_WeatherEditor->SaveParams(&pGameSP->m_WeatherEditor->DefaultParams);
			}
			else
			{
				pGameSP->m_WeatherEditor->PrintToLog();
				pGameSP->m_WeatherEditor->HideDialog();
			}
		}
	};
	virtual void Info(TInfo& I)
	{
		xr_strcpy(I, "---Starts the UI window to help dynamicly tune weather configs");
	}
};

class CCC_ReloadCfgEnvironment : public IConsole_Command
{
public:
	CCC_ReloadCfgEnvironment(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	}
	virtual void Execute(LPCSTR args) {
		Msg("---Reloads config system and restarts weather");
		if (MainMenu()->IsActive() || !g_pGameLevel)
			return;
		Level().ReloadEnvironment();
	}

	virtual void Info(TInfo& I)
	{
		xr_strcpy(I, "---Reloads config system and restarts weather");
	}
};

class CCC_Script : public IConsole_Command {
public:
	CCC_Script(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; bLowerCaseArgs = false; };
	virtual void Execute(LPCSTR args) {
		string256	S;
		S[0] = 0;
		sscanf(args, "%s", S);
		if (!xr_strlen(S))
			Log("* Specify script name!");
		else {
			luabind::functor<void>	lua_function;
			if (ai().script_engine().functor<void>(S, lua_function))
				lua_function();
			else
				Log("* Can't find function: ", S);
		}
	}
};

class CCC_Cam_1 : public IConsole_Command {
public:
	CCC_Cam_1(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		if (!g_pGameLevel) return;

		Actor()->IR_OnKeyboardPress(kCAM_1);
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "cam_1 usage");
	}
};

class CCC_Cam_2 : public IConsole_Command {
public:
	CCC_Cam_2(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		if (!g_pGameLevel) return;

		Actor()->IR_OnKeyboardPress(kCAM_2);
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "cam_2 usage");
	}
};

class CCC_ScriptCommand : public IConsole_Command {
public:
	CCC_ScriptCommand(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; bLowerCaseArgs = false; };
	virtual void	Execute(LPCSTR args) {
		if (!xr_strlen(args))
			Log("* Specify string to run!");
		else {
#if 1
			auto sp = ai().script_engine().script_process(ScriptEngine::eScriptProcessorLevel);
			if (sp) {
				sp->add_script(args, true, true);
			}
#else
			string4096		S;
			shared_str		m_script_name = "console command";
			xr_sprintf(S, "%s\n", args);
			int				l_iErrorCode = luaL_loadbuffer(ai().script_engine().lua(), S, xr_strlen(S), "@console_command");
			if (!l_iErrorCode) {
				l_iErrorCode = lua_pcall(ai().script_engine().lua(), 0, 0, 0);
				if (l_iErrorCode) {
					ai().script_engine().print_output(ai().script_engine().lua(), *m_script_name, l_iErrorCode);
					return;
				}
			}
			else {
				ai().script_engine().print_output(ai().script_engine().lua(), *m_script_name, l_iErrorCode);
				return;
			}
#endif
		}
	}
};

void DemoRecordCallback()
{
	CLevel* level = &Level();

	if (level)
	{
		CActor *actor = Actor();

		if (actor && actor->CanBeDrawLegs() && !actor->IsActorShadowsOn())
		{
			actor->setVisible(true);
			actor->SetDrawLegs(true);
		}

		freeCamIsOn_ = false;
	}
}

class CCC_DemoRecord : public IConsole_Command
{
public:
	CCC_DemoRecord(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (freeCamIsOn_)
		{
			Msg("~ Already Active");
			return;
		}

		if (MainMenu()->IsActive() || !g_pGameLevel)
		{
			Msg("!Level is not loaded");
		}
		else
		{
			if (!args || !xr_strcmp(args, ""))
				args = "freecam";

			CActor			*actor = Actor();
			Console->Hide();
			string_path		fn_;
			strconcat(sizeof(fn_), fn_, args, ".xrdemo");
			string_path		fn;
			FS.update_path(fn, "$game_saves$", fn_);

			if (actor && actor->CanBeDrawLegs() && actor->IsFirstEye() && !actor->IsActorShadowsOn())
			{
				actor->setVisible(false);
				actor->SetDrawLegs(false);
			}

			g_pGameLevel->Cameras().AddCamEffector(xr_new <CDemoRecord>(fn, &DemoRecordCallback));

			freeCamIsOn_ = true;
		}
	}
};

class CCC_ALifeTimeFactor : public IConsole_Command {
public:
	CCC_ALifeTimeFactor(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		float id1 = 0.0f;
		sscanf(args, "%f", &id1);
		if (id1 < EPS_L)
			Msg("Invalid time factor! (%.4f)", id1);

		else 
			SetGameTimeFactor(id1);

	}
};

class CCC_ALifeSwitchDistance : public IConsole_Command {
public:
	CCC_ALifeSwitchDistance(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		if (ai().get_alife())
		{
			float id1 = 0.0f;
			sscanf(args, "%f", &id1);
			if (id1 < 2.0f)
				Msg("Invalid online distance! (%.4f)", id1);
			else
			{
				Level().Server->Server_game_sv_base->alife().set_switch_distance(id1);
			}
		}
		else
			Log("!No ALife!");
	}
};

class CCC_ALifeProcessTime : public IConsole_Command {
public:
	CCC_ALifeProcessTime(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args)
	{
		if (ai().get_alife()) 
		{
			int id1 = 0;
			sscanf(args, "%d", &id1);

			if (id1 < 1)
				Msg("Invalid process time! (%d)", id1);
			else
				Alife()->SetGraphProcessTimeLimit((float)id1);
		}
		else
			Log("!No ALife!");
	}
};


class CCC_ALifeObjectsPerUpdate : public IConsole_Command {
public:
	CCC_ALifeObjectsPerUpdate(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args)
	{
		if (ai().get_alife())
		{
			u32 id1 = 0;
			sscanf(args, "%d", &id1);

			Alife()->SetRegidtryUpdateLimits(id1, Alife()->GetRegUpdTimeLimit());
		}
		else
			Log("!No ALife!");
	}
};

class CCC_ALifeSwitchFactor : public IConsole_Command {
public:
	CCC_ALifeSwitchFactor(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args)
	{
		if (ai().get_alife())
		{
			float id1 = 0;
			sscanf(args, "%f", &id1);
			clamp(id1, .1f, 1.f);

			Alife()->set_switch_factor(id1);
		}
		else
			Log("!No ALife!");
	}
};

bool valid_file_name(LPCSTR file_name)
{

	LPCSTR		I = file_name;
	LPCSTR		E = file_name + xr_strlen(file_name);
	for (; I != E; ++I) {
		if (!strchr("/\\:*?\"<>|", *I))
			continue;

		return	(false);
	};

	return		(true);
}

class CCC_ALifeSave : public IConsole_Command {
public:
	CCC_ALifeSave(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args) {

		if (!g_actor || !Actor()->g_Alive())
		{
			Msg("cannot make saved game because actor is dead");
			return;
		}
		if (!Actor()->b_saveAllowed)
		{
			Msg("cannot make saved game because thats is not allowed now.");
			return;
		}

		string_path				S, S1;
		S[0] = 0;
		xr_strcpy(S, args);

		CTimer					timer;
		timer.Start();

		if (!xr_strlen(S)){
			strconcat(sizeof(S), S, Core.UserName, "_", "quicksave");
			NET_Packet			net_packet;
			net_packet.w_begin(M_SAVE_GAME);
			net_packet.w_stringZ(S);
			net_packet.w_u8(0);
			Level().Send(net_packet, net_flags(TRUE));
		}
		else{
			if (!valid_file_name(S)){
				Msg("invalid file name");
				return;
			}

			NET_Packet			net_packet;
			net_packet.w_begin(M_SAVE_GAME);
			net_packet.w_stringZ(S);
			net_packet.w_u8(1);
			Level().Send(net_packet, net_flags(TRUE));
		}

		Msg("Game save : %f milliseconds", timer.GetElapsed_sec()*1000.f);

		SDrawStaticStruct* _s = CurrentGameUI()->AddCustomStatic("game_saved", true);
		_s->m_endTime = EngineTime() + 3.0f;// 3sec
		string_path					save_name;
		strconcat(sizeof(save_name), save_name, *CStringTable().translate("st_game_saved"), ": ", S);
		_s->wnd()->TextItemControl()->SetText(save_name);

		xr_strcat(S, ".dds");
		FS.update_path(S1, "$game_saves$", S);

		timer.Start();

		MainMenu()->Screenshot(IRender_interface::SM_FOR_GAMESAVE, S1);

		Msg("Screenshot : %f milliseconds", timer.GetElapsed_sec()*1000.f);

	}
};

class CCC_ALifeLoadFrom : public IConsole_Command {
public:
	CCC_ALifeLoadFrom(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		if (!ai().get_alife()) {
			Log("! ALife simulator has not been started yet");
			return;
		}

		string256					saved_game;
		saved_game[0] = 0;
		xr_strcpy(saved_game, args);
		if (!xr_strlen(saved_game)) {
			Log("! Specify file name!");
			return;
		}

		if (!CSavedGameWrapper::saved_game_exist(saved_game)) {
			Msg("! Cannot find saved game %s", saved_game);
			return;
		}

		if (!CSavedGameWrapper::valid_saved_game(saved_game)) {
			Msg("! Cannot load saved game %s, version mismatch or saved game is corrupted", saved_game);
			return;
		}

		if (MainMenu()->IsActive())
			MainMenu()->Activate(false);

		if (Device.Paused())
			Device.Pause(FALSE, TRUE, TRUE, "CCC_ALifeLoadFrom");

		NET_Packet					net_packet;
		net_packet.w_begin(M_LOAD_GAME);
		net_packet.w_stringZ(saved_game);
		Level().Send(net_packet, net_flags(TRUE));
	}
};

class CCC_LoadLastSave : public IConsole_Command {
public:
	CCC_LoadLastSave(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = true;
	}

	virtual void	Execute(LPCSTR args)
	{
		if (args && *args) {
			xr_strcpy(g_last_saved_game, args);
			return;
		}

		if (!*g_last_saved_game) {
			Msg("! cannot load last saved game since it hasn't been specified");
			return;
		}

		string512				command;
		if (ai().get_alife()) {
			strconcat(sizeof(command), command, "load ", g_last_saved_game);
			Console->Execute(command);
			return;
		}

		strconcat(sizeof(command), command, "start server(", g_last_saved_game, "/single/alife/load)");
		Console->Execute(command);
	}

	virtual void	Save(IWriter *F)
	{
		if (!*g_last_saved_game)
			return;

		F->w_printf("%s %s\r\n", cName, g_last_saved_game);
	}
};


class CCC_AlifeSpawnArtefacts : public IConsole_Command 
{
public:
	CCC_AlifeSpawnArtefacts(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		if (IsAlifeInited())
		{
			Alife()->DoForceRespawnArts();
		}
		else
			Msg("! No alife");
	}

	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "forcebly spawn artefacts");
	}
};


#include "attachable_item.h"
#include "attachment_owner.h"
class CCC_TuneAttachableItem : public IConsole_Command
{
public:
	CCC_TuneAttachableItem(LPCSTR N) :IConsole_Command(N){
		bEmptyArgsHandled = true;
	}
	virtual void	Execute(LPCSTR args)
	{
		if (CAttachableItem::m_dbgItem){
			CAttachableItem::m_dbgItem = NULL;
			Msg("---TuneAttachableItem switched to off");
			return;
		};

		shared_str argument = args;
		if (!xr_strcmp(argument, "")){
			Msg("---Allows to change rotation and position offsets for attached item, takes section name of an item");
			Msg("---Controls:");
			Msg("--- Q = move x+; E = move x-; S = move y+; W = move y-; A = move z+; D = move z-;");
			Msg("--- Shift A = rotate x+; Shift D = rotate x-; Shift S = rotate y+; Shift W = rotate y-; Shift Q = rotate z+; Shift E = rotate z-;");
			Msg("--- - = move sensitivity-; + = move sensitivity+; Shift - = rotate sensitivity-; Shift + = rotate sensitivity+;");
			Msg("--- P = Print Result To Log;");
			Msg("--- Use DemoRecord to move around camera and then press numpad * to switch to Tuning control or back to DemoRecord control");
			if (g_pGameLevel){
				if (Level().CurrentViewEntity()){
					CObject* obj = Level().CurrentViewEntity();
					CAttachmentOwner* owner = smart_cast<CAttachmentOwner*>(obj);
					Msg("--- Avalable items to configure;");
					owner->LOG_AttachedItemsList();
				}
			}
			return;
		}

		if (MainMenu()->IsActive() || !g_pGameLevel)
			return;

		if (!Level().CurrentViewEntity())
			return;

		CObject* obj = Level().CurrentViewEntity();
		CAttachmentOwner* owner = smart_cast<CAttachmentOwner*>(obj);
		CAttachableItem* itm = owner->attachedItem(argument);
		if (itm){
			CAttachableItem::m_dbgItem = itm;
			Msg("---TuneAttachableItem switched to ON for [%s]", args);
		}
		else{
			Msg("---TuneAttachableItem cannot find attached item [%s]", args);
			owner->LOG_AttachedItemsList();
		}
	}

	virtual void	Info(TInfo& I)
	{
		xr_sprintf(I, "---Allows to change rotation and position offsets for attached item, takes section name of an item");
	}
};

extern void print_help(lua_State *L);

struct CCC_LuaHelp : public IConsole_Command {
	CCC_LuaHelp(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args) {
		print_help(ai().script_engine().lua());
	}
};

struct DumpTxrsForPrefetching : public IConsole_Command {
	DumpTxrsForPrefetching(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args) {
		g_pGamePersistent->ReportUITxrsForPrefetching();
	}
};

class CCC_DumpInfos : public IConsole_Command {
public:
	CCC_DumpInfos(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void	Execute(LPCSTR args) {
		CActor* A = smart_cast<CActor*>(Level().CurrentEntity());
		if (A)
			A->DumpInfo();
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "dumps all infoportions that actor have");
	}
};

class CCC_MainMenu : public IConsole_Command {
public:
	CCC_MainMenu(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args) {

		bool bWhatToDo = TRUE;
		if (0 == xr_strlen(args)){
			bWhatToDo = !MainMenu()->IsActive();
		};

		if (EQ(args, "on") || EQ(args, "1"))
			bWhatToDo = TRUE;

		if (EQ(args, "off") || EQ(args, "0"))
			bWhatToDo = FALSE;

		MainMenu()->Activate(bWhatToDo);
	}
};


//---Development cheats
class CCC_Spawn : public IConsole_Command {
public:
	CCC_Spawn(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		if (!g_pGameLevel) return;

		if (!pSettings->section_exist(args))
		{
			Msg("! Section [%s] isn`t exist...", args);
			return;
		}

		char	Name[128];	Name[0] = 0;
		sscanf(args, "%s", Name);
		Fvector pos = Actor()->Position();
		pos.y += 3.0f;
		Level().g_cl_Spawn(Name, 0xff, M_SPAWN_OBJECT_LOCAL, pos);
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "name,team,squad,group");
	}
};

#	include "game_graph.h"
struct CCC_JumpToLevel : public IConsole_Command {
	CCC_JumpToLevel(LPCSTR N) : IConsole_Command(N)  {};

	virtual void Execute(LPCSTR args) {
		if (!ai().get_alife()) {
			Msg("! ALife simulator is needed to perform specified command!");
			return;
		}
		string256		level;
		sscanf(args, "%s", level);

		GameGraph::LEVEL_MAP::const_iterator	I = ai().game_graph().header().levels().begin();
		GameGraph::LEVEL_MAP::const_iterator	E = ai().game_graph().header().levels().end();
		for (; I != E; ++I)
			if (!xr_strcmp((*I).second.name(), level)) {
				ai().alife().jump_to_level(level);
				return;
			}
		Msg("! There is no level \"%s\" in the game graph!", level);
	}
};


//---Features
class CCC_PHIterations : public CCC_Integer {
public:
	CCC_PHIterations(LPCSTR N) :
		CCC_Integer(N, &phIterations, 15, 50)
	{};
	virtual void	Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);
		dWorldSetQuickStepNumIterations(NULL, phIterations);
	}
};

class CCC_PHGravity : public IConsole_Command {
public:
	CCC_PHGravity(LPCSTR N) :
		IConsole_Command(N)
	{};
	virtual void	Execute(LPCSTR args)
	{
		if (!physics_world())	return;

		if (!g_pGameLevel || !Level().level_game_cl_base)
		{
			Msg("!Command is not available now");
			return;
		}

		physics_world()->SetGravity(float(atof(args)));
	}
	virtual void	Status(TStatus& S)
	{
		if (physics_world())
			xr_sprintf(S, "%3.5f", physics_world()->Gravity());
		else
			xr_sprintf(S, "%3.5f", default_world_gravity);
		while (xr_strlen(S) && ('0' == S[xr_strlen(S) - 1]))	S[xr_strlen(S) - 1] = 0;
	}

};

class CCC_PHFps : public IConsole_Command {
public:
	CCC_PHFps(LPCSTR N) :
		IConsole_Command(N)
	{};
	virtual void	Execute(LPCSTR args)
	{
		float				step_count = (float)atof(args);
		clamp(step_count, 50.f, 200.f);

		ph_console::ph_step_time = 1.f / step_count;
		//physics_world()->SetStep(1.f/step_count);
		if (physics_world())
			physics_world()->SetStep(ph_console::ph_step_time);
	}
	virtual void	Status(TStatus& S)
	{
		xr_sprintf(S, "%3.5f", 1.f / ph_console::ph_step_time);
	}

};

struct CCC_XrGameSwitchAllMT : public IConsole_Command {
	CCC_XrGameSwitchAllMT(LPCSTR N) : IConsole_Command(N) {};

	virtual void Execute(LPCSTR args)
	{
		string256 value;
		sscanf(args, "%s", value);

		int res = std::stoi(value);

		if (res == 0 || res == 1)
		{
			if (res == 0)
			{
				g_mt_config = { 0 };
			}
			else
			{
				g_mt_config.set(mtAiVision, true);
				g_mt_config.set(mtLevelPath, true);
				g_mt_config.set(mtDetailPath, true);
				g_mt_config.set(mtMap, true);
				g_mt_config.set(mtObjectHandler, true);
				g_mt_config.set(mtSoundPlayer, true);
				g_mt_config.set(mtBullets, true);
				g_mt_config.set(mtLUA_GC, true);
				g_mt_config.set(mtLevelSounds, true);
				g_mt_config.set(mtALife, true);
				g_mt_config.set(mtIKinematics, true);
				g_mt_config.set(mtAIMisc, true);
				g_mt_config.set(mtLoadNPCSounds, true);
			}
		}
		else
			Msg("^ Valid arguments are 0 or 1");
	}
};

//---Gameplay
class CCC_GameDifficulty : public CCC_Token {
public:
	CCC_GameDifficulty(LPCSTR N) : CCC_Token(N, (u32*)&g_SingleGameDifficulty, difficulty_type_token)  {};
	virtual void Execute(LPCSTR args) {
		CCC_Token::Execute(args);
		if (g_pGameLevel && Level().level_game_cl_base){
			VERIFY(Level().level_game_cl_base);
			Level().level_game_cl_base->OnDifficultyChanged();
		}
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "Game difficulty");
	}
};

struct CCC_ChangeLanguage : public IConsole_Command {
	CCC_ChangeLanguage(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled		= true; }

	virtual void Execute(LPCSTR args) {

		if (!args || !*args)
		{
			Msg					("! no arguments passed");
			return;
		}

		bool b_exist = !!pSettings->line_exist("languages",args);
		if (!b_exist)
		{
			Msg							("! Can't find language \"%s\" in the section [languages]!",args);
			return;
		}

		g_language = args;

		if (g_pGamePersistent && !MainMenu()->IsActive())
			CStringTable().ReloadLanguage();
	}

	virtual void	Save				(IWriter *F)
	{
		if (!*g_language)
			return;

		F->w_printf				("%s %s\r\n",cName,g_language.c_str()); 
	}

	virtual void	Status	(TStatus& S)
	{	
		xr_sprintf	(S,"%s",g_language.c_str());	  
	}
};

class	CCC_UiHud_Mode		: public CCC_Token
{
public:
	CCC_UiHud_Mode(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N,V,T)	{}	;

	virtual void	Execute	(LPCSTR args)	{
		CCC_Token::Execute	(args);

		if (g_pGamePersistent && g_pGameLevel && Level().level_game_cl_base)
		{
			if (*value >= 1 && *value <= 3)
			{
				HUD().OnScreenResolutionChanged(); //перезагрузка окон
				CurrentGameUI()->ReinitDialogs(); //реинит диалоговых окон вроде talk wnd
			}
		}
	}
};


//---Debug
class CCC_FloatBlock : public CCC_Float {
public:
	CCC_FloatBlock(LPCSTR N, float* V, float _min = 0, float _max = 1) :
		CCC_Float(N, V, _min, _max)
	{};

	virtual void	Execute(LPCSTR args)
	{
		if (!g_pGameLevel)
			CCC_Float::Execute(args);
		else
		{
			Msg("! Command disabled for this type of game");
		}
	}
};

class CCC_DumpTasks : public IConsole_Command {
public:
	CCC_DumpTasks(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void	Execute(LPCSTR args) {
		CActor* A = smart_cast<CActor*>(Level().CurrentEntity());
		if (A)
			A->DumpTasks();
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "dumps all tasks that actor have");
	}
};

#include "map_manager.h"
class CCC_DumpMap : public IConsole_Command {
public:
	CCC_DumpMap(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void	Execute(LPCSTR args) {
		Level().MapManager().PrintMapLocationsInfo();
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "Dumps all currentmap locations");
	}

};

#ifdef DEBUG
class CCC_InvUpgradesHierarchy : public IConsole_Command
{
public:
	CCC_InvUpgradesHierarchy(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		if (ai().get_alife())
		{
			ai().alife().inventory_upgrade_manager().log_hierarchy();
		}
	}
};

class CCC_InvUpgradesCurItem : public IConsole_Command
{
public:
	CCC_InvUpgradesCurItem(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		if (!g_pGameLevel)
		{
			return;
		}
		CUIGameSP* ui_game_sp = smart_cast<CUIGameSP*>(CurrentGameUI());
		if (!ui_game_sp)
		{
			return;
		}
		PIItem item = ui_game_sp->TalkMenu->GetUpgradeWnd()->get_upgrade_item();
		if (item)
		{
			item->log_upgrades();
		}
		else
		{
			Msg("- Current item in ActorMenu is unknown!");
		}
	}
};
#endif

class CCC_InvDropAllItems : public IConsole_Command
{
public:
	CCC_InvDropAllItems(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		if (!g_pGameLevel)
		{
			return;
		}

		CUIGameSP* ui_game_sp = smart_cast<CUIGameSP*>(CurrentGameUI());

		if (!ui_game_sp)
		{
			return;
		}

		int d = 0;
		sscanf(args, "%d", &d);

		if (ui_game_sp->m_InventoryMenu->DropAllItemsFromRuck(d == 1))
		{
			Msg("- All items from ruck of Actor is dropping now.");
		}
		else
		{
			Msg("! ActorMenu is not in state `Inventory`");
		}
	}
};

#ifdef DEBUG

class CCC_ALifePath : public IConsole_Command {
public:
	CCC_ALifePath(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		if (!ai().get_level_graph())
			Msg("! there is no graph!");
		else {
			int id1=-1, id2=-1;
			sscanf(args ,"%d %d",&id1,&id2);
			if ((-1 != id1) && (-1 != id2))
				if (_max(id1,id2) > (int)ai().game_graph().header().vertex_count() - 1)
					Msg("! there are only %d vertexes!",ai().game_graph().header().vertex_count());
				else
					if (_min(id1,id2) < 0)
						Msg("! invalid vertex number (%d)!",_min(id1,id2));
					else {
						//						Sleep				(1);
						//						CTimer				timer;
						//						timer.Start			();
						//						float				fValue = ai().m_tpAStar->ffFindMinimalPath(id1,id2);
						//						Msg					("* %7.2f[%d] : %11I64u cycles (%.3f microseconds)",fValue,ai().m_tpAStar->m_tpaNodes.size(),timer.GetElapsed_ticks(),timer.GetElapsed_ms()*1000.f);
					}
			else
				Msg("! not enough parameters!");
		}
	}
};

class CCC_DrawGameGraphAll : public IConsole_Command {
public:
				 CCC_DrawGameGraphAll	(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = true;
	}

	virtual void Execute				(LPCSTR args)
	{
		if (!ai().get_level_graph())
			return;

		ai().level_graph().setup_current_level	(-1);
	}
};

class CCC_DrawGameGraphCurrent : public IConsole_Command {
public:
				 CCC_DrawGameGraphCurrent	(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = true;
	}

	virtual void Execute					(LPCSTR args)
	{
		if (!ai().get_level_graph())
			return;

		ai().level_graph().setup_current_level	(
			ai().level_graph().level_id()
		);
	}
};

class CCC_DrawGameGraphLevel : public IConsole_Command {
public:
				 CCC_DrawGameGraphLevel	(LPCSTR N) : IConsole_Command(N)
	{
	}

	virtual void Execute					(LPCSTR args)
	{
		if (!ai().get_level_graph())
			return;

		string256			S;
		S[0]				= 0;
		sscanf				(args,"%s",S);

		if (!*S) {
			ai().level_graph().setup_current_level	(-1);
			return;
		}

		const GameGraph::SLevel	*level = ai().game_graph().header().level(S,true);
		if (!level) {
			Msg				("! There is no level %s in the game graph",S);
			return;
		}

		ai().level_graph().setup_current_level	(level->id());
	}
};

class CCC_ScriptDbg : public IConsole_Command {
public:
	CCC_ScriptDbg(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args) {
		
		if(strstr(cName,"script_debug_break")==cName ){
		
		CScriptDebugger* d = ai().script_engine().debugger();
		if(d){
			if(d->Active())
				d->initiateDebugBreak();
			else
				Msg("Script debugger not active.");
		}else
			Msg("Script debugger not present.");
		}
		else if(strstr(cName,"script_debug_stop")==cName ){
			ai().script_engine().stopDebugger();
		}
		else if(strstr(cName,"script_debug_restart")==cName ){
			ai().script_engine().restartDebugger();
		};
	};
	

	virtual void	Info	(TInfo& I)		
	{
		if(strstr(cName,"script_debug_break")==cName )
			xr_strcpy(I,"initiate script debugger [DebugBreak] command"); 

		else if(strstr(cName,"script_debug_stop")==cName )
			xr_strcpy(I,"stop script debugger activity"); 

		else if(strstr(cName,"script_debug_restart")==cName )
			xr_strcpy(I,"restarts script debugger or start if no script debugger presents"); 
	}
};

#include "alife_graph_registry.h"
class CCC_DumpCreatures : public IConsole_Command {
public:
	CCC_DumpCreatures	(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void	Execute				(LPCSTR args) {
		
		typedef CSafeMapIterator<ALife::_OBJECT_ID,CSE_ALifeDynamicObject>::_REGISTRY::const_iterator const_iterator;

		const_iterator I = ai().alife().graph().level().objects().begin();
		const_iterator E = ai().alife().graph().level().objects().end();
		for ( ; I != E; ++I) {
			CSE_ALifeCreatureAbstract *obj = smart_cast<CSE_ALifeCreatureAbstract *>(I->second);
			if (obj) {
				Msg("\"%s\",",obj->name_replace());
			}
		}		

	}
	virtual void	Info	(TInfo& I)		
	{
		xr_strcpy(I,"dumps all creature names"); 
	}

};

class CCC_DebugFonts : public IConsole_Command {
public:
	CCC_DebugFonts (LPCSTR N) : IConsole_Command(N) {bEmptyArgsHandled = true; }
	virtual void Execute				(LPCSTR args) {
		CUIDebugFonts* temp = xr_new <CUIDebugFonts>();
		temp->ShowDialog(true);
	}
};

class CCC_DebugNode : public IConsole_Command {
public:
	CCC_DebugNode(LPCSTR N) : IConsole_Command(N)  { };

	virtual void Execute(LPCSTR args) {

		string128 param1, param2;
		_GetItem(args,0,param1,' ');
		_GetItem(args,1,param2,' ');

		u32 value1;
		u32 value2;
		
		sscanf(param1,"%u",&value1);
		sscanf(param2,"%u",&value2);
		
		if ((value1 > 0) && (value2 > 0)) {
			g_bDebugNode		= TRUE;
			g_dwDebugNodeSource	= value1;
			g_dwDebugNodeDest	= value2;
		} else {
			g_bDebugNode = FALSE;
		}
	}
};

class CCC_ShowMonsterInfo : public IConsole_Command {
public:
				CCC_ShowMonsterInfo(LPCSTR N) : IConsole_Command(N)  { };

	virtual void Execute(LPCSTR args) {

		string128 param1, param2;
		_GetItem(args,0,param1,' ');
		_GetItem(args,1,param2,' ');

		CObject			*obj = Level().Objects.FindObjectByName(param1);
		CBaseMonster	*monster = smart_cast<CBaseMonster *>(obj);
		if (!monster)	return;
		
		u32				value2;
		
		sscanf			(param2,"%u",&value2);
		monster->set_show_debug_info (u8(value2));
	}
};
class CCC_DbgPhTrackObj : public IConsole_Command {
public:
	CCC_DbgPhTrackObj(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args/**/) {
			//ph_dbg_draw_mask1.set(ph_m1_DbgTrackObject,TRUE);
			//PH_DBG_SetTrackObject(args);
			//CObject* O= Level().Objects.FindObjectByName(args);
			//if(O)
			//{
			//	PH_DBG_SetTrackObject(*(O->ObjectName()));
			//	ph_dbg_draw_mask1.set(ph_m1_DbgTrackObject,TRUE);
			//}

		}
	
	//virtual void	Info	(TInfo& I)		
	//{
	//	xr_strcpy(I,"restart game fast"); 
	//}
};

class CCC_DbgVar : public IConsole_Command {
public:
	CCC_DbgVar(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
	virtual void Execute(LPCSTR arguments)
	{
		if (!arguments || !*arguments)
		{
			return;
		}

		if (_GetItemCount(arguments, ' ') == 1)
		{
			ai_dbg::show_var(arguments);
		}
		else
		{
			char  name[1024];
			float f;
			sscanf(arguments, "%s %f", name, &f);
			ai_dbg::set_var(name, f);
		}

	}
};

void DBG_CashedClear();
class CCC_DBGDrawCashedClear : public IConsole_Command {
public:
	CCC_DBGDrawCashedClear(LPCSTR N) :IConsole_Command(N) { bEmptyArgsHandled = true; }
private:
	virtual void	Execute(LPCSTR args)
	{
		DBG_CashedClear();
	}
};

#endif


#ifdef DEBUG

struct CCC_ShowSmartCastStats : public IConsole_Command {
	CCC_ShowSmartCastStats(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args) {
		show_smart_cast_stats();
	}
};

struct CCC_ClearSmartCastStats : public IConsole_Command {
	CCC_ClearSmartCastStats(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args) {
		clear_smart_cast_stats();
	}
};
#endif



#ifdef DEBUG
class CCC_RadioGroupMask2;
class CCC_RadioMask :public CCC_Mask
{
	CCC_RadioGroupMask2		*group;
public:
	CCC_RadioMask(LPCSTR N, Flags32* V, u32 M):
	  CCC_Mask(N,V,M)
	 {
		group=NULL;
	 }
		void	SetGroup	(CCC_RadioGroupMask2		*G)
	{
		group=G													;
	}
virtual	void	Execute		(LPCSTR args)						;
	
IC		void	Set			(BOOL V)
	  {
		  value->set(mask,V)									;
	  }

};

class CCC_RadioGroupMask2 
{
	CCC_RadioMask *mask0;
	CCC_RadioMask *mask1;
public:
	CCC_RadioGroupMask2(CCC_RadioMask *m0,CCC_RadioMask *m1)
	  {
		mask0=m0;mask1=m1;
		mask0->SetGroup(this);
		mask1->SetGroup(this);
	  }
	void	Execute	(CCC_RadioMask& m,LPCSTR args)
	{
		BOOL value=m.GetValue();
		if(value)
		{
			mask0->Set(!value);mask1->Set(!value);
		}
		m.Set(value);
	}
};


void	CCC_RadioMask::Execute	(LPCSTR args)
{
	CCC_Mask::Execute(args);
	VERIFY2(group,"CCC_RadioMask: group not set");
	group->Execute(*this,args);
}

#define CMD_RADIOGROUPMASK2(p1,p2,p3,p4,p5,p6)		\
{\
static CCC_RadioMask x##CCC_RadioMask1(p1,p2,p3);		Console->AddCommand(&x##CCC_RadioMask1);\
static CCC_RadioMask x##CCC_RadioMask2(p4,p5,p6);		Console->AddCommand(&x##CCC_RadioMask2);\
static CCC_RadioGroupMask2 x##CCC_RadioGroupMask2(&x##CCC_RadioMask1,&x##CCC_RadioMask2);\
}

struct CCC_DbgBullets : public CCC_Integer {
	CCC_DbgBullets(LPCSTR N, int* V, int _min=0, int _max=999) : CCC_Integer(N,V,_min,_max) {};

	virtual void	Execute	(LPCSTR args)
	{
		extern FvectorVec g_hit[];
		g_hit[0].clear();
		g_hit[1].clear();
		g_hit[2].clear();
		CCC_Integer::Execute	(args);
	}
};

#ifdef DEBUG_MEMORY_MANAGER

class CCC_MemAllocShowStats : public IConsole_Command {
public:
	CCC_MemAllocShowStats(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR) {
		mem_alloc_show_stats	();
	}
};

class CCC_MemAllocClearStats : public IConsole_Command {
public:
	CCC_MemAllocClearStats(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR) {
		mem_alloc_clear_stats	();
	}
};

#endif // DEBUG_MEMORY_MANAGER

class CCC_DumpModelBones : public IConsole_Command {
public:
	CCC_DumpModelBones	(LPCSTR N) : IConsole_Command(N)
	{
	}
	
	virtual void Execute(LPCSTR arguments)
	{
		if (!arguments || !*arguments) {
			Msg					("! no arguments passed");
			return;
		}

		string_path				name;
		string_path				fn;

		if (0==strext(arguments))
			strconcat			(sizeof(name),name,arguments,".ogf");
		else
			xr_strcpy			(name,sizeof(name),arguments);

		if (!FS.exist(arguments) && !FS.exist(fn, "$level$", name) && !FS.exist(fn, "$game_meshes$", name)) {
			Msg					("! Cannot find visual \"%s\"",arguments);
			return;
		}

		IRenderVisual			*visual = Render->model_Create(arguments);
		IKinematics				*kinematics = smart_cast<IKinematics*>(visual);
		if (!kinematics) {
			Render->model_Delete(visual);
			Msg					("! Invalid visual type \"%s\" (not a CKinematics)",arguments);
			return;
		}

		Msg						("bones for model \"%s\"",arguments);
		for (u16 i=0, n=kinematics->LL_BoneCount(); i<n; ++i)
			Msg					("%s",*kinematics->LL_GetData(i).name);
		
		Render->model_Delete	(visual);
	}
};

extern void show_animation_stats	();

class CCC_ShowAnimationStats : public IConsole_Command {
public:
	CCC_ShowAnimationStats(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR)
	{
		show_animation_stats	();
	}
};

#endif

static void LoadTokensFromIni(xr_vector<xr_token>& tokens, LPCSTR section)
{
	tokens.clear();
	for (auto &item : pSettings->r_section(section).Data)
	{
		tokens.push_back({item.first.c_str(), atoi(item.second.c_str())});
	}
	tokens.push_back({nullptr, 0});
}

//----------------------------------------Console Commands----------------------------------------

void CCC_RegisterCommands()
{
	g_uCommonFlags = {0};
	g_uCommonFlags.set		(CF_AiUseTorchDynamicLights, true);
	g_uCommonFlags.set		(CF_Prefetch_UI, false);
	psActorFlags.set		(AF_ALWAYSRUN, true);
	psActorFlags.set		(AF_WPN_BOBBING, true);
	psActorFlags.set		(AF_WPN_AUTORELOAD, true);
	psActorFlags.set		(AF_HEAD_BOBBING, true);
	psActorFlags.set		(AF_CORRECT_FIREPOS, false);
	psActorFlags.set		(AF_USE_3D_SCOPES, false);
	psHUD_Flags.set			(HUD_CROSSHAIR, true);
	psHUD_Flags.set			(HUD_WEAPON, true);
	psHUD_Flags.set			(HUD_NPC_COUNTER, true);
	psHUD_Flags.set			(HUD_NPC_DETECTION, true);
	psHUD_Flags.set			(HUD_DRAW, true);
	psHUD_Flags.set			(HUD_INFO, true);
	psHUD_Flags.set			(HUD_INFO_CNAME, false);

	//---Development
	CMD1(CCC_LogPrint,				"print_to_log");
	CMD1(CCC_SetGameTime,			"set_game_time");

	CMD1(CCC_TimeFactor,			"time_factor");
	CMD1(CCC_StartTimeSingle,		"start_time_single");
	CMD1(CCC_TimeFactorGame,		"time_factor_single");

	CMD1(CCC_DynamicWeather,		"set_weather");
	CMD1(CCC_DynamicWeatherSetNext,	"set_next_weather_to_choose");
	CMD1(CCC_DynamicWeatherTune,	"weather_editor");
	CMD1(CCC_ReloadCfgEnvironment,	"cfg_weather_restart");

	CMD1(CCC_DemoRecord,			"demo_record");

	if (CApplication::isDeveloperMode)
	{
		CMD1(CCC_Script,				"run_script");
		CMD1(CCC_ScriptCommand,			"run_string");
	}

	CMD1(CCC_ALifeSave,				"save");
	CMD1(CCC_ALifeLoadFrom,			"load");
	CMD1(CCC_LoadLastSave,			"load_last_save");

	CMD1(CCC_LuaHelp,				"lua_help");

	CMD3(CCC_Mask,					"prefetch_ui_textures", &g_uCommonFlags, CF_Prefetch_UI); //Lowers lagging when ui windows are opened the first time

	CMD4(CCC_Integer,				"hud_adjust_mode",          &hud_adj_mode,      0, 11);
	CMD4(CCC_Integer,				"hud_adjust_item_index",	&hud_adj_item_idx,	0, 1);
	CMD4(CCC_Integer,				"hud_adjust_addon_index",	&hud_adj_addon_idx,	0, 2);

	CMD4(CCC_Float,					"hud_adjust_delta_pos",		&hud_adj_delta_pos,	0.0002f, 1.f);
	CMD4(CCC_Float,					"hud_adjust_delta_rot",		&hud_adj_delta_rot,	0.005f, 10.f);

	CMD1(CCC_TuneAttachableItem,	"adjust_attachable_item");

	CMD1(CCC_MainMenu,				"main_menu");

	CMD1(CCC_ALifeTimeFactor,		"al_time_factor");			// set time factor
	CMD1(CCC_ALifeSwitchDistance,	"al_switch_distance");		// set switch distance
	CMD1(CCC_ALifeProcessTime,		"al_process_time");			// set process time
	CMD1(CCC_ALifeObjectsPerUpdate, "al_objects_per_update");	// set process time
	CMD1(CCC_ALifeSwitchFactor,		"al_switch_factor");		// set switch factor

	CMD4(CCC_Float,					"g_bullet_time_factor", &g_bullet_time_factor, 0.f, 10.f);

	CMD4(CCC_Vector3,				"psp_cam_offset", &CCameraLook2::m_cam_offset, Fvector().set(-1000, -1000, -1000), Fvector().set(1000, 1000, 1000));

	CMD1(CCC_Cam_1,					"g_cam_1");
	CMD1(CCC_Cam_2,					"g_cam_2");

	CMD4(CCC_Integer,				"string_table_error_msg", &CStringTable::m_bWriteErrorsToLog, 0, 1);

	CMD3(CCC_Mask,					"hud_info_cname", &psHUD_Flags, HUD_INFO_CNAME);

	//---Development cheats
	if (CApplication::isDeveloperMode)
	{
		CMD3(CCC_Mask,					"ai_ignore_actor", &psActorFlags, AF_INVISIBLE);
		CMD3(CCC_Mask,					"g_god", &psActorFlags, AF_GODMODE);
		CMD1(CCC_Spawn,					"g_spawn");
		CMD3(CCC_Mask,					"g_unlimitedammo", &psActorFlags, AF_UNLIMITEDAMMO);

		CMD3(CCC_Mask,					"ai_no_invulnarable", &g_uCommonFlags, CF_NoInvulnarable);
	}

	CMD1(CCC_JumpToLevel,			"jump_to_level");
	CMD1(CCC_AlifeSpawnArtefacts,	"alife_spawn_artefacts");


	//---Features
	CMD3(CCC_Mask,					"ph_collision_of_corpses", &psActorFlags, AF_COLLISION);
	CMD1(CCC_PHGravity,				"ph_gravity");
	CMD1(CCC_PHFps,					"ph_frequency");
	CMD1(CCC_PHIterations,			"ph_iterations");

	CMD3(CCC_Token,					"ph_particles_collision", &particlesCollision_, particles_collision_token);
	CMD4(CCC_Float,					"ph_particles_collision_distance", &particlesCollisionDistance_, 20.f, 1000.0f);

	CMD3(CCC_Mask,					"ai_use_torch_dynamic_lights", &g_uCommonFlags, CF_AiUseTorchDynamicLights);
	CMD4(CCC_Integer,				"g_intro", &ps_intro, 0, 1);

	CMD3(CCC_Mask,					"mt_ai_vision", &g_mt_config, mtAiVision);
	CMD3(CCC_Mask,					"mt_ai_misc", &g_mt_config, mtAIMisc);
	CMD3(CCC_Mask,					"mt_level_path", &g_mt_config, mtLevelPath);
	CMD3(CCC_Mask,					"mt_level_map", &g_mt_config, mtMap);
	CMD3(CCC_Mask,					"mt_detail_path", &g_mt_config, mtDetailPath);
	CMD3(CCC_Mask,					"mt_object_handler", &g_mt_config, mtObjectHandler);
	CMD3(CCC_Mask,					"mt_sound_player", &g_mt_config, mtSoundPlayer);
	CMD3(CCC_Mask,					"mt_bullets", &g_mt_config, mtBullets);
	CMD3(CCC_Mask,					"mt_script_gc", &g_mt_config, mtLUA_GC);
	CMD3(CCC_Mask,					"mt_level_sounds", &g_mt_config, mtLevelSounds);
	CMD3(CCC_Mask,					"mt_alife", &g_mt_config, mtALife);
	CMD3(CCC_Mask,					"mt_ikinematics", &g_mt_config, mtIKinematics);
	CMD3(CCC_Mask,					"mt_load_sounds_in_aux_thread", &g_mt_config, mtLoadNPCSounds); // only NPC talk sounds
	CMD1(CCC_XrGameSwitchAllMT,		"mt_xrgame");

	CMD4(CCC_Integer,				"inverse_kinematics", &useInverseKinematics_, 0, 1);

	CMD4(CCC_Integer,				"g_game_objects_lshadows", &gameObjectLightShadows_, 0, 1); // need game save reload
	
	CMD3(CCC_Mask,					"ai_obstacles_avoiding", &psAI_Flags, aiObstaclesAvoiding);
	CMD3(CCC_Mask,					"ai_obstacles_avoiding_static", &psAI_Flags, aiObstaclesAvoidingStatic);
	CMD3(CCC_Mask,					"ai_use_smart_covers", &psAI_Flags, aiUseSmartCovers);
	CMD3(CCC_Mask,					"ai_use_smart_covers_animation_slots", &psAI_Flags, (u32)aiUseSmartCoversAnimationSlot);

	CMD4(CCC_Float,					"ai_smart_factor", &g_smart_cover_factor, 0.f, 1000000.f);
	CMD4(CCC_Float,					"ai_smart_cover_animation_speed_factor", &g_smart_cover_animation_speed_factor, .1f, 10.f);
	

	//---Gameplay
	CMD1(CCC_GameDifficulty,		"g_game_difficulty");
	CMD3(CCC_Mask,					"g_backrun", &psActorFlags, AF_RUN_BACKWARD);
	CMD3(CCC_Mask,					"g_always_run", &psActorFlags, AF_ALWAYSRUN);
	CMD3(CCC_Mask,					"weapon_bobbing", &psActorFlags, AF_WPN_BOBBING);
	CMD3(CCC_Mask,					"weapon_autoreload", &psActorFlags, AF_WPN_AUTORELOAD);
	CMD3(CCC_Mask,					"weapon_hold_zoom", &psHoldZoom, 1);
	CMD3(CCC_Mask,					"g_head_bobbing", &psActorFlags, AF_HEAD_BOBBING);
	CMD3(CCC_Mask,					"g_correct_firepos", &psActorFlags, AF_CORRECT_FIREPOS);
	CMD3(CCC_Mask,					"weapon_strafe_inertion", &psActorFlags, AF_STRAFE_INERT);
	CMD3(CCC_Mask,					"g_actor_body", &psActorFlags, AF_ACTOR_BODY);
	CMD3(CCC_Mask,					"g_autopickup", &psActorFlags, AF_AUTOPICKUP);
	CMD3(CCC_Mask,					"g_first_person_death", &psActorFlags, AF_FST_PSN_DEATH);
	CMD3(CCC_Mask,					"g_3d_scopes", &psActorFlags, AF_USE_3D_SCOPES);

	CMD3(CCC_Mask,					"hud_weapon", &psHUD_Flags, HUD_WEAPON);
	CMD3(CCC_Mask,					"hud_info", &psHUD_Flags, HUD_INFO);
	CMD3(CCC_Mask,					"hud_draw", &psHUD_Flags, HUD_DRAW);
	CMD3(CCC_Mask,					"hud_crosshair", &psHUD_Flags, HUD_CROSSHAIR);
	CMD3(CCC_Mask,					"hud_crosshair_dist", &psHUD_Flags, HUD_CROSSHAIR_DIST);
	CMD3(CCC_Token,					"hud_crosshair_type", &crosshairAnimationType, qcrosshair_type_token);
	CMD3(CCC_Mask,					"hud_npc_detection", &psHUD_Flags, HUD_NPC_DETECTION);
	CMD3(CCC_Mask,					"hud_npc_counter", &psHUD_Flags, HUD_NPC_COUNTER);
	LoadTokensFromIni				(qhud_type_token, "ui_hud_types");
	CMD3(CCC_UiHud_Mode,			"ui_hud_type", &ui_hud_type, qhud_type_token.data());
	CMD3(CCC_Mask,					"cl_dynamiccrosshair", &psHUD_Flags, HUD_CROSSHAIR_DYNAMIC);
	CMD3(CCC_Mask,					"show_clock", &psHUD_Flags, HUD_SHOW_CLOCK);
	CMD1(CCC_ChangeLanguage,		"language");

	CMD4(CCC_Integer,				"quick_save_counter", &quick_save_counter, 0, 128);
	CMD4(CCC_Integer,				"max_quick_saves", &max_quick_saves, 0, 128);

	CMD4(CCC_Integer,				"snd_max_shot_sounds", &sndMaxShotSounds_, 15, 40);

	CMD4(CCC_Float,					"air_resistance_epsilon", &air_resistance_epsilon, .0f, 1.f);


	//---Debug
	CMD1(CCC_DumpMap,				"dump_map_spots");
	CMD4(CCC_Integer,				"display_actor_lum", &showActorLuminocity_, 0, 1);
	CMD1(CCC_DumpInfos,				"dump_infos");
	CMD1(DumpTxrsForPrefetching,	"dump_ui_textures_for_prefetching"); //Prints the list of UI textures, which caused stutterings during game

	CMD4(CCC_Integer,				"lua_gcstep", &psLUA_GCSTEP, 1, 1000);

	CMD4(CCC_Integer,				"keypress_on_start", &g_keypress_on_start, 0, 1);

	CMD4(CCC_Float,					"ai_aim_predict_time", &g_aim_predict_time, 0.f, 10.f);
	CMD4(CCC_Float,					"ai_aim_min_speed", &g_ai_aim_min_speed, 0.f, 10.f*PI);
	CMD4(CCC_Float,					"ai_aim_min_angle", &g_ai_aim_min_angle, 0.f, 10.f*PI);
	CMD4(CCC_Float,					"ai_aim_max_angle", &g_ai_aim_max_angle, 0.f, 10.f*PI);

	CMD4(CCC_Integer,				"ai_use_old_vision", &g_ai_use_old_vision, 0, 1);

	CMD4(CCC_FloatBlock,			"camera_collision_character_shift_z", &camera_collision_character_shift_z, 0.f, 1.f);
	CMD4(CCC_FloatBlock,			"camera_collision_character_skin_depth", &camera_collision_character_skin_depth, 0.f, 1.f);

	CMD1(CCC_InvDropAllItems,		"inv_drop_all_items");

#ifdef DEBUG_MEMORY_MANAGER
	CMD3(CCC_Mask,					"debug_on_frame_gather_stats", &psAI_Flags, aiDebugOnFrameAllocs);
	CMD4(CCC_Float,					"debug_on_frame_gather_stats_frequency", &debug_on_frame_gather_stats_frequency, 0.f, 1.f);
	CMD1(CCC_MemAllocShowStats,		"debug_on_frame_show_stats");
	CMD1(CCC_MemAllocClearStats,	"debug_on_frame_clear_stats");
#endif

#ifdef DEBUG
	CMD4(CCC_FloatBlock,			"ph_timefactor", &phTimefactor, 0.0001f, 1000.f);
	CMD4(CCC_FloatBlock,			"ph_break_common_factor", &ph_console::phBreakCommonFactor, 0.f, 1000000000.f);
	CMD4(CCC_FloatBlock,			"ph_rigid_break_weapon_factor", &ph_console::phRigidBreakWeaponFactor, 0.f, 1000000000.f);
	CMD4(CCC_Integer,				"ph_tri_clear_disable_count", &ph_console::ph_tri_clear_disable_count, 0, 255);
	CMD4(CCC_FloatBlock,			"ph_tri_query_ex_aabb_rate", &ph_console::ph_tri_query_ex_aabb_rate, 1.01f, 3.f);

	CMD1(CCC_InvUpgradesHierarchy,	"inv_upgrades_hierarchy");
	CMD1(CCC_InvUpgradesCurItem,	"inv_upgrades_cur_item");
	CMD4(CCC_Integer,				"inv_upgrades_log", &g_upgrades_log, 0, 1);

	CMD1(CCC_ALifePath,				"al_path");	// build path

	CMD3(CCC_Mask,					"ai_debug", &psAI_Flags, aiDebug);
	CMD3(CCC_Mask,					"ai_dbg_brain", &psAI_Flags, aiBrain);
	CMD3(CCC_Mask,					"ai_dbg_motion", &psAI_Flags, aiMotion);
	CMD3(CCC_Mask,					"ai_dbg_frustum", &psAI_Flags, aiFrustum);
	CMD3(CCC_Mask,					"ai_dbg_funcs", &psAI_Flags, aiFuncs);
	CMD3(CCC_Mask,					"ai_dbg_alife", &psAI_Flags, aiALife);
	CMD3(CCC_Mask,					"ai_dbg_goap", &psAI_Flags, aiGOAP);
	CMD3(CCC_Mask,					"ai_dbg_goap_script", &psAI_Flags, aiGOAPScript);
	CMD3(CCC_Mask,					"ai_dbg_goap_object", &psAI_Flags, aiGOAPObject);
	CMD3(CCC_Mask,					"ai_dbg_cover", &psAI_Flags, aiCover);
	CMD3(CCC_Mask,					"ai_dbg_anim", &psAI_Flags, aiAnimation);
	CMD3(CCC_Mask,					"ai_dbg_vision", &psAI_Flags, aiVision);
	CMD3(CCC_Mask,					"ai_dbg_monster", &psAI_Flags, aiMonsterDebug);
	CMD3(CCC_Mask,					"ai_dbg_stalker", &psAI_Flags, aiStalker);
	CMD3(CCC_Mask,					"ai_stats", &psAI_Flags, aiStats);
	CMD3(CCC_Mask,					"ai_dbg_destroy", &psAI_Flags, aiDestroy);
	CMD3(CCC_Mask,					"ai_dbg_serialize", &psAI_Flags, aiSerialize);
	CMD3(CCC_Mask,					"ai_dbg_dialogs", &psAI_Flags, aiDialogs);
	CMD3(CCC_Mask,					"ai_dbg_infoportion", &psAI_Flags, aiInfoPortion);
	CMD3(CCC_Mask,					"ai_dbg_path", &psAI_Flags, aiPath);
	CMD3(CCC_Mask,					"ai_dbg_restrictors", &psAI_Flags, aiTestCorrectness);
	CMD3(CCC_Mask,					"ai_debug_msg", &psAI_Flags, aiDebugMsg);

	CMD3(CCC_Mask,					"ai_draw_game_graph", &psAI_Flags, aiDrawGameGraph);
	CMD3(CCC_Mask,					"ai_draw_game_graph_stalkers", &psAI_Flags, aiDrawGameGraphStalkers);
	CMD3(CCC_Mask,					"ai_draw_game_graph_objects", &psAI_Flags, aiDrawGameGraphObjects);
	CMD3(CCC_Mask,					"ai_draw_game_graph_real_pos", &psAI_Flags,	aiDrawGameGraphRealPos);
	CMD1(CCC_DrawGameGraphAll,		"ai_draw_game_graph_all");
	CMD1(CCC_DrawGameGraphCurrent,	"ai_draw_game_graph_current_level");
	CMD1(CCC_DrawGameGraphLevel,	"ai_draw_game_graph_level");

	CMD3(CCC_Mask,					"ai_nil_object_access", &psAI_Flags, aiNilObjectAccess);

	CMD3(CCC_Mask,					"ai_draw_visibility_rays", &psAI_Flags, aiDrawVisibilityRays);
	CMD3(CCC_Mask,					"ai_animation_stats", &psAI_Flags, aiAnimationStats);
	CMD1(CCC_ShowAnimationStats,	"ai_show_animation_stats");

	CMD4(CCC_Integer,				"ai_dbg_inactive_time", &g_AI_inactive_time, 0, 1000000);

	CMD1(CCC_DebugNode,				"ai_dbg_node");

	CMD1(CCC_ShowMonsterInfo,		"ai_monster_info");

	CMD4(CCC_Integer,				"ai_dbg_sight", &g_ai_dbg_sight, 0, 1);
	CMD4(CCC_Integer,				"ai_aim_use_smooth_aim", &g_ai_aim_use_smooth_aim, 0, 1);

	CMD4(CCC_Integer,				"ai_debug_doors", &g_debug_doors, 0, 1);\

	CMD1(CCC_ScriptDbg,				"script_debug_break");
	CMD1(CCC_ScriptDbg,				"script_debug_stop");
	CMD1(CCC_ScriptDbg,				"script_debug_restart");

	CMD1(CCC_ShowSmartCastStats,	"show_smart_cast_stats");
	CMD1(CCC_ClearSmartCastStats,	"clear_smart_cast_stats");

	CMD1(CCC_DumpModelBones,		"debug_dump_model_bones");
	CMD1(CCC_DebugFonts,			"debug_fonts");

	CMD4(CCC_Integer,				"debug_step_info", &debug_step_info, FALSE, TRUE);
	CMD4(CCC_Integer,				"debug_step_info_load", &debug_step_info_load, FALSE, TRUE);

	CMD4(CCC_Integer,				"debug_character_material_load", &debug_character_material_load, FALSE, TRUE);

	CMD3(CCC_String,				"stalker_death_anim", dbg_stalker_death_anim, 32);
	CMD4(CCC_Integer,				"death_anim_velocity", &b_death_anim_velocity, FALSE,	TRUE );
	CMD4(CCC_Integer,				"show_wnd_rect_all",			&g_show_wnd_rect2, 0, 1);
	CMD4(CCC_Integer,				"dbg_show_ani_info",	&g_ShowAnimationInfo,	0, 1)	;
	CMD4(CCC_Integer,				"dbg_dump_physics_step", &ph_console::g_bDebugDumpPhysicsStep, 0, 1);

	CMD3(CCC_Mask,					"dbg_draw_actor_alive",		&dbg_net_Draw_Flags, dbg_draw_actor_alive);
	CMD3(CCC_Mask,					"dbg_draw_actor_dead",		&dbg_net_Draw_Flags, dbg_draw_actor_dead);
	CMD3(CCC_Mask,					"dbg_draw_customzone", &dbg_net_Draw_Flags, dbg_draw_customzone);
	CMD3(CCC_Mask,					"dbg_draw_teamzone", &dbg_net_Draw_Flags, dbg_draw_teamzone);
	CMD3(CCC_Mask,					"dbg_draw_invitem",			&dbg_net_Draw_Flags, dbg_draw_invitem);
	CMD3(CCC_Mask,					"dbg_draw_actor_phys",		&dbg_net_Draw_Flags, dbg_draw_actor_phys);
	CMD3(CCC_Mask,					"dbg_draw_customdetector",	&dbg_net_Draw_Flags, dbg_draw_customdetector);
	CMD3(CCC_Mask,					"dbg_destroy",				&dbg_net_Draw_Flags, dbg_destroy);
	CMD3(CCC_Mask,					"dbg_draw_autopickupbox",	&dbg_net_Draw_Flags, dbg_draw_autopickupbox);
	CMD3(CCC_Mask,					"dbg_draw_rp", &dbg_net_Draw_Flags, dbg_draw_rp);
	CMD3(CCC_Mask,					"dbg_draw_climbable", &dbg_net_Draw_Flags, dbg_draw_climbable);
	CMD3(CCC_Mask,					"dbg_draw_skeleton", &dbg_net_Draw_Flags, dbg_draw_skeleton);
	CMD4(CCC_Integer,				"dbg_draw_ragdoll_spawn", &dbg_draw_ragdoll_spawn, FALSE, TRUE);
	CMD4(CCC_Integer,				"dbg_draw_camera_collision", &dbg_draw_camera_collision, FALSE, TRUE);

	CMD3(CCC_Mask,					"dbg_draw_ph_contacts", &ph_dbg_draw_mask, phDbgDrawContacts);
	CMD3(CCC_Mask,					"dbg_draw_ph_enabled_aabbs", &ph_dbg_draw_mask, phDbgDrawEnabledAABBS);
	CMD3(CCC_Mask,					"dbg_draw_ph_intersected_tries", &ph_dbg_draw_mask, phDBgDrawIntersectedTries);
	CMD3(CCC_Mask,					"dbg_draw_ph_saved_tries", &ph_dbg_draw_mask, phDbgDrawSavedTries);
	CMD3(CCC_Mask,					"dbg_draw_ph_tri_trace", &ph_dbg_draw_mask, phDbgDrawTriTrace);
	CMD3(CCC_Mask,					"dbg_draw_ph_positive_tries", &ph_dbg_draw_mask, phDBgDrawPositiveTries);
	CMD3(CCC_Mask,					"dbg_draw_ph_negative_tries", &ph_dbg_draw_mask, phDBgDrawNegativeTries);
	CMD3(CCC_Mask,					"dbg_draw_ph_tri_test_aabb", &ph_dbg_draw_mask, phDbgDrawTriTestAABB);
	CMD3(CCC_Mask,					"dbg_draw_ph_tries_changes_sign", &ph_dbg_draw_mask, phDBgDrawTriesChangesSign);
	CMD3(CCC_Mask,					"dbg_draw_ph_tri_point", &ph_dbg_draw_mask, phDbgDrawTriPoint);
	CMD3(CCC_Mask,					"dbg_draw_ph_explosion_position", &ph_dbg_draw_mask, phDbgDrawExplosionPos);
	CMD3(CCC_Mask,					"dbg_draw_ph_statistics", &ph_dbg_draw_mask, phDbgDrawObjectStatistics);
	CMD3(CCC_Mask,					"dbg_draw_ph_mass_centres", &ph_dbg_draw_mask, phDbgDrawMassCenters);
	CMD3(CCC_Mask,					"dbg_draw_ph_death_boxes", &ph_dbg_draw_mask, phDbgDrawDeathActivationBox);
	CMD3(CCC_Mask,					"dbg_draw_ph_hit_app_pos", &ph_dbg_draw_mask, phHitApplicationPoints);
	CMD3(CCC_Mask,					"dbg_draw_ph_cashed_tries_stats", &ph_dbg_draw_mask, phDbgDrawCashedTriesStat);
	CMD3(CCC_Mask,					"dbg_draw_ph_car_dynamics", &ph_dbg_draw_mask, phDbgDrawCarDynamics);
	CMD3(CCC_Mask,					"dbg_draw_ph_car_plots", &ph_dbg_draw_mask, phDbgDrawCarPlots);
	CMD3(CCC_Mask,					"dbg_ph_ladder", &ph_dbg_draw_mask, phDbgLadder);
	CMD3(CCC_Mask,					"dbg_draw_ph_explosions", &ph_dbg_draw_mask, phDbgDrawExplosions);
	CMD3(CCC_Mask,					"dbg_draw_car_plots_all_trans", &ph_dbg_draw_mask, phDbgDrawCarAllTrnsm);
	CMD3(CCC_Mask,					"dbg_draw_ph_zbuffer_disable", &ph_dbg_draw_mask, phDbgDrawZDisable);
	CMD3(CCC_Mask,					"dbg_ph_obj_collision_damage", &ph_dbg_draw_mask, phDbgDispObjCollisionDammage);
	CMD_RADIOGROUPMASK2(			"dbg_ph_ai_always_phmove", &ph_dbg_draw_mask, phDbgAlwaysUseAiPhMove, "dbg_ph_ai_never_phmove", &ph_dbg_draw_mask, phDbgNeverUseAiPhMove);
	CMD3(CCC_Mask,					"dbg_ph_ik", &ph_dbg_draw_mask, phDbgIK);
	CMD3(CCC_Mask,					"dbg_ph_ik_off", &ph_dbg_draw_mask1, phDbgIKOff);
	CMD3(CCC_Mask,					"dbg_draw_ph_ik_goal", &ph_dbg_draw_mask, phDbgDrawIKGoal);
	CMD3(CCC_Mask,					"dbg_ph_ik_limits", &ph_dbg_draw_mask, phDbgIKLimits);
	CMD3(CCC_Mask,					"dbg_ph_character_control", &ph_dbg_draw_mask, phDbgCharacterControl);
	CMD1(CCC_DBGDrawCashedClear,	"dbg_ph_cashed_clear");
	CMD3(CCC_Mask,					"dbg_draw_ph_ray_motions", &ph_dbg_draw_mask, phDbgDrawRayMotions);
	CMD4(CCC_Float,					"dbg_ph_vel_collid_damage_to_display", &dbg_vel_collid_damage_to_display, 0.f, 1000.f);
	CMD4(CCC_DbgBullets,			"dbg_draw_bullet_hit", &g_bDrawBulletHit, 0, 1);
	CMD1(CCC_DbgPhTrackObj,			"dbg_track_obj");
	CMD3(CCC_Mask,					"dbg_ph_actor_restriction", &ph_dbg_draw_mask1, ph_m1_DbgActorRestriction);
	CMD3(CCC_Mask,					"dbg_draw_ph_hit_anims", &ph_dbg_draw_mask1, phDbgHitAnims);
	CMD3(CCC_Mask,					"dbg_draw_ph_ik_limits", &ph_dbg_draw_mask1, phDbgDrawIKLimits);
	CMD3(CCC_Mask,					"dbg_draw_ph_ik_predict", &ph_dbg_draw_mask1, phDbgDrawIKPredict);
	CMD3(CCC_Mask,					"dbg_draw_ph_ik_collision", &ph_dbg_draw_mask1, phDbgDrawIKCollision);
	CMD3(CCC_Mask,					"dbg_draw_ph_ik_shift_object", &ph_dbg_draw_mask1, phDbgDrawIKSHiftObject);
	CMD3(CCC_Mask,					"dbg_draw_ph_ik_blending", &ph_dbg_draw_mask1, phDbgDrawIKBlending);
	CMD4(CCC_Integer,				"dbg_draw_animation_movement_controller", &dbg_draw_animation_movement_controller, FALSE, TRUE);
	CMD4(CCC_Integer,				"dbg_draw_character_bones", &dbg_draw_character_bones, FALSE, TRUE);
	CMD4(CCC_Integer,				"dbg_draw_character_physics", &dbg_draw_character_physics, FALSE, TRUE);
	CMD4(CCC_Integer,				"dbg_draw_character_binds", &dbg_draw_character_binds, FALSE, TRUE);
	CMD4(CCC_Integer,				"dbg_draw_character_physics_pones", &dbg_draw_character_physics_pones, FALSE, TRUE);
	CMD4(CCC_Integer,				"dbg_draw_doors", &dbg_draw_doors, FALSE, TRUE);

	CMD4(CCC_Integer,				"dbg_bones_snd_player", &dbg_moving_bones_snd_player, FALSE, TRUE);

	CMD3(CCC_Mask,					"dbg_track_obj_blends_bp_0", &dbg_track_obj_flags, dbg_track_obj_blends_bp_0);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_bp_1", &dbg_track_obj_flags, dbg_track_obj_blends_bp_1);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_bp_2", &dbg_track_obj_flags, dbg_track_obj_blends_bp_2);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_bp_3", &dbg_track_obj_flags, dbg_track_obj_blends_bp_3);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_motion_name", &dbg_track_obj_flags, dbg_track_obj_blends_motion_name);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_time", &dbg_track_obj_flags, dbg_track_obj_blends_time);

	CMD3(CCC_Mask,					"dbg_track_obj_blends_ammount", &dbg_track_obj_flags, dbg_track_obj_blends_ammount);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_mix_params", &dbg_track_obj_flags, dbg_track_obj_blends_mix_params);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_flags", &dbg_track_obj_flags, dbg_track_obj_blends_flags);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_state", &dbg_track_obj_flags, dbg_track_obj_blends_state);
	CMD3(CCC_Mask,					"dbg_track_obj_blends_dump", &dbg_track_obj_flags, dbg_track_obj_blends_dump);

	CMD1(CCC_DbgVar,				"dbg_var");

	CMD4(CCC_FloatBlock,			"dbg_text_height_scale", &dbg_text_height_scale, 0.2f, 5.f);

	CMD4(CCC_Integer,				"dbg_imotion_draw_velocity", &dbg_imotion_draw_velocity, FALSE, TRUE);
	CMD4(CCC_Integer,				"dbg_imotion_collide_debug", &dbg_imotion_collide_debug, FALSE, TRUE);

	CMD4(CCC_Integer,				"dbg_imotion_draw_skeleton", &dbg_imotion_draw_skeleton, FALSE, TRUE);
	CMD4(CCC_Float,					"dbg_imotion_draw_velocity_scale", &dbg_imotion_draw_velocity_scale, 0.0001f, 100.0f);

	CMD4(CCC_Float,					"hit_anims_power", &ghit_anims_params.power_factor, 0.0f, 100.0f);
	CMD4(CCC_Float,					"hit_anims_rotational_power", &ghit_anims_params.rotational_power_factor, 0.0f, 100.0f);
	CMD4(CCC_Float,					"hit_anims_side_sensitivity_threshold", &ghit_anims_params.side_sensitivity_threshold, 0.0f, 10.0f);
	CMD4(CCC_Float,					"hit_anims_channel_factor", &ghit_anims_params.anim_channel_factor, 0.0f, 100.0f);

	CMD4(CCC_Float,					"hit_anims_block_blend", &ghit_anims_params.block_blend, 0.f, 1.f);
	CMD4(CCC_Float,					"hit_anims_reduce_blend", &ghit_anims_params.reduce_blend, 0.f, 1.f);
	CMD4(CCC_Float,					"hit_anims_reduce_blend_factor", &ghit_anims_params.reduce_power_factor, 0.0f, 1.0f);
	CMD4(CCC_Integer,				"hit_anims_tune", &tune_hit_anims, 0, 1);

	CMD4(CCC_Integer,				"death_anim_debug", &death_anim_debug, FALSE, TRUE);

	CMD1(CCC_DumpTasks,				"dump_tasks");
	CMD1(CCC_DumpCreatures,			"dump_creatures");
#endif 

	*g_last_saved_game = 0;
}
