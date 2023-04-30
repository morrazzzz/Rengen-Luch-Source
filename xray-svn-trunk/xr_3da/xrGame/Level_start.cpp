#include "stdafx.h"
#include "Level_Bullet_Manager.h"
#include "game_cl_base.h"
#include "../device.h"
#include "../IGame_Persistent.h"
#include "UIGameCustom.h"
#include "../../xrphysics/iphworld.h"
#include "level.h"
#include "../x_ray.h"
#include "PhysicsGamePars.h"
#include "xrServer.h"
#include "../CommonFlags.h"

BOOL CLevel::Gameloading(LPCSTR op_server)
{
	m_GameCreationOptions = op_server;

	if (!loading_save_timer_started)
	{
		loading_save_timer.Start();
		loading_save_timer_started = true;

		pApp->LoadPhaseBegin(LEVEL_LOAD_PHASES);
	}

	g_loading_events.push_back(LOADING_EVENT(this, &CLevel::Gameloading_Stage_1));
	g_loading_events.push_back(LOADING_EVENT(this, &CLevel::Gameloading_Stage_3));
	g_loading_events.push_back(LOADING_EVENT(this, &CLevel::Gameloading_Stage_4));
	g_loading_events.push_back(LOADING_EVENT(this, &CLevel::Gameloading_Stage_5));
	g_loading_events.push_back(LOADING_EVENT(this, &CLevel::Gameloading_Stage_6));
	g_loading_events.push_back(LOADING_EVENT(this, &CLevel::Gameloading_Stage_7));
	
	return TRUE;
}

bool CLevel::Gameloading_Stage_1()
{
	g_pGamePersistent->LoadTitle("st_server_starting", "Starting Server");

	Msg("^ xrGame: Engine Internal Spawn Data Version = %u", SPAWN_VERSION);
	
	Server = xr_new <xrServer>();
	Server->Server_game_sv_base = NULL;

	CLASS_ID clsid = (TEXT2CLSID("SV_SINGL"));
	Server->Server_game_sv_base = smart_cast<game_sv_GameState*> (NEW_INSTANCE(clsid));

	Server->Server_game_sv_base->Create(m_GameCreationOptions);

	Server->SLS_Default();
	m_name = Server->level_name(m_GameCreationOptions);

	// just to update level name and tip
	g_pGamePersistent->LoadTitle("st_server_starting", "Starting Server", true, m_name);

	return true;
}

extern	pureFrame*				g_pNetProcessor;

bool CLevel::Gameloading_Stage_3()
{
	Server->create_direct_client();
	BOOL connected_to_server_result = Connect2Server();
	R_ASSERT(connected_to_server_result);

	LPCSTR level_name = NULL;
	level_name = *name();

	// Determine internal level-ID
	int	level_id = pApp->Level_ID(level_name);
	pApp->Level_Set(level_id);
	m_name = level_name;

	// Load level
	R_ASSERT2(Load(level_id), "Loading failed.");
	return true;
}

#include "phcommander.h"
#include "physics_game.h"

bool CLevel::Gameloading_Stage_4()
{
	// Begin spawn
	g_pGamePersistent->LoadTitle("st_client_spawning", "Spawning Client");

	// Send physics to single or multithreaded mode
	LoadPhysicsGameParams();

	create_physics_world(!!psDeviceFlags.test(mtPhysics), &ObjectSpace, &Objects, &Device);

	R_ASSERT(physics_world());

	m_ph_commander_physics_worldstep = xr_new<CPHCommander>();
	physics_world()->set_update_callback((IPHWorldUpdateCallbck*)m_ph_commander_physics_worldstep);

	physics_world()->set_default_contact_shotmark(ContactShotMark);
	physics_world()->set_default_character_contact_shotmark(CharacterContactShotMark);

	// Send network to single or multithreaded mode
	// *note: release version always has "mt_*" enabled
	Device.seqFrameMT.Remove(g_pNetProcessor);
	Device.seqFrame.Remove(g_pNetProcessor);
	if (psDeviceFlags.test(mtNetwork))	Device.seqFrameMT.Add(g_pNetProcessor, REG_PRIORITY_HIGH + 2);
	else								Device.seqFrame.Add(g_pNetProcessor, REG_PRIORITY_LOW - 100);

	while (!game_configured)
	{
		ClientReceive();
		if (Server)
			Server->Update();
		Sleep(5);
	}

	return true;
}

bool CLevel::Gameloading_Stage_5()
{
	// Textures
	return inherited::LoadTextures();
}

bool CLevel::Gameloading_Stage_6()
{
	// Sync
	g_pGamePersistent->LoadTitle("st_loading_hud", "Loading User Interface");

	g_hud->Load();
	g_hud->OnConnected();

	g_pGamePersistent->LoadTitle("st_client_synchronising", "Syncing Objects");
	Device.PreCache(30, true, true);

	return true;
}

#include "hudmanager.h"

bool CLevel::Gameloading_Stage_7()
{
	g_loading_events.pop_front	();

	//init bullet manager
	BulletManager().Clear		();
	BulletManager().Load		();

	if (CurrentGameUI())
		CurrentGameUI()->OnConnected();

	pApp->LoadPhaseEnd();
	return false;
}

void CLevel::InitializeClientGame(NET_Packet& P)
{
	string256 game_type_name;
	P.r_stringZ(game_type_name); // warnong ok, need to read value and proceed

#ifdef DEBUG
	Msg("* Initialize Client Game %s", game_type_name);
#endif

	xr_delete(level_game_cl_base);
	CLASS_ID clsid			= (TEXT2CLSID("CL_SINGL"));
	level_game_cl_base		= smart_cast<game_cl_GameState*> ( NEW_INSTANCE ( clsid ) );
	m_bGameConfigStarted	= TRUE;
}

