
#include "stdafx.h"
#include "luabind\luabind.hpp"


static void *__cdecl luabind_allocator(luabind::memory_allocation_function_parameter, const void *pointer, size_t const size) //Раньше всего инитится здесь, поэтому пусть здесь и будет
{
	if (!size)
	{
		LPVOID	non_const_pointer = const_cast<LPVOID>(pointer);
		xr_free(non_const_pointer);

		return	(0);
	}

	LPVOID	non_const_pointer = const_cast<LPVOID>(pointer);

	return(xr_realloc(non_const_pointer, size));
}

void setup_luabind_allocator()
{
	luabind::allocator = &luabind_allocator;
	luabind::allocator_parameter = 0;
}