#include "stdafx.h"
#include "../igame_persistent.h"
#include "../environment.h"
#include "../xrRender/dxEnvironmentRender.h"
#include "../CustomHUD.h"

float hclip(float v, float dim) { return 2.f * v / dim - 1.f; }

void CRenderTarget::phase_combine()
{
	PIX_EVENT(phase_combine);

	//	TODO: DX10: Remove half poxel offset
	bool _menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

	u32 Offset = 0;
	Fvector2 p0, p1;

	//*** exposure-pipeline
	u32 gpu_id = CurrentFrame() % HW.Caps.iGPUNum;
	if (RImplementation.currentViewPort == SECONDARY_WEAPON_SCOPE) //--#SM+#-- +SecondVP+
	{
		gpu_id = (CurrentFrame() - 1) % HW.Caps.iGPUNum;	// Фикс "мерцания" tonemapping (HDR) после выключения двойного рендера. 
															// Побочный эффект - при работе двойного рендера скорость изменения tonemapping (HDR) падает в два раза
															// Мерцание связано с тем, что HDR для своей работы хранит уменьшенние копии "прошлых кадров"
															// Эти кадры относительно похожи друг на друга, однако при включенном двойном рендере
															// в половине кадров оказывается картинка из второго рендера, и поскольку она часто может отличатся по цвету\яркости
															// то при попытке создания "плавного" перехода между ними получается эффект мерцания
	}
	{
		t_LUM_src->surface_set(rt_LUM_pool[gpu_id * 2 + 0]->pSurface);
		t_LUM_dest->surface_set(rt_LUM_pool[gpu_id * 2 + 1]->pSurface);
	}

	if (RImplementation.o.ssao_hdao && RImplementation.o.ssao_ultra)
	{
		if (ps_r_ssao > 0)
		{
			phase_hdao();
		}
	}
	else
	{
		if (RImplementation.o.ssao_opt_data)
		{
			phase_downsamp();
			//phase_ssao();
		}
		else if (RImplementation.o.ssao_blur_on)
			phase_ssao();
	}

	FLOAT color_RGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// low/hi RTs
	if (!RImplementation.o.dx10_msaa)
	{
		HW.pContext->ClearRenderTargetView(rt_Generic_0->pRT, color_RGBA);
		HW.pContext->ClearRenderTargetView(rt_Generic_1->pRT, color_RGBA);

		u_setrt(rt_Generic_0, rt_Generic_1, 0, HW.pBaseZB);
	}
	else
	{
		HW.pContext->ClearRenderTargetView(rt_Generic_0_r->pRT, color_RGBA);
		HW.pContext->ClearRenderTargetView(rt_Generic_1_r->pRT, color_RGBA);

		u_setrt(rt_Generic_0_r, rt_Generic_1_r, 0, RImplementation.Target->rt_MSAADepth->pZRT);
	}

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	// draw skybox
	if (1)
	{
		//	Moved to shader!
		RCache.set_ColorWriteEnable();

		//	Moved to shader!
		RCache.set_Z(FALSE);
		g_pGamePersistent->Environment().RenderSky();

		//	Igor: Render clouds before compine without Z-test
		//	to avoid siluets. HOwever, it's a bit slower process.
		g_pGamePersistent->Environment().RenderClouds();

		//	Moved to shader!
		RCache.set_Z(TRUE);
	}

	RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00); // stencil should be >= 1

	if (RImplementation.o.nvstencil)
	{
		u_stencil_optimize(CRenderTarget::SO_Combine);
		RCache.set_ColorWriteEnable();
	}

	// Draw full-screen quad textured with our scene image
	if (!_menu_pp)
	{
		PIX_EVENT(combine_1);

		// Compute params
		Fmatrix m_v2w;
		m_v2w.invert(Device.mView);

		CEnvDescriptorMixer& envdesc = *g_pGamePersistent->Environment().CurrentEnv;

		const float minamb = 0.001f;

		Fvector4 ambclr = { _max(envdesc.ambient.x * 2, minamb), _max(envdesc.ambient.y * 2, minamb), _max(envdesc.ambient.z * 2, minamb), 0 };

		ambclr.mul(ps_r2_sun_lumscale_amb);

		Fvector4 envclr = { envdesc.hemi_color.x * 2 + EPS, envdesc.hemi_color.y * 2 + EPS, envdesc.hemi_color.z * 2 + EPS, envdesc.weight };

		Fvector4 fogclr = { envdesc.fog_color.x, envdesc.fog_color.y, envdesc.fog_color.z, 0 };

		envclr.x *= 2 * ps_r2_sun_lumscale_hemi;
		envclr.y *= 2 * ps_r2_sun_lumscale_hemi;
		envclr.z *= 2 * ps_r2_sun_lumscale_hemi;

		Fvector4 sunclr, sundir;

		float fSSAONoise = 2.0f;
		fSSAONoise *= tan(deg2rad(67.5f / 2.0f));
		fSSAONoise /= tan(deg2rad(Device.fFOV / 2.0f));

		float fSSAOKernelSize = 150.0f;
		fSSAOKernelSize *= tan(deg2rad(67.5f / 2.0f));
		fSSAOKernelSize /= tan(deg2rad(Device.fFOV / 2.0f));

		// sun-params
		{
			CLightSource* fuckingsun = (CLightSource*)RImplementation.Lights.sun_adapted._get();
			Fvector L_dir, L_clr;
			float L_spec;

			L_clr.set(fuckingsun->color.r, fuckingsun->color.g, fuckingsun->color.b);
			L_spec = u_diffuse2s(L_clr);

			Device.mView.transform_dir(L_dir, fuckingsun->direction);

			L_dir.normalize();

			sunclr.set(L_clr.x, L_clr.y, L_clr.z, L_spec);
			sundir.set(L_dir.x, L_dir.y, L_dir.z, 0);
		}

		// Fill VB
		float scale_X = float(Device.dwWidth) / float(TEX_jitter);
		float scale_Y = float(Device.dwHeight) / float(TEX_jitter);

		// Fill vertex buffer
		FVF::TL* pv = (FVF::TL*)	RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);

		pv->set(-1, 1, 0, 1, 0, 0, scale_Y);	pv++;
		pv->set(-1, -1, 0, 0, 0, 0, 0);	pv++;
		pv->set(1, 1, 1, 1, 0, scale_X, scale_Y);	pv++;
		pv->set(1, -1, 1, 0, 0, scale_X, 0);	pv++;

		RCache.Vertex.Unlock(4, g_combine->vb_stride);

		dxEnvDescriptorMixerRender &envdescren = *(dxEnvDescriptorMixerRender*)(&*envdesc.m_pDescriptorMixer);

		// Setup textures
		ID3DBaseTexture* e0 = _menu_pp ? 0 : envdescren.sky_r_textures_env[0].second->surface_get();
		ID3DBaseTexture* e1 = _menu_pp ? 0 : envdescren.sky_r_textures_env[1].second->surface_get();

		t_envmap_0->surface_set(e0);
		_RELEASE(e0);

		t_envmap_1->surface_set(e1);
		_RELEASE(e1);

		// Draw
		RCache.set_Element(s_combine->E[0]);
		RCache.set_Geometry(g_combine);

		RCache.set_c("m_v2w", m_v2w);
		RCache.set_c("L_ambient", ambclr);

		RCache.set_c("Ldynamic_color", sunclr);
		RCache.set_c("Ldynamic_dir", sundir);

		RCache.set_c("env_color", envclr);
		RCache.set_c("fog_color", fogclr);

		RCache.set_c("ssao_noise_tile_factor", fSSAONoise);
		RCache.set_c("ssao_kernel_size", fSSAOKernelSize);

		if (!RImplementation.o.dx10_msaa)
			RCache.BackendRender(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
		else
		{
			RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0);

			RCache.BackendRender(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

			if (RImplementation.o.dx10_msaa_opt)
			{
				RCache.set_Element(s_combine_msaa[0]->E[0]);
				RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);

				RCache.BackendRender(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
			}
			else
			{
				for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
				{
					RCache.set_Element(s_combine_msaa[i]->E[0]);
					StateManager.SetSampleMask(u32(1) << i);
					RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0);

					RCache.BackendRender(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
				}

				StateManager.SetSampleMask(0xffffffff);
			}

			RCache.set_Stencil(FALSE, D3DCMP_EQUAL, 0x01, 0xff, 0);
		}
	}

	// Forward rendering
	{
		PIX_EVENT(Forward_rendering);

		if (!RImplementation.o.dx10_msaa)
			u_setrt(rt_Generic_0, 0, 0, HW.pBaseZB);		// LDR RT
		else
			u_setrt(rt_Generic_0_r, 0, 0, RImplementation.Target->rt_MSAADepth->pZRT);		// LDR RT

		RCache.set_CullMode(CULL_CCW);
		RCache.set_Stencil(FALSE);
		RCache.set_ColorWriteEnable();
		//	TODO: DX10: CHeck this!
		//g_pGamePersistent->Environment().RenderClouds	();
		RImplementation.render_forward();

		if (g_pGamePersistent)
			g_pGamePersistent->OnRenderPPUI_main();	// PP-UI
	}

	//	Igor: for volumetric lights
	//	combine light volume here
	if (m_bHasActiveVolumetric)
		phase_combine_volumetric();

	// Perform blooming filter and distortion if needed
	RCache.set_Stencil(FALSE);

	if (RImplementation.o.dx10_msaa)
	{
	//	phase_msaa_to_rt();
		// we need to resolve rt_Generic_1 into rt_Generic_1_r
		HW.pContext->ResolveSubresource(rt_Generic_1->pTexture->surface_get(), 0, rt_Generic_1_r->pTexture->surface_get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		HW.pContext->ResolveSubresource(rt_Generic_0->pTexture->surface_get(), 0, rt_Generic_0_r->pTexture->surface_get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	}

	//tonemapping
	 phase_luminance();

	// Distortion filter
	BOOL bDistort = RImplementation.o.distortion_enabled; // This can be modified
	{
		if ((0 == RImplementation.mainRenderPrior1DsBuffer_->mapDistort.size()) && !_menu_pp)
			bDistort = FALSE;

		if (bDistort)
		{
			PIX_EVENT(render_distort_objects);
			FLOAT color_RGBA_2[4] = { 127.0f / 255.0f, 127.0f / 255.0f, 0.0f, 127.0f / 255.0f };

			if (!RImplementation.o.dx10_msaa)
			{
				u_setrt(rt_Generic_1, 0, 0, HW.pBaseZB);		// Now RT is a distortion mask
				HW.pContext->ClearRenderTargetView(rt_Generic_1->pRT, color_RGBA_2);
			}
			else
			{
				u_setrt(rt_Generic_1_r, 0, 0, RImplementation.Target->rt_MSAADepth->pZRT);		// Now RT is a distortion mask
				HW.pContext->ClearRenderTargetView(rt_Generic_1_r->pRT, color_RGBA_2);
			}

			RCache.set_CullMode(CULL_CCW);
			RCache.set_Stencil(FALSE);
			RCache.set_ColorWriteEnable();

			RImplementation.r_dsgraph_render_distort(*RImplementation.mainRenderPrior1DsBuffer_);

			if (g_pGamePersistent)
				g_pGamePersistent->OnRenderPPUI_PP();	// PP-UI
		}
	}

	RCache.set_Stencil(FALSE);

	if (ps_r3_use_new_bloom) // maybe later make a choose between GSC and New Bloom?. rafa: eats GPU - lets keep the switch available 
		phase_bloom_new();

	if (ps_r3_use_wet_reflections)
		phase_wet_reflections();

	// FXAA
	if (ps_r2_ls_flags_ext.test(R2FLAGEXT_AA_FXAA))
	{
		PIX_EVENT(FXAA);

		phase_fxaa();
		RCache.set_Stencil(FALSE);
	}

	if (ps_r2_ls_flags_ext.test(R2FLAGEXT_AA_SMAA))
	{
		PIX_EVENT(SMAA);
		//big thanks to RainbowZerg and whole Oxy Team
		phase_smaa();
		RCache.set_Stencil(FALSE);
	}

	if (RImplementation.currentViewPort == MAIN_VIEWPORT) // Wtf Lavcevrot?
	{
		if (ps_r3_use_ss_sunshafts)
			phase_SunShafts();

		if (ps_r2_ls_flags.test(R2FLAG_MBLUR))
			phase_mblur();

		if (ps_r2_ls_flags.test(R2FLAG_DOF))
			phase_dof();
	}

	{
		PIX_EVENT(PostEffects);

		bool hud_effects = g_hud && g_hud->showGameHudEffects_;

		if (hud_effects)
		{
			if (ps_r3_hud_visor_effect && g_hud->hudGlassEffects_.GetCastHudGlassEffects() && !g_hud->nightVisionEffect_.needNVShading_ && RImplementation.currentViewPort == MAIN_VIEWPORT)
				phase_visor_effect();

			if (ps_r3_hud_rain_drops && g_hud->hudGlassEffects_.GetCastHudGlassEffects() && !g_hud->nightVisionEffect_.needNVShading_ && RImplementation.currentViewPort == MAIN_VIEWPORT)
				phase_rain_drops();

			if (g_hud->nightVisionEffect_.needNVShading_)
				phase_nightvision();

			if (RImplementation.currentViewPort == MAIN_VIEWPORT)
				phase_blood();
		}
	}

	// PP enabled ?
	//	Render to RT texture to be able to copy RT even in windowed mode.
	BOOL PP_Complex = u_need_PP() | (BOOL)RImplementation.m_bMakeAsyncSS;

	if (_menu_pp)
		PP_Complex = FALSE;

	// HOLGER - HACK
	PP_Complex = TRUE;

	// Combine everything + perform AA
	if (RImplementation.o.dx10_msaa)
	{
		if (PP_Complex)
			u_setrt(rt_Generic, 0, 0, HW.pBaseZB);			// LDR RT
		else
			u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, HW.pBaseZB);
	}
	else
	{
		if (PP_Complex)
			u_setrt(rt_Color, 0, 0, HW.pBaseZB);			// LDR RT
		else
			u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, HW.pBaseZB);
	}

	RCache.set_CullMode(CULL_NONE);
	RCache.set_Stencil(FALSE);

	if (1)
	{
		PIX_EVENT(combine_2);

		struct v_aa
		{
			Fvector4	p;
			Fvector2	uv0;
			Fvector2	uv1;
			Fvector2	uv2;
			Fvector2	uv3;
			Fvector2	uv4;
			Fvector4	uv5;
			Fvector4	uv6;
		};

		float _w = float(Device.dwWidth);
		float _h = float(Device.dwHeight);
		float ddw = 1.f / _w;
		float ddh = 1.f / _h;

		p0.set(.5f / _w, .5f / _h);
		p1.set((_w + .5f) / _w, (_h + .5f) / _h);

		// Fill vertex buffer
		v_aa* pv = (v_aa*)RCache.Vertex.Lock(4, g_aa_AA->vb_stride, Offset);

		pv->p.set(EPS, float(_h + EPS), EPS, 1.f);
		pv->uv0.set(p0.x, p1.y);
		pv->uv1.set(p0.x - ddw, p1.y - ddh);
		pv->uv2.set(p0.x + ddw, p1.y + ddh);
		pv->uv3.set(p0.x + ddw, p1.y - ddh);
		pv->uv4.set(p0.x - ddw, p1.y + ddh);
		pv->uv5.set(p0.x - ddw, p1.y, p1.y, p0.x + ddw);
		pv->uv6.set(p0.x, p1.y - ddh, p1.y + ddh, p0.x);
		pv++;

		pv->p.set(EPS, EPS, EPS, 1.f);
		pv->uv0.set(p0.x, p0.y);
		pv->uv1.set(p0.x - ddw, p0.y - ddh);
		pv->uv2.set(p0.x + ddw, p0.y + ddh);
		pv->uv3.set(p0.x + ddw, p0.y - ddh);
		pv->uv4.set(p0.x - ddw, p0.y + ddh);
		pv->uv5.set(p0.x - ddw, p0.y, p0.y, p0.x + ddw);
		pv->uv6.set(p0.x, p0.y - ddh, p0.y + ddh, p0.x);
		pv++;

		pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f);
		pv->uv0.set(p1.x, p1.y);
		pv->uv1.set(p1.x - ddw, p1.y - ddh);
		pv->uv2.set(p1.x + ddw, p1.y + ddh);
		pv->uv3.set(p1.x + ddw, p1.y - ddh);
		pv->uv4.set(p1.x - ddw, p1.y + ddh);
		pv->uv5.set(p1.x - ddw, p1.y, p1.y, p1.x + ddw);
		pv->uv6.set(p1.x, p1.y - ddh, p1.y + ddh, p1.x);
		pv++;

		pv->p.set(float(_w + EPS), EPS, EPS, 1.f);
		pv->uv0.set(p1.x, p0.y);
		pv->uv1.set(p1.x - ddw, p0.y - ddh);
		pv->uv2.set(p1.x + ddw, p0.y + ddh);
		pv->uv3.set(p1.x + ddw, p0.y - ddh);
		pv->uv4.set(p1.x - ddw, p0.y + ddh);
		pv->uv5.set(p1.x - ddw, p0.y, p0.y, p1.x + ddw);
		pv->uv6.set(p1.x, p0.y - ddh, p0.y + ddh, p1.x);
		pv++;

		RCache.Vertex.Unlock(4, g_aa_AA->vb_stride);

		// Draw COLOR
		if (!RImplementation.o.dx10_msaa)
		{
			if (ps_r2_ls_flags.test(R2FLAG_AA))
				RCache.set_Element(s_combine->E[bDistort ? 3 : 1]);	// look at blender_combine.cpp
			else
				RCache.set_Element(s_combine->E[bDistort ? 4 : 2]);	// look at blender_combine.cpp
		}
		else
		{
			if (ps_r2_ls_flags.test(R2FLAG_AA))
				RCache.set_Element(s_combine_msaa[0]->E[bDistort ? 3 : 1]);	// look at blender_combine.cpp
			else
				RCache.set_Element(s_combine_msaa[0]->E[bDistort ? 4 : 2]);	// look at blender_combine.cpp
		}

		RCache.set_c("e_barrier", ps_r2_aa_barier.x, ps_r2_aa_barier.y, ps_r2_aa_barier.z, 0);
		RCache.set_c("e_weights", ps_r2_aa_weight.x, ps_r2_aa_weight.y, ps_r2_aa_weight.z, 0);
		RCache.set_c("e_kernel", ps_r2_aa_kernel, ps_r2_aa_kernel, ps_r2_aa_kernel, 0);

		RCache.set_Geometry(g_aa_AA);

		RCache.BackendRender(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	RCache.set_Stencil(FALSE);

	if (ps_r2_ls_flags_ext.test(R2FLAGEXT_SUN_FLARES))
		g_pGamePersistent->Environment().RenderFlares(); // lens-flares

	//	PP-if required
	if (PP_Complex)
	{
		PIX_EVENT(phase_pp);
		phase_pp();
	}

	//	Re-adapt luminance
	RCache.set_Stencil(FALSE);

	//*** exposure-pipeline-clear
	{
		std::swap(rt_LUM_pool[gpu_id * 2 + 0], rt_LUM_pool[gpu_id * 2 + 1]);
		t_LUM_src->surface_set(NULL);
		t_LUM_dest->surface_set(NULL);
	}

#ifdef DEBUG
	RCache.set_CullMode(CULL_CCW);
	static	xr_vector<Fplane>		saved_dbg_planes;
	if (bDebug)		saved_dbg_planes = dbg_planes;
	else			dbg_planes = saved_dbg_planes;
	if (1) for (u32 it = 0; it < dbg_planes.size(); it++)
	{
		Fplane&		P = dbg_planes[it];
		Fvector		zero;
		zero.mul(P.n, P.d);

		Fvector             L_dir, L_up = P.n, L_right;
		L_dir.set(0, 0, 1);                if (_abs(L_up.dotproduct(L_dir)) > .99f)  L_dir.set(1, 0, 0);
		L_right.crossproduct(L_up, L_dir);           L_right.normalize();
		L_dir.crossproduct(L_right, L_up);         L_dir.normalize();

		Fvector				pp0, pp1, pp2, pp3;
		float				sz = 100.f;
		pp0.mad(zero, L_right, sz).mad(L_dir, sz);
		pp1.mad(zero, L_right, sz).mad(L_dir, -sz);
		pp2.mad(zero, L_right, -sz).mad(L_dir, -sz);
		pp3.mad(zero, L_right, -sz).mad(L_dir, +sz);
		RCache.dbg_DrawTRI(Fidentity, pp0, pp1, pp2, 0xffffffff);
		RCache.dbg_DrawTRI(Fidentity, pp2, pp3, pp0, 0xffffffff);
	}

	static	xr_vector<dbg_line_t>	saved_dbg_lines;
	if (bDebug)		saved_dbg_lines = dbg_lines;
	else			dbg_lines = saved_dbg_lines;
	if (1) for (u32 it = 0; it < dbg_lines.size(); it++)
	{
		RCache.dbg_DrawLINE(Fidentity, dbg_lines[it].P0, dbg_lines[it].P1, dbg_lines[it].color);
	}
#endif

#ifdef DEBUG
	dbg_spheres.clear();
	dbg_lines.clear();
	dbg_planes.clear();
#endif
	}

void CRenderTarget::phase_wallmarks()
{
	// Targets
	RCache.set_RT(NULL, 2);
	RCache.set_RT(NULL, 1);

	if (!RImplementation.o.dx10_msaa)
		u_setrt(rt_Color, NULL, NULL, HW.pBaseZB);
	else
		u_setrt(rt_Color, NULL, NULL, rt_MSAADepth->pZRT);

	// Stencil	- draw only where stencil >= 0x1
	RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
	RCache.set_CullMode(CULL_CCW);
	RCache.set_ColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
}

void CRenderTarget::phase_combine_volumetric()
{
	PIX_EVENT(phase_combine_volumetric);

	u32 Offset = 0;

	if (!RImplementation.o.dx10_msaa)
		u_setrt(rt_Generic_0, rt_Generic_1, 0, HW.pBaseZB);
	else
		u_setrt(rt_Generic_0_r, rt_Generic_1_r, 0, RImplementation.Target->rt_MSAADepth->pZRT);

	//	Sets limits to both render targets
	RCache.set_ColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);

	{
		// Fill VB
		float scale_X = float(Device.dwWidth) / float(TEX_jitter);
		float scale_Y = float(Device.dwHeight) / float(TEX_jitter);

		// Fill vertex buffer
		FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);

		pv->set(-1, 1, 0, 1, 0, 0, scale_Y);	pv++;
		pv->set(-1, -1, 0, 0, 0, 0, 0);	pv++;
		pv->set(1, 1, 1, 1, 0, scale_X, scale_Y);	pv++;
		pv->set(1, -1, 1, 0, 0, scale_X, 0);	pv++;

		RCache.Vertex.Unlock(4, g_combine->vb_stride);

		// Draw
		RCache.set_Element(s_combine_volumetric->E[0]);
		RCache.set_Geometry(g_combine);

		RCache.BackendRender(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	RCache.set_ColorWriteEnable();
}
