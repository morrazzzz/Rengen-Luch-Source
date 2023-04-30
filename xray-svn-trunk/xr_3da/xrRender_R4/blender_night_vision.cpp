#include "stdafx.h"

#include "blender_night_vision.h"

CBlender_Night_Vision::CBlender_Night_Vision() { description.CLS = 0; }
CBlender_Night_Vision::~CBlender_Night_Vision() { }

void CBlender_Night_Vision::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("PP_transform", "nightvision_effect", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0);

		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}
}