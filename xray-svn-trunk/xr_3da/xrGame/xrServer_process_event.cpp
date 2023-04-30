#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_base.h"
#include "xrserver_objects.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_Objects_ALife_Monsters.h"

void xrServer::Process_event	(NET_Packet& P, ClientID sender)
{
	u32			timestamp;
	u16			type;
	u16			destination;
	u32			MODE			= net_flags(TRUE,TRUE);

	// correct timestamp with server-unique-time (note: direct message correction)
	P.r_u32		(timestamp	);

	// read generic info
	P.r_u16		(type		);
	P.r_u16		(destination);

	CSE_Abstract*	receiver = Server_game_sv_base->get_entity_from_eid(destination);
	if (receiver)	
	{
		R_ASSERT(receiver->owner);
		receiver->OnEvent						(P,type,timestamp,sender);

	};

	switch		(type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		{
			Process_event_ownership	(P, sender, timestamp, destination, P.r_u16(), true);
		}break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
	case GE_LAUNCH_ROCKET:
		{
			Process_event_reject	(P, sender, timestamp, destination, P.r_u16());
		}break;
	case GE_DESTROY:
		{
			Process_event_destroy	(P,sender,timestamp,destination, NULL);
		}
		break;
	case GE_HIT:
		{
			P.r_pos -=2;

			Server_game_sv_base->AddDelayedEvent(P, GAME_EVENT_ON_HIT, 0, ClientID());
		} break;
	case GE_ASSIGN_KILLER: {
		u16							id_src;
		P.r_u16						(id_src);
		
		CSE_Abstract				*e_dest = receiver;	// кто умер
		// this is possible when hit event is sent before destroy event
		if (!e_dest)
			break;

		CSE_ALifeCreatureAbstract	*creature = smart_cast<CSE_ALifeCreatureAbstract*>(e_dest);
		if (creature)
			creature->set_killer_id(id_src);

//		Msg							("[%d][%s] killed [%d][%s]",id_src,id_src==u16(-1) ? "UNKNOWN" : game->get_entity_from_eid(id_src)->name_replace(),id_dest,e_dest->name_replace());

		break;
	}
	case GE_DIE:
		{
			// Parse message
			u16	id_src;
			P.r_u16				(id_src);


			xrClientData *l_pC	= ID_to_client(sender);
			R_ASSERT(Server_game_sv_base && l_pC);

			CSE_Abstract*		e_dest		= receiver;	// кто умер
			// this is possible when hit event is sent before destroy event
			if (!e_dest)
				break;

			CSE_Abstract*		e_src = Server_game_sv_base->get_entity_from_eid(id_src);	// кто убил
			if (!e_src) {
				Msg("!Im gonna crash here");
				//xrClientData*	C = (xrClientData*)game->get_client(id_src);
				//if (C) e_src = C->owner;
			};
			R_ASSERT(e_src);

			Server_game_sv_base->on_death(e_dest, e_src);

			xrClientData*		c_src		= e_src->owner;				// клиент, чей юнит убил

			if (c_src->owner->ID == id_src) {
				// Main unit
				P.w_begin			(M_EVENT);
				P.w_u32				(timestamp);
				P.w_u16				(type);
				P.w_u16				(destination);
				P.w_u16				(id_src);
				P.w_clientID		(c_src->ID);
			}

			SendBroadcast			(BroadcastCID,P,MODE);

			//////////////////////////////////////////////////////////////////////////
			// 

			P.w_begin			(M_EVENT);
			P.w_u32				(timestamp);
			P.w_u16				(GE_KILL_SOMEONE);
			P.w_u16				(id_src);
			P.w_u16				(destination);
			SendTo				(c_src->ID, P, net_flags(TRUE, TRUE));

			//////////////////////////////////////////////////////////////////////////

		}
		break;
	case GE_TELEPORT_OBJECT:
		{
			Server_game_sv_base->teleport_object(P, destination);
		}break;
	case GE_ADD_RESTRICTION:
		{
			Server_game_sv_base->add_restriction(P, destination);
		}break;
	case GE_REMOVE_RESTRICTION:
		{
			Server_game_sv_base->remove_restriction(P, destination);
		}break;
	case GE_REMOVE_ALL_RESTRICTIONS:
		{
			Server_game_sv_base->remove_all_restrictions(P, destination);
		}break;
	default:
		R_ASSERT2	(0,"Game Event not implemented!!!");
		break;
	}
}
