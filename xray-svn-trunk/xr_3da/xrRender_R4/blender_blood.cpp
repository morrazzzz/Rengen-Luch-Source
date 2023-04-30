#include "stdafx.h"
#pragma hdrstop

#include "blender_blood.h"

CBlender_Blood::CBlender_Blood() { description.CLS = 0; }
CBlender_Blood::~CBlender_Blood() {}

void CBlender_Blood::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	switch (C.iElement)
	{
	case 0:
		C.r_Pass("PP_transform", "blood", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0); 
		C.r_dx10Texture("blood_sampler", "shaders\\blood");

		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_linear");
		C.r_End();
		break;
	}
}
