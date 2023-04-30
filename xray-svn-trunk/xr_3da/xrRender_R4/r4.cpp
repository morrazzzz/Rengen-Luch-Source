#include "stdafx.h"
#include "r4.h"
#include "../xrRender/fbasicvisual.h"
#include "../xr_object.h"
#include "../CustomHUD.h"
#include "../igame_persistent.h"
#include "../environment.h"
#include "../xrRender/SkeletonCustom.h"
#include "../xrRender/LightTrack.h"
#include "../xrRender/dxRenderDeviceRender.h"
#include "../xrRender/dxWallMarkArray.h"
#include "../xrRender/dxUIShader.h"

#include "../xrRenderDX10/3DFluid/dx103DFluidManager.h"
#include "../xrRender/ShaderResourceTraits.h"
auto _printf = printf;
CRender RImplementation;

float r_dtex_range = 50.f;

ShaderElement* CRender::rimp_select_sh_dynamic(dxRender_Visual *pVisual, float cdist_sq, DsGraphBuffer& dsbuffer)
{
	int id = SE_R2_SHADOW;

	if (dsbuffer.phaseType_ == CRender::PHASE_NORMAL)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R) < r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
	}

	return pVisual->shader->E[id]._get();
}

ShaderElement* CRender::rimp_select_sh_static(dxRender_Visual *pVisual, float cdist_sq, DsGraphBuffer& dsbuffer)
{
	int id = SE_R2_SHADOW;

	if (dsbuffer.phaseType_ == CRender::PHASE_NORMAL)
	{
		id = ((_sqrt(cdist_sq)-pVisual->vis.sphere.R) < r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
	}

	return pVisual->shader->E[id]._get();
}

static class cl_parallax : public R_constant_setup
{
	virtual void setup	(R_constant* C)
	{
		float			h = ps_r2_df_parallax_h;
		RCache.set_c	(C, h, -h / 2.f, 1.f / r_dtex_range, 1.f / r_dtex_range);
	}

}	binder_parallax;

static class cl_steep_parallax : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		float			p1 = ps_r2_steep_parallax_h;
		float			p2 = ps_r2_steep_parallax_distance;
		float			p3 = ps_r2_steep_parallax_samples;
		float			p4 = ps_r2_steep_parallax_samples_min;
		RCache.set_c(C, p1, p2, p3, p4);
	}

}	binder_steep_parallax;

static class cl_LOD : public R_constant_setup
{
	virtual void setup	(R_constant* C)
	{
		RCache.LOD.set_LOD(C);
	}

}	binder_LOD;

static class cl_pos_decompress_params : public R_constant_setup
{
	virtual void setup	(R_constant* C)
	{
		float VertTan =  -1.0f * tanf(deg2rad(Device.fFOV / 2.0f));
		float HorzTan =  - VertTan / Device.fASPECT;

		RCache.set_c	(C, HorzTan, VertTan, (2.0f * HorzTan) / (float)Device.dwWidth, (2.0f * VertTan) / (float)Device.dwHeight);
	}

}	binder_pos_decompress_params;

static class cl_water_intensity : public R_constant_setup		
{	
	virtual void setup(R_constant* C)
	{
		if (g_pGamePersistent)
		{
			CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
			float fValue = E.m_fWaterIntensity;
			RCache.set_c(C, fValue, fValue, fValue, 0);
		}
		else
			RCache.set_c(C, 0, 0, 0, 0);
	}

}	binder_water_intensity;

static class cl_water_refl_params : public R_constant_setup
{	
	virtual void setup	(R_constant* C)
	{
		RCache.set_c(C, 0, (float)ps_r3_water_refl_obj, (float)ps_r3_water_refl_moon, 0);
	}

}	binder_water_refl;

static class cl_water_refl_params2 : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, ps_r3_water_refl_env_power, ps_r3_water_refl_intensity, ps_r3_water_refl_range, ps_r3_water_refl_obj_itter);
	}

}	binder_water_refl2;


static class cl_rain_density : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		if (g_pGamePersistent)
		{
			CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
			float fValue = E.rain_density;
			RCache.set_c(C, fValue, fValue, fValue, 0);
		}
		else
			RCache.set_c(C, 0, 0, 0, 0);
	}

}	binder_rain_density;

static class cl_other_consts_2 : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, (float)ps_r2_tonemap_wcorr, ps_r2_ls_flags_ext.test(R2FLAGEXT_GLOSS_BUILD_2218) ? 1.f : 0.f, 0, 0);
	}

}	binder_other_consts_2;


static class cl_sun_shafts_intensity : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		if (g_pGamePersistent)
		{
			CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
			float fValue = E.m_fSunShaftsIntensity;
			RCache.set_c(C, fValue, fValue, fValue, 0);
		}
		else
			RCache.set_c(C, 0, 0, 0, 0);
	}

}	binder_sun_shafts_intensity;

static class cl_alpha_ref : public R_constant_setup 
{	
	virtual void setup (R_constant* C) 
	{ 
		StateManager.BindAlphaRef(C);
	}

}	binder_alpha_ref;

static class cl__test_floats : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, ps_r__test_exp_to_shaders_1, ps_r__test_exp_to_shaders_2, ps_r__test_exp_to_shaders_3, ps_r__test_exp_to_shaders_4);
	}

}	binder_test_floats;

static class cl_tonemap_misc : public R_constant_setup
{
	virtual void setup(R_constant* C)
	{
		RCache.set_c(C, ps_r2_tonemap_white, 0, 0, 0);
	}

}	tone_map_misc;

//////////////////////////////////////////////////////////////////////////
// Just two static storage

void CRender::create()
{
	FlushLog(false);

	CTimer T; T.Start();

	Msg("* Setting up render...");

	Device.seqFrame.Add	(this,REG_PRIORITY_HIGH + 10000);
	Device.seqFrameEnd.Add(this, REG_PRIORITY_NORMAL + 1);

	m_skinning			= -1;
	m_MSAASample		= -1;

	// hardware
	o.smapsize			= ps_r_smap_size;
	o.mrt				= (HW.Caps.raster.dwMRT_count >= 3);
	o.mrtmixdepth		= (HW.Caps.raster.b_MRT_mixdepth);

	// Check for NULL render target support
	//	DX10 disabled
	//D3DFORMAT	nullrt	= (D3DFORMAT)MAKEFOURCC('N','U','L','L');
	//o.nullrt			= HW.support	(nullrt,			D3DRTYPE_SURFACE, D3DUSAGE_RENDERTARGET);
	o.nullrt = false;
	/*
	if (o.nullrt)		{
	Msg				("* NULLRT supported and used");
	};
	*/

	if (o.nullrt)
	{
		Msg("* NULLRT supported");

		//.	    _tzset			();
		//.		??? _strdate	( date, 128 );	???
		//.		??? if (date < 22-march-07)

		if (0)
		{
			u32 device_id	= HW.Caps.id_device;
			bool disable_nullrt = false;
			switch (device_id)	
			{
			case 0x190:
			case 0x191:
			case 0x192:
			case 0x193:
			case 0x194:
			case 0x197:
			case 0x19D:
			case 0x19E:{
				disable_nullrt = true;	//G80
				break;
			}
			case 0x400:
			case 0x401:
			case 0x402:
			case 0x403:
			case 0x404:
			case 0x405:
			case 0x40E:
			case 0x40F:{
				disable_nullrt = true;	//G84
				break;
			}
			case 0x420:
			case 0x421:
			case 0x422:
			case 0x423:
			case 0x424:
			case 0x42D:
			case 0x42E:
			case 0x42F:{
				disable_nullrt = true;	// G86
				break;
			}
			}
			if (disable_nullrt)
				o.nullrt=false;
		};

		if (o.nullrt)
			Msg("* ...and used");
	};


	// SMAP / DST
	o.HW_smap_FETCH4	= FALSE;
	//	DX10 disabled
	//o.HW_smap			= HW.support	(D3DFMT_D24X8,			D3DRTYPE_TEXTURE,D3DUSAGE_DEPTHSTENCIL);
	o.HW_smap			= true;
	o.HW_smap_PCF		= o.HW_smap;

	if (o.HW_smap)		
	{
		//	For ATI it's much faster on DX10 to use D32F format
		if (HW.Caps.id_vendor == 0x1002)
			o.HW_smap_FORMAT	= D3DFMT_D32F_LOCKABLE;
		else
			o.HW_smap_FORMAT	= D3DFMT_D24X8;

		Msg("* HWDST/PCF supported and used");
	}

	//	DX10 disabled
	//o.fp16_filter		= HW.support	(D3DFMT_A16B16G16R16F,	D3DRTYPE_TEXTURE,D3DUSAGE_QUERY_FILTER);
	//o.fp16_blend		= HW.support	(D3DFMT_A16B16G16R16F,	D3DRTYPE_TEXTURE,D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING);
	o.fp16_filter		= true;
	o.fp16_blend		= true;

	// search for ATI formats
	if (!o.HW_smap && (0==strstr(Core.Params,"-nodf24")))
	{
		o.HW_smap = HW.support ((D3DFORMAT)(MAKEFOURCC('D','F','2','4')), D3DRTYPE_TEXTURE, D3DUSAGE_DEPTHSTENCIL);

		if (o.HW_smap)
		{
			o.HW_smap_FORMAT = MAKEFOURCC('D','F','2','4');
			o.HW_smap_PCF	 = FALSE;
			o.HW_smap_FETCH4 = TRUE;
		}

		Msg("* DF24/F4 supported and used [%X]", o.HW_smap_FORMAT);
	}

	// emulate ATI-R4xx series
	if (strstr(Core.Params,"-r4xx"))
	{
		o.mrtmixdepth	= FALSE;
		o.HW_smap		= FALSE;
		o.HW_smap_PCF	= FALSE;
		o.fp16_filter	= FALSE;
		o.fp16_blend	= FALSE;
	}

	VERIFY2(o.mrt && (HW.Caps.raster.dwInstructions >= 256), "Hardware doesn't meet minimum feature-level");

	if (o.mrtmixdepth)
		o.albedo_wo		= FALSE;
	else if (o.fp16_blend)
		o.albedo_wo		= FALSE;
	else
		o.albedo_wo		= TRUE;

	// nvstencil on NV40 and up
	o.nvstencil = FALSE;
	//if ((HW.Caps.id_vendor==0x10DE)&&(HW.Caps.id_device>=0x40))	o.nvstencil = TRUE;
	if (strstr(Core.Params,"-nonvs"))
		o.nvstencil	= FALSE;

	// nv-dbt
	//	DX10 disabled
	//o.nvdbt				= HW.support	((D3DFORMAT)MAKEFOURCC('N','V','D','B'), D3DRTYPE_SURFACE, 0);

	o.nvdbt				= false;

	if (o.nvdbt)
		Msg	("* NV-DBT supported and used");

	// options (smap-pool-size)
	if (strstr(Core.Params, "-smap1024"))	o.smapsize = 1024; // Not stable
	if (strstr(Core.Params, "-smap1536"))	o.smapsize = 1536;
	if (strstr(Core.Params, "-smap2048"))	o.smapsize = 2048;
	if (strstr(Core.Params, "-smap2560"))	o.smapsize = 2560;
	if (strstr(Core.Params, "-smap3072"))	o.smapsize = 3072;
	if (strstr(Core.Params, "-smap4096"))	o.smapsize = 4096;
	if (strstr(Core.Params, "-smap6144"))	o.smapsize = 6144;
	if (strstr(Core.Params, "-smap8192"))	o.smapsize = 8192; //Not for all videocards

	// gloss
	char* g				= strstr(Core.Params, "-gloss ");
	o.forcegloss		= g ?	TRUE	:FALSE;

	if (g)
	{
		o.forcegloss_v = float (atoi (g+xr_strlen("-gloss "))) / 255.f;
	}

	// options
	o.bug				= (strstr(Core.Params,"-bug")) ?		TRUE	:FALSE;
	o.sunfilter			= (strstr(Core.Params,"-sunfilter")) ?	TRUE	:FALSE;

	o.sunstatic			= r2_sun_static;
	o.advancedpp		= r2_advanced_pp;

	o.volumetricfog		= false; //ps_r2_ls_flags.test(R3FLAG_VOLUMETRIC_SMOKE);

	o.sjitter			= (strstr(Core.Params,"-sjitter")) ?	TRUE	:FALSE;
	o.depth16			= (strstr(Core.Params,"-depth16")) ?	TRUE	:FALSE;
	o.noshadows			= (strstr(Core.Params,"-noshadows")) ?	TRUE	:FALSE;
	o.Tshadows			= (strstr(Core.Params,"-tsh")) ?		TRUE	:FALSE;
	o.distortion_enabled= (strstr(Core.Params,"-nodistort")) ?	FALSE	:TRUE;
	o.disasm			= (strstr(Core.Params,"-disasm")) ?		TRUE	:FALSE;
	o.forceskinw		= (strstr(Core.Params,"-skinw")) ?		TRUE	:FALSE;

	o.ssao_blur_on		= ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_BLUR) && (ps_r_ssao != 0);
	o.ssao_opt_data		= ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_OPT_DATA) && (ps_r_ssao != 0);
	o.ssao_half_data	= ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HALF_DATA) && o.ssao_opt_data && (ps_r_ssao != 0);
	o.ssao_hdao			= ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HDAO) && (ps_r_ssao != 0);
	o.ssao_hbao			= !o.ssao_hdao && ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HBAO) && (ps_r_ssao != 0);
	o.ssao_ssdo			= !o.ssao_hdao && !o.ssao_hbao && ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_SSDO) && (ps_r_ssao != 0);

	//	TODO: fix hbao shader to allow to perform per-subsample effect!
	o.hbao_vectorized = false;

	if (o.ssao_hbao)
	{
		if (HW.Caps.id_vendor == 0x1002)
			o.hbao_vectorized = true;

		o.ssao_opt_data = true;
	}

    if(o.ssao_hdao)
        o.ssao_opt_data = false;

	o.dx10_sm4_1		= ps_r2_ls_flags.test((u32)R3FLAG_USE_DX10_1);
	o.dx10_sm4_1		= o.dx10_sm4_1 && (HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1);

	//	MSAA option dependencies

	o.dx10_msaa			= !!ps_r3_msaa;
	o.dx10_msaa_samples = (1 << ps_r3_msaa);

	o.dx10_msaa_opt		= ps_r2_ls_flags.test(R3FLAG_MSAA_OPT);
	o.dx10_msaa_opt		= o.dx10_msaa_opt && o.dx10_msaa && (HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1) || o.dx10_msaa && (HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0);

	o.dx10_msaa_hybrid	= ps_r2_ls_flags.test((u32)R3FLAG_USE_DX10_1);
	o.dx10_msaa_hybrid	&= !o.dx10_msaa_opt && o.dx10_msaa && ( HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1 ) ;

	//	Allow alpha test MSAA for DX10.0
	o.dx10_msaa_alphatest = 0;

	if (o.dx10_msaa)
	{
		if (o.dx10_msaa_opt || o.dx10_msaa_hybrid)
		{
			if (ps_r3_msaa_atest == 1)
				o.dx10_msaa_alphatest = MSAA_ATEST_DX10_1_ATOC;
			else if (ps_r3_msaa_atest == 2)
				o.dx10_msaa_alphatest = MSAA_ATEST_DX10_1_NATIVE;
		}
		else
		{
			if (ps_r3_msaa_atest)
				o.dx10_msaa_alphatest = MSAA_ATEST_DX10_0_ATOC;
		}
	}

	o.dx10_gbuffer_opt = ps_r2_ls_flags.test(R3FLAG_GBUFFER_OPT);

	o.dx10_minmax_sm = ps_r3_minmax_sm;
	o.dx10_minmax_sm_screenarea_threshold = 1600 * 900;

	o.dx11_enable_tessellation = HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0 && ps_r2_ls_flags_ext.test(R2FLAGEXT_ENABLE_TESSELLATION);

	if (o.dx10_minmax_sm == MMSM_AUTODETECT)
	{
		o.dx10_minmax_sm = MMSM_OFF;

		//	AMD device
		if (HW.Caps.id_vendor==0x1002)
		{
			if (ps_r_sun_quality >= 3)
				o.dx10_minmax_sm = MMSM_AUTO;
			else if (ps_r_sun_shafts >= 2)
			{
				o.dx10_minmax_sm = MMSM_AUTODETECT;
				//	Check resolution in runtime in use_minmax_sm_this_frame
				o.dx10_minmax_sm_screenarea_threshold = 1600 * 900;
			}
		}

		//	NVidia boards
		if (HW.Caps.id_vendor == 0x10DE)
		{
			if ((ps_r_sun_shafts >= 2))
			{
				o.dx10_minmax_sm = MMSM_AUTODETECT;
				//	Check resolution in runtime in use_minmax_sm_this_frame
				o.dx10_minmax_sm_screenarea_threshold = 1280 * 1024;
			}
		}
	}

	// constants
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("parallax", &binder_parallax);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("steep_parallax", &binder_steep_parallax);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("water_refl_params_1", &binder_water_refl);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("water_refl_params_2", &binder_water_refl2);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("various_consts_2", &binder_other_consts_2);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("m_AlphaRef",	&binder_alpha_ref);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("pos_decompression_params", &binder_pos_decompress_params);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("triLOD",	&binder_LOD);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("test_dev_floats", &binder_test_floats);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup	("TonemapMisc", &tone_map_misc);

	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup("water_intensity", &binder_water_intensity);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup("sun_shafts_intensity", &binder_sun_shafts_intensity);
	dxRenderDeviceRender::Instance().Resources->RegisterConstantSetup("RainDensity", &binder_rain_density);

	c_lmaterial					= "L_material";
	c_sbase						= "s_base";

	m_bMakeAsyncSS				= false;

	Target						= xr_new<CRenderTarget>(); // Main target

	Models						= xr_new<CModelPool>();

	PSLibrary.OnCreate();
	HWOCC.occq_create(occq_size);

	rmNormal();

	D3D11_QUERY_DESC			qdesc;

	qdesc.MiscFlags				= 0;
	qdesc.Query					= D3D11_QUERY_EVENT;

	ZeroMemory(q_sync_point, sizeof(q_sync_point));

	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		R_CHK(HW.pDevice->CreateQuery(&qdesc,&q_sync_point[i]));

	HW.pContext->End(q_sync_point[0]);

	xrRender_apply_tf();
	::PortalTraverser.initialize();

	FluidManager.Initialize(70, 70, 70);
	FluidManager.SetScreenSize(Device.dwWidth, Device.dwHeight);

	Msg("* Render is set up: %f ms", T.GetElapsed_sec() * 1000.f);
}

void CRender::destroy()
{
	m_bMakeAsyncSS= false;

	FluidManager.Destroy();
	::PortalTraverser.destroy();

	//_RELEASE					(q_sync_point[1]);
	//_RELEASE					(q_sync_point[0]);

	for (u32 i = 0; i < HW.Caps.iGPUNum; ++i)
		_RELEASE (q_sync_point[i]);
	
	HWOCC.occq_destroy();

	xr_delete(Models);
	xr_delete(Target);

	PSLibrary.OnDestroy();
	Device.seqFrame.Remove(this);
	Device.seqFrameEnd.Remove(this);

	r_dsgraph_destroy();
}

void CRender::reset_begin()
{
	for(u32 j = 0; j< VIEW_PORTS_CNT; ++j)
	{
		xr_vector<CLightSource*>& pool = j == 0 ? viewPortBuffer1.Lights_LastFrame : viewPortBuffer2.Lights_LastFrame;

		u32 it = 0;

		for (it = 0; it < pool.size(); it++)
		{
			if (!pool[it])
				continue;

			pool[it]->svis.resetoccq();
		}

		pool.clear	();
	}

	if (b_loaded && (dm_current_size != dm_size))
	{
		Details->Unload();

		xr_delete(Details);
	}

	xr_delete(Target);
	HWOCC.occq_destroy();

	//_RELEASE					(q_sync_point[1]);
	//_RELEASE					(q_sync_point[0]);

	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		_RELEASE(q_sync_point[i]);
}

void CRender::reset_end()
{
	D3D11_QUERY_DESC qdesc;
	qdesc.MiscFlags = 0;
	qdesc.Query = D3D11_QUERY_EVENT;

	for (u32 i=0; i<HW.Caps.iGPUNum; ++i)
		R_CHK(HW.pDevice->CreateQuery(&qdesc,&q_sync_point[i]));

	//	Prevent error on first get data
	HW.pContext->End(q_sync_point[0]);

	HWOCC.occq_create(occq_size);

	Target = xr_new<CRenderTarget>	();

	if (b_loaded && (dm_current_size != dm_size))
	{
		Details = xr_new<CDetailManager>();
		Details->Load();
	}

	xrRender_apply_tf();
	FluidManager.SetScreenSize(Device.dwWidth, Device.dwHeight);

	// Set this flag true to skip the first render frame,
	// that some data is not ready in the first frame (for example device camera position)
	m_bFirstFrameAfterReset		= true;

	ClearRuntime();
}

void CRender::OnFrame()
{
#ifdef MEASURE_ON_FRAME
	CTimer measure_on_frame; measure_on_frame.Start();
#endif


	Models->DeleteQueue();


#ifdef MEASURE_ON_FRAME
	Device.Statistic->onframe_Render_ += measure_on_frame.GetElapsed_ms_f();
#endif
}

extern float r_ssaDISCARD_VP1;

void CRender::OnFrameEnd()
{
	// Restore view frustum anyway as for main viewport, since it is used in OnFrame code(ex. IKinematics)
	Fmatrix m = Device.currentVpSavedView->GetFullTransform_saved();
	baseViewFrustum_.CreateFromMatrix(m, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

	// Delayed light, glow deletion
	Lights.DeleteQueue();
	if (L_Glows)
		L_Glows->DeleteQueue();

	// Details calc should be started after light deletion is complete, since it uses lights
	// Start processing details viasbility for next frame; Try to do it before the first details rendering call is issued from main thread
	if (Details)
	{
		if (ps_r_mt_flags.test(R_FLAG_MT_CALC_HOM_DETAILS) && !Details->alreadySentToAuxThread_) // since we send calculation to the "OnTheGo" thread - make sure we don't stack it up each frame if "OnTheGo" thread is buisy
		{
			// Send details visability calculation to secondary thread
			Details->alreadySentToAuxThread_ = true;
			Details->frameStartedMTCalc_ = CurrentFrame(); // mark for the next frame main thread decision
			Details->ssaDISCARD_ = r_ssaDISCARD_VP1;

			SetCanCalcMTDetails(true); // set debug check

			// push at the front
			Device.AddAtFrontToIndAuxThread_3_Pool(fastdelegate::FastDelegate0<>(Details, &CDetailManager::MT_CALC));
		}
	}

	// Process mt loaded textures, if any
	protectDefTexturesPool_.Enter();

	// make a copy of corrent pool, so that the access to pool is not raced by threads
	mtDefferedLoadedTexturesCBTemp_ = mtDefferedLoadedTexturesCB_;
	mtDefferedLoadedTexturesCB_.clear_not_free();

	protectDefTexturesPool_.Leave();

	for (u32 pit = 0; pit < mtDefferedLoadedTexturesCBTemp_.size(); pit++)
	{
		mtDefferedLoadedTexturesCBTemp_[pit]();
	}

	mtDefferedLoadedTexturesCBTemp_.clear_not_free();

	// Process mt safe texture removal
	DEV->DeleteTextureQueue();
}

BOOL CRender::is_sun_active()
{
	if (o.sunstatic)
		return FALSE;

	Fcolor sun_color = ((CLightSource*)Lights.sun_adapted._get())->color;

	return (ps_r2_ls_flags.test(R2FLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS));
}

// Implementation
IRender_ObjectSpecific*	CRender::ros_create(IRenderable* parent)
{
	return xr_new<CROS_impl>();
}

void CRender::ros_destroy(IRender_ObjectSpecific* &p)
{
	xr_delete(p);
}

IRenderVisual* CRender::model_Create(LPCSTR name, IReader* data)
{
	return Models->Create(name,data);
}

IRenderVisual* CRender::model_CreateChild(LPCSTR name, IReader* data)
{
	return Models->CreateChild(name,data);
}

IRenderVisual* CRender::model_Duplicate(IRenderVisual* V)
{
	return Models->Instance_Duplicate((dxRender_Visual*)V);
}

void CRender::model_Delete(IRenderVisual* &V, BOOL bDiscard)	
{ 
	if (V)
	{
		dxRender_Visual* pVisual = (dxRender_Visual*)V;

		Models->DeleteVisual(pVisual, bDiscard);

		V = nullptr;
	}
}

IRender_DetailModel* CRender::model_CreateDM(IReader*F)
{
	CDetail* D = xr_new<CDetail> ();
	D->Load(F);

	return D;
}

void CRender::model_Delete(IRender_DetailModel* & F)
{
	if (F)
	{
		CDetail* D = (CDetail*)F;

		D->Unload();
		xr_delete(D);

		F = NULL;
	}
}

IRenderVisual* CRender::model_CreatePE(LPCSTR name)	
{ 
	PS::CPEDef*	SE = PSLibrary.FindPED(name);
	
	R_ASSERT3(SE, "Particle effect doesn't exist", name);

	return Models->CreatePE(SE);
}

IRenderVisual* CRender::model_CreateParticles(LPCSTR name)	
{ 
	PS::CPEDef*	SE = PSLibrary.FindPED	(name);

	if (SE)
		return Models->CreatePE(SE);
	else
	{
		PS::CPGDef*	SG = PSLibrary.FindPGD(name);

		R_ASSERT3(SG, "Particle effect or group doesn't exist", name);

		return Models->CreatePG(SG);
	}
}

void CRender::models_Prefetch()
{
	Models->Prefetch();
}

void CRender::models_Clear(BOOL b_complete)
{
	Models->ClearPool(b_complete);
}

ref_shader CRender::getShader(int id)
{
	VERIFY(id<int(Shaders.size()));

	return Shaders[id];
}

IRender_Portal* CRender::getPortal(int id)
{
	VERIFY(id<int(Portals.size()));

	return Portals[id];
}

IRender_Sector* CRender::getSector(int id)
{
	VERIFY(id<int(Sectors.size()));

	return Sectors[id];
}

IRender_Sector* CRender::getSectorActive()
{
	return pLastSector;
}

IRenderVisual* CRender::getVisual(int id)
{
	VERIFY(id<int(Visuals.size()));
	return Visuals[id];
}

D3DVERTEXELEMENT9* CRender::getVB_Format (int id, BOOL _alt)
{ 
	if (_alt)
	{
		VERIFY(id<int(xDC.size()));

		return xDC[id].begin();
	}
	else
	{
		VERIFY(id<int(nDC.size()));

		return nDC[id].begin();
	}
}
ID3DVertexBuffer*	CRender::getVB					(int id, BOOL	_alt)	{
	if (_alt)	{ VERIFY(id<int(xVB.size()));	return xVB[id];		}
	else		{ VERIFY(id<int(nVB.size()));	return nVB[id];		}
}

ID3DIndexBuffer* CRender::getIB(int id, BOOL _alt)
{ 
	if (_alt)
	{
		VERIFY(id<int(xIB.size()));	return xIB[id];
	}
	else
	{
		VERIFY(id<int(nIB.size()));	return nIB[id];
	}
}

FSlideWindowItem* CRender::getSWI(int id)
{
	VERIFY(id<int(SWIs.size()));

	return &SWIs[id];
}

IRender_Target* CRender::getTarget()
{
	return Target;										
}

IRender_Light* CRender::light_create()
{
	return Lights.CreateLight();
}

void CRender::light_destroy(IRender_Light*& ligth_to_delete)
{
	Lights.DeleteLight(ligth_to_delete);
}

IRender_Glow* CRender::glow_create()
{
	return xr_new<CGlow>();
}

void CRender::glow_destroy(IRender_Glow*& p_)
{
	L_Glows->DeleteGlow(p_);
}

BOOL CRender::occ_visible(vis_data& P)
{
	return HOM.hom_visible(P);
}

BOOL CRender::occ_visible(sPoly& P)
{
	return HOM.hom_visible(P);
}

BOOL CRender::occ_visible(Fbox& P)
{
	return HOM.hom_visible(P);
}

void CRender::add_Visual(IRenderVisual* V, IRenderBuffer& render_buffer)
{
	add_leafs_Dynamic((dxRender_Visual*)V, render_buffer, V->ignoreGeometryDrawOptimization_);
}

void CRender::add_Geometry(IRenderVisual* V, IRenderBuffer& render_buffer, xr_vector<CFrustum>* sector_frustums)
{
	add_Static((dxRender_Visual*)V, render_buffer, sector_frustums, u64(-1));
}

void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (T->suppress_wm)
		return;

	VERIFY2(_valid(P) && _valid(s) && T && verts && (s>EPS_L), "Invalid static wallmark params");

	Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::add_StaticWallmark(IWallMarkArray *pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
	dxWallMarkArray *pWMA = (dxWallMarkArray *)pArray;
	ref_shader *pShader = pWMA->dxGenerateWallmark();
	if (pShader) add_StaticWallmark(*pShader, P, s, T, V);
}

void CRender::add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
	dxUIShader* pShader = (dxUIShader*)&*S;
	add_StaticWallmark(pShader->hShader, P, s, T, V);
}

void CRender::clear_static_wallmarks()
{
	Wallmarks->clear();
}

void CRender::add_SkeletonWallmark(intrusive_ptr<CSkeletonWallmark> wm, DsGraphBuffer& buffer)
{
	Wallmarks->AddSkeletonWallmark(wm, buffer);
}

void CRender::add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size, IRenderBuffer* render_buffer)
{
	Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size, render_buffer);
}

void CRender::add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray *pArray, const Fvector& start, const Fvector& dir, float size, IRenderBuffer* render_buffer)
{
	dxWallMarkArray *pWMA = (dxWallMarkArray *)pArray;
	ref_shader *pShader = pWMA->dxGenerateWallmark();
	if (pShader) add_SkeletonWallmark(xf, (CKinematics*)obj, *pShader, start, dir, size, render_buffer);
}

void CRender::add_Occluder(Fbox2& bb_screenspace)
{
	HOM.occlude(bb_screenspace);
}

void CRender::rmNear()
{
	IRender_Target* T = getTarget();
	D3D_VIEWPORT VP = { 0, 0, (float)T->get_width(), (float)T->get_height(), 0, 0.02f };

	HW.pContext->RSSetViewports(1, &VP);
	//CHK_DX				(HW.pDevice->SetViewport(&VP));
}

void CRender::rmFar()
{
	IRender_Target* T = getTarget();
	D3D_VIEWPORT VP = { 0, 0, (float)T->get_width(), (float)T->get_height(), 0.99999f, 1.f };

	HW.pContext->RSSetViewports(1, &VP);
	//CHK_DX				(HW.pDevice->SetViewport(&VP));
}

void CRender::rmNormal()
{
	IRender_Target* T = getTarget();
	D3D_VIEWPORT VP = { 0, 0, (float)T->get_width(), (float)T->get_height(), 0, 1.f };

	HW.pContext->RSSetViewports(1, &VP);
	//CHK_DX				(HW.pDevice->SetViewport(&VP));
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CRender::CRender() : m_bFirstFrameAfterReset(false)
{
	init_cacades();

	viewPortBuffer1.mainRenderPrior0DsBuffer1_ = xr_new<DsGraphBuffer>();
	viewPortBuffer1.mainRenderPrior0DsBuffer2_ = xr_new<DsGraphBuffer>();
	viewPortBuffer2.mainRenderPrior0DsBuffer1_ = xr_new<DsGraphBuffer>();
	viewPortBuffer2.mainRenderPrior0DsBuffer2_ = xr_new<DsGraphBuffer>();
	mainRenderPrior1DsBuffer_ = xr_new<DsGraphBuffer>();
	wetRainDsBuffer_ = xr_new<DsGraphBuffer>();
	viewPortBuffer1.rainWetSurfRuntime_ = xr_new<SWetRainBuffer>();
	viewPortBuffer2.rainWetSurfRuntime_ = xr_new<SWetRainBuffer>();

	SetDsBuffersNames();

	viewPortBuffer1.rainCanCalcDeffered_ = false;
	viewPortBuffer2.rainCanCalcDeffered_ = false;

	viewPortBuffer1.renderPrior0BufferSwitch_ = 0;
	viewPortBuffer2.renderPrior0BufferSwitch_ = 0;

	uLastLTRACK = 0;

	bonesCalced_ = false;
	canCalcMTDetails_ = true;

	actualViewPortBufferNow = nullptr;
}

CRender::~CRender()
{
	delete_cascades_data();

	xr_delete(viewPortBuffer1.mainRenderPrior0DsBuffer1_);
	xr_delete(viewPortBuffer1.mainRenderPrior0DsBuffer2_);
	xr_delete(viewPortBuffer2.mainRenderPrior0DsBuffer1_);
	xr_delete(viewPortBuffer2.mainRenderPrior0DsBuffer2_);
	xr_delete(mainRenderPrior1DsBuffer_);
	xr_delete(wetRainDsBuffer_);
	xr_delete(viewPortBuffer1.rainWetSurfRuntime_);
	xr_delete(viewPortBuffer2.rainWetSurfRuntime_);
}

#include "../GameFont.h"
void CRender::Statistics (CGameFont* _F)
{
	CGameFont&	F	= *_F;

	F.OutNext	(" **** LT:%2d,LV:%2d **** ",stats.l_total,stats.l_visible); 
	stats.l_visible = 0;

	F.OutNext	("    S(%2d)   | (%2d)NS   ",stats.l_shadowed,stats.l_unshadowed);
	F.OutNext	("smap use[%2d], merge[%2d], finalclip[%2d]", stats.s_used, stats.s_merged - stats.s_used, stats.s_finalclip);
	stats.s_used = 0;
	stats.s_merged = 0;
	stats.s_finalclip = 0;
	F.OutSkip	();
	F.OutNext	(" **** Occ-Q(%03.1f) **** ", 100.f * f32(stats.o_culled) / f32(stats.o_queries ? stats.o_queries : 1));
	F.OutNext	(" total  : %2d",	stats.o_queries);
	stats.o_queries = 0;
	F.OutNext	(" culled : %2d",	stats.o_culled);
	stats.o_culled = 0;

	F.OutSkip	();

	u32	ict		= stats.ic_total + stats.ic_culled;
	F.OutNext	(" **** iCULL(%03.1f) **** ",100.f * f32(stats.ic_culled) / f32(ict ? ict : 1));
	F.OutNext	(" visible: %2d",	stats.ic_total);
	stats.ic_total = 0;
	F.OutNext	(" culled : %2d",	stats.ic_culled);
	stats.ic_culled = 0;

#ifdef DEBUG
	HOM.stats	();
#endif
}


#pragma comment(lib,"d3dx9.lib")


void CRender::addShaderOption(const char* name, const char* value)
{
	D3D_SHADER_MACRO macro = {name, value};
	m_ShaderOptions.push_back(macro);
}

// XXX nitrocaster: workaround to eliminate conflict between different GUIDs from DXSDK/Windows SDK
// 0a233719-3960-4578-9d7c-203b8b1d9cc1
static const GUID guidShaderReflection = {0x0a233719, 0x3960, 0x4578, {0x9d, 0x7c, 0x20, 0x3b, 0x8b, 0x1d, 0x9c, 0xc1}};

template <typename T>
static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const	file_name, T*& result, bool const disasm)
{
	result->sh = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size);

	ID3DShaderReflection *pReflection = 0;

	HRESULT const _hr = D3DReflect(buffer, buffer_size, guidShaderReflection, (void**)&pReflection);

	if (SUCCEEDED(_hr) && pReflection)
	{
		// Parse constant table data
		result->constants.parse(pReflection, ShaderTypeTraits<T>::GetShaderDest());

		_RELEASE(pReflection);
	}
	else
	{
		Msg("! D3DReflectShader %s hr == 0x%08x", file_name, _hr);
	}

	return				_hr;
}

static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name, void*& result, bool const disasm)
{
	HRESULT _result = E_FAIL;

	if (pTarget[0] == 'p')
	{
		SPS* sps_result = (SPS*)result;

		_result = HW.pDevice->CreatePixelShader(buffer, buffer_size, 0, &sps_result->ps);

		if (!SUCCEEDED(_result))
		{
			Log("! PS: ", file_name);
			Msg("! CreatePixelShader hr == 0x%08x", _result);

			return E_FAIL;
		}

		ID3DShaderReflection *pReflection = 0;

		_result = D3DReflect(buffer, buffer_size, guidShaderReflection, (void**)&pReflection);

		//	Parse constant, texture, sampler binding
		//	Store input signature blob
		if (SUCCEEDED(_result) && pReflection)
		{
			//	Let constant table parse it's data
			sps_result->constants.parse(pReflection, RC_dest_pixel);

			_RELEASE(pReflection);
		}
		else
		{
			Log("! PS: ", file_name);
			Msg("! D3DReflectShader hr == 0x%08x", _result);
		}
	}
	else if (pTarget[0] == 'v')
	{
		SVS* svs_result = (SVS*)result;

		_result = HW.pDevice->CreateVertexShader(buffer, buffer_size, 0, &svs_result->vs);

		if (!SUCCEEDED(_result))
		{
			Log("! VS: ", file_name);
			Msg("! CreatePixelShader hr == 0x%08x", _result);

			return E_FAIL;
		}

		ID3DShaderReflection *pReflection = 0;

		_result = D3DReflect(buffer, buffer_size, guidShaderReflection, (void**)&pReflection);

		//	Parse constant, texture, sampler binding
		//	Store input signature blob
		if (SUCCEEDED(_result) && pReflection)
		{
			//	TODO: DX10: share the same input signatures

			//	Store input signature (need only for VS)
			//CHK_DX( D3DxxGetInputSignatureBlob(pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(), &_vs->signature) );
			ID3DBlob*	pSignatureBlob;
			CHK_DX(D3DGetInputSignatureBlob(buffer, buffer_size, &pSignatureBlob));
			VERIFY(pSignatureBlob);

			svs_result->signature = dxRenderDeviceRender::Instance().Resources->_CreateInputSignature(pSignatureBlob);

			_RELEASE(pSignatureBlob);

			//	Let constant table parse it's data
			svs_result->constants.parse(pReflection, RC_dest_vertex);

			_RELEASE(pReflection);
		}
		else
		{
			Log("! VS: ", file_name);
			Msg("! D3DXFindShaderComment hr == 0x%08x", _result);
		}
	}
	else if (pTarget[0] == 'g')
	{
		SGS* sgs_result = (SGS*)result;

		_result = HW.pDevice->CreateGeometryShader(buffer, buffer_size, 0, &sgs_result->gs);

		if (!SUCCEEDED(_result))
		{
			Log("! GS: ", file_name);
			Msg("! CreateGeometryShaderhr == 0x%08x", _result);

			return E_FAIL;
		}

		ID3DShaderReflection *pReflection = 0;

		_result = D3DReflect(buffer, buffer_size, guidShaderReflection, (void**)&pReflection);

		//	Parse constant, texture, sampler binding
		//	Store input signature blob
		if (SUCCEEDED(_result) && pReflection)
		{
			//	Let constant table parse it's data
			sgs_result->constants.parse(pReflection, RC_dest_geometry);

			_RELEASE(pReflection);
		}
		else
		{
			Log("! PS: ", file_name);
			Msg("! D3DReflectShader hr == 0x%08x", _result);
		}
	}

	else if (pTarget[0] == 'c')
	{
		_result = create_shader(pTarget, buffer, buffer_size, file_name, (SCS*&)result, disasm);
	}
	else if (pTarget[0] == 'h')
	{
		_result = create_shader(pTarget, buffer, buffer_size, file_name, (SHS*&)result, disasm);
	}
	else if (pTarget[0] == 'd')
	{
		_result = create_shader(pTarget, buffer, buffer_size, file_name, (SDS*&)result, disasm);
	}
	else
	{
		NODEFAULT;
	}

	if (disasm)
	{
		ID3DBlob* disasm_ = 0;
		D3DDisassemble(buffer, buffer_size, FALSE, 0, &disasm_);
		//D3DXDisassembleShader		(LPDWORD(code->GetBufferPointer()), FALSE, 0, &disasm_ );
		string_path		dname;

		strconcat(sizeof(dname), dname, "disasm\\", file_name, ('v' == pTarget[0]) ? ".vs" : ('p' == pTarget[0]) ? ".ps" : ".gs");

		IWriter* W = FS.w_open("$logs$", dname);

		W->w(disasm_->GetBufferPointer(), (u32)disasm_->GetBufferSize());

		FS.w_close(W);

		_RELEASE(disasm_);
	}

	return _result;
}

//--------------------------------------------------------------------------------------------------------------

class includer : public ID3DInclude
{
public:
	HRESULT __stdcall Open (D3D10_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
	{
		string_path pname;

		strconcat(sizeof(pname), pname, ::Render->getShaderPath(), pFileName);

		IReader* R = FS.r_open("$game_shaders$", pname);

		if (0 == R)
		{
			// possibly in shared directory or somewhere else - open directly
			R = FS.r_open("$game_shaders$", pFileName);

			if (0 == R)
				return E_FAIL;
		}

		// duplicate and zero-terminate
		u32 size = R->length();
		u8* data = xr_alloc<u8>(size + 1);

		CopyMemory(data, R->pointer(), size);

		data[size] = 0;
		FS.r_close(R);

		*ppData = data;
		*pBytes = size;

		return D3D_OK;
	}

	HRESULT __stdcall Close(LPCVOID pData)
	{
		xr_free	(pData);

		return D3D_OK;
	}
};

#include <boost/crc.hpp>

static inline bool match_shader_id(LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result);

HRESULT	CRender::shader_compile(LPCSTR name, DWORD const* pSrcData, UINT SrcDataLen, LPCSTR pFunctionName, LPCSTR pTarget, DWORD Flags, void*& result)
{
	D3D_SHADER_MACRO				defines			[128];

	int								def_it			= 0;
	char							c_smapsize		[32];
	char							c_gloss			[32];
	char							c_sun_shafts	[32];
	char							c_ssao			[32];
	char							c_sun_quality	[32];

	char							c_aa[32];
	char sh_name[MAX_PATH] = "";
	
	for (u32 i = 0; i < m_ShaderOptions.size(); ++i)
	{
		defines[def_it++] = m_ShaderOptions[i];
	}

	u32 len = xr_strlen(sh_name);
	// options
	{
		xr_sprintf(c_smapsize, "%04d", u32(o.smapsize));

		defines[def_it].Name		=	"SMAP_size";
		defines[def_it].Definition	=	c_smapsize;

		def_it++;

		VERIFY(xr_strlen(c_smapsize) == 4);

		xr_strcat(sh_name, c_smapsize); len+=4;
	}

	if (o.fp16_filter)
	{
		defines[def_it].Name		=	"FP16_FILTER";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.fp16_filter); ++len;

	if (o.fp16_blend)
	{
		defines[def_it].Name		=	"FP16_BLEND";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.fp16_blend); ++len;

	if (o.HW_smap)
	{
		defines[def_it].Name		=	"USE_HWSMAP";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.HW_smap); ++len;

	if (o.HW_smap_PCF)
	{
		defines[def_it].Name		=	"USE_HWSMAP_PCF";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.HW_smap_PCF); ++len;

	if (o.HW_smap_FETCH4)
	{
		defines[def_it].Name		=	"USE_FETCH4";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.HW_smap_FETCH4); ++len;

	if (o.sjitter)
	{
		defines[def_it].Name		=	"USE_SJITTER";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.sjitter); ++len;

	if (HW.Caps.raster_major >= 3)
	{
		defines[def_it].Name		=	"USE_BRANCHING";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(HW.Caps.raster_major >= 3); ++len;

	if (HW.Caps.geometry.bVTF)	{
		defines[def_it].Name		=	"USE_VTF";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(HW.Caps.geometry.bVTF); ++len;

	if (o.Tshadows)
	{
		defines[def_it].Name		=	"USE_TSHADOWS";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.Tshadows); ++len;

	if (o.sunfilter)
	{
		defines[def_it].Name		=	"USE_SUNFILTER";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.sunfilter); ++len;

	if (o.sunstatic)
	{
		defines[def_it].Name		=	"USE_R2_STATIC_SUN";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.sunstatic); ++len;

	if (o.forcegloss)
	{
		xr_sprintf						(c_gloss, "%f", o.forcegloss_v);

		defines[def_it].Name		=	"FORCE_GLOSS";
		defines[def_it].Definition	=	c_gloss;

		def_it++;
	}

	sh_name[len] = '0' + char(o.forcegloss); ++len;

	if (o.forceskinw)
	{
		defines[def_it].Name		=	"SKIN_COLOR";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(o.forceskinw); ++len;

	if (o.ssao_blur_on)
	{
		defines[def_it].Name		=	"USE_SSAO_BLUR";
		defines[def_it].Definition	=	"1";
		def_it						++;
	}
	sh_name[len] = '0' + char(o.ssao_blur_on); ++len;

    if (o.ssao_hdao)
    {
        defines[def_it].Name		=	"HDAO";
        defines[def_it].Definition	=	"1";

        def_it						++;

		sh_name[len]='1'; ++len;
		sh_name[len]='0'; ++len;
		sh_name[len]='0'; ++len;
    }
	else
	{
		sh_name[len] = '0'; ++len;
		sh_name[len] = '0' + char(o.ssao_hbao); ++len;
		sh_name[len] = '0' + char(o.ssao_half_data); ++len;

		if (o.ssao_hbao)
		{
			defines[def_it].Name		=	"SSAO_OPT_DATA";

			if (o.ssao_half_data)
			{
				defines[def_it].Definition	=	"2";
			}
			else
			{
				defines[def_it].Definition	=	"1";
			}

			def_it++;

			if (o.hbao_vectorized)
			{
				defines[def_it].Name		=	"VECTORIZED_CODE";
				defines[def_it].Definition	=	"1";

				def_it++;
			}

			defines[def_it].Name		=	"USE_HBAO";
			defines[def_it].Definition	=	"1";

			def_it++;
		}

		if (o.ssao_ssdo)
		{

			defines[def_it].Name = "USE_SSDO";
			defines[def_it].Definition = "1";

			def_it++;
		}
	}

    if(o.dx10_msaa)
	{
		static char def[ 256 ];

		//if( m_MSAASample < 0 )
		//{
			def[0]= '0';
		//	sh_name[len]='0'; ++len;
		//}
		//else
		//{
		//	def[0]= '0' + char(m_MSAASample);
		//	sh_name[len]='0' + char(m_MSAASample); ++len;
		//}

		def[1] = 0;

		defines[def_it].Name		=	"ISAMPLE";
		defines[def_it].Definition	=	def;

		def_it++;

		sh_name[len] = '0'; ++len;
	}
	else
	{
		sh_name[len] = '0'; ++len;
	}

	// skinning
	if (m_skinning < 0)
	{
		defines[def_it].Name		=	"SKIN_NONE";
		defines[def_it].Definition	=	"1";

		def_it++;

		sh_name[len] = '1'; ++len;
	}
	else
	{
		sh_name[len] = '0'; ++len;
	}

	if (0 == m_skinning)
	{
		defines[def_it].Name		=	"SKIN_0";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(0 == m_skinning); ++len;

	if (1 == m_skinning)
	{
		defines[def_it].Name		=	"SKIN_1";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(1 == m_skinning); ++len;

	if (2 == m_skinning)
	{
		defines[def_it].Name		=	"SKIN_2";
		defines[def_it].Definition	=	"1";

		def_it++;
	}
	sh_name[len] = '0' + char(2==m_skinning); ++len;

	if (3 == m_skinning)
	{
		defines[def_it].Name		=	"SKIN_3";
		defines[def_it].Definition	=	"1";

		def_it++;
	}
	sh_name[len] = '0' + char(3==m_skinning); ++len;

	if (4 == m_skinning)
	{
		defines[def_it].Name		=	"SKIN_4";
		defines[def_it].Definition	=	"1";

		def_it++;
	}

	sh_name[len] = '0' + char(4==m_skinning); ++len;

	//	Igor: need restart options
	if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_WATER))
	{
		defines[def_it].Name		=	"USE_SOFT_WATER";
		defines[def_it].Definition	=	"1";

		def_it++;

		sh_name[len] = '1'; ++len;
	}
	else
	{
		sh_name[len] = '0'; ++len;
	}

	if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES))
	{
		defines[def_it].Name		=	"USE_SOFT_PARTICLES";
		defines[def_it].Definition	=	"1";

		def_it++;

		sh_name[len] = '1'; ++len;
	}
	else
	{
		sh_name[len] = '0'; ++len;
	}

	if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DOF) && ps_r2_ls_flags_ext.test(R2FLAGEXT_BOKEH))
	{
		defines[def_it].Name		=	"BOKEH";
		defines[def_it].Definition	=	"1";

		def_it++;

		sh_name[len] = '1'; ++len;
	}
	else
	{
		sh_name[len] = '0'; ++len;
	}

	if (ps_r__debug_ssao)
	{
		defines[def_it].Name = "DEBUG_AMBIENT_OCCLUSION";
		defines[def_it].Definition = "1";

		def_it++;

		sh_name[len] = '1'; ++len;
	}
	else
	{
		sh_name[len] = '0'; ++len;
	}

	if (RImplementation.o.advancedpp && ps_r_sun_shafts)
	{
		xr_sprintf(c_sun_shafts, "%d", ps_r_sun_shafts);

		defines[def_it].Name = "SUN_SHAFTS_QUALITY";
		defines[def_it].Definition = c_sun_shafts;

		def_it++;

		sh_name[len] = '0' + char(ps_r_sun_shafts);

		++len;
	}
	else
	{
		sh_name[len] = '0';
		++len;
	}

	if (RImplementation.o.advancedpp && ps_r_ssao)
	{
		xr_sprintf(c_ssao, "%d", ps_r_ssao);

		defines[def_it].Name = "SSAO_QUALITY";
		defines[def_it].Definition = c_ssao;

		def_it++;

		sh_name[len] = '0' + char(ps_r_ssao);

		++len;
	}
	else
	{
		sh_name[len] = '0';

		++len;
	}

	if (RImplementation.o.advancedpp && ps_r_aa)
	{
		xr_sprintf(c_aa, "%d", ps_r_aa);

		defines[def_it].Name = "SMAA_QUALITY";
		defines[def_it].Definition = c_aa;

		def_it++;

		sh_name[len] = '0' + char(ps_r_aa);

		++len;
	}
	else
	{
		sh_name[len] = '0';

		++len;
	}

	if (RImplementation.o.advancedpp && ps_r_sun_quality)
	{
		xr_sprintf(c_sun_quality, "%d", ps_r_sun_quality);

		defines[def_it].Name = "SUN_QUALITY";
		defines[def_it].Definition = c_sun_quality;

		def_it++;

		sh_name[len] = '0' + char(ps_r_sun_quality);

		++len;
	}
	else
	{
		sh_name[len] = '0';

		++len;
	}

	if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_STEEP_PARALLAX))
	{
		defines[def_it].Name		=	"ALLOW_STEEPPARALLAX";
		defines[def_it].Definition	=	"1";

		def_it++;

		sh_name[len] = '1'; ++len;
	}
	else
	{
		sh_name[len] = '0'; ++len;
	}

	if( o.dx10_gbuffer_opt)
	{
		defines[def_it].Name		=	"GBUFFER_OPTIMIZATION";
		defines[def_it].Definition	=	"1";

		def_it++;
	}
	sh_name[len] = '0' + char(o.dx10_gbuffer_opt); ++len;

	if(o.dx10_sm4_1)
	{
		defines[def_it].Name		=	"SM_4_1";
		defines[def_it].Definition	=	"1";

		def_it++;
	}
	sh_name[len] = '0' + char(o.dx10_sm4_1); ++len;

#pragma todo("Rafa: GSC pixel shaders have some incompatable code with shader model 5. this brakes sun shafts and shadows. Disabled!")
	/*
	R_ASSERT						( HW.FeatureLevel>=D3D_FEATURE_LEVEL_11_0 );
	if( HW.FeatureLevel>=D3D_FEATURE_LEVEL_11_0 )
	{
		defines[def_it].Name		=	"SM_5";
		defines[def_it].Definition	=	"1";
		def_it++;
	}
	sh_name[len]='0'+char(HW.FeatureLevel>=D3D_FEATURE_LEVEL_11_0); ++len;
	*/

	if (o.dx10_minmax_sm)
	{
	   defines[def_it].Name		=	"USE_MINMAX_SM";
	   defines[def_it].Definition	=	"1";
	   def_it++;
	}
	sh_name[len] = '0' + char(o.dx10_minmax_sm != 0); ++len;

	//Be carefull!!!!! this should be at the end to correctly generate
	//compiled shader name;
	// add a #define for DX10_1 MSAA support
	if( o.dx10_msaa )
	{
		defines[def_it].Name			=	"USE_MSAA";
		defines[def_it].Definition	=	"1";

		def_it++;

		sh_name[len] = '1'; ++len;

		static char samples[2];

		defines[def_it].Name		=	"MSAA_SAMPLES";

		samples[0] = char(o.dx10_msaa_samples) + '0';
		samples[1] = 0;
		defines[def_it].Definition	= samples;	

		def_it++;

		sh_name[len] = '0' + char(o.dx10_msaa_samples); ++len;

		if( o.dx10_msaa_opt )
		{
			defines[def_it].Name			=	"MSAA_OPTIMIZATION";
			defines[def_it].Definition	=	"1";

			def_it++;
		}
		sh_name[len] = '0' + char(o.dx10_msaa_opt); ++len;

		switch(o.dx10_msaa_alphatest)
		{
		case MSAA_ATEST_DX10_0_ATOC:
			defines[def_it].Name		=	"MSAA_ALPHATEST_DX10_0_ATOC";
			defines[def_it].Definition	=	"1";

			def_it ++;

			sh_name[len] = '1'; ++len;
			sh_name[len] = '0'; ++len;
			sh_name[len] = '0'; ++len;
			break;
		case MSAA_ATEST_DX10_1_ATOC:
			defines[def_it].Name		=	"MSAA_ALPHATEST_DX10_1_ATOC";
			defines[def_it].Definition	=	"1";

			def_it ++;

			sh_name[len] = '0'; ++len;
			sh_name[len] = '1'; ++len;
			sh_name[len] = '0'; ++len;

			break;
		case MSAA_ATEST_DX10_1_NATIVE:
			defines[def_it].Name		=	"MSAA_ALPHATEST_DX10_1";
			defines[def_it].Definition	=	"1";

			def_it ++;

			sh_name[len] ='0'; ++len;
			sh_name[len] ='0'; ++len;
			sh_name[len] ='1'; ++len;

			break;
		default:
			sh_name[len] = '0'; ++len;
			sh_name[len] = '0'; ++len;
			sh_name[len] = '0'; ++len;
		}
   }
   else {
		sh_name[len] = '0'; ++len;
		sh_name[len] = '0'; ++len;
		sh_name[len] = '0'; ++len;
		sh_name[len] = '0'; ++len;
		sh_name[len] = '0'; ++len;
		sh_name[len] = '0'; ++len;
   }

	sh_name[len] = 0;

	// finish
	defines[def_it].Name = 0;
	defines[def_it].Definition = 0;

	def_it++; 

#pragma todo("Rafa: should be 5_0, but settinge ps_5_0 brakes pixel shaders")
	if (0 == xr_strcmp(pFunctionName, "main"))
	{
		if ('v' == pTarget[0])
		{
			if (HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
				pTarget = "vs_4_0";
			else if (HW.FeatureLevel == D3D_FEATURE_LEVEL_10_1 || HW.FeatureLevel == D3D_FEATURE_LEVEL_11_0)
				pTarget = "vs_4_1";
			// else if (HW.FeatureLevel == D3D_FEATURE_LEVEL_11_0 || D3D_FEATURE_LEVEL_11_0)
			//	 pTarget = "vs_5_0";
		}
		else if ('p' == pTarget[0])
		{
			if (HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
				pTarget = "ps_4_0";
			else if (HW.FeatureLevel == D3D_FEATURE_LEVEL_10_1 || HW.FeatureLevel == D3D_FEATURE_LEVEL_11_0)
				pTarget = "ps_4_1";
#pragma todo("Rafa: Broken sun shafts starts here if pTarget becomes ps_5_0")
			//else if( HW.FeatureLevel == D3D_FEATURE_LEVEL_11_0 )
			//  pTarget = "ps_5_0";
		}
		else if ('g' == pTarget[0])
		{
			if (HW.FeatureLevel == D3D_FEATURE_LEVEL_10_0)
				pTarget = "gs_4_0";
			else if (HW.FeatureLevel == D3D_FEATURE_LEVEL_10_1 || HW.FeatureLevel == D3D_FEATURE_LEVEL_11_0)
				pTarget = "gs_4_1";
			// else if( HW.FeatureLevel == D3D_FEATURE_LEVEL_11_0 )
			//	pTarget = "gs_5_0";
		}
		else if ('c' == pTarget[0])
		{
			if (HW.FeatureLevel == D3D_FEATURE_LEVEL_11_0)
				pTarget = "cs_4_1";
		}
	}

	HRESULT _result = E_FAIL;

	string_path	folder_name, folder;

	xr_strcpy(folder, "r3\\objects\\r4\\");
	xr_strcat(folder, name);
	xr_strcat(folder, ".");

	char extension[3];

	strncpy_s(extension, pTarget, 2);
	xr_strcat(folder, extension);

	FS.update_path(folder_name, "$game_shaders$", folder);
	xr_strcat(folder_name, "\\");

	m_file_set.clear();

	FS.file_list(m_file_set, folder_name, FS_ListFiles | FS_RootOnly, "*");

	string_path temp_file_name, file_name;

	if (!match_shader_id(name, sh_name, m_file_set, temp_file_name))
	{
		string_path file;

		xr_strcpy(file, "shaders_cache\\r4\\");
		xr_strcat(file, name);
		xr_strcat(file, ".");
		xr_strcat(file, extension);
		xr_strcat(file, "\\");
		xr_strcat(file, sh_name);

		FS.update_path(file_name, "$app_data_root$", file);
	}
	else
	{
		xr_strcpy(file_name, folder_name);
		xr_strcat(file_name, temp_file_name);
	}

	if (FS.exist(file_name))
	{
		IReader* file = FS.r_open(file_name);

		if (file->length() > 4)
		{
			u32 crc = 0;
			crc = file->r_u32();

			boost::crc_32_type processor;

			processor.process_block(file->pointer(), ((char*)file->pointer()) + file->elapsed());
			u32 const real_crc = processor.checksum();

			if (real_crc == crc)
			{
				_result = create_shader(pTarget, (DWORD*)file->pointer(), file->elapsed(), file_name, result, o.disasm);
			}
		}

		file->close();
	}

	if (FAILED(_result))
	{
		includer Includer;
		LPD3DBLOB pShaderBuf = NULL;
		LPD3DBLOB pErrorBuf = NULL;

		_result = D3DCompile
		(
			pSrcData,
			SrcDataLen,
			"",//NULL, //LPCSTR pFileName,	//	NVPerfHUD bug workaround.
			defines, &Includer, pFunctionName,
			pTarget,
			Flags, 0,
			&pShaderBuf,
			&pErrorBuf
		);

		if (SUCCEEDED(_result))
		{
			if (ps_r2_ls_flags.test(R_FLAG_SHADERS_CACHE))
			{
				IWriter* file = FS.w_open(file_name);

				boost::crc_32_type processor;

				processor.process_block(pShaderBuf->GetBufferPointer(), ((char*)pShaderBuf->GetBufferPointer()) + pShaderBuf->GetBufferSize());

				u32 const crc = processor.checksum();

				file->w_u32(crc);
				file->w(pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize());

				FS.w_close(file);
			}

			_result = create_shader(pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), (u32)pShaderBuf->GetBufferSize(), file_name, result, o.disasm);
		}
		else
		{
			//			Msg						( "! shader compilation failed" );
			Log("! ", file_name);

			if (pErrorBuf)
				Log("! error: ", (LPCSTR)pErrorBuf->GetBufferPointer());
			else
				Msg("Can't compile shader hr=0x%08x", _result);
		}
	}

	return _result;
}

static inline bool match_shader(LPCSTR const debug_shader_id, LPCSTR const full_shader_id, LPCSTR const mask, size_t const mask_length)
{
	u32 const full_shader_id_length = xr_strlen(full_shader_id);

	R_ASSERT2(full_shader_id_length == mask_length, make_string("bad cache for shader %s, [%s], [%s]", debug_shader_id,	mask, full_shader_id));

	char const* i = full_shader_id;
	char const* const e = full_shader_id + full_shader_id_length;
	char const* j = mask;

	for (; i != e; ++i, ++j)
	{
		if (*i == *j)
			continue;

		if (*j == '_')
			continue;

		return false;
	}

	return true;
}

static inline bool match_shader_id(LPCSTR const debug_shader_id, LPCSTR const full_shader_id, FS_FileSet const& file_set, string_path& result)
{
#if 0
	strcpy_s(result, "");
	return false;
#else

#ifdef DEBUG
	LPCSTR temp = "";
	bool found = false;
	FS_FileSet::const_iterator	i = file_set.begin();
	FS_FileSet::const_iterator	const e = file_set.end();
	for (; i != e; ++i) {
		if (match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size())) {
			VERIFY(!found);
			found = true;
			temp = (*i).name.c_str();
		}
	}

	xr_strcpy(result, temp);
	return						found;
#else

	FS_FileSet::const_iterator	i = file_set.begin();
	FS_FileSet::const_iterator	const e = file_set.end();

	for (; i != e; ++i)
	{
		if (match_shader(debug_shader_id, full_shader_id, (*i).name.c_str(), (*i).name.size()))
		{
			xr_strcpy(result, (*i).name.c_str());

			return true;
		}
	}

	return false;
#endif
#endif
}

void dxRenderDeviceRender::OnNewFrame()
{
	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_HOM_DETAILS) && !RImplementation.HOM.alreadySentToAuxThread_)
	{
		RImplementation.HOM.alreadySentToAuxThread_ = true;

		Device.AddAtFrontToIndAuxThread_1_Pool(fastdelegate::FastDelegate0<>(&RImplementation.HOM, &CHOM::MT_RENDER));
	}
}

void dxRenderDeviceRender::RenderFinish(ViewPort vp)
{
	if (!HW.pDevice)
	{
		Msg("FATAL ERROR: dxRenderDeviceRender::RenderFinish(): HW.pDevice is lost!");
		FlushLog();
	}

	VERIFY(HW.pDevice);

	if (HW.Caps.SceneMode)
		overdrawEnd();

	Memory.dbg_check();

	if (RImplementation.currentViewPort == MAIN_VIEWPORT)
		RImplementation.ScreenshotAsync();

	// Use V-Sync if set by user, or if in main menu - but only in fullscreen, to avoid strange flickering.
	if (RImplementation.needPresenting) //--#SM+#-- +SecondVP+ Не выводим кадр из второго рендера на экран
	{
		bool bUseVSync = psDeviceFlags.is(rsFullscreen) && (psDeviceFlags.test(rsVSync));

		HW.lastPresentStatus = HW.m_pSwapChain->Present(bUseVSync ? 1 : 0, 0);
	}

	RCache.OnRenderFinish(vp);

	RImplementation.RenderFinish();
}
