#include "stdafx.h"

void	CRenderTarget::phase_wet_reflections()
{
	if (!g_pGameLevel)			return;

	//return;
	float fRainFactor = g_pGamePersistent->Environment().CurrentEnv->rain_density;

	if (fRainFactor < EPS_L)
		return;

	render_simple_quad(rt_Reflections, s_reflections->E[0],1);

	render_simple_quad(rt_Generic, s_reflections->E[1],1);
	HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
}