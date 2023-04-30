#include "stdafx.h"

#include "blender_visor.h"

CBlender_Visor::CBlender_Visor() { description.CLS = 0; }
CBlender_Visor::~CBlender_Visor() { }

void CBlender_Visor::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("PP_transform", "visor_effect_texture", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0);
		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("SamplerBloom_final", r2_RT_texBloom3);

		C.r_dx10Texture("s_gasmask", "shaders\\Mask\\Good");
		C.r_dx10Texture("s_dirt", "shaders\\Mask\\lens_dirt");

		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_nofilter");
		C.r_End();
		break;
	case 1:
		C.r_Pass("PP_transform", "visor_effect_texture", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0);
		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("SamplerBloom_final", r2_RT_texBloom3);

		C.r_dx10Texture("s_gasmask", "shaders\\Mask\\Medium");
		C.r_dx10Texture("s_dirt", "shaders\\Mask\\lens_dirt");

		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_nofilter");
		C.r_End();
		break;
	case 2:
		C.r_Pass("PP_transform", "visor_effect_texture", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0);
		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("SamplerBloom_final", r2_RT_texBloom3);

		C.r_dx10Texture("s_gasmask", "shaders\\Mask\\Bad");
		C.r_dx10Texture("s_dirt", "shaders\\Mask\\lens_dirt");

		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_nofilter");
		C.r_End();
		break;
	case 3:
		C.r_Pass("PP_transform", "visor_effect_texture", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0);
		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("SamplerBloom_final", r2_RT_texBloom3);

		C.r_dx10Texture("s_gasmask", "shaders\\Mask\\Pizdec");
		C.r_dx10Texture("s_dirt", "shaders\\Mask\\lens_dirt");

		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_nofilter");
		C.r_End();
		break;
	}
}