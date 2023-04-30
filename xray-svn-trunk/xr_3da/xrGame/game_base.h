//Base Class for game_client and game_server instances
//Cleaned from Multiplayer

#pragma once

#include "script_export_space.h"
class	NET_Packet;

class	game_GameState : public DLL_Pure
{
public:
									game_GameState			();
	virtual							~game_GameState			()								{}
	virtual		void				Create					(shared_str& options){};

	DECLARE_SCRIPT_REGISTER_FUNCTION
};


add_to_type_list(game_GameState)
#undef script_type_list
#define script_type_list save_type_list(game_GameState)

