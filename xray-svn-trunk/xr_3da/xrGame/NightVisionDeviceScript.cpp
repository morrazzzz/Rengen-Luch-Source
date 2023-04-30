#include "pch_script.h"
#include "NightVisionDevice.h"

using namespace luabind;

#pragma optimize("s",on)
void CNightVisionDevice::script_register(lua_State *L)
{
	module(L)
		[
			class_<CNightVisionDevice, CGameObject>("CNightVisionDevice")
			.def(constructor<>())
		];
}
