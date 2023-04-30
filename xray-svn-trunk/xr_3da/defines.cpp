#include "stdafx.h"

#ifdef DEBUG
	ECORE_API BOOL bDebug	= FALSE;
	
#endif

// Video
u32			psCurrentVidMode[2] = {1024,768};

Flags32		psDeviceFlags		= {rsDetails|mtPhysics|mtSound|mtNetwork|rsRefresh60hz|rsPrefObjects};

// textures 
int			psTextureLOD		= 1;
