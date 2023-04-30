#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"

bool xrServer::Process_event_reject	(NET_Packet& P, const ClientID sender, const u32 time, const u16 id_parent, const u16 id_entity, bool send_message)
{
	// Parse message
	CSE_Abstract*		e_parent = Server_game_sv_base->get_entity_from_eid(id_parent);
	CSE_Abstract*		e_entity = Server_game_sv_base->get_entity_from_eid(id_entity);

//	R_ASSERT2( e_entity, make_string( "entity not found. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, CurrentFrame()).c_str() );
	VERIFY2(e_entity, make_string("entity not found. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, CurrentFrame()).c_str());
	if ( !e_entity ) {
		Msg("! ERROR on rejecting: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, CurrentFrame());
		return false;
	}

//	R_ASSERT2( e_parent, make_string( "parent not found. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, CurrentFrame()).c_str() );
	VERIFY2(e_parent, make_string("parent not found. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, CurrentFrame()).c_str());
	if ( !e_parent ) {
		Msg("! ERROR on rejecting: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, CurrentFrame());
		return false;
	}

#ifdef DEBUG
	Msg ( "--- SV: Process reject: parent[%d][%s], item[%d][%s]", id_parent, e_parent->name_replace(), id_entity, e_entity->name());
#endif

	xr_vector<u16>& C		= e_parent->children;
	xr_vector<u16>::iterator c	= std::find	(C.begin(),C.end(),id_entity);
	if (c == C.end())
	{
		Msg("! ERROR: SV: can't find children [%d] of parent [%d]", id_entity, e_parent);
		return false;
	}
	Server_game_sv_base->OnDetach(id_parent, id_entity);

	if (0xffff == e_entity->ID_Parent) 
	{
		Msg	("! ERROR: can't detach independant object. entity[%s][%d], parent[%s][%d], section[%s]",
			e_entity->name_replace(), id_entity, e_parent->name_replace(), id_parent, e_entity->s_name.c_str() );
		return			(false);
	}

	// Rebuild parentness
	if (e_entity->ID_Parent != id_parent)
	{
		Msg("! ERROR: e_entity->ID_Parent = [%d]  parent = [%d][%s]  entity_id = [%d]  frame = [%d]",
			e_entity->ID_Parent, id_parent, e_parent->name_replace(), id_entity, CurrentFrame());
		//it can't be !!!
	}
	e_entity->ID_Parent		= 0xffff;
//	xr_vector<u16>& C		= e_parent->children;

//	xr_vector<u16>::iterator c	= std::find	(C.begin(),C.end(),id_entity);
//	R_ASSERT3				(C.end()!=c,e_entity->name_replace(),e_parent->name_replace());
	C.erase					(c);

	// Signal to everyone (including sender)
	if (send_message)
	{
		DWORD MODE		= net_flags(TRUE,TRUE, FALSE, TRUE);
		SendBroadcast	(BroadcastCID,P,MODE);
	}
	
	return				(true);
}
