// stdafx.cpp : source file that includes just the standard includes
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifndef LUABIND_NO_EXCEPTIONS
namespace boost
{
	void throw_exception(const std::exception &) {}
}
#endif