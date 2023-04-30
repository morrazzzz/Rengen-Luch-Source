#pragma once

#pragma warning(disable:4995)
#include "../stdafx.h"
#include <dplay8.h>
#pragma warning(default:4995)
#pragma warning( 4 : 4018 )
#pragma warning( 4 : 4244 )
#pragma warning(disable:4505)

// this include MUST be here, since smart_cast is used >1800 times in the project
#include "smart_cast.h"

#if XRAY_EXCEPTIONS
IC	xr_string	string2xr_string(LPCSTR s) {return *shared_str(s ? s : "");}
IC	void		throw_and_log(const xr_string &s) {Msg("! %s",s.c_str()); throw *shared_str(s.c_str());}
#	define		THROW(xpr)				if (!(xpr)) {throw_and_log (__FILE__LINE__" Expression \""#xpr"\"");}
#	define		THROW2(xpr,msg0)		if (!(xpr)) {throw *shared_str(xr_string(__FILE__LINE__).append(" \"").append(#xpr).append(string2xr_string(msg0)).c_str());}
#	define		THROW3(xpr,msg0,msg1)	if (!(xpr)) {throw *shared_str(xr_string(__FILE__LINE__).append(" \"").append(#xpr).append(string2xr_string(msg0)).append(", ").append(string2xr_string(msg1)).c_str());}
#else
#	define		THROW					R_ASSERT
#	define		THROW2					R_ASSERT2
#	define		THROW3					R_ASSERT3
#endif

#ifdef XRGAME_EXPORTS
#include "../gamefont.h"
#include "../xr_object.h"
#include "../igame_level.h"
#include "xr_time.h"
#include "../../xrode/include/ode/common.h"
#include "../xrPhysics/xrPhysics.h"
#endif

#ifndef DEBUG
#	define MASTER_GOLD
#endif // DEBUG