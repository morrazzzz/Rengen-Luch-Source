#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"
#include "level.h"
#include "../xr_object.h"
#include "gameobject.h"
#include "xrServer_Objects_ALife_Monsters.h"

void ReplaceOwnershipHeader	(NET_Packet& P)
{
	//способ очень грубый, но на данный момент иного выбора нет. Заранее приношу извинения
	u16 NewType = GE_OWNERSHIP_TAKE;
	CopyMemory(&P.B.data[6],&NewType,2);
};

bool is_object_valid_on_svclient(u16 id_entity)
{
	CObject* tmp_obj		= Level().Objects.net_Find(id_entity);
	if (!tmp_obj)
		return false;
	
	CGameObject* tmp_gobj	= smart_cast<CGameObject*>(tmp_obj);
	if (!tmp_gobj)
		return false;

	if (tmp_obj->getDestroy())
		return false;

//	if (tmp_gobj->object_removed())
//		return false;
	
	return true;
};

void xrServer::Process_event_ownership(NET_Packet& P, ClientID sender, u32 time, const u16 id_parent, const u16 id_entity, bool send_event, BOOL bForced)
{
	u32 MODE			= net_flags(TRUE,TRUE, FALSE, TRUE);

	CSE_Abstract*		e_parent = Server_game_sv_base->get_entity_from_eid(id_parent);
	CSE_Abstract*		e_entity = Server_game_sv_base->get_entity_from_eid(id_entity);
	
	
#ifdef DEBUG
	Msg( "--- SV: Process ownership take: parent [%d][%s], item [%d][%s]", 
		id_parent, e_parent ? e_parent->name_replace() : "null_parent",
		id_entity, e_entity ? e_entity->name() : "null_entity");
#endif
	
	if ( !e_parent ) {
		Msg("! ERROR on ownership: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, CurrentFrame());
		return;
	}
	if ( !e_entity ) {
		Msg("! ERROR on ownership: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, CurrentFrame());
		return;
	}
	
	if (!is_object_valid_on_svclient(id_parent))
	{
		Msg("! ERROR on ownership: parent object is not valid on sv client. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, CurrentFrame());
		return;
	}

	if (!is_object_valid_on_svclient(id_entity))
	{
		Msg("! ERROR on ownership: entity object is not valid on sv client. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, CurrentFrame());
		return;
	}

	if (0xffff != e_entity->ID_Parent)	return;

	xrClientData*		c_parent		= e_parent->owner;
	xrClientData*		c_from			= ID_to_client	(sender);

	if ( (GetServerClient() != c_from) && (c_parent != c_from) )
	{
		// trust only ServerClient or new_ownerClient
		return;
	}

	// Game allows ownership of entity
	if (Server_game_sv_base->OnTouch(id_parent, id_entity, bForced))
	{
		// Rebuild parentness
		e_entity->ID_Parent			= id_parent;
		e_parent->children.push_back(id_entity);

		if (bForced)
		{
			ReplaceOwnershipHeader(P);
		}

		if (send_event)	// Signal to everyone (including sender)
			SendBroadcast(BroadcastCID, P, MODE);
	}

}
