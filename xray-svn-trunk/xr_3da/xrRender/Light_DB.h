#pragma once

#include "light.h"
#include "light_package.h"

struct LDBViewPortBuffers
{
	light_Package			rawPackage_;
	light_Package			rawPackageDeffered_;
};

class	CLight_DB
{
private:
	xr_vector<ref_light>	v_static;
	xr_vector<ref_light>	v_hemi;

	AccessLock				protectLightDeletion_;

	xr_vector<IRender_Light*>	lightsToDelete_;
public:
	ref_light				sun_original;
	ref_light				sun_adapted;

	LDBViewPortBuffers		ldbViewPortBuffer1;
	LDBViewPortBuffers		ldbViewPortBuffer2;

	LDBViewPortBuffers*		ldbTargetViewPortBuffer;

	u32						createdLightsCnt_;
	u32						deletedLightsCnt_;
public:
	void					add_light			(CLightSource* L);

	void					Load				(IReader* fs);
	void					LoadHemi			();
	void					Unload				();

	CLightSource*			CreateLight			();
	void					DeleteLight			(IRender_Light*& light_to_delete);

	void					ClearLightReffs		(CLightSource* light_to_delete);

	void					DeleteQueue			();

	void					UpdateLight			(ViewPort viewport);

	CLight_DB				();
	~CLight_DB				();
};
