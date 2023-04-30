#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"



void xrServer::Export_game_type(IClient* CL)
{
	NET_Packet			P;
	u32					mode = net_flags(TRUE,TRUE);
	P.w_begin			(M_SV_CONFIG_NEW_CLIENT);
	P.w_stringZ("single");
	SendTo				(CL->ID,P,mode);
}

