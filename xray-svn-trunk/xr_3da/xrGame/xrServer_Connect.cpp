#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_base.h"
#include "xrMessages.h"

IClient* xrServer::new_client( SClientConnectData* cl_data )
{
	IClient* CL		= client_Find_Get( cl_data->clientID );
	VERIFY( CL );
#pragma todo("needs simplification, too dull for single player")
	// copy entity
	CL->ID			= cl_data->clientID;
	CL->process_id	= cl_data->process_id;
	
	string64 new_name;
	xr_strcpy( new_name, cl_data->name );
	CL->name._set( new_name );
	
	Server_game_sv_base->NewPlayerName_Generate(CL, new_name);
	Server_game_sv_base->NewPlayerName_Replace(CL, new_name);

	CL->name._set( new_name );
	CL->pass._set( cl_data->pass );

	NET_Packet		P;
	P.B.count		= 0;
	P.r_pos			= 0;
	
	Server_game_sv_base->AddDelayedEvent(P, GAME_EVENT_CREATE_CLIENT, 0, CL->ID);

	return CL;
}

void xrServer::AttachNewClient			(IClient* CL)
{
	MSYS_CONFIG	msgConfig;
	msgConfig.sign1 = 0x12071980;
	msgConfig.sign2 = 0x26111975;

	//single_game
	SV_Client = CL;
	CL->flags.bLocal = 1;
	SendTo_LL( SV_Client->ID, &msgConfig, sizeof(msgConfig), net_flags(TRUE,TRUE,TRUE,TRUE) );

	CL->m_guid[0]=0;
}


