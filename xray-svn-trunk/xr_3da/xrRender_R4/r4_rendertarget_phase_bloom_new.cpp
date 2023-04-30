#include"stdafx.h"
#include "../xrRender/xrRender_console.h"

void CRenderTarget::phase_bloom_new()
{
	// constants
	Fvector4 params = { 0,0,0,0 };
	params.x = ps_r2_bloom_treshold;

	render_simple_quad(rt_bloom_treshold, s_bloom_new->E[0], 0.5f);
	RCache.set_c("bloom_params", params);

	render_simple_quad(rt_bloom_blur, s_bloom_new->E[1], 0.25f);
	RCache.set_c("direction", 0, 1, 0, 0);

	render_simple_quad(rt_bloom_final, s_bloom_new->E[2], 0.0625f);
	RCache.set_c("direction", 1, 0, 0, 0);

	//combine
	render_simple_quad(rt_Generic, s_bloom_new->E[3], 1);
	HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());

}
