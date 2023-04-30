#pragma once

namespace GameConstants
{
	void				LoadConstants();
	void				LoadHandGranadesTypes();

	int					GetNoviceRankStart();
	int					GetExperiencesRankStart();
	int					GetVeteranRankStart();
	int					GetMasterRankStart();


	//Gets grenades types assigned in weapons_common_constants
	xr_vector<shared_str>&	GetHandGrandesTypes();

	// Max artefacts in game that are not yet touched by actor or other alife entities
	u32					GetMaxAnomalySpawnedArtefacts();

	// Gets the aditional cost penalty persentage for X amount of missing eatable item portions
	float				GetCostPenaltyForMissingPortions(u8 num_of_missing_portions);

	// Gets the "empty eatable item" weight (ex. weight of bottle without water)
	float				GetEIContainerWeightPerc();

	bool				GetLampsRestore(); // do lamps restore?
	u32					GetLampsRestoreTime(); // in hours

	bool				GetUseCharIcons();
	bool				GetUseCharInfoInWnds();

	float				GetDontRemoveBoltDist();
	float				GetDontRemovedDestroyableDist();

	float				GetDistantSndApplyDistance();

	// F pressing times
	float				GetUseTimePickUp();
	float				GetUseTimePickUpLong();
	float				GetUseTimeScriptUsable();
	float				GetUseTimeHolder();
	float				GetUseTimeTalk();
	float				GetUseTimeStash();
	float				GetUseTimeBodySearch();
	float				GetUseTimeCapturePh();

	float				GetSinglePickUpFov();
	float				GetGroupPickUpFov();
	u32					GetGroupPickUpLimit();


};