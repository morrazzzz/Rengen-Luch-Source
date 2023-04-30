#include "StdAfx.h"
#include "GameConstants.h"
#include "GamePersistent.h"

xr_vector<shared_str> handGrenadesTypes_;

int noviceRankStart_			= 0;
int experiencedRankStart_		= 0;
int veteranRankStart_			= 0;
int masterRankStart_			= 0;

u32	maxAnomalySpawnedArtefacts_ = 0;

float costPenaltyPercentage_1_ = 0.f;
float costPenaltyPercentage_2_ = 0.f;
float costPenaltyPercentage_3_ = 0.f;
float costPenaltyPercentage_4_ = 0.f;

float eatableContainerWeightPerc_ = 0.f;

bool restoreLamps_;
u32 lampRestoreTimeInHours_ = 48;

bool useCharIcons_ = true;
bool useCharInfoInNPPCInteracWnds_ = true;

float dontRemoveBoltDist_ = 50.f;
float dontRemovedDestroyableDist_ = 50.f;

float distantSndApplyDistance_ = 150.f;

float useTimePickUp = 0.15f;
float useTimePickUpLong = 0.8f;
float useTimeScriptUse = 0.5f;
float useTimeHolder = 1.f;
float useTimeTalk = 0.3f;
float useTimeStash = 0.8f;
float useTimeBodySearch = 0.5f;
float useTimeCapturePh = 0.8f;

float singlePickUpFov = 10.f;
float groupPickUpFov = 20.f;

u32	groupPickUpLimit = 10;

// developer and misc
extern bool noPathBuildError;

namespace GameConstants
{
	void LoadConstants()
	{
		LoadHandGranadesTypes();

		noviceRankStart_			= pSettings->r_s32("game_relations", "novice_start");
		experiencedRankStart_		= pSettings->r_s32("game_relations", "experienced_start");
		veteranRankStart_			= pSettings->r_s32("game_relations", "veteran_start");
		masterRankStart_			= pSettings->r_s32("game_relations", "master_start");

		maxAnomalySpawnedArtefacts_ = pSettings->r_u32("alife", "max_anomaly_spawned_artefacts");

		restoreLamps_				= !!pSettings->r_bool("alife", "restore_lamps");
		lampRestoreTimeInHours_		= pSettings->r_u32("alife", "restore_lamps_time_hours");

		costPenaltyPercentage_1_	= pSettings->r_float("inventory", "portions_cost_penalty_1");
		costPenaltyPercentage_2_	= pSettings->r_float("inventory", "portions_cost_penalty_2");
		costPenaltyPercentage_3_	= pSettings->r_float("inventory", "portions_cost_penalty_3");
		costPenaltyPercentage_4_	= pSettings->r_float("inventory", "portions_cost_penalty_4");

		eatableContainerWeightPerc_	= pSettings->r_float("inventory", "eatable_item_container_weight_percentage");

		useCharIcons_					= !!pSettings->r_bool("various_game_consts", "use_char_icons");
		useCharInfoInNPPCInteracWnds_	= !!pSettings->r_bool("various_game_consts", "use_char_info_in_npc_interaction_wnds");

		dontRemoveBoltDist_				= pSettings->r_float("various_game_consts", "bolt_dont_remove_dist");
		dontRemovedDestroyableDist_		= pSettings->r_float("various_game_consts", "physics_dont_remove_dist");

		distantSndApplyDistance_		= pSettings->r_float("various_game_consts", "distant_snd_apply_distance");

		useTimePickUp					= pSettings->r_float("various_game_consts", "use_time_pick_up");
		useTimePickUpLong				= pSettings->r_float("various_game_consts", "use_time_pick_up_nearest");
		useTimeScriptUse				= pSettings->r_float("various_game_consts", "use_time_script_use");
		useTimeHolder					= pSettings->r_float("various_game_consts", "use_time_mounted_turret");
		useTimeTalk						= pSettings->r_float("various_game_consts", "use_time_talk");
		useTimeStash					= pSettings->r_float("various_game_consts", "use_time_stash");
		useTimeBodySearch				= pSettings->r_float("various_game_consts", "use_time_body_loot");
		useTimeCapturePh				= pSettings->r_float("various_game_consts", "use_time_capture_physics");

		singlePickUpFov					= pSettings->r_float("various_game_consts", "single_pick_up_fov");
		groupPickUpFov					= pSettings->r_float("various_game_consts", "group_pick_up_fov");
		groupPickUpLimit				= pSettings->r_u32("various_game_consts", "group_pick_max_cnt");

		noPathBuildError				= strstr(Core.Params, "-no_build_path_errors");

		Msg("# GameConstants are Loaded");
	}

	void LoadHandGranadesTypes()
	{
		LPCSTR grenade_types = pSettings->r_string("weapons_common_constants", "hand_grenades_types");

		xr_vector<shared_str> known_types;

		if (grenade_types && grenade_types[0])
		{
			string128 grenade_section;
			int	count = _GetItemCount(grenade_types);
			for (int i = 0; i < count; ++i)
			{
				_GetItem(grenade_types, i, grenade_section);
				known_types.push_back(grenade_section);
			}
		}

		handGrenadesTypes_ = known_types;
	}

	xr_vector<shared_str>& GetHandGrandesTypes()
	{
		return handGrenadesTypes_;
	}

	int GetNoviceRankStart()
	{
		return noviceRankStart_;
	}

	int	GetExperiencesRankStart()
	{
		return experiencedRankStart_;
	}

	int	GetVeteranRankStart()
	{
		return veteranRankStart_;
	}

	int	GetMasterRankStart()
	{
		return masterRankStart_;
	}

	u32	GetMaxAnomalySpawnedArtefacts()
	{
		return maxAnomalySpawnedArtefacts_;
	}

	float GetCostPenaltyForMissingPortions(u8 num_of_missing_portions)
	{
		if (num_of_missing_portions == 0)
			return 0.f;

		switch (num_of_missing_portions)
		{
		case 1:
			return costPenaltyPercentage_1_;
		case 2:
			return costPenaltyPercentage_2_;
		case 3:
			return costPenaltyPercentage_3_;
		case 4:
			return costPenaltyPercentage_4_;
		default:
			return costPenaltyPercentage_4_;
		}
	}

	float GetEIContainerWeightPerc()
	{
		return eatableContainerWeightPerc_;
	}

	bool GetLampsRestore()
	{
		return restoreLamps_;
	}

	u32 GetLampsRestoreTime()
	{
		return lampRestoreTimeInHours_;
	}

	bool GetUseCharIcons()
	{
		return useCharIcons_;
	}

	bool GetUseCharInfoInWnds()
	{
		return useCharInfoInNPPCInteracWnds_;
	}

	float GetDontRemoveBoltDist()
	{
		return dontRemoveBoltDist_;
	}

	float GetDontRemovedDestroyableDist()
	{
		return dontRemovedDestroyableDist_;
	}

	float GetDistantSndApplyDistance()
	{
		return distantSndApplyDistance_;
	}

	float GetUseTimePickUp()
	{
		return useTimePickUp;
	}

	float GetUseTimePickUpLong()
	{
		return useTimePickUpLong;
	}

	float GetUseTimeScriptUsable()
	{
		return useTimeScriptUse;
	}

	float GetUseTimeHolder()
	{
		return useTimeHolder;
	}

	float GetUseTimeTalk()
	{
		return useTimeTalk;
	}

	float GetUseTimeStash()
	{
		return useTimeStash;
	}

	float GetUseTimeBodySearch()
	{
		return useTimeBodySearch;
	}

	float GetUseTimeCapturePh()
	{
		return useTimeCapturePh;
	}

	float GetSinglePickUpFov()
	{
		return singlePickUpFov;
	}

	float GetGroupPickUpFov()
	{
		return groupPickUpFov;
	}

	u32 GetGroupPickUpLimit()
	{
		return groupPickUpLimit;
	}
}