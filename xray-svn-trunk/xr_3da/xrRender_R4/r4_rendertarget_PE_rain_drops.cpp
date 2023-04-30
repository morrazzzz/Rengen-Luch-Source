#include"stdafx.h"
#include "../xrRender/xrRender_console.h"
#include "../CustomHUD.h"

void CRenderTarget::phase_rain_drops()
{
	//return;
	float fRainFactor = g_pGamePersistent->Environment().CurrentEnv->rain_density;

	if (fRainFactor < EPS_L)
		return;

	RCache.set_c("rain_drops_and_visor", g_hud->hudGlassEffects_.GetActorHudWetness1());
	render_simple_quad(rt_Generic, s_rain_drops->E[0],1);
	HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
}
