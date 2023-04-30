////////////////////////////////////////////////////////////////////////////
//	Module 		: game_sv_base_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Base server game script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "game_sv_base.h"
#include "xrMessages.h"
#include "UI/UIGameTutorial.h"
#include "string_table.h"
#include "infoportion.h"
#include "../x_ray.h"

using namespace luabind;

CUISequencer* g_tutorial = NULL;
CUISequencer* g_tutorial2 = NULL;

void start_tutorial(LPCSTR name)
{
	if(g_tutorial){
		VERIFY				(!g_tutorial2);
		g_tutorial2			= g_tutorial;
	};
		
	g_tutorial							= xr_new <CUISequencer>();
	g_tutorial->Start					(name);
	if(g_tutorial2)
		g_tutorial->m_pStoredInputReceiver = g_tutorial2->m_pStoredInputReceiver;

}

bool is_registered_info(LPCSTR info_id)
{
	return CInfoPortion::IsComplexInfoPortion(info_id);
}

LPCSTR translate_string(LPCSTR str)
{
	return *CStringTable().translate(str);
}

void reload_language()
{
	CStringTable().ReloadLanguage();
}

bool has_active_tutotial()
{
	return (g_tutorial!=NULL);
}

bool is_r2_active()
{
	return (psDeviceFlags.test(rsR2)) ? true : false;
}

bool developer_mode()
{
	return CApplication::isDeveloperMode;
}


u32 get_game_time_u32()
{
	return (u32)GetGameTime();
}

u32 get_game_time_in_seconds_u32()
{
	return GetGameTimeInSecU();
}

float get_game_time_in_seconds()
{
	return GetGameTimeInSec();
}

#pragma optimize("s",on)
void game_sv_GameState::script_register(lua_State *L)
{

	module(L,"game")
	[
	class_< xrTime >("CTime")
		.enum_("date_format")
		[
			value("DateToDay",		int(edpDateToDay)),
			value("DateToMonth",	int(edpDateToMonth)),
			value("DateToYear",		int(edpDateToYear))
		]
		.enum_("time_format")
		[
			value("TimeToHours",	int(etpTimeToHours)),
			value("TimeToMinutes",	int(etpTimeToMinutes)),
			value("TimeToSeconds",	int(etpTimeToSeconds)),
			value("TimeToMilisecs",	int(etpTimeToMilisecs))
		]
		.def(						constructor<>()				)
		.def(						constructor<const xrTime&>())
		.def(						constructor<int,int,int,int,int>())
		.def(const_self <			xrTime()					)
		.def(const_self <=			xrTime()					)
		.def(const_self >			xrTime()					)
		.def(const_self >=			xrTime()					)
		.def(const_self ==			xrTime()					)
		.def(self +					xrTime()					)
		.def(self -					xrTime()					)

		.def("diffSec"				,&xrTime::diffSec_script)
		.def("add"					,&xrTime::add_script)
		.def("sub"					,&xrTime::sub_script)

		.def("setHMS"				,&xrTime::setHMS)
		.def("setHMSms"				,&xrTime::setHMSms)
		.def("set"					,(void (xrTime::*)(int,int,int,int,int,int,int))(&xrTime::set))
//		.def("get"					,&xrTime::get, out_value(_2) + out_value(_3) + out_value(_4) + out_value(_5) + out_value(_6) + out_value(_7) + out_value(_8))
		.def("get"					,(void (xrTime::*) (u32&,u32&,u32&,u32&,u32&,u32&,u32&))(&xrTime::get), out_value(_2) + out_value(_3) + out_value(_4) + out_value(_5) + out_value(_6) + out_value(_7) + out_value(_8))
		.def("dateToString"			,&xrTime::dateToString)
		.def("timeToString"			,&xrTime::timeToString),
		// declarations
		def("time",					get_game_time_u32),
		def("elapsed_seconds",		get_game_time_in_seconds),
		def("elapsed_seconds_u",	get_game_time_in_seconds_u32),
		def("time",					get_game_time_u32),
		def("get_game_time",		get_time_struct),
//		def("get_surge_time",	Game::get_surge_time),
//		def("get_object_by_name",Game::get_object_by_name),
	
	class_< game_sv_GameState, game_GameState >("game_sv_GameState")

	.def("u_EventSend",			&game_sv_GameState::u_EventSend),

	def("start_tutorial",		&start_tutorial),
	def("has_active_tutorial",	&has_active_tutotial),
	def("translate_string",		&translate_string),
	def("reload_language",		&reload_language),
	def("convert_time",			((xrTime (*) (u32)) &convert_time)),
	def("convert_time",			((u32	 (*) (const xrTime &)) &convert_time)),
	def("is_registered_info",	&is_registered_info),
	def("developer_mode",		&developer_mode)
	];
}
