#include "pch_script.h"
#include "game_cl_base.h"
#include "script_engine.h"
#include "level.h"
#include "GamePersistent.h"
#include "UIGameSP.h"
#include "clsid_game.h"
#include "actor.h"

ESingleGameDifficulty g_SingleGameDifficulty = egdStalker;

xr_token	difficulty_type_token[] = {
	{ "gd_novice", egdNovice },
	{ "gd_stalker", egdStalker },
	{ "gd_veteran", egdVeteran },
	{ "gd_master", egdMaster },
	{ 0, 0 }
};

game_cl_GameState::game_cl_GameState()
{
	shedule.t_min				= 5;
	shedule.t_max				= 20;
	shedule_register			();
}

game_cl_GameState::~game_cl_GameState()
{
	shedule_unregister();
}

float game_cl_GameState::shedule_Scale		()
{
	return 1.0f;
}

void game_cl_GameState::ScheduledUpdate(u32 dt)
{
	ISheduled::ScheduledUpdate(dt);
};

CUIGameCustom* game_cl_GameState::createGameUI()
{
	CLASS_ID clsid = CLSID_GAME_UI_SINGLE;
	CUIGameSP*	pUIGame = smart_cast<CUIGameSP*> (NEW_INSTANCE(clsid));
	R_ASSERT(pUIGame);
	pUIGame->Load();
	pUIGame->Init(0);
	pUIGame->Init(1);
	pUIGame->Init(2);
	return					pUIGame;
}

void game_cl_GameState::u_EventGen(NET_Packet& P, u16 type, u16 dest)
{
	P.w_begin	(M_EVENT);
	P.w_u32		(Level().timeServer());
	P.w_u16		(type);
	P.w_u16		(dest);
}

void game_cl_GameState::u_EventSend(NET_Packet& P)
{
	Level().Send(P,net_flags(TRUE,TRUE));
}

void game_cl_GameState::SendPickUpEvent(u16 ID_who, u16 ID_what)
{
	CObject* O		= Level().Objects.net_Find	(ID_what);
	Level().m_feel_deny.feel_touch_deny			(O, 1000);

	NET_Packet		P;
	u_EventGen		(P,GE_OWNERSHIP_TAKE, ID_who);
	P.w_u16			(ID_what);
	u_EventSend		(P);
};

//Game difficulty
void game_cl_GameState::OnDifficultyChanged()
{
	Actor()->OnDifficultyChanged();
}

//---Alife
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_time_manager.h"

using namespace luabind;
#pragma optimize("s",on)
void CScriptGameDifficulty::script_register(lua_State *L)
{
	module(L)
		[
			class_<enum_exporter<ESingleGameDifficulty> >("game_difficulty")
			.enum_("game_difficulty")
			[
				value("novice", int(egdNovice)),
				value("stalker", int(egdStalker)),
				value("veteran", int(egdVeteran)),
				value("master", int(egdMaster))
			]
		];
}