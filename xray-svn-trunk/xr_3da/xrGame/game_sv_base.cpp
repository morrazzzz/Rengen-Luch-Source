#include "stdafx.h"
#include "game_sv_event_queue.h"
#include "game_sv_base.h"
#include "xrServer.h"
#include "script_process.h"
#include "script_engine.h"
#include "level.h"
#include "gamepersistent.h"
#include "ai_space.h"
#include "object_broker.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "alife_time_manager.h"
#include "../x_ray.h"

game_sv_GameState::game_sv_GameState()
{
	VERIFY(g_pGameLevel);
	m_server = Level().Server;
	m_event_queue = xr_new <GameEventQueue>();

	m_alife_simulator = NULL;
}

game_sv_GameState::~game_sv_GameState()
{
	ai().script_engine().remove_script_process(ScriptEngine::eScriptProcessorGame);
	xr_delete(m_event_queue);
	delete_data(m_alife_simulator);
}

CSE_Abstract* game_sv_GameState::get_entity_from_eid(u16 id)
{
	return				m_server->ID_to_entity(id);
}

xr_vector<u16>* game_sv_GameState::get_children(ClientID id)
{
	xrClientData*	C	= (xrClientData*)m_server->ID_to_client	(id);
	if (0==C)			return 0;
	CSE_Abstract* E	= C->owner;
	if (0==E)			return 0;
	return	&(E->children);
}

void game_sv_GameState::Create(shared_str &options)
{
	// loading scripts
	ai().script_engine().remove_script_process(ScriptEngine::eScriptProcessorGame);

	ai().script_engine().add_script_process(ScriptEngine::eScriptProcessorGame, xr_new <CScriptProcess>("game", ""));

	if (strstr(*options, "/alife"))
		m_alife_simulator = xr_new <CALifeSimulator>(&server(), &options);
}

void game_sv_GameState::Update()
{
	if (Level().level_game_cl_base) {
		CScriptProcess				*script_process = ai().script_engine().script_process(ScriptEngine::eScriptProcessorGame);
		if (script_process)
			script_process->update();
	}
}

void game_sv_GameState::u_EventGen(NET_Packet& P, u16 type, u16 dest)
{
	P.w_begin(M_EVENT);
	P.w_u32(Level().timeServer());
	P.w_u16(type);
	P.w_u16(dest);
}

void game_sv_GameState::u_EventSend(NET_Packet& P, u32 dwFlags)
{
	m_server->SendBroadcast(BroadcastCID, P, dwFlags);
}

void game_sv_GameState::OnEvent(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender)
{
	switch (type)
	{
	case GAME_EVENT_ON_HIT:
	{
		m_server->SendBroadcast(BroadcastCID, tNetPacket, net_flags(TRUE, TRUE));
	}break;
	case GAME_EVENT_CREATE_CLIENT:
	{
		IClient* CL = (IClient*)m_server->ID_to_client(sender);
		if (CL == NULL) { break; }

		CL->flags.bConnected = TRUE;
		m_server->AttachNewClient(CL);
	}break;
	default:
	{
		string16 tmp;
		R_ASSERT3(0, "Game Event not implemented!!!", itoa(type, tmp, 10));
	};
	};
}

void game_sv_GameState::AddDelayedEvent(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender)
{
	m_event_queue->Create(tNetPacket, type, time, sender);
}

void game_sv_GameState::ProcessDelayedEvent()
{
	GameEvent* ge = NULL;
	while ((ge = m_event_queue->Retreive()) != 0) {
		OnEvent(ge->P, ge->type, ge->time, ge->sender);
		m_event_queue->Release();
	}
}

void game_sv_GameState::OnCreate(u16 id_who)
{
	if (!ai().get_alife())
		return;

	CSE_Abstract			*e_who = get_entity_from_eid(id_who);
	VERIFY(e_who);
	if (!e_who->m_bALifeControl)
		return;

	CSE_ALifeObject			*alife_object = smart_cast<CSE_ALifeObject*>(e_who);
	if (!alife_object)
		return;

	alife_object->m_bOnline = true;

	if (alife_object->ID_Parent != 0xffff) {
		CSE_ALifeDynamicObject			*parent = ai().alife().objects().object(alife_object->ID_Parent, true);
		if (parent) {
			CSE_ALifeTraderAbstract		*trader = smart_cast<CSE_ALifeTraderAbstract*>(parent);
			if (trader)
				alife().create(alife_object);
			else {
				CSE_ALifeInventoryBox* const	box = smart_cast<CSE_ALifeInventoryBox*>(parent);
				if (box)
					alife().create(alife_object);
				else
					alife_object->m_bALifeControl = false;
			}
		}
		else
			alife_object->m_bALifeControl = false;
	}
	else
		alife().create(alife_object);
}

BOOL game_sv_GameState::OnTouch(u16 eid_who, u16 eid_what, BOOL bForced)
{
	CSE_Abstract*		e_who = get_entity_from_eid(eid_who);		VERIFY(e_who);
	CSE_Abstract*		e_what = get_entity_from_eid(eid_what);	VERIFY(e_what);

	if (ai().get_alife()) {
		CSE_ALifeInventoryItem	*l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>	(e_what);
		CSE_ALifeDynamicObject	*l_tpDynamicObject = smart_cast<CSE_ALifeDynamicObject*>	(e_who);

		if (
			l_tpALifeInventoryItem &&
			l_tpDynamicObject &&
			ai().alife().graph().level().object(l_tpALifeInventoryItem->base()->ID, true) &&
			ai().alife().objects().object(e_who->ID, true) &&
			ai().alife().objects().object(e_what->ID, true)
			)
			alife().graph().attach(*e_who, l_tpALifeInventoryItem, l_tpDynamicObject->m_tGraphID, false, false);
#ifdef DEBUG
		else
			if (psAI_Flags.test(aiALife)) {
				Msg("Cannot attach object [%s][%s][%d] to object [%s][%s][%d]", e_what->name_replace(), *e_what->s_name, e_what->ID, e_who->name_replace(), *e_who->s_name, e_who->ID);
			}
#endif
	}
	return TRUE;
}

void game_sv_GameState::OnDetach(u16 eid_who, u16 eid_what)
{
	if (ai().get_alife())
	{
		CSE_Abstract*		e_who = get_entity_from_eid(eid_who);		VERIFY(e_who);
		CSE_Abstract*		e_what = get_entity_from_eid(eid_what);	VERIFY(e_what);

		CSE_ALifeInventoryItem *l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(e_what);
		if (!l_tpALifeInventoryItem)
			return;

		CSE_ALifeDynamicObject *l_tpDynamicObject = smart_cast<CSE_ALifeDynamicObject*>(e_who);
		if (!l_tpDynamicObject)
			return;

		if (
			ai().alife().objects().object(e_who->ID, true) &&
			!ai().alife().graph().level().object(l_tpALifeInventoryItem->base()->ID, true) &&
			ai().alife().objects().object(e_what->ID, true)
			)
			alife().graph().detach(*e_who, l_tpALifeInventoryItem, l_tpDynamicObject->m_tGraphID, false, false);
		else {
			if (!ai().alife().objects().object(e_what->ID, true)) {
				u16				id = l_tpALifeInventoryItem->base()->ID_Parent;
				l_tpALifeInventoryItem->base()->ID_Parent = 0xffff;

				CSE_ALifeDynamicObject *dynamic_object = smart_cast<CSE_ALifeDynamicObject*>(e_what);
				VERIFY(dynamic_object);
				dynamic_object->m_tNodeID = l_tpDynamicObject->m_tNodeID;
				dynamic_object->m_tGraphID = l_tpDynamicObject->m_tGraphID;
				dynamic_object->m_bALifeControl = true;
				dynamic_object->m_bOnline = true;
				alife().create(dynamic_object);
				l_tpALifeInventoryItem->base()->ID_Parent = id;
			}
#ifdef DEBUG
			else
				if (psAI_Flags.test(aiALife)) {
					Msg("Cannot detach object [%s][%s][%d] from object [%s][%s][%d]", l_tpALifeInventoryItem->base()->name_replace(), *l_tpALifeInventoryItem->base()->s_name, l_tpALifeInventoryItem->base()->ID, l_tpDynamicObject->base()->name_replace(), l_tpDynamicObject->base()->s_name, l_tpDynamicObject->ID);
				}
#endif
		}
	}
}

void game_sv_GameState::on_death(CSE_Abstract *e_dest, CSE_Abstract *e_src)
{
	if (!ai().get_alife())
		return;

	alife().on_death(e_dest, e_src);

	CSE_ALifeCreatureAbstract	*creature = smart_cast<CSE_ALifeCreatureAbstract*>(e_dest);
	if (!creature)
		return;

	VERIFY(creature->get_killer_id() == ALife::_OBJECT_ID(-1));
	creature->set_killer_id(e_src->ID);
}

void game_sv_GameState::NewPlayerName_Generate(void* pClient, LPSTR NewPlayerName)
{
	if (!pClient || !NewPlayerName)
		return;

	NewPlayerName[21] = 0;

	printf(NewPlayerName, "%s_%d", NewPlayerName, 1);
}

void game_sv_GameState::NewPlayerName_Replace(void* pClient, LPCSTR NewPlayerName)
{
	if (!pClient || !NewPlayerName) return;
	IClient* CL = (IClient*)pClient;
	if (!CL->name || xr_strlen(CL->name.c_str()) == 0) return;

	CL->name._set(NewPlayerName);
}

bool game_sv_GameState::change_level(NET_Packet &net_packet, ClientID sender)
{
	if (ai().get_alife())
		return					(alife().change_level(net_packet));
	else
		return					(true);
}

void game_sv_GameState::save_game(NET_Packet &net_packet, ClientID sender)
{
	if (!ai().get_alife())
		return;

	alife().save(net_packet);
}

bool game_sv_GameState::load_game(NET_Packet &net_packet, ClientID sender)
{
	if (!ai().get_alife()){
		Msg("ALife missing?!");
		return false;
	}
	shared_str						game_name;
	net_packet.r_stringZ(game_name);
	return						(alife().load_game(*game_name, true));
}

void game_sv_GameState::teleport_object(NET_Packet &net_packet, u16 id)
{
	if (!ai().get_alife())
		return;

	GameGraph::_GRAPH_ID		game_vertex_id;
	u32						level_vertex_id;
	Fvector					position;

	net_packet.r(&game_vertex_id, sizeof(game_vertex_id));
	net_packet.r(&level_vertex_id, sizeof(level_vertex_id));
	net_packet.r_vec3(position);

	alife().teleport_object(id, game_vertex_id, level_vertex_id, position);
}

void game_sv_GameState::add_restriction(NET_Packet &packet, u16 id)
{
	if (!ai().get_alife())
		return;

	ALife::_OBJECT_ID		restriction_id;
	packet.r(&restriction_id, sizeof(restriction_id));

	RestrictionSpace::ERestrictorTypes	restriction_type;
	packet.r(&restriction_type, sizeof(restriction_type));

	alife().add_restriction(id, restriction_id, restriction_type);
}

void game_sv_GameState::remove_restriction(NET_Packet &packet, u16 id)
{
	if (!ai().get_alife())
		return;

	ALife::_OBJECT_ID		restriction_id;
	packet.r(&restriction_id, sizeof(restriction_id));

	RestrictionSpace::ERestrictorTypes	restriction_type;
	packet.r(&restriction_type, sizeof(restriction_type));

	alife().remove_restriction(id, restriction_id, restriction_type);
}

void game_sv_GameState::remove_all_restrictions(NET_Packet &packet, u16 id)
{
	if (!ai().get_alife())
		return;

	RestrictionSpace::ERestrictorTypes	restriction_type;
	packet.r(&restriction_type, sizeof(restriction_type));

	alife().remove_all_restrictions(id, restriction_type);
}

shared_str game_sv_GameState::level_name(const shared_str &server_options) const
{
	if (!ai().get_alife()){
		Msg("ALife missing?!");
		return NULL;
	}
	return					(alife().level_name());
}

void game_sv_GameState::sls_default()
{
	alife().update_switch();
}

LPCSTR default_map_version	= "1.0";
LPCSTR map_ver_string		= "ver=";

shared_str game_sv_GameState::parse_level_version			(const shared_str &server_options)
{
	const char* map_ver = strstr(server_options.c_str(), map_ver_string);
	string128	result_version;
	if (map_ver)
	{
		map_ver += sizeof(map_ver_string);
		if (strchr(map_ver, '/'))
			strncpy_s(result_version, map_ver, strchr(map_ver, '/') - map_ver);
		else
			xr_strcpy(result_version, map_ver);
	} else
	{
		xr_strcpy(result_version, default_map_version);
	}
	return shared_str(result_version);
}

shared_str game_sv_GameState::parse_level_name(const shared_str &server_options)
{
	string64			l_name = "";
	VERIFY				(_GetItemCount(*server_options,'/'));
	return				(_GetItem(*server_options,0,l_name,'/'));
}

void game_sv_GameState::restart_simulator(LPCSTR saved_game_name)
{
	shared_str				&options = *alife().server_command_line();

	delete_data(m_alife_simulator);
	server().clear_ids();

	xr_strcpy(g_pGamePersistent->m_game_params.m_game_or_spawn, saved_game_name);
	xr_strcpy(g_pGamePersistent->m_game_params.m_new_or_load, "load");

	pApp->ls_header[0] = '\0';
	pApp->ls_tip_number[0] = '\0';
	pApp->ls_tip[0] = '\0';

	m_alife_simulator = xr_new <CALifeSimulator>(&server(), &options);
	g_pGamePersistent->LoadTitle("st_loading_data", "Loading Data");
	Device.PreCache(30, true, true);
}
