//----------------------------------------------------
// file: BuilderGame.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "Builder.h"
#include "Scene.h"
#include "../../../xr_3da/xrGame/LevelGameDef.h"
#include "SoundManager_LE.h"
#include "CustomObject.h"

HMODULE hXRSE_FACTORY = NULL;

typedef void __cdecl extern_func_1(bool v);
typedef void __cdecl extern_func_2();

extern_func_1* SetGatherRestrStats = NULL;
extern_func_2* ClearRestrStats = NULL;
extern_func_2* FlushRestrStats = NULL;

BOOL SceneBuilder::BuildGame()
{
	// Load functions to get statistics for restrictors count and types
	hXRSE_FACTORY = LoadLibrary("xrSE_Factory.dll");

	if(hXRSE_FACTORY)
	{
		SetGatherRestrStats = (extern_func_1*)GetProcAddress(hXRSE_FACTORY, "SetGatherRestrictorsStats");
		ClearRestrStats = (extern_func_2*)GetProcAddress(hXRSE_FACTORY, "ClearRestrictorsStatistics");
		FlushRestrStats = (extern_func_2*)GetProcAddress(hXRSE_FACTORY, "FlushRestrictorsBuildStatistics");
		
		VERIFY(SetGatherRestrStats && ClearRestrStats && FlushRestrStats);
		
		if(SetGatherRestrStats)
			SetGatherRestrStats(true);
		
		if(ClearRestrStats)
			ClearRestrStats();
	}
	
	
	// Do server exportings
	SExportStreams 		F;
    if (!Scene->ExportGame(&F))				return FALSE;

	// Free statistics
	if(hXRSE_FACTORY)
	{
		if(FlushRestrStats)
			FlushRestrStats();
		
		if(ClearRestrStats)
			ClearRestrStats();
		
		if(SetGatherRestrStats)
			SetGatherRestrStats(false);
		
		FreeLibrary(hXRSE_FACTORY);
	}

	FlushLog();
	
    BOOL bRes = TRUE;
	
    // save spawn
    {
        xr_string lev_spawn 	  			= MakeLevelPath("level.spawn");
        EFS.MarkFile						(lev_spawn.c_str(),true);
        if (F.spawn.chunk)
            if (!F.spawn.stream.save_to		(lev_spawn.c_str())) bRes = FALSE;
    }

    // save game
    {
        CMemoryWriter GAME; 
        GAME.w_chunk(WAY_PATROLPATH_CHUNK,	F.patrolpath.stream.pointer(),	F.patrolpath.stream.size());
        GAME.w_chunk(RPOINT_CHUNK,			F.rpoint.stream.pointer(),		F.rpoint.stream.size());
        xr_string lev_game 					= MakeLevelPath("level.game");
        EFS.MarkFile						(lev_game.c_str(),true);
        if (GAME.size())
            if (!GAME.save_to				(lev_game.c_str())) bRes = FALSE;
    }

    // save weather env modificator
    {
        xr_string lev_env_mod				= MakeLevelPath("level.env_mod");
        EFS.MarkFile						(lev_env_mod.c_str(),true);
        if (F.envmodif.chunk)
	        if (!F.envmodif.stream.save_to	(lev_env_mod.c_str())) bRes = FALSE;
    }

    // save static sounds
    {
        xr_string lev_sound_static 			= MakeLevelPath("level.snd_static");
        EFS.MarkFile						(lev_sound_static.c_str(),true);
        if (F.sound_static.chunk)    	
            if (!F.sound_static.stream.save_to	(lev_sound_static.c_str())) bRes = FALSE;
    }

    // save sound envs
    {
        xr_string lev_sound_env 			= MakeLevelPath("level.snd_env");
        EFS.MarkFile						(lev_sound_env.c_str(),true);
        if (LSndLib->MakeEnvGeometry		(F.sound_env_geom.stream,false))
            if (!F.sound_env_geom.stream.save_to(lev_sound_env.c_str())) bRes = FALSE;
    }

    // save static PG
    {
        xr_string lev_pe_static 			= MakeLevelPath("level.ps_static");
        EFS.MarkFile						(lev_pe_static.c_str(),true);
        if (F.pe_static.chunk)    	
            if (!F.pe_static.stream.save_to	(lev_pe_static.c_str())) bRes = FALSE;
    }
	
	FlushLog();
	
    return bRes;
}

