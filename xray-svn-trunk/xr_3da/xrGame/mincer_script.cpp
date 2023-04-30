#include "pch_script.h"
#include "mincer.h"
#include "zonegalantine.h"
#include "RadioactiveZone.h"

using namespace luabind;

#pragma optimize("s",on)
void CMincer::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CMincer,CGameObject>("CMincer")
			.def(constructor<>()),
		class_<CZoneGalantine,CGameObject>("CZoneGalantine")
			.def(constructor<>()),
		class_<CRadioactiveZone, CGameObject>("CRadioactiveZone")
		.def(constructor<>())
	];
}
