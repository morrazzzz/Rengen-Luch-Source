#include"stdafx.h"
#include "../xrRender/xrRender_console.h"

void CRenderTarget::phase_dof()
{
	//Set up variable
	Fvector2 vDofKernel;
	Fvector3 dof;

	// skyloader: get value from weather or from console
	float kernel_size = ps_r2_dof_kernel_size;
	float dof_sky = ps_r2_dof_sky;
	Fvector3 dof_value = ps_r2_dof;

	if (ps_r2_ls_flags_ext.test(R2FLAGEXT_DOF_WEATHER))
	{
		kernel_size = g_pGamePersistent->Environment().CurrentEnv->dof_kernel;
		dof_sky = g_pGamePersistent->Environment().CurrentEnv->dof_sky;
		dof_value = g_pGamePersistent->Environment().CurrentEnv->dof_value;
	}

	g_pGamePersistent->SetBaseDof(dof_value);

	vDofKernel.set(0.5f / Device.dwWidth, 0.5f / Device.dwHeight);
	vDofKernel.mul(kernel_size);

	g_pGamePersistent->GetCurrentDof(dof);

	RCache.set_c("dof_params", dof.x, dof.y, dof.z, dof_sky);
	RCache.set_c("dof_kernel", vDofKernel.x, vDofKernel.y, kernel_size, 0);

	//combine
	render_simple_quad(rt_Generic, s_dof->E[0],1);
	HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
}
