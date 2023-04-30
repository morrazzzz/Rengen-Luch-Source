#include "stdafx.h"
#include "Light_Package.h"

void light_Package::ClearPackage()
{
	v_point.clear		();
	v_spot.clear		();
	v_shadowed.clear	();

	sortedShadowed_.clear();
	sortedShadowedCopy_.clear();
}
