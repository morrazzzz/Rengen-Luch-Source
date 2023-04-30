#pragma once
// xrServer.h: interface for the xrServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XRSERVER_H__65728A25_16FC_4A7B_8CCE_D798CA5EC64E__INCLUDED_)
#define AFX_XRSERVER_H__65728A25_16FC_4A7B_8CCE_D798CA5EC64E__INCLUDED_
#pragma once

#include "../../xrNetServer/net_server.h"
#include "game_sv_base.h"
#include "id_generator.h"

class CSE_Abstract;

const u32	NET_Latency		= 50;		// time in (ms)

extern CSE_Abstract	*F_entity_Create(LPCSTR section);

// t-defs
typedef xr_map<u16, CSE_Abstract*>	xrS_entities;

class xrClientData	: public IClient
{
public:
	CSE_Abstract*			owner;
	BOOL					net_Ready;

							xrClientData			();
	virtual					~xrClientData			();
	virtual void			Clear					();
};


// main
struct	svs_respawn
{
	u32		timestamp;
	u16		phantom;
};
IC bool operator < (const svs_respawn& A, const svs_respawn& B)	{ return A.timestamp<B.timestamp; }

class xrServer	: public IPureServer  
{
private:
	xrS_entities				entities;
	
	u32							m_last_updates_size;
	u32							m_last_update_time;


	struct DelayedPacket
	{
		ClientID		SenderID;
		NET_Packet		Packet;
		bool operator == (const DelayedPacket& other)
		{
			return SenderID == other.SenderID;
		}
	};
	AccessLock					DelayedPackestCS;
	xr_deque<DelayedPacket>		m_aDelayedPackets;
	void						ProceedDelayedPackets	();
	void						AddDelayedPacket		(NET_Packet& Packet, ClientID Sender);
	u32							OnDelayedMessage		(NET_Packet& P, ClientID sender);


	typedef 
		CID_Generator<
			u32,		// time identifier type
			u8,			// compressed id type 
			u16,		// id type
			u8,			// block id type
			u16,		// chunk id type
			0,			// min value
			u16(-2),	// max value
			256,		// block size
			u16(-1)		// invalid id
		> id_generator_type;

	id_generator_type					m_tID_Generator;
public:
	game_sv_GameState*		Server_game_sv_base;

	void					Export_game_type		(IClient* CL);
	
	IC void					clear_ids				()
	{
		m_tID_Generator		= id_generator_type();
	}
	IC u16					PerformIDgen			(u16 ID)
	{
		return				(m_tID_Generator.tfGetID(ID));
	}
	IC void					FreeID					(u16 ID, u32 time)
	{
		return				(m_tID_Generator.vfFreeID(ID, time));
	}

	void					Perform_connect_spawn	(CSE_Abstract* E, xrClientData* to, NET_Packet& P);
	void					Perform_transfer		(NET_Packet &PR, NET_Packet &PT, CSE_Abstract* what, CSE_Abstract* from, CSE_Abstract* to);
	void					Perform_reject			(CSE_Abstract* what, CSE_Abstract* from, int delta);
	void					Perform_destroy			(CSE_Abstract* tpSE_Abstract, u32 mode);

	CSE_Abstract*			Process_spawn			(NET_Packet& P, ClientID sender, BOOL bSpawnWithClientsMainEntityAsParent=FALSE, CSE_Abstract* tpExistedEntity=0);
	void					Process_update			(NET_Packet& P, ClientID sender);
	void					Process_save			(NET_Packet& P, ClientID sender);
	void					Process_event			(NET_Packet& P, ClientID sender);
	void					Process_event_ownership	(NET_Packet& P, ClientID sender, u32 time, const u16 id_parent, const u16 id_entity, bool send_event, BOOL bForced = FALSE);
	bool					Process_event_reject	(NET_Packet& P, const ClientID sender, const u32 time, const u16 id_parent, const u16 id_entity, bool send_message = true);
	void					Process_event_destroy	(NET_Packet& P, ClientID sender, u32 time, u16 ID, NET_Packet* pEPack);
	
	xrClientData*			SelectBestClientToMigrateTo		(CSE_Abstract* E, BOOL bForceAnother=FALSE){return (xrClientData*)SV_Client;};
	void					AttachNewClient			(IClient* CL);
protected:
	virtual IClient*		new_client				( SClientConnectData* cl_data );
	void					SendConnectionData		(IClient* CL);

public:
	// constr / destr
	xrServer				();
	virtual ~xrServer		();

	// extended functionality
	virtual u32				OnMessage			(NET_Packet& P, ClientID sender);	// Non-Zero means broadcasting with "flags" as returned
	virtual void			OnCL_Connected		(IClient* CL);
	virtual bool			OnCL_QueryHost		();
	virtual void			SendTo_LL			(ClientID ID, void* data, u32 size, u32 dwFlags=DPNSEND_GUARANTEED, u32 dwTimeout=0);
	virtual	void			SendBroadcast		(ClientID exclude, NET_Packet& P, u32 dwFlags=DPNSEND_GUARANTEED);

	virtual IClient*		client_Create		();								// create client info
	virtual IClient*		client_Find_Get		(ClientID ID);					// Find earlier disconnected client
	virtual void			client_Destroy		(IClient* C);					// destroy client info

	// utilities
	CSE_Abstract*			entity_Create		(LPCSTR name);
	void					entity_Destroy		(CSE_Abstract *&P);
	u32						GetEntitiesNum		()			{ return entities.size(); };
	u32 const				GetLastUpdatesSize	() const { return m_last_updates_size; };

	xrClientData*			ID_to_client		(ClientID ID, bool ScanAll = false ) { return (xrClientData*)(IPureServer::ID_to_client( ID, ScanAll)); }
	CSE_Abstract*			ID_to_entity		(u16 ID);

	// main
	virtual EConnect		Connect(shared_str& session_name){ return ErrNoError; };
	virtual void			Disconnect			();
	virtual void			Update				();
	void					SLS_Default			();
	void					SLS_Clear			();
	void					SLS_Save			(IWriter&	fs);
			shared_str		level_name			(const shared_str &server_options) const;
			shared_str		level_version		(const shared_str &server_options) const;

	void					create_direct_client();

	virtual void			Assign_ServerType	( string512& res ) {};
public:
	xr_string				ent_name_safe		(u16 eid);
};

#endif 
