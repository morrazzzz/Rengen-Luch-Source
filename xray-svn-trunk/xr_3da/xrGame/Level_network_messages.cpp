#include "stdafx.h"
#include "level.h"
#include "xrmessages.h"
#include "game_cl_base.h"
#include "net_queue.h"
#include "xrServer.h"
#include "Actor.h"
#include "ai_space.h"
#include "saved_game_wrapper.h"
#include "level_graph.h"
#include "../../xrphysics/iphworld.h"

void CLevel::ClientReceive()
{
	Device.Statistic->netClient1.Begin();

	m_dwRPC = 0;
	m_dwRPS = 0;

	for (NET_Packet* P = net_msg_Retreive(); P; P=net_msg_Retreive())
	{
		//-----------------------------------------------------
		m_dwRPC++;
		m_dwRPS += P->B.count;
		//-----------------------------------------------------
		u16			m_type;
		P->r_begin	(m_type);
		switch (m_type)
		{
		case M_SPAWN:			
			{
				if (!m_bGameConfigStarted || !bReady) 
				{
					Msg ("Unconventional M_SPAWN received : cgf[%s] | bReady[%s]",
						(m_bGameConfigStarted) ? "true" : "false",
						(bReady) ? "true" : "false");
					break;
				}
				game_events->insert		(*P);
			}
			break;
		case M_EVENT:
			game_events->insert		(*P);
			break;
		case M_EVENT_PACK:
			NET_Packet	tmpP;
			while (!P->r_eof())
			{
				tmpP.B.count = P->r_u8();
				P->r(&tmpP.B.data, tmpP.B.count);
				tmpP.timeReceive = P->timeReceive;

				game_events->insert		(tmpP);
			};			
			break;
		case M_SV_CONFIG_NEW_CLIENT:
			InitializeClientGame(*P);
			break;
		case M_SV_CONFIG_FINISHED:
			game_configured			= TRUE;
			Msg("- Game configuring : Finished ");
			break;		
		case M_LOAD_GAME:
		case M_CHANGE_LEVEL:
			{
				if(m_type==M_LOAD_GAME)
				{
					string256						saved_name;
					P->r_stringZ					(saved_name);
					if(xr_strlen(saved_name) && ai().get_alife())
					{
						CSavedGameWrapper			wrapper(saved_name);
						if (wrapper.level_id() == ai().level_graph().level_id()) 
						{
							Engine.Event.Defer	("Game:QuickLoad", size_t(xr_strdup(saved_name)), 0);

							break;
						}
					}
				}
				string4096	disconect_reason = "Level Change";
				Engine.Event.Defer("KERNEL:disconnect", u64(xr_strdup(disconect_reason)), 0);
				Engine.Event.Defer("KERNEL:start", size_t(xr_strdup(*m_GameCreationOptions)), 0);
			}break;
		case M_SAVE_GAME:
			{
				ClientSave			();
			}break;
		}

		net_msg_Release();
	}

	Device.Statistic->netClient1.End();
}

void				CLevel::OnMessage				(void* data, u32 size)
{	
	IPureClient::OnMessage(data, size);	
}

