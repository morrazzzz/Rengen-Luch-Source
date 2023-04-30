#include"stdafx.h"
#include "../xrRender/xrRender_console.h"
#include "../CustomHUD.h"

void CRenderTarget::phase_nightvision()
{
	RCache.set_c("nv_color", g_hud->nightVisionEffect_.nvColor_.x, g_hud->nightVisionEffect_.nvColor_.y, g_hud->nightVisionEffect_.nvColor_.z, 0);
	RCache.set_c("nv_various", g_hud->nightVisionEffect_.nvSaturation_, g_hud->nightVisionEffect_.nightVisionGoogleType_, g_hud->nightVisionEffect_.nvSaturationOut_, 0);
	RCache.set_c("nv_various_2", g_hud->nightVisionEffect_.borderShadowingStop_, g_hud->nightVisionEffect_.borderShadowingOut_, g_hud->nightVisionEffect_.monocularRadius_, g_hud->nightVisionEffect_.nvHasBrokenCamEffect_ ? 1.f : 0.f);
	RCache.set_c("nv_grain_conts", g_hud->nightVisionEffect_.nvGrainPower_, g_hud->nightVisionEffect_.nvGrainSize_, g_hud->nightVisionEffect_.nvGrainClrIntensity_, 0);

	render_simple_quad(rt_Generic, s_nightvision->E[0],1);
	HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
}