#include "stdafx.h"

#include "blender_dof.h"

CBlender_DOF::CBlender_DOF() { description.CLS = 0; }
CBlender_DOF::~CBlender_DOF() { }

void CBlender_DOF::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	switch (C.iElement)
	{
	case 0:
		C.r_Pass("PP_transform", "depth_of_field", FALSE, FALSE, FALSE);
		C.r_dx10Texture("s_image", r2_RT_generic0);
		C.r_dx10Texture("s_position", r2_RT_P);

		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_nofilter");
		C.r_End();
		break;
	}
}