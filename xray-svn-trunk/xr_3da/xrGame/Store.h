#pragma once
#ifndef __STORE_H__
#define __STORE_H__
#include "object_interfaces.h"
#include "script_export_space.h"
#include "alife_space.h"

class CStoreHouse : public IPureSerializeObject<IReader, IWriter>
{
	public:
								CStoreHouse		();
		virtual					~CStoreHouse	();
		virtual void			load			(IReader& stream);
		virtual void			save			(IWriter& stream);

	public:
		DECLARE_SCRIPT_REGISTER_FUNCTION;
};

add_to_type_list(CStoreHouse)
#undef script_type_list
#define script_type_list save_type_list(CStoreHouse)

#endif