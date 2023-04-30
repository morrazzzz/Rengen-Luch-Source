#pragma once

#include "light.h"

class	light_Package
{
public:
	xr_vector<CLightSource*>		v_point;
	xr_vector<CLightSource*>		v_spot;
	xr_vector<CLightSource*>		v_shadowed; // usorted pool, which can contain not needed lights yet

	xr_vector<CLightSource*>		sortedShadowed_;
	xr_vector<CLightSource*>		sortedShadowedCopy_; // a copy for mt visability calcs 
public:
	void							ClearPackage();
};
