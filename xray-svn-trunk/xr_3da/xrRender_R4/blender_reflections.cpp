#include "stdafx.h"

#include "blender_reflections.h"

CBlender_reflections::CBlender_reflections() { description.CLS = 0; }
CBlender_reflections::~CBlender_reflections() {	}

void	CBlender_reflections::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("PP_transform", "ogse_wet_reflections_nomsaa", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_normal", r2_RT_N);
		C.r_dx10Texture("s_image", r2_RT_generic0);

		C.r_dx10Texture("s_shader_params", r2_RT_shader_params);

		C.r_dx10Texture("s_env0", "$user$sky0");
		C.r_dx10Texture("s_env1", "$user$sky1");
		C.r_dx10Texture("s_water_wave", "water\\water_wave");
		
		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 1:
		C.r_Pass("PP_transform", "apply_refl", FALSE, FALSE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_dx10Texture("s_pp", r2_RT_reflections);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}
}

CBlender_reflections_MSAA::CBlender_reflections_MSAA() { description.CLS = 0; }
CBlender_reflections_MSAA::~CBlender_reflections_MSAA() { }

void CBlender_reflections_MSAA::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	if (Name)
		::Render->m_MSAASample = atoi(Definition);
	else
		::Render->m_MSAASample = -1;

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("PP_transform", "ogse_wet_reflections_msaa", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_normal", r2_RT_N);
		C.r_dx10Texture("s_image", r2_RT_generic0);

		C.r_dx10Texture("s_shader_params", r2_RT_shader_params);

		C.r_dx10Texture("s_env0", "$user$sky0");
		C.r_dx10Texture("s_env1", "$user$sky1");
		C.r_dx10Texture("s_water_wave", "water\\water_wave");

		C.r_dx10Sampler("smp_base");
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	case 1:
		C.r_Pass("PP_transform", "apply_refl", FALSE, FALSE, FALSE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
		C.r_dx10Texture("s_pp", r2_RT_reflections);
		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}

	::Render->m_MSAASample = -1;
}
