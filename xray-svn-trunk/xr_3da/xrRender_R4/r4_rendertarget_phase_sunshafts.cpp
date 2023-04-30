#include "stdafx.h"

void	CRenderTarget::phase_SunShafts()
{
	//return;
	float intensity = g_pGamePersistent->Environment().CurrentEnv->m_fSunShaftsIntensity;

	if (intensity < EPS_L)
		return;

	Fvector4 params = { 0,0,0,0 };
	params.x = ps_r2_prop_ss_sample_step_phase0;
	params.y = ps_r2_prop_ss_radius;

	// Mask
	render_simple_quad(rt_SunShaftsMask, s_SunShafts->E[0],1);

	// Smoothed mask
	render_simple_quad(rt_SunShaftsMaskSmoothed, s_SunShafts->E[1],1);

	// Pass 0
	RCache.set_c("SSParams", params);
	render_simple_quad(rt_SunShaftsPass0, s_SunShafts->E[2],1);

	params.x = ps_r2_prop_ss_sample_step_phase1;
	params.y = ps_r2_prop_ss_radius;

	// Pass 1
	RCache.set_c("SSParams", params);
	render_simple_quad(rt_SunShaftsMaskSmoothed, s_SunShafts->E[3],1);

	params.x = ps_r2_prop_ss_intensity;
	params.y = ps_r2_prop_ss_blend;

	// Combine
	RCache.set_c("SSParamsDISPLAY", params);
	render_simple_quad(rt_Generic, s_SunShafts->E[4],1);
	HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
}