#include "stdafx.h"
#include "pch_script.h"
#include "AnomDetectorBase.h"
#include "SimpleAnomDetector.h"
#include "ArtDetectorBase.h"
#include "SimpleArtDetector.h"
#include "AdvArtDetector.h"
#include "EliteArtDetector.h"

using namespace luabind;

#pragma optimize("s",on)
void CCustomDetectorR::script_register(lua_State *L)
{
	module(L)
	[
		class_<CCustomDetectorR, CGameObject>("CCustomDetectorR").def(constructor<>()),//base
		class_<CCustomDetector, CGameObject>("CCustomDetector").def(constructor<>()),
		class_<CSimpleAnomDetector, CGameObject>("CSimpleAnomDetector").def(constructor<>()),
		class_<CSimpleArtDetector, CGameObject>("CSimpleArtDetector").def(constructor<>()), //hud
		class_<CAdvancedDetector, CGameObject>("CAdvancedDetector").def(constructor<>()),
		class_<CEliteDetector, CGameObject>("CEliteDetector").def(constructor<>())
	];
}