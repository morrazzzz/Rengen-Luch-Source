#include "stdafx.h"

void CRender::prepare_lights_mt_1()
{
#ifdef MEASURE_MT
	CTimer T; T.Start();
#endif


	// build visability structure for normal lights(some part)
	if (actualViewPortBufferNow->LP_normal.sortedShadowedCopy_.size() > 0)
	{
		// Calculate one part of workload, other part is done in other thread (lightsWorkloadSplit_)
		for (int i = (int)actualViewPortBufferNow->LP_normal.sortedShadowedCopy_.size() - 1; i >= lightsWorkloadSplit_; i--) // backward is important to avoid delays, since lights are rendered back to front
		{
			CLightSource* L = actualViewPortBufferNow->LP_normal.sortedShadowedCopy_[i]; // We are using a copy of the pool, to avoid interfearing with light renderings

			build_light_vis_struct(L);
		}
	}


#ifdef MEASURE_MT
	Device.Statistic->mtVisStructTime_ += T.GetElapsed_sec();
#endif
}

void CRender::prepare_lights_mt_2()
{
#ifdef MEASURE_MT
	CTimer T; T.Start();
#endif


	// build visability structure for normal lights(another part)
	if (actualViewPortBufferNow->LP_normal.sortedShadowedCopy_.size() > 0)
	{
		// Calculate one part of workload, other part is done in other thread (lightsWorkloadSplit_)
		for (int i = (int)lightsWorkloadSplit_ - 1; i >= 0; i--) // backward is important to avoid delays, since lights are rendered back to front
		{
			CLightSource* L = actualViewPortBufferNow->LP_normal.sortedShadowedCopy_[i]; // We are using a copy of the pool, to avoid interfearing with light renderings

			build_light_vis_struct(L);
		}
	}

	// build visability structure for pending lights
	for (int i = (int)actualViewPortBufferNow->LP_pending.sortedShadowedCopy_.size() - 1; i >= 0; i--) // backward is important to avoid delays, since lights are rendered back to front
	{
		CLightSource* L = actualViewPortBufferNow->LP_pending.sortedShadowedCopy_[i]; // We are using a copy of the pool, to avoid interfearing with light renderings

		build_light_vis_struct(L);
	}


#ifdef MEASURE_MT
	Device.Statistic->mtVisStructTime_ += T.GetElapsed_sec();
#endif
}

void CRender::sort_lights()
{
	actualViewPortBufferNow->LP_normalNextFrame_.ClearPackage();
	actualViewPortBufferNow->LP_pendingNextFrame_.ClearPackage();

#ifdef DEBUG
	for (u8 i = 0; i < 3; i++)
	{
		xr_vector<CLightSource*>& storage = i == 0 ? Lights.ldbTargetViewPortBuffer->rawPackageDeffered_.v_point : i == 1 ? Lights.ldbTargetViewPortBuffer->rawPackageDeffered_.v_shadowed : Lights.ldbTargetViewPortBuffer->rawPackageDeffered_.v_spot;

		for (u32 y = 0; y < storage.size(); y++)
		{
			R_ASSERT(storage[y]);

			if (storage[y])
				R_ASSERT(storage[y]->lightDsGraphBuffer_);
		}
	}
#endif

	{
		PIX_EVENT(DEFER_TEST_LIGHT_VIS);

		// perform tests
		u32	count = 0;
		light_Package& LP = Lights.ldbTargetViewPortBuffer->rawPackageDeffered_;

		// stats
		stats.l_shadowed = LP.v_shadowed.size();
		stats.l_unshadowed = LP.v_point.size() + LP.v_spot.size();
		stats.l_total = stats.l_shadowed + stats.l_unshadowed;

		// perform tests
		count = _max(count, LP.v_point.size());
		count = _max(count, LP.v_spot.size());
		count = _max(count, LP.v_shadowed.size());

		for (u32 it = 0; it < count; it++)
		{
			if (it < LP.v_point.size())
			{
				CLightSource* L = LP.v_point[it];

				R_ASSERT(L);
				R_ASSERT(L->lightDsGraphBuffer_);

				L->vis_prepare();

				if (L->vis.pending)
					actualViewPortBufferNow->LP_pendingNextFrame_.v_point.push_back(L);
				else
					actualViewPortBufferNow->LP_normalNextFrame_.v_point.push_back(L);
			}

			if (it < LP.v_spot.size())
			{
				CLightSource* L = LP.v_spot[it];

				R_ASSERT(L);
				R_ASSERT(L->lightDsGraphBuffer_);

				L->vis_prepare();

				if (L->vis.pending)
					actualViewPortBufferNow->LP_pendingNextFrame_.v_spot.push_back(L);
				else
					actualViewPortBufferNow->LP_normalNextFrame_.v_spot.push_back(L);
			}

			if (it< LP.v_shadowed.size())
			{
				CLightSource* L = LP.v_shadowed[it];

				R_ASSERT(L);
				R_ASSERT(L->lightDsGraphBuffer_);

				L->vis_prepare();

				if (L->vis.pending)
					actualViewPortBufferNow->LP_pendingNextFrame_.v_shadowed.push_back(L);
				else
					actualViewPortBufferNow->LP_normalNextFrame_.v_shadowed.push_back(L);
			}
		}
	}
}

void CRender::flush_lights_object_occ()
{
	// Check the occ result for static geometry for last frame lights
	{
		PIX_EVENT(DEFER_FLUSH_OCCLUSION);

		u32 it = 0;

		for (it = 0; it < actualViewPortBufferNow->Lights_LastFrame.size(); it++)
		{
			if (!actualViewPortBufferNow->Lights_LastFrame[it])
				continue;

			if (actualViewPortBufferNow->Lights_LastFrame[it]->get_use_static_occ())
			{
				actualViewPortBufferNow->Lights_LastFrame[it]->svis.flushoccq();
			}
		}

		actualViewPortBufferNow->Lights_LastFrame.clear();
	}
}

void CRender::build_light_vis_struct(CLightSource* light)
{
	R_ASSERT(light);
	R_ASSERT(light->lightDsGraphBuffer_);

	light->protectMtLightCalc_.Enter();

	if (!light->lightDsGraphBuffer_->IsCalculated() && light->canCalcDeffered_)
	{
		light->lightDsGraphBuffer_->useHOM_ = !!ps_r_misc_flags.test(R_MISC_USE_HOM_LIGHT);
		light->lightDsGraphBuffer_->portalTraverserOpt = (light->lightDsGraphBuffer_->useHOM_ ? CPortalTraverser::VQ_HOM : 0) + CPortalTraverser::VQ_SSA;

		if (RImplementation.o.Tshadows)
		{
			light->lightDsGraphBuffer_->phaseMask[0] = true;
			light->lightDsGraphBuffer_->phaseMask[1] = true;
			light->lightDsGraphBuffer_->phaseMask[2] = false;
		}
		else
		{
			light->lightDsGraphBuffer_->phaseMask[0] = true;
			light->lightDsGraphBuffer_->phaseMask[1] = false;
			light->lightDsGraphBuffer_->phaseMask[2] = false;
		}

		light->lightDsGraphBuffer_->phaseType_ = PHASE_SMAP;

		if (light->get_use_static_occ()) // Lights with changable position or propeties should ignore occ check for static geometry vis
		{
			light->svis.smvisbegin(*light->lightDsGraphBuffer_); // Important to do
		}

		CFrustum temp;
		temp.CreateFromMatrix(light->X.S.combine, FRUSTUM_P_ALL &(~FRUSTUM_P_NEAR));

		light->lightDsGraphBuffer_->dsgraphBufferViewFrustum_ = temp;

		render_subspace_MT(light->lightDsGraphBuffer_, light->GetCurrentSector(), light->X.S.combine, light->position, true);

		light->lightDsGraphBuffer_->SetCalculated(true);
	}

	light->protectMtLightCalc_.Leave();
}

void CRender::render_lights(light_Package& LP)
{
	PIX_EVENT(SHADOWED_LIGHTS);

	render_phase = PHASE_SMAP;

	while (LP.sortedShadowed_.size()) // use the sorted ones, which we sorted and updated in previous frame
	{
		// if (has_spot_shadowed)
		xr_vector<CLightSource*> L_spot_s;

		stats.s_used++;

		// generate spot shadowmap
		Target->phase_smap_spot_clear();

		xr_vector<CLightSource*>& source = LP.sortedShadowed_;

		CLightSource* L = source.back();

		u16 sid = L->vis.smap_ID;

		while (true)
		{
			if (source.empty())
				break;

			L = source.back();

			if (L->vis.smap_ID != sid)
				break;

			source.pop_back();

			R_ASSERT(L);
			R_ASSERT(L->lightDsGraphBuffer_);

			actualViewPortBufferNow->Lights_LastFrame.push_back(L);

			// Sync MT vis struct calc
			if (ps_r_mt_flags.test(R_FLAG_MT_CALC_LGHTS_VIS_STRUCT) && L->canCalcDeffered_)
			{
				CTimer dull_waitng_timer; dull_waitng_timer.Start();
						
				SyncLightMT(*L->lightDsGraphBuffer_);

				float dull_time = dull_waitng_timer.GetElapsed_sec() * 1000.f;

				vpStats->RenderMtWait += dull_time;

#ifdef MEASURE_MT
				Device.Statistic->mtRenderDelayLights_ += dull_time;
#endif
			}
			else
			{
				L->canCalcDeffered_ = true;

				build_light_vis_struct(L);
			}

			// Render

			bool bNormal = L->lightDsGraphBuffer_->mapNormalPasses[0][0].size() || L->lightDsGraphBuffer_->mapMatrixPasses[0][0].size();
			bool bSpecial = L->lightDsGraphBuffer_->mapNormalPasses[1][0].size() || L->lightDsGraphBuffer_->mapMatrixPasses[1][0].size() || L->lightDsGraphBuffer_->mapSorted.size();

			if (bNormal || bSpecial)
			{
				stats.s_merged++;

				L_spot_s.push_back(L);

				Target->phase_smap_spot(L);

				RCache.set_xform_world(Fidentity);
				RCache.set_xform_view(L->X.S.view);
				RCache.set_xform_project(L->X.S.project);

				r_dsgraph_render_graph(0, *L->lightDsGraphBuffer_); // Render geometry

				L->X.S.transluent = FALSE;

				if (ps_r2_ls_flags.test(ps_light_details_shadow && L->usefullForDetailsShadows_))
				{
					Details->Render(L->m_shadowed_details); // Render shadows for grass nearby this light
				}

				if (bSpecial)
				{
					L->X.S.transluent = TRUE;
					Target->phase_smap_spot_tsh(L);

					PIX_EVENT(SHADOWED_LIGHTS_RENDER_GRAPH);

					r_dsgraph_render_graph(1, *L->lightDsGraphBuffer_); // normal level, secondary priority

					PIX_EVENT(SHADOWED_LIGHTS_RENDER_SORTED);

					r_dsgraph_render_sorted(*L->lightDsGraphBuffer_); // strict-sorted geoms
				}

			}
			else
			{
				stats.s_finalclip++;
			}

			if (L->get_use_static_occ())
			{
				L->svis.smvisend(*L->lightDsGraphBuffer_);
			}

			L->canCalcDeffered_ = true;
			L->lightDsGraphBuffer_->SetCalculated(false);
		}

		PIX_EVENT(UNSHADOWED_LIGHTS);

		//switch-to-accumulator
		Target->phase_accumulator();

		PIX_EVENT(POINT_LIGHTS);

		// if (has_point_unshadowed)	-> 	accum point unshadowed

		if (!LP.v_point.empty())
		{
			CLightSource* p_light = LP.v_point.back();

			LP.v_point.pop_back();

			R_ASSERT(p_light);
			R_ASSERT(p_light->lightDsGraphBuffer_);

			if (p_light->vis.visible)
			{
				Target->accum_point(p_light);
				render_indirect(p_light);
			}
		}

		PIX_EVENT(SPOT_LIGHTS);

		// if (has_spot_unshadowed)	-> accum spot unshadowed
		if (!LP.v_spot.empty())
		{
			CLightSource* s_light = LP.v_spot.back();

			LP.v_spot.pop_back();

			R_ASSERT(s_light);
			R_ASSERT(s_light->lightDsGraphBuffer_);

			if (s_light->vis.visible)
			{
				LR.compute_xf_spot(s_light);
				Target->accum_spot(s_light);

				render_indirect(s_light);
			}
		}

		PIX_EVENT(SPOT_LIGHTS_ACCUM_VOLUMETRIC);

		// if (was_spot_shadowed) -> accum spot shadowed

		if (!L_spot_s.empty())
		{
			PIX_EVENT(ACCUM_SPOT);

			for (u32 it = 0; it<L_spot_s.size(); it++)
			{
				R_ASSERT(L_spot_s[it]);
				R_ASSERT(L_spot_s[it]->lightDsGraphBuffer_);

				Target->accum_spot(L_spot_s[it]);
				render_indirect(L_spot_s[it]);
			}

			PIX_EVENT(ACCUM_VOLUMETRIC);

			if (RImplementation.o.advancedpp && ps_r2_ls_flags.is(R2FLAG_VOLUMETRIC_LIGHTS))
				for (u32 it = 0; it < L_spot_s.size(); it++)
				{
					R_ASSERT(L_spot_s[it]);
					R_ASSERT(L_spot_s[it]->lightDsGraphBuffer_);

					Target->accum_volumetric(L_spot_s[it]);
				}

			L_spot_s.clear();
		}
	}

	PIX_EVENT(POINT_LIGHTS_ACCUM);

	// Point lighting (unshadowed, if left)
	if (!LP.v_point.empty())
	{
		xr_vector<CLightSource*>&	Lvec = LP.v_point;

		for (u32 pid = 0; pid<Lvec.size(); pid++)
		{
			R_ASSERT(Lvec[pid]);
			R_ASSERT(Lvec[pid]->lightDsGraphBuffer_);

			if (Lvec[pid]->vis.visible)
			{
				render_indirect(Lvec[pid]);
				Target->accum_point(Lvec[pid]);
			}
		}

		Lvec.clear();
	}

	PIX_EVENT(SPOT_LIGHTS_ACCUM);

	// Spot lighting (unshadowed, if left)
	if (!LP.v_spot.empty())
	{
		xr_vector<CLightSource*>&	Lvec = LP.v_spot;

		for (u32 pid = 0; pid<Lvec.size(); pid++)
		{
			R_ASSERT(Lvec[pid]);
			R_ASSERT(Lvec[pid]->lightDsGraphBuffer_);

			if (Lvec[pid]->vis.visible)
			{
				LR.compute_xf_spot(Lvec[pid]);
				render_indirect(Lvec[pid]);
				Target->accum_spot(Lvec[pid]);
			}
		}

		Lvec.clear();
	}
}

void CRender::render_indirect(CLightSource* L)
{
	if (!ps_r2_ls_flags.test(R2FLAG_GI))
		return;

	CLightSource							LIGEN;

	LIGEN.set_type							(IRender_Light::REFLECTED);
	LIGEN.set_shadow						(false);
	LIGEN.set_cone							(PI_DIV_2 * 2.f);


	xr_vector<light_indirect>& Lvec = L->indirect;

	if (Lvec.empty())
		return;

	float LE = L->color.intensity();

	for (u32 it = 0; it < Lvec.size(); it++)
	{
		light_indirect&	LI = Lvec[it];

		// energy and color
		float LIE = LE * LI.E;

		if (LIE < ps_r2_GI_clip)
			continue;

		Fvector T;
		T.set(L->color.r, L->color.g, L->color.b).mul(LI.E);

		LIGEN.set_color					(T.x, T.y, T.z);

		// geometric
		Fvector L_up, L_right;
		L_up.set(0, 1, 0);

		if (_abs(L_up.dotproduct(LI.D)) > .99f)
			L_up.set(0, 0, 1);

		L_right.crossproduct			(L_up, LI.D).normalize();

		LIGEN.spatial.current_sector	= LI.S;
		LIGEN.set_position				(LI.P);
		LIGEN.set_rotation				(LI.D, L_right);

		// range
		// dist^2 / range^2 = A - has infinity number of solutions
		// approximate energy by linear fallof Emax / (1 + x) = Emin
		float Emax					= LIE;
		float Emin					= 1.f / 255.f;
		float x						= (Emax - Emin)/Emin;

		if(x < 0.1f)
			continue;

		LIGEN.set_range					(x);

		Target->accum_reflected			(&LIGEN);
	}

	LIGEN.readyToDestroy_ = true;
}

IC bool pred_area(CLightSource* _1, CLightSource* _2)
{
	u32		a0 = _1->X.S.size;
	u32		a1 = _2->X.S.size;

	return	a0 > a1; // reverse -> descending
}

void CRender::calc_shadowed_deffered_lights(light_Package& LP)
{
	// Deffered sorting - sort lights and render them in next frame

	//////////////////////////////////////////////////////////////////////////
	// sort lights by importance???
	// while (has_any_lights_that_cast_shadows) {
	//		if (has_point_shadowed)		->	generate point shadowmap
	//		if (has_spot_shadowed)		->	generate spot shadowmap
	//		switch-to-accumulator
	//		if (has_point_unshadowed)	-> 	accum point unshadowed
	//		if (has_spot_unshadowed)	-> 	accum spot unshadowed
	//		if (was_point_shadowed)		->	accum point shadowed
	//		if (was_spot_shadowed)		->	accum spot shadowed
	//	}
	//	if (left_some_lights_that_doesn't cast shadows)
	//		accumulate them
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Refactor order based on ability to pack shadow-maps
	// 1. calculate area + sort in descending order
	// const	u16		smap_unassigned		= u16(-1);
	{
		xr_vector<CLightSource*>& source = LP.v_shadowed;

		for (u32 it = 0; it<source.size(); it++)
		{
			CLightSource* L = source[it];

			L->vis_update();

			if (!L->vis.visible)
			{
				source.erase(source.begin() + it);
				it--;
			}
			else
			{
				LR.compute_xf_spot(L);
			}
		}
	}

	// 2. refactor - infact we could go from the backside and sort in ascending order
	{
		xr_vector<CLightSource*>& source = LP.v_shadowed;

		LP.sortedShadowed_.clear();
		LP.sortedShadowed_.reserve(source.size());

		u32 total = source.size();

		for (u16 smap_ID = 0; LP.sortedShadowed_.size() != total; smap_ID++)
		{
			LP_smap_pool.initialize(RImplementation.o.smapsize);

			std::sort(source.begin(), source.end(), pred_area);

			for (u32 test = 0; test<source.size(); test++)
			{
				CLightSource* L = source[test];
				SMAP_Rect R;

				if (LP_smap_pool.push(R, L->X.S.size))
				{
					// OK
					L->X.S.posX = R.min.x;
					L->X.S.posY = R.min.y;
					L->vis.smap_ID = smap_ID;

					LP.sortedShadowed_.push_back(L);
					source.erase(source.begin() + test);

					test--;
				}
			}
		}

		// save (lights are popped from back)
		std::reverse(LP.sortedShadowed_.begin(), LP.sortedShadowed_.end());

		LP.sortedShadowedCopy_ = LP.sortedShadowed_;
	}
}

void CRender::lights_occ_get(light_Package& LP)
{
	for (u32 i = 0; i < LP.v_point.size(); i++)
	{
		CLightSource* L = LP.v_point[i];

		VERIFY(L);

		L->vis_update();
	}

	for (u32 i = 0; i < LP.v_spot.size(); i++)
	{
		CLightSource* L = LP.v_point[i];

		VERIFY(L);

		L->vis_update();
	}
}

void CRender::SyncLightMT(DsGraphBuffer& dsbuffer)
{
	while (!dsbuffer.IsCalculated())
	{
		Sleep(0);
	}
}