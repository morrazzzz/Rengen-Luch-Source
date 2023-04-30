#include "stdafx.h"

void CRenderTarget::phase_accumulator()
{
	// Targets
	if (!needClearAccumulation)
	{
		// normal operation - setup
		if (!RImplementation.o.dx10_msaa)
		{
			if (RImplementation.o.fp16_blend)
				u_setrt(rt_Accumulator, NULL, NULL, HW.pBaseZB);
			else
				u_setrt(rt_Accumulator_temp, NULL, NULL, HW.pBaseZB);
		}
		else
		{
			if (RImplementation.o.fp16_blend)
				u_setrt(rt_Accumulator, NULL, NULL, rt_MSAADepth->pZRT);
			else
				u_setrt(rt_Accumulator_temp, NULL, NULL, rt_MSAADepth->pZRT);
		}
	}
	else
	{
		// initial setup
		needClearAccumulation = false;

		// clear
		if (!RImplementation.o.dx10_msaa)
			u_setrt(rt_Accumulator, NULL, NULL, HW.pBaseZB);
		else
			u_setrt(rt_Accumulator, NULL, NULL, rt_MSAADepth->pZRT);

		reset_light_marker();

		//	Igor: AMD bug workaround. Should be fixed in 8.7 catalyst
		//	Need for MSAA to work correctly.
		if (RImplementation.o.dx10_msaa)
		{
			HW.pContext->OMSetRenderTargets(1, &(rt_Accumulator->pRT), 0);
		}

		FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		HW.pContext->ClearRenderTargetView(rt_Accumulator->pRT, ColorRGBA);

		// Stencil	- draw only where stencil >= 0x1
		RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
		RCache.set_CullMode(CULL_NONE);
		RCache.set_ColorWriteEnable();

	}

	//	Restore viewport after shadow map rendering
	RImplementation.rmNormal();
}

void	CRenderTarget::phase_vol_accumulator()
{
	if (!m_bHasActiveVolumetric)
	{
		m_bHasActiveVolumetric = true;

		if (!RImplementation.o.dx10_msaa)
			u_setrt(rt_Generic_2, NULL, NULL, HW.pBaseZB);
		else
			u_setrt(rt_Generic_2, NULL, NULL, RImplementation.Target->rt_MSAADepth->pZRT);

		FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		HW.pContext->ClearRenderTargetView(rt_Generic_2->pRT, ColorRGBA);
	}
	else
	{
		if (!RImplementation.o.dx10_msaa)
			u_setrt(rt_Generic_2, NULL, NULL, HW.pBaseZB);
		else
			u_setrt(rt_Generic_2, NULL, NULL, RImplementation.Target->rt_MSAADepth->pZRT);
	}

	RCache.set_Stencil(FALSE);
	RCache.set_CullMode(CULL_NONE);
	RCache.set_ColorWriteEnable();
}