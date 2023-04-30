////////////////////////////////////////////////////////////////////////////
//	Module 		: autosave_manager.cpp
//	Created 	: 04.11.2004
//  Modified 	: 04.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Autosave manager
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "autosave_manager.h"
#include "date_time.h"
#include "ai_space.h"
#include "level.h"
#include "xrMessages.h"

extern LPCSTR alife_section;

CAutosaveManager::CAutosaveManager			()
{
	u32							hours,minutes,seconds;
	LPCSTR						section = alife_section;

	sscanf						(pSettings->r_string(section,"autosave_interval"),"%d:%d:%d",&hours,&minutes,&seconds);
	m_autosave_interval			= (u32)generate_time(1,1,1,hours,minutes,seconds);
	m_last_autosave_time		= EngineTimeU();

	sscanf						(pSettings->r_string(section,"delay_autosave_interval"),"%d:%d:%d",&hours,&minutes,&seconds);
	m_delay_autosave_interval	= (u32)generate_time(1,1,1,hours,minutes,seconds);

	m_not_ready_count			= 0;

	shedule.t_min				= 5000;
	shedule.t_max				= 5000;
	shedule_register			();
}

CAutosaveManager::~CAutosaveManager			()
{
	shedule_unregister			();
}

float CAutosaveManager::shedule_Scale		()
{
	return						(.5f);
}

#include "UIGameCustom.h"
#include "Actor.h"
#include "MainMenu.h"

void CAutosaveManager::ScheduledUpdate(u32 dt)
{
	inherited::ScheduledUpdate(dt);

	if (!ai().get_alife())
		return;

	if (last_autosave_time() + autosave_interval() >= EngineTimeU())
		return;

	if (Device.dwPrecacheFrame || !g_actor || !ready_for_autosave() || !Actor()->g_Alive()) {
		delay_autosave();

		return;
	}
		
	update_autosave_time();

	string_path					temp;
	strconcat					(sizeof(temp),temp,Core.UserName," - ","autosave");
	NET_Packet					net_packet;

	net_packet.w_begin			(M_SAVE_GAME);
	net_packet.w_stringZ		(temp);
	net_packet.w_u8				(0);

	Level().Send				(net_packet,net_flags(TRUE));

	string_path					S1;

	xr_strcat					(temp, sizeof(temp), ".dds");
	FS.update_path				(S1, "$game_saves$", temp);

	MainMenu()->Screenshot		(IRender_interface::SM_FOR_GAMESAVE, S1);
	
	SDrawStaticStruct* s		= CurrentGameUI()->AddCustomStatic("autosave", true);
	s->m_endTime				= EngineTime() + 3.0f;// 3sec
}

void CAutosaveManager::on_game_loaded	()
{
	m_last_autosave_time		= EngineTimeU();
}