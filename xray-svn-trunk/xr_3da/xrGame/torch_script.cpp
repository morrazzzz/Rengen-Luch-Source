#include "pch_script.h"
#include "torch.h"
#include "PDA.h"
#include "SimpleArtDetector.h"
#include "EliteArtDetector.h"
#include "AdvArtDetector.h"

using namespace luabind;

#pragma optimize("s",on)
void CTorch::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CTorch,CGameObject>("CTorch")
			.def(constructor<>()),
		class_<CPda, CGameObject>("CPda")
			.def(constructor<>()),
		class_<CScientificArtDetector,CGameObject>("CScientificArtDetector")
			.def(constructor<>()),
		class_<CEliteArtDetector,CGameObject>("CEliteArtDetector")
			.def(constructor<>()),
		class_<CAdvArtDetector,CGameObject>("CAdvArtDetector")
			.def(constructor<>()),
		class_<CSimpleArtDetector,CGameObject>("CSimpleArtDetector")
			.def(constructor<>())
	];
}
