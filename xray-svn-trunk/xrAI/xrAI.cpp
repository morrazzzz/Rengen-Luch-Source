// xrAI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "xr_ini.h"
#include "process.h"
#include "xrAI.h"

#include "xr_graph_merge.h"
#include "game_spawn_constructor.h"
#include "xrCrossTable.h"
//#include "path_test.h"
#include "game_graph_builder.h"
#include <mmsystem.h>
#include "spawn_patcher.h"
#include "../xr_3da/xrGame/SpawnVersion.h"

#pragma comment(linker,"/STACK:0x800000,0x400000")

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"IMAGEHLP.LIB")
#pragma comment(lib,"winmm.LIB")
#pragma comment(lib,"xrcdb.LIB")
#pragma comment(lib,"MagicFM.LIB")
#pragma comment(lib,"xrCore.LIB")

extern LPCSTR LEVEL_GRAPH_NAME;

extern void	xrCompiler			(LPCSTR name, bool draft_mode, bool pure_covers, LPCSTR out_name);
extern void logThread			(void *dummy);
extern volatile BOOL bClose;
extern void test_smooth_path	(LPCSTR name);
extern void test_hierarchy		(LPCSTR name);
extern void	xrConvertMaps		();
extern void	test_goap			();
extern void	smart_cover			(LPCSTR name);
extern void	verify_level_graph	(LPCSTR name, bool verbose);
//extern void connectivity_test	(LPCSTR);
extern void compare_graphs		(LPCSTR level_name);
extern void test_levels			();

static const char* h_str = 
	"The following keys are supported / required:\n"
	"-? or -help  |  This is help\n\n"
	"-create_ai_map <NAME>  |  Compile level ai map with covers. Ex. -create_ai_map la02_garbage\n\n"
	"-ai_map_draft  |  A 'create_ai_map' property. Makes compiler to create a draft (no covers and lighting) ai map. Ex. -create_ai_map la02_garbage -ai_map_draft\n\n"
	"-ai_map_pure_covers  |  A 'create_ai_map' property. Makes all ai map nodes become cover points. Ex. -create_ai_map la02_garbage -ai_map_pure_covers\n\n"
	"-ai_map_name <NAME>  |  A 'create_ai_map' property. Makes comiler to name .ai file into specified string. Ex. -create_ai_map la02_garbage -ai_map_name garbage.ai \n\n"
	"-verify_ai_map <NAME>  |  Check ai map of specified level for broken/invalid nodes/connections. Ex. -verify_ai_map la02_garbage\n\n"
	"-ai_map_noverbose  |  A 'verify_ai_map' property. Don't log single linked level graph vertexes, while veriifying graph\n\n"
	//"-create_level_graph <NAME>  |  Build offline AI graph and cross-table to online ai map. Ex. -create_level_graph la02_garbage\n\n"
	"-create_game_graph  |  Merge level graphs into 'game.graph'. Ex. -merge_level_graphs\n\n"
	"-level_graphs_rebuild  |  A 'merge_level_graphs' property. Makes compiler to rebuild all level graphs, before merging. Ex. -merge_level_graphs -level_graphs_rebuild\n\n"
	"-create_spawn  |  Build game .spawn file. Ex. -create_spawn\n\n"
	"-spawn_name <NAME>  |  A 'create_spawn' property. The name of output .spawn file. Otherwise it will be named with actor spawn level Ex. -create_spawn -spawn_name all\n\n"
	"-spawn_level <NAME>  |  A 'create_spawn' property. If several levels have actor spawn point - the specified level will be chosen. Ex. -create_spawn -spawn_name all -spawn_level la02_garbage\n\n"
	"-no_separator_check  |  A 'create_spawn' property. Something related to space restrictors conectivity to ai map (???)\n\n"
	"-threads <NUM>  |  Number of threads to run in various stages. Ex -threads 16\n\n"
	"-no_ok_message  |  Don't show completion 'OK' message\n\n"
	"\n"
	"NOTE: The -create_ai_map or -verify_ai_map or -create_level_graph or -merge_level_graphs or -create_spawn keys are required for any functionality\n";

void Help()
{
	Msg("\nHelp:\n%s", h_str); MessageBox(0, h_str, "Command line options", MB_OK | MB_ICONINFORMATION);
}

string_path INI_FILE;

extern  HWND logWindow;

extern LPCSTR GAME_CONFIG;
extern void clear_temp_folder();

#define FSLTX "fs.ltx"

#define SE_FACTORY_HELP_COMPILER_F "SetHelpingAICompiler"
typedef void __cdecl assing_se_factory_to_help(bool value);
assing_se_factory_to_help* AssignSEFactoryHelpingXRAI = NULL;

void execute	(LPSTR cmd)
{
	// Load project
	string4096 name;
	name[0]=0;

#ifndef MERGE_GRAPH_IN_SPAWN
	if (strstr(cmd,"-patch_spawn")) {
		string256		spawn_id, previous_spawn_id;
		sscanf			(strstr(cmd,"-patch_spawn") + xr_strlen("-patch_spawn"),"%s %s",spawn_id,previous_spawn_id);
		spawn_patcher	a(spawn_id,previous_spawn_id);
		return;
	}
	else
#endif
	{
		if (strstr(cmd, "-create_ai_map"))
			sscanf(strstr(cmd, "-create_ai_map") + xr_strlen("-create_ai_map"), "%s", name);
		else
#if 0
			if (strstr(cmd, "-create_level_graph"))
				sscanf(strstr(cmd, "-create_level_graph") + xr_strlen("-create_level_graph"), "%s", name);

			else
#endif
				if (strstr(cmd, "-create_spawn"))
					; // sscanf(strstr(cmd, "-create_spawn") + xr_strlen("-create_spawn"), "%s", name); // Does it make compile only specified levels?
				else 
					if (strstr(cmd,"-verify_ai_map"))
						sscanf	(strstr(cmd,"-verify_ai_map") + xr_strlen("-verify_ai_map"), "%s" ,name);

		if (xr_strlen(name))
			xr_strcat			(name,"\\");
	}

	string_path			prjName;
	prjName				[0] = 0;
	bool				can_use_name = false;
	if (xr_strlen(name) < sizeof(string_path)) {
		can_use_name	= true;
		FS.update_path	(prjName,"$game_levels$",name);
	}

	FS.update_path		(INI_FILE,"$game_config$",GAME_CONFIG);
	
	if (strstr(cmd,"-create_ai_map")) {
		R_ASSERT3		(can_use_name,"Too big level name",name);
		
		char			*output = strstr(cmd,"-ai_map_name");
		string256		temp0;
		if (output) {
			output		+= xr_strlen("-ai_map_name");
			sscanf		(output,"%s",temp0);
			_TrimLeft	(temp0);
			output		= temp0;
		}
		else
			output		= (pstr)LEVEL_GRAPH_NAME;

		xrCompiler		(prjName,!!strstr(cmd,"-ai_map_draft"),!!strstr(cmd,"ai_map_pure_covers"),output);
	}
	else
#if 0 // Unsupported, since we compile game graph as in CoP
		if (strstr(cmd,"-create_level_graph"))
		{
			R_ASSERT3 (can_use_name,"Too big level name",name);

			string_path levelgraph, levelgct;

			strconcat(sizeof(levelgraph), levelgraph, name, GAME_LEVEL_GRAPH);
			strconcat(sizeof(levelgct), levelgct, name, CROSS_TABLE_NAME_RAW);

			FS.BackUpFile(levelgct, true);
			FS.BackUpFile(levelgraph, true);

			CGameGraphBuilder().build_graph	(levelgraph, levelgct, prjName);
		}
		else
#endif
		{
			if (strstr(cmd,"-create_game_graph"))
			{
				xrMergeGraphs		(prjName,!!strstr(cmd,"-level_graphs_rebuild"));
			}
			else
				if (strstr(cmd,"-create_spawn"))
				{
					if (xr_strlen(name))
						name[xr_strlen(name) - 1] = 0;

					string256 temp1;

					string256 spawn_name;
					if (strstr(Core.Params, "-spawn_name"))
					{
						sscanf(strstr(Core.Params, "-spawn_name") + xr_strlen("-spawn_name"), "%s", spawn_name);

					//	strconcat(sizeof(spawn_name), spawn_name, spawn_name, ".spawn");
					}

					char* start = strstr(cmd,"-spawn_level");
					if (start)
					{
						start			+= xr_strlen("-spawn_level");
						sscanf			(start,"%s",temp1);
						_TrimLeft		(temp1);
						start			= temp1;
					}
					char				*no_separator_check = strstr(cmd,"-no_separator_check");

					clear_temp_folder	();
					CGameSpawnConstructor(name, spawn_name, start, !!no_separator_check);
				}
				else
					if (strstr(cmd,"-verify_ai_map")) {
						R_ASSERT3			(can_use_name,"Too big level name",name);
						verify_level_graph	(prjName,!strstr(cmd,"-ai_map_noverbose"));
					}
		}
}

void Startup(LPSTR     lpCmdLine)
{
	string4096 cmd;

	xr_strcpy(cmd,lpCmdLine);
	xr_strlwr(cmd);
	if (strstr(cmd,"-?") || strstr(cmd,"-help"))			{ Help(); return; }
	
	// -patch_spawn is not used in our project
	
	if ((strstr(cmd,"-create_ai_map")==0) && (strstr(cmd,"-create_game_graph")==0) && (strstr(cmd,"-create_spawn")==0) && (strstr(cmd,"-verify_ai_map")==0) && (strstr(cmd,"-patch_spawn")==0))	{ Help(); return; }

	// Give a LOG-thread a chance to startup
	InitCommonControls	();
	Sleep				(150);
	thread_spawn		(logThread,	"log-update", 1024*1024,0);
	while				(!logWindow)	Sleep		(150);

	Msg("To get list of possible params - run xrAI directly from exe file or prompt '-?' or '-help'");
	
	u32					dwStartupTime	= timeGetTime();

	Msg("^ xrAI: Engine Internal Spawn Data Version = %u", SPAWN_VERSION);

	execute				(cmd);
	// Show statistic
	char				stats[256];
	extern				std::string make_time(u32 sec);
	extern				HWND logWindow;
	u32					dwEndTime = timeGetTime();

	xr_sprintf			(stats,"Time elapsed: %s",make_time((dwEndTime-dwStartupTime)/1000).c_str());

	Msg("Spawn compilation completed successfully. %s", stats);

	if (!strstr(Core.Params, "-no_ok_message"))
		MessageBox			(logWindow,stats,"Congratulation!",MB_OK|MB_ICONINFORMATION);

	bClose				= TRUE;
	FlushLog			();
	Sleep				(500);
}

#include "factory_api.h"

Factory_Create	*create_entity	= 0;
Factory_Destroy	*destroy_entity	= 0;

void buffer_vector_test();

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// check custom fsgame.ltx
	string_path	fsgame				= "";
	LPCSTR		fsgame_ltx_name		= "-fsltx ";
	if (strstr(lpCmdLine, fsgame_ltx_name)) 
	{
		u32 sz = xr_strlen	(fsgame_ltx_name);
		sscanf				(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
	}

	Debug._initialize		(false);
	Core._initialize		("xrai", 0, 1, fsgame[0] ? fsgame : FSLTX);

	buffer_vector_test();

	HMODULE					hFactory;

	LPCSTR					g_name	= "xrSE_Factory.dll";

	Log						("Loading DLL:",g_name);

	hFactory				= LoadLibrary	(g_name);

	if (0 == hFactory)
		R_ASSERT2(false, make_string("GetLastError = %u", GetLastError()));

	R_ASSERT2				(hFactory,"Factory DLL raised exception during loading or there is no factory DLL at all");
#ifdef _WIN64
	create_entity = (Factory_Create*)GetProcAddress(hFactory, "create_entity");
	destroy_entity = (Factory_Destroy*)GetProcAddress(hFactory, "destroy_entity");
#else
	create_entity = (Factory_Create*)GetProcAddress(hFactory, "_create_entity@4");
	destroy_entity = (Factory_Destroy*)GetProcAddress(hFactory, "_destroy_entity@4");
#endif
	R_ASSERT(create_entity);



	AssignSEFactoryHelpingXRAI = (assing_se_factory_to_help*)GetProcAddress(hFactory, SE_FACTORY_HELP_COMPILER_F);

	AssignSEFactoryHelpingXRAI(true); // Signal XR_SeFactory, that we compile spawn and make it do specific stuff for spawn compiling

	R_ASSERT(destroy_entity);

	Startup					(lpCmdLine);

	FreeLibrary				(hFactory);

	Core._destroy			();

	return					(0);
}