#include"stdafx.h"
#include "../xrRender/xrRender_console.h"
#include "../CustomHUD.h"

void CRenderTarget::phase_visor_effect()
{
	int states = 0;
	
	float maskcondition = g_hud->hudGlassEffects_.GetMaskCondition();

	if(maskcondition <= 25.f)
		states = 3;
	else if(maskcondition <= 50.f)
		states = 2;
	else if(maskcondition <= 75.f)
		states = 1;
	else if(maskcondition <= 100.f)
		states = 0;


	RCache.set_c("visor_shadowing", ps_r3_hud_visor_shadowing);
	render_simple_quad(rt_Generic, s_visor->E[states],1);
	HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
}
