#include"stdafx.h"
#include "../xrRender/xrRender_console.h"
#include "../CustomHUD.h"

void CRenderTarget::phase_blood()
{
	RCache.set_c("bloodSplatsParams",
		(g_hud->hudGlassEffects_.GetActorMaxHealth() -
			g_hud->hudGlassEffects_.GetActorHealth()),
		0, 0, 0);
	render_simple_quad(rt_Generic, s_blood->E[0], 1);
	HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
}
