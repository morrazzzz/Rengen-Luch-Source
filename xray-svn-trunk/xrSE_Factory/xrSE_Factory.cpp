////////////////////////////////////////////////////////////////////////////
//	Module 		: xrSE_Factory.cpp
//	Created 	: 18.06.2004
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Precompiled header creatore
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrSE_Factory.h"
#include "../xr_3da/xrGame/ai_space.h"
#include "../xr_3da/xrGame/script_engine.h"
#include "../xr_3da/xrGame/object_factory.h"
#include "../editors/xrEProps/xrEProps.h"
#include "xrSE_Factory_import_export.h"
#include "script_properties_list_helper.h"

#include "../xr_3da/xrGame/character_info.h"
#include "../xr_3da/xrGame/specific_character.h"
#include "../xr_3da/xrGame/SpawnVersion.h"
//#include "character_community.h"
//#include "monster_community.h"
//#include "character_rank.h"
//#include "character_reputation.h"
#pragma comment(lib,"xrCore.lib")

extern CSE_Abstract *F_entity_Create	(LPCSTR section);

bool isCpmpilingSpawn_ = false;

extern CScriptPropertiesListHelper	*g_property_list_helper;
extern HMODULE						prop_helper_module;

// For SDK ussage
extern bool doWriteRestrStatistics_;

extern xr_vector <std::string> not_restrs;
extern xr_vector <std::string> none_restrs;
extern xr_vector <std::string> out_restrs;
extern xr_vector <std::string> in_restrs;
extern void setup_luabind_allocator();

extern "C" {
	FACTORY_API	ISE_Abstract* __stdcall create_entity	(LPCSTR section)
	{
		setup_luabind_allocator();
		return					(F_entity_Create(section));
		return 0;
	}

	FACTORY_API	void		__stdcall destroy_entity	(ISE_Abstract *&abstract)
	{
		CSE_Abstract			*object = smart_cast<CSE_Abstract*>(abstract);
		F_entity_Destroy		(object);
		abstract				= 0;
	}
	
	__declspec(dllexport) void	__cdecl	FlushSEFactoryLog()
	{
		Msg("FlushSEFactoryLog");
		FlushLog(true);
	}

	__declspec(dllexport) void	__cdecl	SetHelpingAICompiler(bool value)
	{
		Msg("SetIsHelpingAICompiler %d", value);
		isCpmpilingSpawn_ = value;
	}

	__declspec(dllexport) void __cdecl ClearRestrictorsStatistics()
	{
		not_restrs.clear();
		none_restrs.clear();
		in_restrs.clear();
		out_restrs.clear();
	}

	__declspec(dllexport) void __cdecl FlushRestrictorsBuildStatistics()
	{
		Msg(LINE_SPACER);
		Msg("Not A Restrictors: %u", not_restrs.size());

		for (u32 i = 0; i < not_restrs.size(); i++)
		{
			Msg("%s", not_restrs[i].c_str());
		}

		Msg(LINE_SPACER);
		Msg("None Restrictors: %u", none_restrs.size());

		for (u32 i = 0; i < none_restrs.size(); i++)
		{
			Msg("%s", none_restrs[i].c_str());
		}

		Msg(LINE_SPACER);
		Msg("Out Restrictors: %u", out_restrs.size());

		for (u32 i = 0; i < out_restrs.size(); i++)
		{
			Msg("%s", out_restrs[i].c_str());
		}

		Msg(LINE_SPACER);
		Msg("In Restrictors: %u", in_restrs.size());

		for (u32 i = 0; i < in_restrs.size(); i++)
		{
			Msg("%s", in_restrs[i].c_str());
		}

		Msg(LINE_SPACER);
	}

	__declspec(dllexport) void __cdecl SetGatherRestrictorsStats(bool value)
	{
		doWriteRestrStatistics_ = value;
	}
};

typedef void DUMMY_STUFF (const void*,const u32&,void*);
XRCORE_API DUMMY_STUFF	*g_temporary_stuff;

#define TRIVIAL_ENCRYPTOR_DECODER
#include "../xrCore/trivial_encryptor.h"

BOOL APIENTRY DllMain		(HANDLE module_handle, DWORD call_reason, LPVOID reserved)
{
	switch (call_reason) {
		case DLL_PROCESS_ATTACH: {
			g_temporary_stuff			= &trivial_encryptor::decode;
			Debug._initialize(false,true);
			
 			Core._initialize			("xrSE_Factory",NULL,TRUE,"fsfactory.ltx",true);
			string_path					SYSTEM_LTX;
			FS.update_path				(SYSTEM_LTX,"$game_config$","system.ltx");
#ifdef DEBUG
			Msg							("Updated path to system.ltx is %s", SYSTEM_LTX);
#endif // #ifdef DEBUG
			pSettings					= new CInifile(SYSTEM_LTX);

			Msg("^ xrSE_Factory: Engine Internal Spawn Data Version = %u", SPAWN_VERSION);

			CCharacterInfo::InitInternal					();
	CSpecificCharacter::InitInternal				();
//			CHARACTER_COMMUNITY::InitInternal				();
//			CHARACTER_RANK::InitInternal					();
//			CHARACTER_REPUTATION::InitInternal				();
//			MONSTER_COMMUNITY::InitInternal					();

			break;
		}
		case DLL_PROCESS_DETACH: {
			CCharacterInfo::DeleteSharedData				();
			CCharacterInfo::DeleteIdToIndexData				();
			CSpecificCharacter::DeleteSharedData			();
			CSpecificCharacter::DeleteIdToIndexData			();
//			CHARACTER_COMMUNITY::DeleteIdToIndexData		();
//			CHARACTER_RANK::DeleteIdToIndexData				();
//			CHARACTER_REPUTATION::DeleteIdToIndexData		();
//			MONSTER_COMMUNITY::DeleteIdToIndexData			();


			xr_delete					(g_object_factory);
			CInifile **tmp				= (CInifile**) &pSettings;
			xr_delete					(*tmp);
			xr_delete					(g_property_list_helper);
			xr_delete					(g_ai_space);
			xr_delete					(g_object_factory);
			if (prop_helper_module)
				FreeLibrary				(prop_helper_module);
			Core._destroy				();
			break;
		}
	}
    return				(TRUE);
}
