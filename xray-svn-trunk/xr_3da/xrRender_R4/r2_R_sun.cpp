#include "stdafx.h"

#include "../xrRender/FBasicVisual.h"
#include "r4_R_sun_support.h"

float			OLES_SUN_LIMIT_27_01_07			= 100.f		;
const float tweak_COP_initial_offs = 1200.f;

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static Fvector3 corners [8] = 
{
	{ -1, -1,  0 },		{ -1, -1, +1},
	{ -1, +1, +1 },		{ -1, +1,  0},
	{ +1, +1, +1 },		{ +1, +1,  0},
	{ +1, -1, +1},		{ +1, -1,  0}
};

static int facetable[6][4] =
{
	{ 6, 7, 5, 4 },		{ 1, 0, 7, 6 },
	{ 1, 2, 3, 0 },		{ 3, 2, 4, 5 },		
	// near and far planes
	{ 0, 3, 5, 7 },		{  1, 6, 4, 2 },
};

Fvector3 wform(Fmatrix& m, Fvector3 const& v)
{
	Fvector4	r;
	r.x			= v.x*m._11 + v.y*m._21 + v.z*m._31 + m._41;
	r.y			= v.x*m._12 + v.y*m._22 + v.z*m._32 + m._42;
	r.z			= v.x*m._13 + v.y*m._23 + v.z*m._33 + m._43;
	r.w			= v.x*m._14 + v.y*m._24 + v.z*m._34 + m._44;
	// VERIFY		(r.w>0.f);
	float invW = 1.0f/r.w;
	Fvector3	r3 = { r.x*invW, r.y*invW, r.z*invW };
	return		r3;
}


void CRender::init_cacades()
{
	u32 cascade_count = 3;
	m_sun_cascades.resize(cascade_count);

	float fBias = -0.0000025f;
	m_sun_cascades[0].reset_chain = true;
	m_sun_cascades[0].size = 8;
	m_sun_cascades[0].bias = m_sun_cascades[0].size*fBias;

	m_sun_cascades[1].size = 32;
	m_sun_cascades[1].bias = m_sun_cascades[1].size*fBias;

 	m_sun_cascades[2].size = 192;
 	m_sun_cascades[2].bias = m_sun_cascades[2].size*fBias;


	for (u8 i = 0; i < 3; i++)
	{
		SSunCascadeBuffer* temp = xr_new<SSunCascadeBuffer>();
		viewPortBuffer1.sunCascadesRuntime_.push_back(temp);
		temp = xr_new<SSunCascadeBuffer>();
		viewPortBuffer2.sunCascadesRuntime_.push_back(temp);

		DsGraphBuffer* buff = xr_new<DsGraphBuffer>();
		buff->debugName.printf("sun cascade dsgraph_buffer %u %u", i, buff->visMarkerId_);
		sunCascedsDsBuffersVect_.push_back(buff);
	}

	viewPortBuffer1.sunCanCalcDeffered_ = false;
	viewPortBuffer2.sunCanCalcDeffered_ = false;
}

void CRender::delete_cascades_data()
{
	for (u8 i = 0; i < 3; i++)
	{
		xr_delete(viewPortBuffer1.sunCascadesRuntime_[i]);
		xr_delete(viewPortBuffer2.sunCascadesRuntime_[i]);

		xr_delete(sunCascedsDsBuffersVect_[i]);
	}
}

void CRender::render_sun_cascades()
{
	bool b_need_to_render_sunshafts = RImplementation.Target->need_to_render_sunshafts();
	bool last_cascade_chain_mode = m_sun_cascades.back().reset_chain;

	m_sun_cascades[0].size = ps_r2_sun_shadows_near_casc;
	m_sun_cascades[1].size = ps_r2_sun_shadows_midl_casc;
	m_sun_cascades[2].size = ps_r2_sun_shadows_far_casc;

	if (b_need_to_render_sunshafts)
		m_sun_cascades[m_sun_cascades.size() - 1].reset_chain = true;

	for (u32 i = 0; i < m_sun_cascades.size(); ++i)
		render_sun_cascade(i);

	if (b_need_to_render_sunshafts)
		m_sun_cascades[m_sun_cascades.size() - 1].reset_chain = last_cascade_chain_mode;

	actualViewPortBufferNow->sunCanCalcDeffered_ = true;
}

void CRender::render_sun_cascade(u32 cascade_ind)
{
	// Check if mt has calculated vis struct or calculate it ourself if mt is disabled
	SyncSunCascadeMT(cascade_ind);

	// Prepare the variables
	sun_cascade_prerender(cascade_ind);

	if (!actualViewPortBufferNow->sunCanCalcDeffered_) // calculate vis struct ourself if deffered is not allowed
		calc_sun_cascade_vis_struct(cascade_ind);

	CLightSource* fuckingsun = (CLightSource*)Lights.sun_adapted._get();

	// Render shadow-map
	render_phase = PHASE_SMAP;

	//. !!! We should clip based on shrinked frustum (again)
	{
		bool bNormal = sunCascedsDsBuffersVect_[cascade_ind]->mapNormalPasses[0][0].size() || sunCascedsDsBuffersVect_[cascade_ind]->mapMatrixPasses[0][0].size();
		bool bSpecial = sunCascedsDsBuffersVect_[cascade_ind]->mapNormalPasses[1][0].size() || sunCascedsDsBuffersVect_[cascade_ind]->mapMatrixPasses[1][0].size() || sunCascedsDsBuffersVect_[cascade_ind]->mapSorted.size();
		
		if (bNormal || bSpecial)
		{
			Target->phase_smap_direct(fuckingsun, SE_SUN_FAR);

			RCache.set_xform_world(Fidentity);
			RCache.set_xform_view(Fidentity);
			RCache.set_xform_project(fuckingsun->X.D.combine);

			r_dsgraph_render_graph(0, *sunCascedsDsBuffersVect_[cascade_ind]); // draw prepared geometry into shadow map

			if (ps_r2_ls_flags.test(R2FLAG_DETAILS_SUN_SHADOWS))
				if (cascade_ind == 0) // Only render in first cascade
				{ 
					// Render shortened details pool
					if(currentViewPort == MAIN_VIEWPORT)
						Details->Render(Details->m_visibles_sun_shadows);
					else if (currentViewPort == SECONDARY_WEAPON_SCOPE)
						Details->Render(Details->m_visibles_sun_shadows2);
				}

			fuckingsun->X.D.transluent = FALSE;

			if (bSpecial)
			{
				fuckingsun->X.D.transluent = TRUE;
				Target->phase_smap_direct_tsh(fuckingsun, SE_SUN_FAR);

				r_dsgraph_render_graph(1, *sunCascedsDsBuffersVect_[cascade_ind]); // normal level, secondary priority
				r_dsgraph_render_sorted(*sunCascedsDsBuffersVect_[cascade_ind]); // strict-sorted geoms
			}
		}
	}

	// Accumulate
	Target->phase_accumulator();

	if (Target->use_minmax_sm_this_frame())
	{
		PIX_EVENT(SE_SUN_NEAR_MINMAX_GENERATE);
		Target->create_minmax_SM();
	}

	PIX_EVENT(SE_SUN_NEAR);

	if (cascade_ind == 0)
		Target->accum_direct_cascade(SE_SUN_NEAR, actualViewPortBufferNow->sunCascadesRuntime_[cascade_ind]->sunCascadeCull_xform, actualViewPortBufferNow->sunCascadesRuntime_[cascade_ind]->sunCascadeCull_xform,	m_sun_cascades[cascade_ind].bias);
	else if (cascade_ind < m_sun_cascades.size() - 1)
		Target->accum_direct_cascade(SE_SUN_MIDDLE, actualViewPortBufferNow->sunCascadesRuntime_[cascade_ind]->sunCascadeCull_xform, actualViewPortBufferNow->sunCascadesRuntime_[cascade_ind - 1]->sunCascadeCull_xform, m_sun_cascades[cascade_ind].bias);
	else
		Target->accum_direct_cascade(SE_SUN_FAR, actualViewPortBufferNow->sunCascadesRuntime_[cascade_ind]->sunCascadeCull_xform, actualViewPortBufferNow->sunCascadesRuntime_[cascade_ind - 1]->sunCascadeCull_xform, m_sun_cascades[cascade_ind].bias);

	// Restore XForms
	RCache.set_xform_world(Fidentity);
	RCache.set_xform_view(Device.mView);
	RCache.set_xform_project(Device.mProject);

	sunCascedsDsBuffersVect_[cascade_ind]->SetCalculated(false); // Mark that in next frame we need to start calculation again
}

void CRender::calc_sun_cascades_vis_mt()
{
#ifdef MEASURE_MT
	CTimer T; T.Start();
#endif

	for (u8 i = 0; i < 3; i++)
	{
		actualViewPortBufferNow->sunCascadesRuntime_[i]->mtCalcCascadeProtect_.Enter();

		if (sunCascedsDsBuffersVect_[i]->IsCalculated())
			actualViewPortBufferNow->sunCascadesRuntime_[i]->mtCalcCascadeProtect_.Leave();
		else
		{
			calc_sun_cascade_vis_struct(i);

			actualViewPortBufferNow->sunCascadesRuntime_[i]->mtCalcCascadeProtect_.Leave();
		}
	}

#ifdef MEASURE_MT
	Device.Statistic->mtVisStructTime_ += T.GetElapsed_sec();
#endif
}


void CRender::sun_cascade_prerender(u32 casc_index)
{
	CLightSource* fuckingsun = (CLightSource*)Lights.sun_adapted._get();

	// calculate view-frustum bounds in world space
	Fmatrix ex_project, ex_full, ex_full_inverse;
	{
		ex_project = Device.mProject;
		ex_full.mul(ex_project, Device.mView);
		D3DXMatrixInverse((D3DXMATRIX*)&ex_full_inverse, 0, (D3DXMATRIX*)&ex_full);
	}

	// Compute volume(s) - something like a frustum for infinite directional light
	// Also compute virtual light position and sector it is inside
	CFrustum cull_frustum;
	xr_vector<Fplane> cull_planes;
	Fvector3 cull_COP;
	CSector* cull_sector;

	Fmatrix cull_xform;

	{
//		FPU::m64r();

		// Lets begin from base frustum
		Fmatrix fullxform_inv = ex_full_inverse;
#ifdef _DEBUG
		typedef DumbConvexVolume<true> t_volume;
#else
		typedef DumbConvexVolume<false> t_volume;
#endif

		//******************************* Need to be placed after cuboid built **************************
		// Search for default sector - assume "default" or "outdoor" sector is the largest one
		//. hack: need to know real outdoor sector
		CSector* largest_sector = 0;
		float largest_sector_vol = 0;

		for (u32 s = 0; s < Sectors.size(); s++)
		{
			CSector* S = (CSector*)Sectors[s];
			dxRender_Visual* V = S->root();
			float vol = V->vis.box.getvolume();
			if (vol > largest_sector_vol)
			{
				largest_sector_vol = vol;
				largest_sector = S;
			}
		}

		cull_sector = largest_sector;

		// COP - 100 km away
		cull_COP.mad(Device.vCameraPosition, fuckingsun->direction, -tweak_COP_initial_offs);

		// Create approximate ortho-xform
		// view: auto find 'up' and 'right' vectors
		Fmatrix mdir_View, mdir_Project;
		Fvector L_dir, L_up, L_right, L_pos;

		L_pos.set(fuckingsun->position);
		L_dir.set(fuckingsun->direction).normalize();
		L_right.set(1, 0, 0);

		if (_abs(L_right.dotproduct(L_dir)) > .99f)
			L_right.set(0, 0, 1);

		L_up.crossproduct(L_dir, L_right).normalize();
		L_right.crossproduct(L_up, L_dir).normalize();

		mdir_View.build_camera_dir(L_pos, L_dir, L_up);

		//////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
		typedef FixedConvexVolume<true> t_cuboid;
#else
		typedef FixedConvexVolume<false> t_cuboid;
#endif

		t_cuboid light_cuboid;
		{
			// Initialize the first cascade rays, then each cascade will initialize rays for next one.
			if (casc_index == 0 || m_sun_cascades[casc_index].reset_chain)
			{
				Fvector3 near_p, edge_vec;

				for (int p = 0; p < 4; p++)
				{
					// 					Fvector asd = Device.vCameraDirection;
					// 					asd.mul(-2);
					// 					asd.add(Device.vCameraPosition);
					// 					near_p		= Device.vCameraPosition;//wform		(fullxform_inv,asd); //
					near_p = wform(fullxform_inv, corners[facetable[4][p]]);

					edge_vec = wform(fullxform_inv, corners[facetable[5][p]]);
					edge_vec.sub(near_p);
					edge_vec.normalize();

					light_cuboid.view_frustum_rays.push_back(sun::ray(near_p, edge_vec));
				}
			}
			else
				light_cuboid.view_frustum_rays = m_sun_cascades[casc_index].rays;

			light_cuboid.view_ray.P = Device.vCameraPosition;
			light_cuboid.view_ray.D = Device.vCameraDirection;
			light_cuboid.light_ray.P = L_pos;
			light_cuboid.light_ray.D = L_dir;
		}

		// THIS NEED TO BE A CONSTATNT
		Fplane light_top_plane;
		light_top_plane.build_unit_normal(L_pos, L_dir);
		float dist = light_top_plane.classify(Device.vCameraPosition);

		float map_size = m_sun_cascades[casc_index].size;
		D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)&mdir_Project, -map_size * 0.5f, map_size * 0.5f, -map_size * 0.5f, map_size * 0.5f, 0.1, dist + /*sqrt(2)*/ 1.41421f * map_size);

		// build viewport xform
		float view_dim = float(RImplementation.o.smapsize);
		Fmatrix m_viewport = { view_dim / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, -view_dim / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, view_dim / 2.f, view_dim / 2.f, 0.0f, 1.0f };
		Fmatrix m_viewport_inv;

		D3DXMatrixInverse((D3DXMATRIX*)&m_viewport_inv, 0, (D3DXMATRIX*)&m_viewport);

		// snap view-position to pixel
		cull_xform.mul(mdir_Project, mdir_View);
		Fmatrix cull_xform_inv;
		cull_xform_inv.invert(cull_xform);

		// light_cuboid.light_cuboid_points.reserve(9);

		for (int p = 0; p < 8; p++)
		{
			Fvector3 xf = wform(cull_xform_inv, corners[p]);
			light_cuboid.light_cuboid_points[p] = xf;
		}

		// only side planes
		for (int plane = 0; plane < 4; plane++)
			for (int pt = 0; pt < 4; pt++)
			{
				int asd = facetable[plane][pt];
				light_cuboid.light_cuboid_polys[plane].points[pt] = asd;
			}

		Fvector lightXZshift;
		light_cuboid.compute_caster_model_fixed(cull_planes, lightXZshift, m_sun_cascades[casc_index].size, m_sun_cascades[casc_index].reset_chain);

		Fvector proj_view = Device.vCameraDirection;

		proj_view.y = 0;
		proj_view.normalize();

		//			lightXZshift.mad(proj_view, 20);

		// Initialize rays for the next cascade
		if (casc_index < m_sun_cascades.size() - 1)
			m_sun_cascades[casc_index + 1].rays = light_cuboid.view_frustum_rays;

		// #ifdef	_DEBUG

		static bool draw_debug = false;

		if (draw_debug && casc_index == 0)
			for (u32 it = 0; it < cull_planes.size(); it++)
				RImplementation.Target->dbg_addplane(cull_planes[it], it * 0xFFF);
		//#endifDDS

		Fvector cam_shifted = L_pos;
		cam_shifted.add(lightXZshift);

		// rebuild the view transform with the shift.
		mdir_View.identity();
		mdir_View.build_camera_dir(cam_shifted, L_dir, L_up);

		cull_xform.identity();
		cull_xform.mul(mdir_Project, mdir_View);
		cull_xform_inv.invert(cull_xform);

		// Create frustum for query
		cull_frustum._clear();

		for (u32 p = 0; p < cull_planes.size(); p++)
			cull_frustum._add(cull_planes[p]);

		{
			Fvector cam_proj = Device.vCameraPosition;
			const float align_aim_step_coef = 4.f;

			cam_proj.set(floorf(cam_proj.x / align_aim_step_coef) + align_aim_step_coef / 2, floorf(cam_proj.y / align_aim_step_coef) + align_aim_step_coef / 2,
				floorf(cam_proj.z / align_aim_step_coef) + align_aim_step_coef / 2);

			cam_proj.mul(align_aim_step_coef);
			Fvector cam_pixel = wform(cull_xform, cam_proj);
			cam_pixel = wform(m_viewport, cam_pixel);
			Fvector shift_proj = lightXZshift;

			cull_xform.transform_dir(shift_proj);
			m_viewport.transform_dir(shift_proj);

			const float align_granularity = 4.f;

			shift_proj.x = shift_proj.x > 0 ? align_granularity : -align_granularity;
			shift_proj.y = shift_proj.y > 0 ? align_granularity : -align_granularity;
			shift_proj.z = 0;

			cam_pixel.x = cam_pixel.x / align_granularity - floorf(cam_pixel.x / align_granularity);
			cam_pixel.y = cam_pixel.y / align_granularity - floorf(cam_pixel.y / align_granularity);
			cam_pixel.x *= align_granularity;
			cam_pixel.y *= align_granularity;
			cam_pixel.z = 0;

			cam_pixel.sub(shift_proj);

			m_viewport_inv.transform_dir(cam_pixel);
			cull_xform_inv.transform_dir(cam_pixel);

			Fvector diff = cam_pixel;
			static float sign_test = -1.f;
			diff.mul(sign_test);
			Fmatrix adjust;
			adjust.translate(diff);

			cull_xform.mulB_44(adjust);
		}

		// Save sun cascade values for this cascade
		s32 limit = RImplementation.o.smapsize - 1;

		fuckingsun->X.D.minX = 0;
		fuckingsun->X.D.maxX = limit;
		fuckingsun->X.D.minY = 0;
		fuckingsun->X.D.maxY = limit;

		fuckingsun->X.D.combine = cull_xform;

		// full-xform
		//FPU::m24r();
	}

	// Set cascade values
	actualViewPortBufferNow->sunCascadesRuntime_[casc_index]->sunCascadeCull_frustum = cull_frustum;
	actualViewPortBufferNow->sunCascadesRuntime_[casc_index]->sunCascadeCull_COP = cull_COP;
	actualViewPortBufferNow->sunCascadesRuntime_[casc_index]->sunCascadeCull_sector = cull_sector;
	actualViewPortBufferNow->sunCascadesRuntime_[casc_index]->sunCascadeCull_xform = cull_xform;
}


void CRender::calc_sun_cascade_vis_struct(u32 casc_index)
{
	// Prepare render buffer

	if (RImplementation.o.Tshadows)
	{
		sunCascedsDsBuffersVect_[casc_index]->phaseMask[0] = true;
		sunCascedsDsBuffersVect_[casc_index]->phaseMask[1] = true;
		sunCascedsDsBuffersVect_[casc_index]->phaseMask[2] = false;
	}
	else
	{
		sunCascedsDsBuffersVect_[casc_index]->phaseMask[0] = true;
		sunCascedsDsBuffersVect_[casc_index]->phaseMask[1] = false;
		sunCascedsDsBuffersVect_[casc_index]->phaseMask[2] = false;
	}

	sunCascedsDsBuffersVect_[casc_index]->phaseType_ = PHASE_SMAP;
	sunCascedsDsBuffersVect_[casc_index]->useHOM_ = !!ps_r_misc_flags.test(R_MISC_USE_HOM_SUN);
	sunCascedsDsBuffersVect_[casc_index]->portalTraverserOpt = (sunCascedsDsBuffersVect_[casc_index]->useHOM_ ? CPortalTraverser::VQ_HOM : 0) + CPortalTraverser::VQ_SSA;
	sunCascedsDsBuffersVect_[casc_index]->dsgraphBufferViewFrustum_ = actualViewPortBufferNow->sunCascadesRuntime_[casc_index]->sunCascadeCull_frustum;

	// Fill the database - determine geometry for shadow map

	auto buffer = actualViewPortBufferNow->sunCascadesRuntime_[casc_index];
	render_subspace_MT(sunCascedsDsBuffersVect_[casc_index], buffer->sunCascadeCull_sector, buffer->sunCascadeCull_xform, buffer->sunCascadeCull_COP, true);

	sunCascedsDsBuffersVect_[casc_index]->SetCalculated(true); // mark for multithreading
}

void CRender::SyncSunCascadeMT(u32 cascade_ind)
{
	if (actualViewPortBufferNow->sunCanCalcDeffered_)
	{
		if (ps_r_mt_flags.test(R_FLAG_MT_CALC_SUN_VIS_STRUCT))
		{
			CTimer dull_waitng_timer; dull_waitng_timer.Start();

			while (!sunCascedsDsBuffersVect_[cascade_ind]->IsCalculated())
				Sleep(0);

			float dull_time = dull_waitng_timer.GetElapsed_sec() * 1000.f;

			vpStats->RenderMtWait += dull_time;

#ifdef MEASURE_MT
			Device.Statistic->mtRenderDelaySun_ += dull_time;
#endif
		}
		else
			calc_sun_cascade_vis_struct(cascade_ind);
	}

}