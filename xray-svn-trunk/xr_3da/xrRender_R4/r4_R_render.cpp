#include "stdafx.h"
#include "../igame_persistent.h"
#include "../xrRender/FBasicVisual.h"
#include "../customhud.h"
#include "../xr_object.h"
#include "../xrRender/SkeletonCustom.h"

#include "../xrRender/QueryHelper.h"
#include "../xrRender/dxRenderDeviceRender.h"

float CPU_wait_GPU_lastFrame_ = 0.f;

#include "../PS_instance.h"

IC bool pred_sp_sort(ISpatial* _1, ISpatial* _2)
{
	try {
		R_ASSERT(_1);
		R_ASSERT(_2);

		R_ASSERT(&_1->spatial);
		R_ASSERT(&_2->spatial);

		Fvector cam_pos = Device.currentVpSavedView->GetCameraPosition_saved();

		float d1 = _1->spatial.sphere.P.distance_to_sqr(cam_pos);
		float d2 = _2->spatial.sphere.P.distance_to_sqr(cam_pos);

		return d1 < d2;
	}
	catch (...)
	{
		R_ASSERT(false);

		R_ASSERT(_1);
		R_ASSERT(_2);

		CObject* obj = dynamic_cast<CObject*>(_1);
		CPS_Instance* ps = dynamic_cast<CPS_Instance*>(_1);
		CGlow* glow = dynamic_cast<CGlow*>(_1);
		CLightSource* light = dynamic_cast<CLightSource*>(_1);

		Msg("_1 %s %s %s %s", obj ? "obj" : "no obj", ps ? "ps" : "no ps", glow ? "glow" : "no glow", light ? "light" : "no light");
		
		obj = dynamic_cast<CObject*>(_2);
		ps = dynamic_cast<CPS_Instance*>(_2);
		glow = dynamic_cast<CGlow*>(_2);
		light = dynamic_cast<CLightSource*>(_2);
		
		Msg("_2 %s %s %s %s", obj ? "obj" : "no obj", ps ? "ps" : "no ps", glow ? "glow" : "no glow", light ? "light" : "no light");
	}
}

IC bool dist_to_cam(CLightSource* _1, CLightSource* _2)
{
	float distnce_to_viewport = Device.vCameraPosition.distance_to(_1->position);
	float distnce_to_viewport2 = Device.vCameraPosition.distance_to(_2->position);

	return distnce_to_viewport < distnce_to_viewport2;
}

void CRender::render_details()
{
	if (Details)
	{
		if(currentViewPort == firstViewPort)
			Details->CheckDetails(); // Mt sync point

		SetCanCalcMTDetails(false); // set debug check

		if(currentViewPort == MAIN_VIEWPORT)
			Details->Render(Details->m_visibles);
		else
			Details->Render(Details->m_visibles2);
	}
}

void CRender::render_main_prior_0_mt()
{
	// Gets the main geometry that needs to be rendered. Also get glows, lights, walmarks
	mtCalcRMain0Protect_.Enter();

	if (!actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->IsCalculated())
	{
#ifdef MEASURE_MT
		CTimer T; T.Start();
#endif

		// enable priority "0", + capture wmarks
		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->phaseMask[0] = true;
		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->phaseMask[1] = false;
		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->phaseMask[2] = true;

		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->phaseType_ = PHASE_NORMAL;

		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->useHOM_ = !!ps_r_misc_flags.test(R_MISC_USE_HOM_PRIOR_0);
		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->portalTraverserOpt = (actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->useHOM_ ? CPortalTraverser::VQ_HOM : 0) + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE;
		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->distortedGeom_ = false;

		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->dsgraphBufferViewFrustum_ = baseViewFrustum_;

		Fmatrix full_trans;

		if (currentViewPort == SECONDARY_WEAPON_SCOPE)
			full_trans = Device.currentVpSavedView->GetShrinkedFullTransform_saved();
		else
			full_trans = Device.currentVpSavedView->GetFullTransform_saved();

		Fvector cam_pos = Device.currentVpSavedView->GetCameraPosition_saved();

		render_subspace_MT(actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_, pLastSector, full_trans, cam_pos, true, true, true, true);

		actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->SetCalculated(true);

#ifdef MEASURE_MT
		if (ps_r_mt_flags.test(R_FLAG_MT_CALC_PRIOR0_VIS_STRUCT)) Device.Statistic->mtVisStructTime_ += T.GetElapsed_sec();
#endif
	}

	mtCalcRMain0Protect_.Leave();
}

void CRender::render_main_prior_1_mt()
{
	// Gets the second priority (tranparent) geometry.
	mtCalcRMain1Protect_.Enter();

	if (!mainRenderPrior1DsBuffer_->IsCalculated())
	{
#ifdef MEASURE_MT
		CTimer T; T.Start();
#endif

		// enable priority "1"
		mainRenderPrior1DsBuffer_->phaseMask[0] = false;
		mainRenderPrior1DsBuffer_->phaseMask[1] = true;
		mainRenderPrior1DsBuffer_->phaseMask[2] = false;

		mainRenderPrior1DsBuffer_->phaseType_ = PHASE_NORMAL;

		mainRenderPrior1DsBuffer_->useHOM_ = !!ps_r_misc_flags.test(R_MISC_USE_HOM_FORWARD);
		mainRenderPrior1DsBuffer_->portalTraverserOpt = (mainRenderPrior1DsBuffer_->useHOM_ ? CPortalTraverser::VQ_HOM : 0) + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE;

		mainRenderPrior1DsBuffer_->distortedGeom_ = true && o.distortion_enabled;

		mainRenderPrior1DsBuffer_->dsgraphBufferViewFrustum_ = baseViewFrustum_;

		Fmatrix full_trans;

		if (currentViewPort == SECONDARY_WEAPON_SCOPE)
			full_trans = Device.currentVpSavedView->GetShrinkedFullTransform_saved();
		else
			full_trans = Device.currentVpSavedView->GetFullTransform_saved();

		Fvector cam_pos = Device.currentVpSavedView->GetCameraPosition_saved();

		render_subspace_MT(mainRenderPrior1DsBuffer_, pLastSector, full_trans, cam_pos);

		mainRenderPrior1DsBuffer_->SetCalculated(true);

#ifdef MEASURE_MT
		if (ps_r_mt_flags.test(R_FLAG_MT_CALC_PRIOR1_VIS_STRUCT)) Device.Statistic->mtVisStructTime_ += T.GetElapsed_sec();
#endif

	}

	mtCalcRMain1Protect_.Leave();
}


void CRender::render_add_light(ISpatial* spatial)
{
	CLightSource* L = (CLightSource*)(spatial->dcast_Light());

	VERIFY(L);

	float lod = L->get_LOD();

	if (lod > EPS_L)
	{
		vis_data& vis = L->get_homdata();

		if (HOM.hom_visible(vis))
			Lights.add_light(L);
	}
}

void CRender::render_add_glow(ISpatial* spatial, bool vis_res, DsGraphBuffer& buffer)
{
	CGlow* glow = dynamic_cast<CGlow*>(spatial);

	if (glow)
	{
		auto I = std::find_if(actualViewPortBufferNow->defferedGlowsVect_.begin(), actualViewPortBufferNow->defferedGlowsVect_.end(), GlowRemoverPred(glow));

		if (I != actualViewPortBufferNow->defferedGlowsVect_.end() && vis_res == true) // rewrite only if visible
		{
			(*I).visible = true;
		}
		else
			actualViewPortBufferNow->defferedGlowsVect_.push_back(RawGlow(glow, vis_res));
	}
}

void CRender::render_subspace_MT(DsGraphBuffer* buffer, IRender_Sector* start_sector, Fmatrix& full_trans, Fvector base_pos, bool _dynamic, bool add_lights, bool add_glows, bool calc_lum, bool _precise_portals)
{
	buffer->visMarker_++; // !!! Critical here. Mark that the buffer is having its next pass, to avoid geometry not getting into vis structure
	buffer->setRenderable_ = nullptr;
	buffer->sectorS.clear(); // just in case

#if 0 // Dont remove. Offenly used
	Msg("%s : Dyn %u | Static %u", *buffer->debugName, buffer->dynCnt, buffer->staticCnt);
#endif

	buffer->dynCnt = 0;
	buffer->staticCnt = 0;

	buffer->mapSorted.clear();
	buffer->mapHUD.clear();
	buffer->mapHUDEmissive.clear();
	buffer->mapHUDSorted.clear();
	buffer->mapLOD.clear();
	buffer->mapDistort.clear();
	buffer->mapWmark.clear();
	buffer->mapEmissive.clear();

	// --1 Traverse sector/portal structure	//. disabled scissoring (HW.Caps.bScissor?CPortalTraverser::VQ_SCISSOR:0) // generate scissoring info
	
	protectMTUnsafeRendering_.Enter(); // Enter MT unsafe part
	{
		if (_precise_portals && rmPortals)
		{
			// Check if camera is too near to some portal - if so force DualRender
			Fvector box_radius;
			box_radius.set(EPS_L * 20, EPS_L * 20, EPS_L * 20);
#pragma note("MT warning: usage of global Sectors_xrc, which can be used in other threads(GI, sector detection)")
			Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
			Sectors_xrc.box_query(rmPortals, base_pos, box_radius);

			for (int K = 0; K < Sectors_xrc.r_count(); K++)
			{
				CPortal* pPortal = (CPortal*)Portals[rmPortals->get_tris()[Sectors_xrc.r_begin()[K].id].dummy];
				pPortal->bDualRender = TRUE;
			}
		}

		PortalTraverser.traverse(start_sector, buffer->dsgraphBufferViewFrustum_, base_pos, full_trans, buffer->portalTraverserOpt, *buffer);
	}
	protectMTUnsafeRendering_.Leave(); // Leave MT unsafe part

	// --2 Determine visibility for dynamic geometry

	if (_dynamic)
	{
		// Pick objects visible in subrender view
		g_SpatialSpace->q_frustum(buffer->renderables_, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE + STYPE_LIGHTSOURCE, buffer->dsgraphBufferViewFrustum_);

		// Exact(almost) sorting order (front-to-back)
		std::sort(buffer->renderables_.begin(), buffer->renderables_.end(), pred_sp_sort);

		// Loop picked dynamic objects
		for (u32 o_it = 0; o_it < buffer->renderables_.size(); o_it++)
		{
			ISpatial* spatial = buffer->renderables_[o_it];

			IRender_Sector* I_sector = nullptr;
			spatial->spatial_updatesector_and_get_res(CurrentFrame(), I_sector);
			CSector* sector = (CSector*)I_sector;

			if (!sector)
				continue; // disassociated from Sectors space

			// Lights ignore sectors // Mt usafe. Lights should be added only once per frame and before the lights processing is started
			if (spatial->spatial.s_type & STYPE_LIGHTSOURCE && add_lights)
			{
				render_add_light(spatial);

				continue;
			}

			if ((spatial->spatial.s_type & STYPE_RENDERABLE) == 0)
				continue;

			LocalSubrenderSectorData& subrender_data = sector->GetSubrenderData(buffer->visMarkerId_);

			// Check if spatial sector is marked by our subrender this frame
			if (buffer->portalTraverserMark != subrender_data.r_marker)
				continue; // inactive (untouched) sector

			IRenderable* renderable = spatial->dcast_Renderable();

			// Test HOM
			if (renderable && buffer->useHOM_)
			{

				if (!HOM.hom_visible(renderable))
					continue;
			}
			// Now test spatial with each sector frustums
			for (u32 v_it = 0; v_it < subrender_data.r_frustums.size(); v_it++)
			{
				BOOL vis_res = subrender_data.r_frustums[v_it].testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R);

				// Mt usafe! Glows should be added only once per frame and from the same render_main_vis_structure call place
				if (!renderable && add_glows)
				{
					if (!ps_r2_ls_flags_ext.test(R2FLAGEXT_GLOWS_ENABLE))
						break;

					render_add_glow(spatial, !!vis_res, *buffer);

					if (vis_res)
						break; // exit loop of frustums
				}
				// Regular dynamic geometry
				else if (renderable && vis_res)
				{
					buffer->setRenderable_ = renderable;

					// Call rendarable's "add me" function, which can contain necessary local "before render" procedures
					renderable->renderable_Render(*buffer);

					buffer->setRenderable_ = nullptr;

					break; // exit loop of frustums
				}
				// There was a 'break' statement in this line, which was causing bug with dynamic objects getting disapeared. Need to loop through all frustums of a sector!
			}
		}
	}

	// --3 Determine visibility for static geometry hierrarhy

	for (u32 s_it = 0; s_it < buffer->sectorS.size(); s_it++)
	{
		// Get the root geometry of sectors
		CSector* sector = (CSector*)buffer->sectorS[s_it];
		dxRender_Visual* root = sector->root();
		LocalSubrenderSectorData& subrender_data = sector->GetSubrenderData(buffer->visMarkerId_);

		VERIFY(subrender_data.r_frustums.size() < 64); // frustums vector mask is 64 bit, so if exeede - need to redo it other way

		add_Geometry(root, *buffer, ps_r_misc_flags.test(R_MISC_USE_SECTORS_FOR_STATIC) ? &subrender_data.r_frustums : nullptr);
	}
}

void CRender::render_menu()
{
	PIX_EVENT(render_menu);

	//	Globals
	RCache.set_CullMode(CULL_CCW);

	RCache.set_Stencil(FALSE);

	RCache.set_ColorWriteEnable();

	// Main Render
	{
		Target->u_setrt(Target->rt_Generic_0, 0, 0, HW.pBaseZB); // LDR RT

		g_pGamePersistent->OnRenderPPUI_main();	// PP-UI
	}

	// Distort
	{
		FLOAT ColorRGBA[4] = {127.0f / 255.0f, 127.0f / 255.0f, 0.0f, 127.0f / 255.0f};

		Target->u_setrt(Target->rt_Generic_1,0,0,HW.pBaseZB); // Now RT is a distortion mask

		HW.pContext->ClearRenderTargetView(Target->rt_Generic_1->pRT, ColorRGBA);		

		g_pGamePersistent->OnRenderPPUI_PP();// PP-UI
	}

	// Actual Display
	Target->u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, HW.pBaseZB);

	RCache.set_Shader(Target->s_menu);
	RCache.set_Geometry(Target->g_menu);

	Fvector2 p0, p1;

	u32	Offset;

	u32 C = color_rgba	(255, 255, 255, 255);

	float _w = float(Device.dwWidth);
	float _h = float(Device.dwHeight);

	float d_Z = EPS_S;
	float d_W = 1.f;

	p0.set(.5f / _w, .5f / _h);
	p1.set((_w + .5f) / _w, (_h + .5f) / _h );

	FVF::TL* pv = (FVF::TL*) RCache.Vertex.Lock(4,Target->g_menu->vb_stride,Offset);


	pv->set (EPS, float(_h+EPS), d_Z, d_W, C, p0.x, p1.y); pv++;

	pv->set (EPS, EPS, d_Z, d_W, C, p0.x, p0.y); pv++;

	pv->set (float(_w+EPS), float(_h+EPS), d_Z, d_W, C, p1.x, p1.y); pv++;

	pv->set (float(_w+EPS), EPS, d_Z, d_W, C, p1.x, p0.y); pv++;


	RCache.Vertex.Unlock(4, Target->g_menu->vb_stride);

	RCache.BackendRender(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
}

void CRender::RenderFrame(ViewPort viewport)
{

	CTimer T, TT; T.Start(); TT.Start();

	vpStats = &(viewport == MAIN_VIEWPORT ? Device.Statistic->viewPortStats1 : Device.Statistic->viewPortStats2);
	ViewPortRenderingStats& vp_stats = *vpStats;

	Target->u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, HW.pBaseZB); // Set up HW base as RT and ZB

	rmNormal(); // Set up view port

	Target->needClearAccumulation = true;

	if (viewport == MAIN_VIEWPORT)
	{
		bool _menu_pp = g_pGamePersistent ? g_pGamePersistent->OnRenderPPUI_query() : false;

		if (_menu_pp)
		{
			render_menu();

			Device.auxThread_4_Allowed_.Set();

			return;
		};
	}

	bool is_blocked = g_pGamePersistent ? g_pGamePersistent->SceneRenderingBlocked() : false;

	if (!(g_pGameLevel && g_hud) || is_blocked)
	{
		Target->u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, HW.pBaseZB);

		if (viewport == MAIN_VIEWPORT)
			Device.auxThread_4_Allowed_.Set();

		return;
	}

	if (viewport == MAIN_VIEWPORT)
	{
		if (m_bFirstFrameAfterReset)
		{
			m_bFirstFrameAfterReset = false;

			Device.auxThread_4_Allowed_.Set();

			return;
		}
	}

	R_ASSERT(actualViewPortBufferNow);

	// Configure

	Fvector sun_color = g_pGamePersistent->Environment().CurrentEnv->sun_color;
	BOOL bSUN = ps_r2_ls_flags.test(R2FLAG_SUN) && (u_diffuse2s(sun_color.x, sun_color.x, sun_color.x) > EPS);

	if (o.sunstatic)
		bSUN = FALSE;

	if (currentViewPort == SECONDARY_WEAPON_SCOPE)
	{
		Fmatrix m = Device.currentVpSavedView->GetShrinkedFullTransform_saved();
		baseViewFrustum_.CreateFromMatrix(m, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);
	}
	else
		baseViewFrustum_.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

	// MT HOM calc waiting
	if (viewport == firstViewPort)
		HOM.CheckHOM();

	Target->phase_scene_prepare();

	if (currentViewPort == SECONDARY_WEAPON_SCOPE)
		Target->phase_cut();

	vp_stats.preRender_ = T.GetElapsed_sec() * 1000.f; T.Start();

#pragma todo("Do we need separate light storage and occ for secondary vp?")
	// Use deffered lights
	actualViewPortBufferNow->LP_normal = actualViewPortBufferNow->LP_normalNextFrame_;
	actualViewPortBufferNow->LP_pending = actualViewPortBufferNow->LP_pendingNextFrame_;

	// Important to be called before mt lights are started!
	flush_lights_object_occ();

	// Deffered glows
	ProcessGlowsAdding();

	if (viewport == lastViewPort)
	{
		portectTempKinematicsVec_.Enter();

		objectsToCalcBonesCopy_ = objectsToCalcBones_;
		objectsToCalcBones_.clear();

		portectTempKinematicsVec_.Leave();

		portectTempKinematicsVec2_.Enter();

		objectsToCalcBonesNonExactCopy_ = objectsToCalcBonesNonExact_;
		objectsToCalcBonesNonExact_.clear();

		portectTempKinematicsVec2_.Leave();
	}

	CalcWalmarks();

	// Order is important, to minimize dull delays in main thread

	// Send rain visability structure calculation to aux thread, if enabled
	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_RAIN_VIS_STRUCT) && actualViewPortBufferNow->rainCanCalcDeffered_)
		Device.AddToIndAuxThread_1_Pool(fastdelegate::FastDelegate0<>(this, &CRender::prepare_rain_vis_struct));

	// Sent sun casc visability structure calculation to aux thread, if enabled
	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_SUN_VIS_STRUCT) && bSUN && actualViewPortBufferNow->sunCanCalcDeffered_)
		Device.AddToIndAuxThread_1_Pool(fastdelegate::FastDelegate0<>(this, &CRender::calc_sun_cascades_vis_mt));

	// Split light vis workload to make it perform on two threads
	lightsWorkloadSplit_ = int(actualViewPortBufferNow->LP_normal.sortedShadowedCopy_.size() * 0.25f);
	// Send lights visability structures calculation to aux thread, if enabled
	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_LGHTS_VIS_STRUCT))
		Device.AddToIndAuxThread_1_Pool(fastdelegate::FastDelegate0<>(this, &CRender::prepare_lights_mt_1));

	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_LGHTS_VIS_STRUCT))
		Device.AddToIndAuxThread_2_Pool(fastdelegate::FastDelegate0<>(this, &CRender::prepare_lights_mt_2));

	// Second priority visability structure sent to aux thread, if enabled
	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_PRIOR1_VIS_STRUCT))
	{
		mainRenderPrior1DsBuffer_->SetCalculated(false);

		Device.AddToIndAuxThread_3_Pool(fastdelegate::FastDelegate0<>(this, &CRender::render_main_prior_1_mt));
	}

	vp_stats.preCalc_ = T.GetElapsed_sec() * 1000.f; T.Start();


	//Delayed primary geometry

	{
		// A trick: To avoid messing with copying the buffer, which would not work, since render lists(FixedMaps) are not made to be able to be copied,
		// we are going to use two buffers and just switch them up: In a given frame, one is gonna be used to calc and store visible structure, and the second one
		// is the source of visible structure to be drawn. Next frame we just switch the pointers vice versa.
		if (actualViewPortBufferNow->renderPrior0BufferSwitch_ == 0)
		{
			actualViewPortBufferNow->renderPrior0BufferSwitch_ = 1;

			actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_ = actualViewPortBufferNow->mainRenderPrior0DsBuffer1_;
			actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_ = actualViewPortBufferNow->mainRenderPrior0DsBuffer2_;
		}
		else
		{
			actualViewPortBufferNow->renderPrior0BufferSwitch_ = 0;

			actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_ = actualViewPortBufferNow->mainRenderPrior0DsBuffer2_;
			actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_ = actualViewPortBufferNow->mainRenderPrior0DsBuffer1_;
		}
	}

	actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->SetCalculated(false);

	// Determine main geometry we need to draw for next frame
	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_PRIOR0_VIS_STRUCT))
		Device.AddToIndAuxThread_2_Pool(fastdelegate::FastDelegate0<>(this, &CRender::render_main_prior_0_mt));
	else
		render_main_prior_0_mt();


	if (viewport == lastViewPort)
	{
		SetBonesCalced(false);

		if (ps_r_mt_flags.test(R_FLAG_MT_BONES_PRECALC))
			Device.AddToIndAuxThread_2_Pool(fastdelegate::FastDelegate0<>(this, &CRender::CalcBones));
		else
			CalcBones();
	}

	vp_stats.mainGeomVis_ = T.GetElapsed_sec() * 1000.f; T.Start();


	// GPU Sync point. Sync only in first vp

	// If there are two viewports - let the driver to do the sync. Its more effective
	if ((ps_r_cpu_wait_gpu && viewPortsThisFrame.size() == 1))
	{
		CTimer wait_timer; wait_timer.Start();

		{
			BOOL result = FALSE;
			HRESULT	hr = S_FALSE;

			//while	((hr=q_sync_point[q_sync_count]->GetData	(&result,sizeof(result),D3DGETDATA_FLUSH))==S_FALSE) {
			while ((hr = GetData(q_sync_point[q_sync_count], &result, sizeof(result))) == S_FALSE)
			{
				if (!SwitchToThread())
					Sleep(ps_r2_wait_sleep);

				if (wait_timer.GetElapsed_ms() > 500)
				{
					result = FALSE;

					break;
				}
			}
		}
		q_sync_count = (q_sync_count + 1) % HW.Caps.iGPUNum;

		CHK_DX(EndQuery(q_sync_point[q_sync_count]));
	}



	vp_stats.GPU_Sync_Point = T.GetElapsed_sec() * 1000.f; T.Start();


	R_ASSERT(actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_->mapSorted.size() == 0);

	render_phase = PHASE_NORMAL;

	// Set the RT states for scene rendering
	Target->phase_scene_begin();

	// Need to do the hud vis adding before main geometry is drawn. Can't do it in MT
#pragma todo("If need to render weapon in scopes - remove this condition")
	if (viewport == MAIN_VIEWPORT)
	{
		g_hud->Render_First(*actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);
		g_hud->Render_Last(*actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);
	}

	R_ASSERT(actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_->mapHUDSorted.size() == 0);

	r_dsgraph_render_hud(*actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);

	// Draw main geometry visible for viewport, that was calced in previous frame (dynamic)
	r_dsgraph_render_graph_M(0, *actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);

	if (viewport == lastViewPort)
		Device.auxThread_4_Allowed_.Set(); // allow mt kinematics thread to start

	// (static)
	r_dsgraph_render_graph_N(0, *actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);

	vp_stats.mainGeomDraw_ = T.GetElapsed_sec() * 1000.f; T.Start();


	// Occlusion testing of volume-limited light-sources
	Target->disable_aniso();
	Target->phase_occq();
	RCache.set_RT(RImplementation.Target->rt_temp->pRT);
	if(RImplementation.o.dx10_msaa)
      RCache.set_ZB(RImplementation.Target->rt_MSAADepth->pZRT);

	// Rough sorting of lights. We are going to use them in next frame(deffered lights). Also Occ begining for light
	sort_lights();

	// Main render: Part 2. Draw hud and lods
	// Set the RT states for scene rendering (Again)
	Target->phase_scene_begin();

	r_dsgraph_render_lods(true, true, *actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);

	vp_stats.complexStage1_ = T.GetElapsed_sec() * 1000.f; T.Start();


	// Regular (non shadow map) grass rendering
	render_details();

	vp_stats.detailsDraw_ = T.GetElapsed_sec() * 1000.f; T.Start();


	Target->phase_scene_end();

	if (viewport == MAIN_VIEWPORT && g_hud && g_hud->RenderActiveItemUIQuery()) // ui for art detectors and like those
	{
		Target->phase_wallmarks();

		r_dsgraph_render_hud_ui(*actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);
	}

	vp_stats.hudItemsWorldUI_ = T.GetElapsed_sec() * 1000.f; T.Start();


	// Wallmarks
	if(Wallmarks)	
	{
#pragma todo("!!!Need to create a hardware rendering algo for wallmarks. Extreemely slow now. 1 wallmark is ~0.2ms")
		Target->phase_wallmarks();

		Wallmarks->Render(); // wallmarks has priority as normal geometry
	}

	vp_stats.wallmarksDraw_ = T.GetElapsed_sec() * 1000.f; T.Start();


   // full screen pass to mark msaa-edge pixels in highest stencil bit
	if(RImplementation.o.dx10_msaa)
		Target->mark_msaa_edges();

	vp_stats.MSAAEdges_ = T.GetElapsed_sec()*1000.f; T.Start();


	//	TODO: DX10: Implement DX10 rain.
	if (ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF))
	{
		render_rain();

		wetRainDsBuffer_->SetCalculated(false);
	}

	vp_stats.wetSurfacesDraw_ = T.GetElapsed_sec()*1000.f; T.Start();


	// Directional light - fucking sun
	if (bSUN)	
	{
		RImplementation.stats.l_visible ++;

		render_sun_cascades();

		Target->accum_direct_blend();
	}

	vp_stats.sunSmDraw_ = T.GetElapsed_sec() * 1000.f; T.Start();


	{
		Target->phase_accumulator();

		// Render emissive geometry, stencil - write 0x0 at pixel pos
		RCache.set_xform_project	(Device.mProject); 
		RCache.set_xform_view		(Device.mView);

		// Stencil - write 0x1 at pixel pos - 
		if (!RImplementation.o.dx10_msaa)
			RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
		else
			RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x01, 0xff, 0x7f, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);

		//RCache.set_Stencil				(TRUE,D3DCMP_ALWAYS,0x00,0xff,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		RCache.set_CullMode					(CULL_CCW);

		RCache.set_ColorWriteEnable();

		RImplementation.r_dsgraph_render_emissive(*actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);
		RImplementation.r_dsgraph_render_hud_emissive(*actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_);
	}

	vp_stats.emissiveGeom_ = T.GetElapsed_sec() * 1000.f;	T.Start();


	// MT Details must not run during this!!

	// Copy actual lights for mt details shadow map calc

#ifdef DEBUG
	for (u32 y = 0; y < actualViewPortBufferNow->LP_normal.sortedShadowed_.size(); y++)
	{
		R_ASSERT(actualViewPortBufferNow->LP_normal.sortedShadowed_[y]);

		if (actualViewPortBufferNow->LP_normal.sortedShadowed_[y])
			R_ASSERT(actualViewPortBufferNow->LP_normal.sortedShadowed_[y]->lightDsGraphBuffer_);
	}
#endif

#pragma todo ("todo: Check if we need to also check secondary view prots lights")
	if(viewport == MAIN_VIEWPORT)
		Details->lightsToCheck_ = actualViewPortBufferNow->LP_normal.sortedShadowed_;

	// Sort by distance to cam
	try // temporary
	{
		std::sort(Details->lightsToCheck_.begin(), Details->lightsToCheck_.end(), dist_to_cam);
	}
	catch (...)
	{
		MessageBox(NULL, "Details crash sorting", "Error", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST);
	}

	for (u32 i = 0; i < Details->lightsToCheck_.size(); i++)
		Details->lightsToCheck_[i]->X.S.combine_mt_copy = Details->lightsToCheck_[i]->X.S.combine; // copy matrix for mt safty in MT_DETAILS

	// Lighting, not waiting occ result
	{
		Target->phase_accumulator();
		/*ID3D11ShaderResourceView*null = 0;
		HW.pContext->PSSetShaderResources(0, 1, &null);
		Target->u_setrt(Target->rt_temp, 0, 0);*/
		render_lights(actualViewPortBufferNow->LP_normal); // We are actually rendering lights from 2 frames ago
	}

	vp_stats.lightsNormalTime_ = T.GetElapsed_sec() * 1000.f;	T.Start();


	// Lighting, waiting occ result
	{
		/*ID3D11ShaderResourceView*null = 0;
		HW.pContext->PSSetShaderResources(0, 1, &null);
		Target->u_setrt(Target->rt_temp, 0, 0);*/
		render_lights(actualViewPortBufferNow->LP_pending); // We are actually rendering lights from 2 frames ago
	}

	vp_stats.lightsPendingTime_ = T.GetElapsed_sec() * 1000.f; T.Start();


	// Postprocess and second priority geometry
	{
		Target->phase_combine();
	}

	vp_stats.combinerAnd2ndGeom_ = T.GetElapsed_sec() * 1000.f; T.Start();


	// Prepare stuff and also wait occ results for next frame lights
	calc_shadowed_deffered_lights(actualViewPortBufferNow->LP_normalNextFrame_); // update and sort shadowed lights for nex frame
	calc_shadowed_deffered_lights(actualViewPortBufferNow->LP_pendingNextFrame_); // update and sort shadowed lights for nex frame
	lights_occ_get(actualViewPortBufferNow->LP_pendingNextFrame_);

	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_PRIOR0_VIS_STRUCT))
	{
		SyncPrimaryPriorCalcMT();
	}

	if (ps_r_mt_flags.test(R_FLAG_MT_BONES_PRECALC))
	{
		SyncBonesCalcMT();
	}

	// Dynamic objects aproximate lum
	CalcLuminocity(*actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_);

	if (viewport == lastViewPort)
		CPU_wait_GPU_lastFrame_ = Device.Statistic->viewPortStats1.GPU_Sync_Point + Device.Statistic->viewPortStats2.GPU_Sync_Point;

	vp_stats.postCalc_ = T.GetElapsed_sec() * 1000.f;

	vp_stats.RenderTotalTime = TT.GetElapsed_sec() * 1000.f;

	if (viewport == lastViewPort)
	{
		RDEVICE.Statistic->occ_pool_size = HWOCC.GetPoolSize();
		RDEVICE.Statistic->occ_used_size = HWOCC.GetUsedSize();
		RDEVICE.Statistic->occ_freed_ids_size = HWOCC.GetFreedIdsSize();

		if (HWOCC.GetPoolSize() < 256)
			ResetHWOcclusion();
	}
}


void CRender::render_forward()
{
	//******* Main render - second order geometry (the one, that doesn't support deffering)
	//.todo: should be done inside "combine" with estimation of of luminance, tone-mapping, etc.
	{
		render_phase = PHASE_NORMAL;

		// Wait for vis structure for second priority
		SyncSecondaryPriorrCalcMT();

		//	Igor: we don't want to render old lods on next frame.
		mainRenderPrior1DsBuffer_->mapLOD.clear();

		// Do hud adding again, to get forward geometry. Can't do it in MT
		if (currentViewPort == MAIN_VIEWPORT)
		{
			g_hud->Render_First(*mainRenderPrior1DsBuffer_);
			g_hud->Render_Last(*mainRenderPrior1DsBuffer_);
		}

		R_ASSERT(mainRenderPrior1DsBuffer_->mapEmissive.size() == 0);
		R_ASSERT(mainRenderPrior1DsBuffer_->mapHUDEmissive.size() == 0);

		r_dsgraph_render_graph(1, *mainRenderPrior1DsBuffer_); // normal level, secondary priority

		protectMTUnsafeRendering_.Enter();

		PortalTraverser.fade_render();	// faded-portals

		protectMTUnsafeRendering_.Leave();

		r_dsgraph_render_sorted(*mainRenderPrior1DsBuffer_); // strict-sorted geoms of forward scene
		r_dsgraph_render_hud_sorted(*mainRenderPrior1DsBuffer_); // hud sorted

		if (g_pGameLevel)
			g_pGameLevel->RenderTracers();

		if (L_Glows && ps_r2_ls_flags_ext.test(R2FLAGEXT_GLOWS_ENABLE))
			L_Glows->Render(); // glows

		g_pGamePersistent->Environment().RenderLast();// rain/thunder-bolts
	}

	mainRenderPrior1DsBuffer_->SetCalculated(false);
}


void CRender::RenderFinish()
{

}

void CRender::SyncPrimaryPriorCalcMT()
{
	CTimer dull_waitng_timer; dull_waitng_timer.Start();

	while (!actualViewPortBufferNow->mainRenderPrior0CalculatingDsBuffer_->IsCalculated())
		Sleep(0);

	float dull_time = dull_waitng_timer.GetElapsed_sec() * 1000.f;

	vpStats->RenderMtWait += dull_time;

#ifdef MEASURE_MT
	Device.Statistic->mtRenderDelayPrior0_ += dull_time;
#endif
}

void CRender::SyncSecondaryPriorrCalcMT()
{
	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_PRIOR1_VIS_STRUCT))
	{
		CTimer dull_waitng_timer; dull_waitng_timer.Start();

		while (!mainRenderPrior1DsBuffer_->IsCalculated())
			Sleep(0);

		float dull_time = dull_waitng_timer.GetElapsed_sec() * 1000.f;

		vpStats->RenderMtWait += dull_time;

#ifdef MEASURE_MT
		Device.Statistic->mtRenderDelayPrior1_ += dull_time;
#endif
	}
	else
		render_main_prior_1_mt();
}

void CRender::SyncBonesCalcMT()
{
	CTimer dull_waitng_timer; dull_waitng_timer.Start();

	while (!IsBonesCalced())
		Sleep(0);

	float dull_time = dull_waitng_timer.GetElapsed_sec() * 1000.f;

	vpStats->RenderMtWait += dull_time;

#ifdef MEASURE_MT
	Device.Statistic->mtRenderDelayBones_ += dull_time;
#endif
}

void CRender::CalcLuminocity(DsGraphBuffer& buffer)
{
	// update light-vis for current entity / actor
	CObject* O = g_pGameLevel->CurrentViewEntity();

	if (O)
	{
		CROS_impl* R = (CROS_impl*)O->ROS();

		if (R)
			R->update_ros(O);
	}

	// update light-vis for entities
	// track lighting environment

	if (uLastLTRACK + 1 >= buffer.renderables_.size())
		uLastLTRACK = 0;

	for (u32 i = uLastLTRACK, y = 0; i < buffer.renderables_.size() && y < 50; i++, y++)
	{
		IRenderable* renderable = buffer.renderables_[i]->dcast_Renderable();

		if (renderable && !renderable->renderable.needToDelete)
		{
			CROS_impl* T = (CROS_impl*)renderable->renderable_ROS();

			if (T)
				T->update_ros(renderable);
		}

		uLastLTRACK = i;
	}
}

void CRender::ProcessGlowsAdding()
{
	if (!ps_r2_ls_flags_ext.test(R2FLAGEXT_GLOWS_ENABLE))
		return;

	for (u32 i = 0; i < actualViewPortBufferNow->defferedGlowsVect_.size(); i++)
	{
		if (actualViewPortBufferNow->defferedGlowsVect_[i].glow)
		{
			if (actualViewPortBufferNow->defferedGlowsVect_[i].visible)
				L_Glows->add(actualViewPortBufferNow->defferedGlowsVect_[i].glow);
			else
				actualViewPortBufferNow->defferedGlowsVect_[i].glow->hide_glow();
		}
	}

	actualViewPortBufferNow->defferedGlowsVect_.clear();
}

void CRender::CalcBones()
{
#ifdef MEASURE_MT
	CTimer T; T.Start();
#endif

	for (u32 i = 0; i < objectsToCalcBonesCopy_.size(); i++)
	{
		if (objectsToCalcBonesCopy_[i])
			objectsToCalcBonesCopy_[i]->CalculateBones(BonesCalcType::need_actual);
	}

	objectsToCalcBonesCopy_.clear();

	for (u32 i = 0; i < objectsToCalcBonesNonExactCopy_.size(); i++)
	{
		if (objectsToCalcBonesNonExactCopy_[i])
			objectsToCalcBonesNonExactCopy_[i]->CalculateBones();
	}

	objectsToCalcBonesNonExactCopy_.clear();

	SetBonesCalced(true);

#ifdef MEASURE_MT
	if (ps_r_mt_flags.test(R_FLAG_MT_BONES_PRECALC)) Device.Statistic->mtMeshBones_ += T.GetElapsed_sec();
#endif
}

void CRender::CalcWalmarks()
{
	//std::random_shuffle(actualViewPortBufferNow->objectsToCalcWallMarks_.begin(), actualViewPortBufferNow->objectsToCalcWallMarks_.end()); // might help to minize simultanius drawing and bones calculating sync time

	for (u32 i = 0; i < actualViewPortBufferNow->objectsToCalcWallMarks_.size(); i++)
	{
		if (actualViewPortBufferNow->objectsToCalcWallMarks_[i])
			actualViewPortBufferNow->objectsToCalcWallMarks_[i]->CalculateWallmarks(*actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_, actualViewPortBufferNow->mainRenderPrior0DrawingDsBuffer_->dsgraphBufferViewFrustum_);
	}

	actualViewPortBufferNow->objectsToCalcWallMarks_.clear();
}

// Перед началом прохода рендера
void CRender::BeforeViewRender(ViewPort viewport)
{

}

// После прохода рендера и пост-эффектов --#SM+#--
ENGINE_API extern BOOL debugSecondViewPort;

void CRender::AfterViewRender(ViewPort viewport)
{
	if (RImplementation.currentViewPort == SECONDARY_WEAPON_SCOPE)
	{
		// We are now copying the HW.pBaseRT surface into svp image target surface. We can't use swapchain buffer here, since its not being used in svp rendering
		ID3DResource* res;
		HW.pBaseRT->GetResource(&res);

		HW.pContext->CopyResource(Target->rt_secondVP->pSurface, res); // rt sizes must match, to be able to copy
	}

	if (debugSecondViewPort && RImplementation.currentViewPort == MAIN_VIEWPORT) // Copy svp image into swapchain buffer((MAIN_VIEWPORT).baseRT) to draw it on screen
	{
		ID3DResource* res = Target->rt_secondVP->pSurface;
		ID3DResource* res2;
		HW.viewPortHWResList.at(MAIN_VIEWPORT).baseRT->GetResource(&res2);

		D3D11_BOX sourceRegion;
		sourceRegion.left = 0;
		sourceRegion.right = Device.m_SecondViewport.screenWidth;
		sourceRegion.top = 0;
		sourceRegion.bottom = Device.m_SecondViewport.screenHeight;
		sourceRegion.front = 0;
		sourceRegion.back = 1;

		HW.pContext->CopySubresourceRegion(res2, 0, 0, 0, 0, res, 0, &sourceRegion);
	}
}