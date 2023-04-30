
#include "pch_script.h"
#include "xrServer.h"
#include "xrMessages.h"
#include "xrServer_Objects_ALife_All.h"
#include "level.h"
#include "ai_space.h"
#include "object_broker.h"
#include "object_factory.h"

#include "ui/UIInventoryUtilities.h"

xrClientData::xrClientData	() :
	IClient(Device.GetTimerGlobal())
{
	Clear		();
}

void	xrClientData::Clear()
{
	owner									= NULL;
	net_Ready								= FALSE;
};


xrClientData::~xrClientData()
{

}


xrServer::xrServer() : IPureServer(Device.GetTimerGlobal())
{
	m_aDelayedPackets.clear();

	m_last_updates_size	= 0;
	m_last_update_time	= 0;
}

xrServer::~xrServer()
{
	struct ClientDestroyer
	{
		static bool true_generator(IClient*)
		{
			return true;
		}
	};

	IClient* tmp_client = net_players.GetFoundClient(&ClientDestroyer::true_generator);
	while (tmp_client)
	{
		client_Destroy(tmp_client);
		tmp_client = net_players.GetFoundClient(&ClientDestroyer::true_generator);
	}
	m_aDelayedPackets.clear();
	entities.clear();
}

CSE_Abstract*	xrServer::ID_to_entity		(u16 ID)
{
	if (0xffff==ID)				return 0;
	xrS_entities::iterator	I	= entities.find	(ID);
	if (entities.end()!=I)		return I->second;
	else						return 0;
}

IClient*	xrServer::client_Create		()
{
	return xr_new <xrClientData>();
}

IClient*	xrServer::client_Find_Get	(ClientID ID)
{
	ip_address tmp_ip_address;

	//for single player temp ip
	tmp_ip_address.set( "127.0.0.1" );

	IClient* newCL = client_Create();
	newCL->ID = ID;

	newCL->server			= this;
	net_players.AddNewClient(newCL);

	return newCL;
};

u32	g_sv_Client_Reconnect_Time = 3;

void		xrServer::client_Destroy	(IClient* C)
{
	IClient* alife_client = net_players.FindAndEraseClient(
		std::bind1st(std::equal_to<IClient*>(), C)
	);

	if (alife_client)
	{
		DelayedPacket pp;
		pp.SenderID = alife_client->ID;
		xr_deque<DelayedPacket>::iterator it;
		do{
			it						=std::find(m_aDelayedPackets.begin(),m_aDelayedPackets.end(),pp);
			if(it!=m_aDelayedPackets.end())
			{
				m_aDelayedPackets.erase	(it);
				Msg("removing packet from delayed event storage");
			}else
				break;
		}while(true);
	}
}

void xrServer::Update	()
{
	ProceedDelayedPackets();

	Server_game_sv_base->ProcessDelayedEvent();
	Server_game_sv_base->Update();

	Flush_Clients_Buffers			();
}

u32 xrServer::OnDelayedMessage	(NET_Packet& P, ClientID sender)			// Non-Zero means broadcasting with "flags" as returned
{
	u16						type;
	P.r_begin				(type);

	switch (type)
	{
		case M_CLIENT_REQUEST_CONNECTION_DATA:
		{
			IClient* tmp_client = net_players.GetFoundClient(
				ClientIdSearchPredicate(sender));
			VERIFY(tmp_client);
			OnCL_Connected(tmp_client);
		}break;

	}
	return 0;
}
u32 xrServer::OnMessage	(NET_Packet& P, ClientID sender)
{
	u16			type;
	P.r_begin	(type);

	xrClientData* CL				= ID_to_client(sender);

	switch (type)
	{
	case M_UPDATE:	
		{
			Process_update			(P,sender);
		}break;

	case M_SPAWN:	
		{
			if (CL->flags.bLocal)
				Process_spawn		(P,sender);	
		}break;

	case M_EVENT:	
		{
			Process_event			(P,sender);
		}break;

	case M_EVENT_PACK:
		{
			NET_Packet	tmpP;
			while (!P.r_eof())
			{
				tmpP.B.count		= P.r_u8();
				P.r					(&tmpP.B.data, tmpP.B.count);

				OnMessage			(tmpP, sender);
			};			
		}break;

	case M_CL_UPDATE:
		{
			Msg("Server M_CL_UPDATE");
			xrClientData* CL		= ID_to_client	(sender);
			if (!CL)				break;
			CL->net_Ready			= TRUE;

			u32 ClientPing = CL->stats.getPing();
			P.w_seek(P.r_tell()+2, &ClientPing, 4);

			if (SV_Client) 
				SendTo	(SV_Client->ID, P, net_flags(TRUE, TRUE));
		}break;

	case M_CHANGE_LEVEL:
		{
			if (Server_game_sv_base->change_level(P, sender))
			{
				Msg(LINE_SPACER);
				Msg("----------- Changing Level -----------");

				SendBroadcast(BroadcastCID,P,net_flags(TRUE,TRUE));
			}
		}break;

	case M_SAVE_GAME:
		{
			Msg(LINE_SPACER);
			Msg("----------- Saving Game -----------");

			Server_game_sv_base->save_game(P, sender);
		}break;

	case M_LOAD_GAME:
		{
			Msg(LINE_SPACER);
			Msg("----------- Loading Game -----------");

			Server_game_sv_base->load_game(P, sender);
			SendBroadcast(BroadcastCID,P,net_flags(TRUE,TRUE));
		}break;

	case M_SAVE_PACKET:
		{
			Process_save			(P,sender);
		}break;
	case M_CLIENT_REQUEST_CONNECTION_DATA:
		{
			AddDelayedPacket(P, sender);
		}break;
	}
	return							IPureServer::OnMessage(P, sender);
}

void xrServer::SendTo_LL			(ClientID ID, void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
	// single player needs only this
	Level().OnMessage			(data,size);
}
void xrServer::SendBroadcast(ClientID exclude, NET_Packet& P, u32 dwFlags)
{
	struct ClientExcluderPredicate
	{
		ClientID id_to_exclude;
		ClientExcluderPredicate(ClientID exclude) :
			id_to_exclude(exclude)
		{}
		bool operator()(IClient* client)
		{
			if (client->ID == id_to_exclude)
				return false;

			if (!client->flags.bConnected)
				return false;

			return true;
		}
	};
	struct ClientSenderFunctor
	{
		xrServer*		m_owner;
		void*			m_data;
		u32				m_size;
		u32				m_dwFlags;
		ClientSenderFunctor(xrServer* owner, void* data, u32 size, u32 dwFlags) :
			m_owner(owner), m_data(data), m_size(size), m_dwFlags(dwFlags)
		{}
		void operator()(IClient* client)
		{
			m_owner->SendTo_LL(client->ID, m_data, m_size, m_dwFlags);			
		}
	};
	ClientSenderFunctor temp_functor(this, P.B.data, P.B.count, dwFlags);
	net_players.ForFoundClientsDo(ClientExcluderPredicate(exclude), temp_functor);
}
//--------------------------------------------------------------------
CSE_Abstract*	xrServer::entity_Create		(LPCSTR name)
{
	return F_entity_Create(name);
}

void			xrServer::entity_Destroy	(CSE_Abstract *&P)
{
	R_ASSERT					(P);
	entities.erase				(P->ID);
	m_tID_Generator.vfFreeID	(P->ID,Device.TimerAsync());

	if(P->owner && P->owner->owner==P)
		P->owner->owner		= NULL;

	P->owner = NULL;
	if (!ai().get_alife() || !P->m_bALifeControl)
	{
		F_entity_Destroy		(P);
	}
}

bool		xrServer::OnCL_QueryHost		() 
{
	return false;
};

shared_str xrServer::level_name				(const shared_str &server_options) const
{
	return	(Server_game_sv_base->level_name(server_options));
}
shared_str xrServer::level_version			(const shared_str &server_options) const
{
	return	(game_sv_GameState::parse_level_version(server_options));						
}

void xrServer::create_direct_client()
{
	SClientConnectData cl_data;
	cl_data.clientID.set(1);
	xr_strcpy( cl_data.name, "single_player" );
	cl_data.process_id = GetCurrentProcessId();
	
	new_client( &cl_data );
}


void xrServer::ProceedDelayedPackets()
{
	DelayedPackestCS.Enter();
	while (!m_aDelayedPackets.empty())
	{
		DelayedPacket& DPacket	= *m_aDelayedPackets.begin();
		OnDelayedMessage(DPacket.Packet, DPacket.SenderID);
//		OnMessage(DPacket.Packet, DPacket.SenderID);
		m_aDelayedPackets.pop_front();
	}
	DelayedPackestCS.Leave();
};

void xrServer::AddDelayedPacket	(NET_Packet& Packet, ClientID Sender)
{
	DelayedPackestCS.Enter();

	m_aDelayedPackets.push_back(DelayedPacket());
	DelayedPacket* NewPacket = &(m_aDelayedPackets.back());
	NewPacket->SenderID = Sender;
	CopyMemory	(&(NewPacket->Packet),&Packet,sizeof(NET_Packet));	

	DelayedPackestCS.Leave();
}

void xrServer::Disconnect()
{
	IPureServer::Disconnect();
	SLS_Clear();
	xr_delete(Server_game_sv_base);
}

