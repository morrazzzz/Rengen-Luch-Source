#include "stdafx.h"

#include "blender_rain_drops.h"

CBlender_Rain_Drops::CBlender_Rain_Drops() { description.CLS = 0; }
CBlender_Rain_Drops::~CBlender_Rain_Drops() { }

void CBlender_Rain_Drops::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("PP_transform", "raindrops_effect", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0);

		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}
}