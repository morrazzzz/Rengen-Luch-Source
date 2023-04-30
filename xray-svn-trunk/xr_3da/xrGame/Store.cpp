#include "stdafx.h"
#include "Store.h"
#include "pch_script.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_space.h"
#include "script_engine_space.h"
#include "script_engine.h"

CStoreHouse::CStoreHouse()
{

}

CStoreHouse::~CStoreHouse() 
{

}

void CStoreHouse::save(IWriter &memory_stream)
{
	Msg("* Writing Store...");

	memory_stream.open_chunk	(STORE_CHUNK_DATA);

	string256					fn;
	luabind::functor<LPCSTR>	callback;
	xr_strcpy					(fn, pSettings->r_string("lost_alpha_cfg", "on_save_store_callback"));
	R_ASSERT					(ai().script_engine().functor<LPCSTR>(fn, callback));

	LPCSTR						str = callback();
	
	memory_stream.w_stringZ		(str);

	memory_stream.close_chunk	();
	
//	Msg("* %d store values successfully saved", size);
}

void CStoreHouse::load(IReader &file_stream)
{
	R_ASSERT2					(file_stream.find_chunk(STORE_CHUNK_DATA),"Can't find chunk STORE_CHUNK_DATA!");
	Msg							("* Loading Store...");

	string256					fn;
	luabind::functor<void>		callback;

	xr_strcpy					(fn, pSettings->r_string("lost_alpha_cfg", "on_load_store_callback"));
	R_ASSERT					(ai().script_engine().functor<void>(fn, callback));

	xr_string					str;
	
	file_stream.r_stringZ		(str);

	callback					(str.c_str());

}

