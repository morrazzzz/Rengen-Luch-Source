#ifndef _INCDEF_XRMESSAGES_H_
#define _INCDEF_XRMESSAGES_H_

#pragma once

enum {
	M_UPDATE = 0,	// DUAL: Update state
	M_SPAWN,		// DUAL: Spawning, full state

	M_SV_CONFIG_NEW_CLIENT,
	M_SV_CONFIG_FINISHED,

	M_EVENT,

	M_CL_UPDATE,
	
	M_CHANGE_LEVEL,
	M_LOAD_GAME,
	M_SAVE_GAME,
	M_SAVE_PACKET,

	M_EVENT_PACK,

	M_CLIENT_REQUEST_CONNECTION_DATA,

	MSG_FORCEDWORD				= u32(-1)
};

enum {
	GE_OWNERSHIP_TAKE,			// DUAL: Client request for ownership of an item
	GE_OWNERSHIP_REJECT,		// DUAL: Client request ownership rejection

	GE_HIT,						//
	GE_DIE,						//
	GE_ASSIGN_KILLER,			//
	GE_DESTROY,					// authorative client request for entity-destroy
	GE_TELEPORT_OBJECT,

	GE_ADD_RESTRICTION,
	GE_REMOVE_RESTRICTION,
	GE_REMOVE_ALL_RESTRICTIONS,

	GE_TRADE_SELL,
	GE_TRADE_BUY,

	GE_INV_BOX_STATUS,
	GE_INV_OWNER_STATUS,
	GE_GAME_EVENT,

	GE_KILL_SOMEONE,

	GE_LAUNCH_ROCKET,

	GE_FORCEDWORD				= u32(-1)
};


enum EGameMessages {  //game_cl <----> game_sv messages
	GAME_EVENT_CREATE_CLIENT,
	GAME_EVENT_ON_HIT,

	GAME_EVENT_FORCEDWORD				= u32(-1)
};

enum
{
	M_SPAWN_OBJECT_LOCAL		= (1<<0),	// after spawn it becomes local (authorative)
	M_SPAWN_OBJECT_HASUPDATE	= (1<<2),	// after spawn info it has update inside message
	M_SPAWN_OBJECT_ASPLAYER		= (1<<3),	// after spawn it must become viewable
	M_SPAWN_OBJECT_PHANTOM		= (1<<4),	// after spawn it must become viewable
	M_SPAWN_VERSION				= (1<<5),	// control version
	M_SPAWN_UPDATE				= (1<<6),	// + update packet
	M_SPAWN_TIME				= (1<<7),	// + spawn time
	M_SPAWN_DENIED				= (1<<8),	// don't spawn entity with this flag

	M_SPAWN_OBJECT_FORCEDWORD	= u32(-1)
};

#endif /*_INCDEF_XRMESSAGES_H_*/
