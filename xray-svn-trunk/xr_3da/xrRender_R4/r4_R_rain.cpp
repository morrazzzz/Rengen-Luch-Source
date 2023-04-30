#include "stdafx.h"
#include "../igame_persistent.h"
#include "../irenderable.h"
#include "../xrRender/FBasicVisual.h"

#include "r4_R_sun_support.h"

const float tweak_rain_COP_initial_offs			= 1200.f;
const float tweak_rain_ortho_xform_initial_offs	= 1000.f;

// Defined in r2_R_sun.cpp
Fvector3 wform(Fmatrix& m, Fvector3 const& v);

// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z

static Fvector3		corners [8]			= {
	{ -1, -1,  0 },		{ -1, -1, +1},
	{ -1, +1, +1 },		{ -1, +1,  0},
	{ +1, +1, +1 },		{ +1, +1,  0},
	{ +1, -1, +1},		{ +1, -1,  0}
};

static int			facetable[6][4]		= {
	{ 0, 3, 5, 7 },		{ 1, 2, 3, 0 },
	{ 6, 7, 5, 4 },		{ 4, 2, 1, 6 },
	{ 3, 2, 4, 5 },		{ 1, 0, 7, 6 },
};

void CRender::rain_prerender(CLightSource& rain_light)
{
	D3DXMATRIX m_LightViewProj;

	// Use light as placeholder for rain data.

	//static const float	source_offset		= 40.f;
	static const float source_offset = 10000.f;

	rain_light.direction.set(0.0f, -1.0f, 0.0f);

	rain_light.position.set(Device.vCameraPosition.x, Device.vCameraPosition.y + source_offset, Device.vCameraPosition.z);

	float fBoundingSphereRadius = 0;

	// calculate view-frustum bounds in world space
	Fmatrix	ex_project, ex_full, ex_full_inverse;
	{
		const float fRainFar = ps_r3_dyn_wet_surf_far;

		ex_project.build_projection(deg2rad(Device.fFOV), Device.fASPECT, VIEWPORT_NEAR, fRainFar);

		ex_full.mul(ex_project, Device.mView);
		D3DXMatrixInverse((D3DXMATRIX*)&ex_full_inverse, 0, (D3DXMATRIX*)&ex_full);

		//	Calculate view frustum were we can see dynamic rain radius
		{
			//	b^2 = 2RH, B - side enge of the pyramid, h = height
			//	R = b^2/(2*H)
			const float H = fRainFar;
			const float a = tanf(deg2rad(Device.fFOV) / 2);
			const float c = tanf(deg2rad(Device.fFOV*Device.fASPECT) / 2);
			const float b_2 = H * H * (1.0f + a * a + c * c);

			fBoundingSphereRadius = b_2 / (2.0f * H);
		}
	}

	// Compute volume(s) - something like a frustum for infinite directional light
	// Also compute virtual light position and sector it is inside

	CFrustum					cull_frustum;
	xr_vector<Fplane>			cull_planes;
	Fvector3					cull_COP;
	CSector*					cull_sector;

	Fmatrix						cull_xform;
	{
		//FPU::m64r();

		// Lets begin from base frustum
		Fmatrix fullxform_inv = ex_full_inverse;

#ifdef	_DEBUG
		typedef		DumbConvexVolume<true>	t_volume;
#else
		typedef		DumbConvexVolume<false>	t_volume;
#endif
		t_volume hull;
		{
			hull.points.reserve(9);

			for (int p = 0; p < 8; p++)
			{
				Fvector3 xf = wform(fullxform_inv, corners[p]);
				hull.points.push_back(xf);
			}

			for (int plane = 0; plane<6; plane++)
			{
				hull.polys.push_back(t_volume::_poly());

				for (int pt = 0; pt < 4; pt++)
					hull.polys.back().points.push_back(facetable[plane][pt]);
			}
		}

		//hull.compute_caster_model	(cull_planes,fuckingsun->direction);
		hull.compute_caster_model(cull_planes, rain_light.direction);

#ifdef	_DEBUG
		for (u32 it = 0; it<cull_planes.size(); it++)
			RImplementation.Target->dbg_addplane(cull_planes[it], 0xffffffff);
#endif

		// Search for default sector - assume "default" or "outdoor" sector is the largest one
		//. hack: need to know real outdoor sector
		CSector*	largest_sector = 0;
		float		largest_sector_vol = 0;

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
		cull_COP.mad(Device.vCameraPosition, rain_light.direction, -tweak_rain_COP_initial_offs);

		cull_COP.x += fBoundingSphereRadius*Device.vCameraDirection.x;
		cull_COP.z += fBoundingSphereRadius*Device.vCameraDirection.z;

		// Create frustum for query
		cull_frustum._clear();

		for (u32 p = 0; p < cull_planes.size(); p++)
			cull_frustum._add(cull_planes[p]);


		// Create approximate ortho-xform
		// view: auto find 'up' and 'right' vectors
		Fmatrix						mdir_View, mdir_Project;
		Fvector						L_dir, L_up, L_right, L_pos;

		L_pos.set(rain_light.position);
		L_dir.set(rain_light.direction).normalize();
		L_right.set(1, 0, 0);

		if (_abs(L_right.dotproduct(L_dir)) > .99f)
			L_right.set(0, 0, 1);

		L_up.crossproduct(L_dir, L_right).normalize();
		L_right.crossproduct(L_up, L_dir).normalize();

		mdir_View.build_camera_dir(L_pos, L_dir, L_up);

		// projection: box
		//	Simple
		Fbox frustum_bb;
		frustum_bb.invalidate();

		for (int it = 0; it < 8; it++)
		{
			//for (int it=0; it<9; it++)	{
			Fvector	xf = wform(mdir_View, hull.points[it]);
			frustum_bb.modify(xf);
		}

		Fbox& bb = frustum_bb;
		bb.grow(EPS);

		//	HACK
		//	TODO: DX10: Calculate bounding sphere for view frustum
		//	TODO: DX10: Reduce resolution.
		//bb.min.x = -50;
		//bb.max.x = 50;
		//bb.min.y = -50;
		//bb.max.y = 50;

		//	Offset RainLight position to center rain shadowmap
		Fvector3 vRectOffset;
		vRectOffset.set(fBoundingSphereRadius*Device.vCameraDirection.x, 0, fBoundingSphereRadius*Device.vCameraDirection.z);

		bb.min.x = -fBoundingSphereRadius + vRectOffset.x;
		bb.max.x = fBoundingSphereRadius + vRectOffset.x;
		bb.min.y = -fBoundingSphereRadius + vRectOffset.z;
		bb.max.y = fBoundingSphereRadius + vRectOffset.z;

		//D3DXMatrixOrthoOffCenterLH	((D3DXMATRIX*)&mdir_Project,bb.min.x,bb.max.x,  bb.min.y,bb.max.y,  bb.min.z-tweak_rain_ortho_xform_initial_offs,bb.max.z);
		D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)&mdir_Project, bb.min.x, bb.max.x, bb.min.y, bb.max.y, bb.min.z - tweak_rain_ortho_xform_initial_offs, bb.min.z + 2 * tweak_rain_ortho_xform_initial_offs);

		cull_xform.mul(mdir_Project, mdir_View);

		s32		limit = _min(RImplementation.o.smapsize, ps_r3_dyn_wet_surf_sm_res);

		// build viewport xform
		float	view_dim = float(limit);
		float	fTexelOffs = (.5f / RImplementation.o.smapsize);

		Fmatrix	m_viewport = {
			view_dim / 2.f, 0.0f, 0.0f, 0.0f,
			0.0f, -view_dim / 2.f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			view_dim / 2.f + fTexelOffs, view_dim / 2.f + fTexelOffs, 0.0f, 1.0f
		};

		Fmatrix				m_viewport_inv;
		D3DXMatrixInverse((D3DXMATRIX*)&m_viewport_inv, 0, (D3DXMATRIX*)&m_viewport);

		// snap view-position to pixel
		//	snap zero point to pixel
		Fvector cam_proj = wform(cull_xform, Fvector().set(0, 0, 0));
		Fvector	cam_pixel = wform(m_viewport, cam_proj);

		cam_pixel.x = floorf(cam_pixel.x);
		cam_pixel.y = floorf(cam_pixel.y);

		Fvector cam_snapped = wform(m_viewport_inv, cam_pixel);
		Fvector diff;		diff.sub(cam_snapped, cam_proj);

		Fmatrix adjust;		adjust.translate(diff);

		cull_xform.mulA_44(adjust);

		rain_light.X.D.minX = 0;
		rain_light.X.D.maxX = limit;
		rain_light.X.D.minY = 0;
		rain_light.X.D.maxY = limit;

		// full-xform
		//FPU::m24r();
	}

	// Finalize & Cleanup
	rain_light.X.D.combine = cull_xform;

	actualViewPortBufferNow->rainWetSurfRuntime_->rainCullSector_ = cull_sector;
	actualViewPortBufferNow->rainWetSurfRuntime_->rainCullFrustum_ = cull_frustum;
	actualViewPortBufferNow->rainWetSurfRuntime_->rainCullXform_ = cull_xform;
	actualViewPortBufferNow->rainWetSurfRuntime_->rainCullCOP_ = cull_COP;
}

void CRender::prepare_rain_vis_struct()
{
	float fRainFactor = g_pGamePersistent->Environment().CurrentEnv->rain_density;

	if (fRainFactor < EPS_L)
	{
		return;
	}

	actualViewPortBufferNow->protectRainVisStructCalc_.Enter();

	if (!wetRainDsBuffer_->IsCalculated())
	{
#ifdef MEASURE_MT
		CTimer T; T.Start();
#endif

		// build visability structure for rain wet surfaces
		{
			wetRainDsBuffer_->useHOM_ = false;
			wetRainDsBuffer_->portalTraverserOpt = 0;

			wetRainDsBuffer_->phaseMask[0] = true;
			wetRainDsBuffer_->phaseMask[1] = false;
			wetRainDsBuffer_->phaseMask[2] = false;

			wetRainDsBuffer_->phaseType_ = PHASE_SMAP;

			wetRainDsBuffer_->dsgraphBufferViewFrustum_ = actualViewPortBufferNow->rainWetSurfRuntime_->rainCullFrustum_;
		}

		// Fill the database
		render_subspace_MT(wetRainDsBuffer_, actualViewPortBufferNow->rainWetSurfRuntime_->rainCullSector_, actualViewPortBufferNow->rainWetSurfRuntime_->rainCullXform_, actualViewPortBufferNow->rainWetSurfRuntime_->rainCullCOP_, false);

		wetRainDsBuffer_->SetCalculated(true);

#ifdef MEASURE_MT
		Device.Statistic->mtVisStructTime_ += T.GetElapsed_sec();
#endif
	}

	actualViewPortBufferNow->protectRainVisStructCalc_.Leave();
}

void CRender::render_rain()
{
	float fRainFactor = g_pGamePersistent->Environment().CurrentEnv->rain_density;

	if (fRainFactor < EPS_L)
		return;

	CLightSource rain_light; // The wet surfaces use shadow map-like algorithm, so we need a light for it
	rain_light.readyToDestroy_ = true;

	// Check if mt has calculated vis struct or calculate it ourself if mt is disabled
	SyncRainCalcMT();

	// Prepare the variables
	rain_prerender(rain_light);

	if (!actualViewPortBufferNow->rainCanCalcDeffered_) // calculate vis struct ourself if deffered is not allowed
		prepare_rain_vis_struct();

	// Render as shadow-map
	render_phase = PHASE_SMAP;

	//. !!! We should clip based on shrinked frustum (again)
	{
		bool bNormal = wetRainDsBuffer_->mapNormalPasses[0][0].size() || wetRainDsBuffer_->mapMatrixPasses[0][0].size();
		bool bSpecial = wetRainDsBuffer_->mapNormalPasses[1][0].size() || wetRainDsBuffer_->mapMatrixPasses[1][0].size() || wetRainDsBuffer_->mapSorted.size();

		if (bNormal || bSpecial)
		{
			Target->phase_smap_direct(&rain_light, SE_SUN_RAIN_SMAP);

			RCache.set_xform_world(Fidentity);
			RCache.set_xform_view(Fidentity);
			RCache.set_xform_project(actualViewPortBufferNow->rainWetSurfRuntime_->rainCullXform_);

			r_dsgraph_render_graph(0, *wetRainDsBuffer_);

			//????
			//if (ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS))	
			//	Details->Render					()	;
		}
	}

	// Restore XForms
	RCache.set_xform_world		(Fidentity);
	RCache.set_xform_view		(Device.mView);
	RCache.set_xform_project	(Device.mProject);

	// Accumulate
	Target->phase_rain();
	Target->draw_rain(rain_light);

	actualViewPortBufferNow->rainCanCalcDeffered_ = true;
}

void CRender::SyncRainCalcMT()
{
	if (actualViewPortBufferNow->rainCanCalcDeffered_)
	{
		if (ps_r_mt_flags.test(R_FLAG_MT_CALC_RAIN_VIS_STRUCT))
		{
			CTimer dull_waitng_timer; dull_waitng_timer.Start();

			while (!wetRainDsBuffer_->IsCalculated())
				Sleep(0);

			float dull_time = dull_waitng_timer.GetElapsed_sec() * 1000.f;

			vpStats->RenderMtWait += dull_time;

#ifdef MEASURE_MT
			Device.Statistic->mtRenderDelayRain_ += dull_time;
#endif
		}
		else
			prepare_rain_vis_struct();
	}

}