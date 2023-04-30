
#include "stdafx.h"
#include "luabind\luabind.hpp"
static LPVOID __cdecl luabind_allocator(
	luabind::memory_allocation_function_parameter const,
	void const * const pointer,
	size_t const size
)
{
	if (!size) {
		LPVOID	non_const_pointer = const_cast<LPVOID>(pointer);
		xr_free(non_const_pointer);
		return	(0);
	}
	LPVOID	non_const_pointer = const_cast<LPVOID>(pointer);

	return		(xr_realloc(non_const_pointer, size));
}
bool Initialized = false;
void setup_luabind_allocator()
{
	if (Initialized)return;
	Initialized = true;
	luabind::allocator = &luabind_allocator;
	luabind::allocator_parameter = 0;
}