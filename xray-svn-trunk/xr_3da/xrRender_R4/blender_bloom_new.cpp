#include "stdafx.h"
#pragma hdrstop

#include "blender_bloom_new.h"

CBlender_bloom_new::CBlender_bloom_new() { description.CLS = 0; }
CBlender_bloom_new::~CBlender_bloom_new() {	}

void	CBlender_bloom_new::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	switch (C.iElement)
	{
	case 0:
		C.r_Pass("PP_transform", "Bloom_Bright", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0);

		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 1:
		C.r_Pass("PP_transform", "Bloom_Blur", FALSE, FALSE, FALSE);
		C.r_dx10Texture("SamplerBloom", r2_RT_treshold);

		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 2:
		C.r_Pass("PP_transform", "Bloom_Blur", FALSE, FALSE, FALSE);
		C.r_dx10Texture("SamplerBloom", r2_RT_texBloom2);

		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 3:
		C.r_Pass("PP_transform", "Bloom_Combine", FALSE, FALSE, FALSE);
		C.r_dx10Texture("SamplerBloom", r2_RT_treshold);
		C.r_dx10Texture("SamplerBloom_final", r2_RT_texBloom3);
		C.r_dx10Texture("s_image", r2_RT_generic0);

		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}
}

