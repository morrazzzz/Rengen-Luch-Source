//Server's Game Base Instance
//Cleaned from Multiplayer

#pragma once

#include "game_base.h"
#include "alife_space.h"
#include "script_export_space.h"

class CSE_Abstract;
class xrServer;
class GameEventQueue;
class CALifeSimulator;

class	game_sv_GameState	: public game_GameState
{
	typedef game_GameState inherited;
protected:

	xrServer*					m_server;
	GameEventQueue*				m_event_queue;
		
	void						OnEvent					(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender );

	CALifeSimulator				*m_alife_simulator;
public:
								game_sv_GameState();
	virtual						~game_sv_GameState();

	void						NewPlayerName_Generate	(void* pClient, LPSTR NewPlayerName);
	void						NewPlayerName_Replace	(void* pClient, LPCSTR NewPlayerName);

	CSE_Abstract*				get_entity_from_eid		(u16 id);

	// Utilities
	xr_vector<u16>*				get_children			(ClientID id_who);
	void						u_EventGen				(NET_Packet& P, u16 type, u16 dest	);
	void						u_EventSend				(NET_Packet& P, u32 dwFlags = DPNSEND_GUARANTEED);

	// Events
	void						OnCreate				(u16 id_who);
	BOOL						OnTouch					(u16 eid_who, u16 eid_target, BOOL bForced = FALSE);			// TRUE=allow ownership, FALSE=denied
	void						OnDetach				(u16 eid_who, u16 eid_target);	

	// Main
	virtual		void			Create					(shared_str& options);
	void						Update();

	bool						change_level			(NET_Packet &net_packet, ClientID sender);
	void						save_game				(NET_Packet &net_packet, ClientID sender);
	bool						load_game				(NET_Packet &net_packet, ClientID sender);

	void						AddDelayedEvent			(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender );
	void						ProcessDelayedEvent();

	void						teleport_object			(NET_Packet &packet, u16 id);

	void						add_restriction			(NET_Packet &packet, u16 id);
	void						remove_restriction		(NET_Packet &packet, u16 id);
	void						remove_all_restrictions	(NET_Packet &packet, u16 id);

	bool						custom_sls_default()	{return !!m_alife_simulator;};
	void						sls_default();

	shared_str					level_name				(const shared_str &server_options) const;
	static	shared_str			parse_level_name		(const shared_str &server_options);	
	static	shared_str			parse_level_version		(const shared_str &server_options);

	void						on_death				(CSE_Abstract *e_dest, CSE_Abstract *e_src);

	void						restart_simulator(LPCSTR saved_game_name);

	IC	xrServer		&server() const
	{
		VERIFY(m_server);
		return(*m_server);
	}

	IC	CALifeSimulator	&alife() const
	{
		VERIFY(m_alife_simulator);
		return(*m_alife_simulator);
	}

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(game_sv_GameState)
#undef script_type_list
#define script_type_list save_type_list(game_sv_GameState)