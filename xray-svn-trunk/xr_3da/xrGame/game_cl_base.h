//Client's Game Base Instance
//Cleaned from Multiplayer
#pragma once
#include "game_base.h"


class	NET_Packet;
class	CUIGameCustom;

// game difficulty
enum ESingleGameDifficulty{
	egdNovice = 0,
	egdStalker = 1,
	egdVeteran = 2,
	egdMaster = 3,
	egdCount,
	egd_force_u32 = u32(-1)
};

extern ESingleGameDifficulty g_SingleGameDifficulty;
xr_token		difficulty_type_token[];

typedef enum_exporter<ESingleGameDifficulty> CScriptGameDifficulty;
add_to_type_list(CScriptGameDifficulty)
#undef script_type_list
#define script_type_list save_type_list(CScriptGameDifficulty)

class	game_cl_GameState	: public game_GameState, public ISheduled
{
	typedef game_GameState	inherited;
protected:

	shared_str						SchedulerName			() const		{ return shared_str("game_cl_GameState"); };
	float							shedule_Scale();
	bool							shedule_Needed()				{return true;};
public:
									game_cl_GameState		();
	virtual							~game_cl_GameState		();

	CUIGameCustom*					createGameUI();

	void							ScheduledUpdate			(u32 dt);

	void							u_EventGen				(NET_Packet& P, u16 type, u16 dest);
	void							u_EventSend				(NET_Packet& P);

	void							SendPickUpEvent			(u16 ID_who, u16 ID_what);

	void							OnDifficultyChanged();

	IC bool							IsDifficultyStalkerOrHigher() { return g_SingleGameDifficulty >= egdStalker; };
	IC bool							IsDifficultyVeteranOrHigher() { return g_SingleGameDifficulty >= egdVeteran; };
};
