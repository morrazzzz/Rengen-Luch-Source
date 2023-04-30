#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "xrserver_objects.h"
#include "xrserver_objects_alife.h"
#include "Level.h"

xr_vector<u16> g_perform_spawn_ids;

void xrServer::Perform_connect_spawn(CSE_Abstract* E, xrClientData* CL, NET_Packet& P)
{
	xr_vector<u16>::iterator it = std::find(g_perform_spawn_ids.begin(), g_perform_spawn_ids.end(), E->ID);
	if(it!=g_perform_spawn_ids.end())
		return;
	
	g_perform_spawn_ids.push_back(E->ID);

	if (E->net_Processed)						return;
	if (E->s_flags.is(M_SPAWN_OBJECT_PHANTOM))	return;

	// Connectivity order
	CSE_Abstract* Parent = ID_to_entity	(E->ID_Parent);
	if (Parent)		Perform_connect_spawn	(Parent,CL,P);

	// Process
	Flags16			save = E->s_flags;
	//-------------------------------------------------
	E->s_flags.set	(M_SPAWN_UPDATE,TRUE);
	if (0==E->owner)	
	{
		// PROCESS NAME; Name this entity
		if (E->s_flags.is(M_SPAWN_OBJECT_ASPLAYER))
		{
			CL->owner		= E;
			E->set_name_replace	(*CL->name);
		}

		// Associate
		E->owner		= CL;
		E->Spawn_Write	(P,TRUE	);
		E->UPDATE_Write	(P);

		CSE_ALifeObject*	object = smart_cast<CSE_ALifeObject*>(E);
		VERIFY				(object);
		if (!object->keep_saved_data_anyway())
			object->client_data.clear	();
	}
	else				
	{
		// Just inform
		E->Spawn_Write	(P,FALSE);
		E->UPDATE_Write	(P);
	}
	//-----------------------------------------------------
	E->s_flags			= save;
	SendTo				(CL->ID,P,net_flags(TRUE,TRUE));
	E->net_Processed	= TRUE;
}

void xrServer::SendConnectionData(IClient* _CL)
{
	g_perform_spawn_ids.clear_not_free();
	xrClientData*	CL				= (xrClientData*)_CL;
	NET_Packet		P;
	u32			mode				= net_flags(TRUE,TRUE);
	// Replicate current entities on to this client
	xrS_entities::iterator	I=entities.begin(),E=entities.end();
	for (; I!=E; ++I)						I->second->net_Processed	= FALSE;
	for (I=entities.begin(); I!=E; ++I)		Perform_connect_spawn		(I->second,CL,P);

	// Send "finished" signal
	P.w_begin						(M_SV_CONFIG_FINISHED);
	SendTo							(CL->ID,P,mode);
};

void xrServer::OnCL_Connected		(IClient* _CL)
{
	xrClientData* CL = (xrClientData*)_CL;

	Export_game_type(CL);
	SendConnectionData(CL);
}
