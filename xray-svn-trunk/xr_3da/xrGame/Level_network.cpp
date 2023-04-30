#include "pch_script.h"
#include "Level.h"
#include "Level_Bullet_Manager.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "PHCommander.h"
#include "net_queue.h"
#include "MainMenu.h"
#include "space_restriction_manager.h"
#include "ai_space.h"
#include "script_engine.h"
#include "stalker_animation_data_storage.h"
#include "client_spawn_manager.h"
#include "UIGameCustom.h"
#include "UI/UIGameTutorial.h"
#include "ui/UIPdaWnd.h"

const int max_objects_size			= 2*1024;
const int max_objects_size_in_save	= 8*1024;

extern bool	g_b_ClearGameCaptions;

void CLevel::remove_objects()
{
	// Clear aux threads from irrelevant stuff

	Msg("# Clearing threads workload");

	// event based aux threads are simple to clear - they are deffinitly are not working now
	Device.auxThreadPool_1_.clear_not_free();
	Device.auxThreadPool_2_.clear_not_free();
	Device.auxThreadPool_3_.clear_not_free();
	Device.auxThreadPool_4_.clear_not_free();
	Device.auxThreadPool_5_.clear_not_free();

	// Independant aux threads are more complex, since they can run right now: Need to enter the processing protecting crit section, to ether wait aux thread to finish current pass, 
	// and to clear workload pool, or to just clear workload pool and thus prevent thread from doing anything more
	Device.IndAuxProcessingProtection_1.Enter();
	Device.independableAuxThreadPool_1_.clear();
	Device.IndAuxProcessingProtection_1.Leave();

	Device.IndAuxProcessingProtection_2.Enter();
	Device.independableAuxThreadPool_2_.clear();
	Device.IndAuxProcessingProtection_2.Leave();

	Device.IndAuxProcessingProtection_3.Enter();
	Device.independableAuxThreadPool_3_.clear();
	Device.IndAuxProcessingProtection_3.Leave();

	Device.resUploadingProcessingProtection_1.Enter();
	Device.resourceUploadingThreadPool_1_.clear();
	Device.resUploadingProcessingProtection_1.Leave();

	// Hack, to allow assertion-less destroying of obects

	::Render->ClearRuntime();
	
	int loop = 5;

	while(loop)
	{
		R_ASSERT				(Server);
		Server->SLS_Clear		();

		for (int i=0; i<20; ++i) 
		{
			snd_Events.clear		();
			psNET_Flags.set			(NETFLAG_MINIMIZEUPDATES,FALSE);
			// ugly hack for checks that update is twice on frame
			// we need it since we do updates for checking network messages
			u32 frame = CurrentFrame();

			SetCurrentFrame(frame + 1);

			ClientReceive			();
			ProcessGameEvents		();
			Objects.ObjectListUpdate(true, true);
			Objects.DestroyOldObjects(true);
			#ifdef DEBUG
			Msg						("Update objects list...");
			#endif
			Objects.dump_all_objects();
		}

		if(Objects.o_count()==0)
			break;
		else
		{
			--loop;
			Msg						("Objects removal next loop. Active objects count=%d", Objects.o_count());
		}

	}

	BulletManager().Clear		();
	ph_commander().clear		();
	ph_commander_scripts().clear();

	space_restriction_manager().clear	();

	g_b_ClearGameCaptions		= true;

	ai().script_engine().collect_all_garbage	();

	stalker_animation_data_storage().clear		();
	
	R_ASSERT									(Render);
	Render->models_Clear						(FALSE);
	
	Render->clear_static_wallmarks				();

#ifdef DEBUG
	if (!client_spawn_manager().registry().empty())
		client_spawn_manager().dump();
#endif

	VERIFY										(client_spawn_manager().registry().empty());
	client_spawn_manager().clear			();

	lastApplyCameraVPNear = -1.f;
}

#ifdef DEBUG
	extern void	show_animation_stats	();
#endif

extern CUISequencer * g_tutorial;
extern CUISequencer * g_tutorial2;

void CLevel::net_Stop		()
{
	if(CurrentGameUI())
	{
		CurrentGameUI()->HideShownDialogs();
		CurrentGameUI()->PdaMenu().Reset();
	}

	if(g_tutorial && !g_tutorial->Persistent())
		g_tutorial->Stop();

	if(g_tutorial2 && !g_tutorial->Persistent())
		g_tutorial2->Stop();


	bReady						= false;
	m_bGameConfigStarted		= FALSE;
	game_configured				= FALSE;

	remove_objects				();

	if (ambient_particles)
		DestroyParticleInstance(ambient_particles);

	IGame_Level::net_Stop		();
	IPureClient::Disconnect		();

	if (Server) 
	{
		Server->Disconnect		();
		xr_delete				(Server);
	}

	ai().script_engine().collect_all_garbage	();

#ifdef DEBUG
	show_animation_stats		();
#endif
}


void CLevel::ClientSend()
{
	NET_Packet P;
	u32	start = 0;

	if (CurrentControlEntity()) 
	{
		CObject* pObj = CurrentControlEntity();
		if (!pObj->getDestroy() && pObj->NeedDataExport())
		{				
			P.w_begin		(M_CL_UPDATE);
			P.w_u16			(u16(pObj->ID()));
			P.w_u32			(0); //reserved place for client's ping

			pObj->ExportDataToServer(P); //Send to current controlling entity (usually to actor)
		}			
	}		

	while (1)
	{

		P.w_begin(M_UPDATE);
		start = Objects.ExportObjectsDataToServer(&P, start, max_objects_size); //Export objects data to server storage

		if (P.B.count>2)
		{
			Device.Statistic->TEST3.Begin();

			Send(P, net_flags(FALSE));

			Device.Statistic->TEST3.End();
		}
		else
			break;
	}
}

u32	CLevel::Objects_net_Save	(NET_Packet* _Packet, u32 start, u32 max_object_size)
{
	NET_Packet& Packet	= *_Packet;
	u32			position;
	for (; start<Objects.o_count(); start++)	{
		CObject		*_P = Objects.o_get_by_iterator(start);
		CGameObject *P = smart_cast<CGameObject*>(_P);

		if (P && !P->getDestroy() && P->net_SaveRelevant())	{
			Packet.w_u16			(u16(P->ID())	);
			Packet.w_chunk_open16	(position);

			P->net_Save				(Packet);
#ifdef DEBUG
			u32 size = u32(Packet.w_tell()-position)-sizeof(u16);

			if	(size>=65536)
			{
				Debug.fatal	(DEBUG_INFO,"Object [%s][%d] exceed network-data limit\n size=%d, Pend=%d, Pstart=%d",
					*P->ObjectName(), P->ID(), size, Packet.w_tell(), position);
			}
#endif

			Packet.w_chunk_close16	(position);

			if (max_object_size >= (NET_PacketSizeLimit - Packet.w_tell()))
				break;
		}
	}
	return	++start;
}

void CLevel::ClientSave	()
{
	NET_Packet		P;
	u32				start	= 0;
	int				packet_count = 0;
	int				total_stored = 0;
	int				object_count = 0;
	for (;;) {
		P.w_begin	(M_SAVE_PACKET);
		
		start		= Objects_net_Save(&P, start, max_objects_size_in_save);

		if (P.B.count>2)
			Send	(P, net_flags(FALSE));
		else
			break;
		packet_count++;
		total_stored += P.w_tell();
		object_count += start;
		//Msg("~ NetPacket #%d stores %d objects in %db", packet_count, start, P.w_tell());
	}
	Msg("~ Used %d NetPacket(s) total objects %d total stored %db", packet_count, object_count, total_stored);
}

void CLevel::Send(NET_Packet& P, u32 dwFlags, u32 dwTimeout)
{
	ClientID _clid;
	_clid.set(1);

	Server->OnMessage(P, _clid);
}

void CLevel::net_Update()
{
	if(game_configured)
	{
		// If we have enought bandwidth - replicate client data on to server
		Device.Statistic->netClient2.Begin	();
		ClientSend					();
		Device.Statistic->netClient2.End		();
	}

	// If server - perform server-update
	if (Server)
	{
		Device.Statistic->netServer.Begin();
		Server->Update					();
		Device.Statistic->netServer.End	();
	}
}

struct _NetworkProcessor	: public pureFrame
{
	virtual void OnFrame()
	{
#ifdef MEASURE_MT
		CTimer measure_mt; measure_mt.Start();
#endif


		if (g_pGameLevel && !Device.Paused() )
			g_pGameLevel->net_Update();

		
#ifdef MEASURE_MT
		Device.Statistic->mtNetworkTime_ += measure_mt.GetElapsed_sec();
#endif
	}

}	NET_processor;

pureFrame* g_pNetProcessor = &NET_processor;

BOOL CLevel::Connect2Server()
{
	//removed unnecessary code, only single player
	NET_Packet P;
	net_Syncronised = TRUE;

	P.w_begin(M_CLIENT_REQUEST_CONNECTION_DATA);

	Send(P, net_flags(TRUE, TRUE, TRUE, TRUE));

	return TRUE;
};

void			CLevel::ClearAllObjects				()
{
	u32 CLObjNum = Level().Objects.o_count();

	bool ParentFound = true;
	
	while (ParentFound)
	{	
		ParentFound = false;
		for (u32 i=0; i<CLObjNum; i++)
		{
			CObject* pObj = Level().Objects.o_get_by_iterator(i);
			if (!pObj->H_Parent()) continue;
			//-----------------------------------------------------------
			NET_Packet			GEN;
			GEN.w_begin			(M_EVENT);
			//---------------------------------------------		
			GEN.w_u32			(Level().timeServer());
			GEN.w_u16			(GE_OWNERSHIP_REJECT);
			GEN.w_u16			(pObj->H_Parent()->ID());
			GEN.w_u16			(u16(pObj->ID()));
			game_events->insert	(GEN);
			//-------------------------------------------------------------
			ParentFound = true;
			//-------------------------------------------------------------
		};
		ProcessGameEvents();
	};

	CLObjNum = Level().Objects.o_count();

	for (u32 i=0; i<CLObjNum; i++)
	{
		CObject* pObj = Level().Objects.o_get_by_iterator(i);
		R_ASSERT(pObj->H_Parent()==NULL);
		//-----------------------------------------------------------
		NET_Packet			GEN;
		GEN.w_begin			(M_EVENT);
		//---------------------------------------------		
		GEN.w_u32			(Level().timeServer());
		GEN.w_u16			(GE_DESTROY);
		GEN.w_u16			(u16(pObj->ID()));
		game_events->insert	(GEN);
		//-------------------------------------------------------------
		ParentFound = true;
		//-------------------------------------------------------------
	};
	ProcessGameEvents();
};