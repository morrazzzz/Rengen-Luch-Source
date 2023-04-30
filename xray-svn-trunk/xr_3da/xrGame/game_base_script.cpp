#include "pch_script.h"
#include "game_base.h"
#include "xrServer_script_macroses.h"

using namespace luabind;

template <typename T>
struct CWrapperBase : public T, public luabind::wrap_base {
	typedef T inherited;
	typedef CWrapperBase<T>	self_type;

	DEFINE_LUA_WRAPPER_METHOD_R2P1_V1(ExportDataToServer, NET_Packet)
	DEFINE_LUA_WRAPPER_METHOD_V0(clear)
};

#pragma optimize("s",on)

void game_GameState::script_register(lua_State *L)
{

	module(L)
		[
			luabind::class_< game_GameState, DLL_Pure >("game_GameState")
			.def(	constructor<>())

		];

}
