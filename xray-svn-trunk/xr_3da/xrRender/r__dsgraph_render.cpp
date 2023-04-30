#include "stdafx.h"

#include "../xr_3da/render.h"
#include "../xr_3da/irenderable.h"
#include "../xr_3da/igame_persistent.h"
#include "../xr_3da/environment.h"
#include "../xr_3da/CustomHUD.h"

#include "FBasicVisual.h"

extern ENGINE_API float viewPortNearK;

using namespace		R_dsgraph;

extern float		r_ssaGLOD_start;
extern float		r_ssaGLOD_end;

bool needSorting = false;

ICF float calcLOD(float ssa, float R)
{
	return _sqrt(clampr((ssa - r_ssaGLOD_end) / (r_ssaGLOD_start - r_ssaGLOD_end), 0.f, 1.f));
}

// NORMAL
void __fastcall mapNormal_Render(mapNormalItems& N)
{
	// *** DIRECT ***
	_NormalItem* I = &*N.begin(), *E = &*N.end();

	for (; I != E; I++)
	{
		_NormalItem& Ni = *I;

		if (RCache.LOD.c_LOD)
		{
			// don't know if this is correct, but i did not see rendering objects of this type, so could not test it

			Fmatrix m;
			Ni.pVisual->getVisData().box.xform(m);
			Fvector o_position = m.c;

			float adjusted_distane = Device.GetDistFromCamera(o_position);

			RCache.LOD.set_LOD(adjusted_distane);
		}

		float LOD = calcLOD(Ni.ssa, Ni.pVisual->vis.sphere.R);

		Ni.pVisual->Render(LOD);
	}
}

// Matrix

void __fastcall mapMatrix_Render(mapMatrixItems& N)
{
	// *** DIRECT ***
	_MatrixItem* I = &*N.begin(), *E = &*N.end();

	for (; I != E; I++)
	{
		_MatrixItem& Ni = *I;

		if (Ni.pObject->renderable.needToDelete)
			continue;

		Ni.pVisual->GetDrawingLock().Enter();

		RCache.set_xform_world(Ni.Matrix);

		RImplementation.apply_object(Ni.pObject);
		RImplementation.apply_lmaterial();

		if (RCache.LOD.c_LOD)
		{
			Fvector o_position = Ni.pObject->renderable.xform.c;

			float adjusted_distane = Device.GetDistFromCamera(o_position);

			RCache.LOD.set_LOD(adjusted_distane);
		}

		float LOD = calcLOD(Ni.ssa, Ni.pVisual->vis.sphere.R);
		Ni.pVisual->Render(LOD);

		Ni.pVisual->GetDrawingLock().Leave();
	}

	N.clear();
}

void R_dsgraph_structure::r_dsgraph_render_graph(u32 _priority, DsGraphBuffer& render_buffer, bool _clear)
{
	r_dsgraph_render_graph_N(_priority, render_buffer, _clear);
	r_dsgraph_render_graph_M(_priority, render_buffer, _clear);
}

void R_dsgraph_structure::r_dsgraph_render_graph_N(u32 _priority, DsGraphBuffer& render_buffer, bool _clear)
{
	Device.Statistic->RenderDUMP.Begin();

	// **************************************************** NORMAL
	// Perform sorting based on ScreenSpaceArea
	// Sorting by SSA and changes minimizations
	RCache.set_xform_world(Fidentity);

	// Render several passes
	for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
	{
		mapNormalVS& vs = render_buffer.mapNormalPasses[_priority][iPass];

		for (u32 vs_id = 0; vs_id < vs.size(); vs_id++)
		{
			mapNormalVS::TNode*	Nvs = &vs[vs_id];

			RCache.set_VS(Nvs->key);

			mapNormalGS& gs = Nvs->val;

			for (u32 gs_id = 0; gs_id < gs.size(); gs_id++)
			{
				mapNormalGS::TNode*	Ngs = &gs[gs_id];

				RCache.set_GS(Ngs->key);

				mapNormalPS& ps = Ngs->val;

				for (u32 ps_id = 0; ps_id < ps.size(); ps_id++)
				{
					mapNormalPS::TNode*	Nps = &ps[ps_id];

					RCache.set_PS(Nps->key);

					mapNormalCS& cs = Nps->val.mapCS;

					RCache.set_HS(Nps->val.hs);
					RCache.set_DS(Nps->val.ds);

					for (u32 cs_id = 0; cs_id < cs.size(); cs_id++)
					{
						mapNormalCS::TNode*	Ncs = &cs[cs_id];

						RCache.set_Constants(Ncs->key);

						mapNormalStates& states = Ncs->val;

						for (u32 state_id = 0; state_id < states.size(); state_id++)
						{
							mapNormalStates::TNode*	Nstate = &states[state_id];
							RCache.set_States(Nstate->key);

							mapNormalTextures& tex = Nstate->val;

							for (u32 tex_id = 0; tex_id < tex.size(); tex_id++)
							{
								mapNormalTextures::TNode* Ntex = &tex[tex_id];

								RCache.set_Textures(Ntex->key);

								RImplementation.apply_lmaterial();

								mapNormalItems& items = Ntex->val;

								mapNormal_Render(items);

								if (_clear)
									items.clear();
							}

							if (_clear)
								tex.clear();
						}

						if (_clear)
							states.clear();
					}

					if (_clear)
						cs.clear();
				}

				if (_clear)
					ps.clear();
			}

			if (_clear)
				gs.clear();
		}

		if (_clear)
			vs.clear();
	}
	
	Device.Statistic->RenderDUMP.End();
}

void R_dsgraph_structure::r_dsgraph_render_graph_M(u32 _priority, DsGraphBuffer& render_buffer, bool _clear)
{
	// **************************************************** MATRIX
	// Perform sorting based on ScreenSpaceArea
	// Sorting by SSA and changes minimizations
	// Render several passes

	Device.Statistic->RenderDUMP.Begin();

	for (u32 iPass = 0; iPass < SHADER_PASSES_MAX; ++iPass)
	{
		mapMatrixVS& vs = render_buffer.mapMatrixPasses[_priority][iPass];

		for (u32 vs_id = 0; vs_id < vs.size(); vs_id++)
		{
			mapMatrixVS::TNode*	Nvs = &vs[vs_id];

			RCache.set_VS(Nvs->key);

			mapMatrixGS& gs = Nvs->val;

			for (u32 gs_id = 0; gs_id < gs.size(); gs_id++)
			{
				mapMatrixGS::TNode*	Ngs = &gs[gs_id];

				RCache.set_GS(Ngs->key);

				mapMatrixPS& ps = Ngs->val;

				for (u32 ps_id = 0; ps_id < ps.size(); ps_id++)
				{
					mapMatrixPS::TNode*	Nps = &ps[ps_id];

					RCache.set_PS(Nps->key);

					mapMatrixCS& cs = Nps->val.mapCS;

					RCache.set_HS(Nps->val.hs);
					RCache.set_DS(Nps->val.ds);

					for (u32 cs_id = 0; cs_id < cs.size(); cs_id++)
					{
						mapMatrixCS::TNode*	Ncs = &cs[cs_id];
						RCache.set_Constants(Ncs->key);

						mapMatrixStates& states = Ncs->val;

						for (u32 state_id = 0; state_id < states.size(); state_id++)
						{
							mapMatrixStates::TNode*	Nstate = &states[state_id];
							RCache.set_States(Nstate->key);

							mapMatrixTextures& tex = Nstate->val;

							for (u32 tex_id = 0; tex_id < tex.size(); tex_id++)
							{
								mapMatrixTextures::TNode* Ntex = &tex[tex_id];

								RCache.set_Textures(Ntex->key);

								RImplementation.apply_lmaterial();

								mapMatrixItems& items = Ntex->val;

								mapMatrix_Render(items);
							}

							if (_clear)
								tex.clear();
						}

						if (_clear)
							states.clear();
					}

					if (_clear)
						cs.clear();
				}

				if (_clear)
					ps.clear();
			}

			if (_clear)
				gs.clear();
		}

		if (_clear)
			vs.clear();
	}

	Device.Statistic->RenderDUMP.End();
}

extern ENGINE_API float psHUD_FOV;
extern ENGINE_API float hudFovRuntime_K;
extern ENGINE_API float VIEWPORT_NEAR_HUD;

// ALPHA
void __fastcall sorted_L1(mapSorted_Node *N)
{
	VERIFY(N);

	dxRender_Visual *V = N->val.pVisual;

	V->GetDrawingLock().Enter();

	VERIFY(V && V->shader._get());

	RCache.set_Element(N->val.se);
	RCache.set_xform_world(N->val.Matrix);

	RImplementation.apply_object(N->val.pObject);
	RImplementation.apply_lmaterial();

	V->Render(calcLOD(N->key, V->vis.sphere.R));

	V->GetDrawingLock().Leave();
}

// HUD render
void R_dsgraph_structure::r_dsgraph_render_hud(DsGraphBuffer& render_buffer)
{
	if (render_buffer.mapHUD.size() == 0)
		return;

	// Change projection
	Fmatrix Pold = Device.mProject;
	Fmatrix FTold = Device.mFullTransform;

	Device.mProject.build_projection(deg2rad(psHUD_FOV * hudFovRuntime_K * Device.fFOV), Device.fASPECT, VIEWPORT_NEAR_HUD * viewPortNearK, g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.mFullTransform.mul(Device.mProject, Device.mView);

	RCache.set_xform_project(Device.mProject);

	// Rendering
	rmNear();
	render_buffer.mapHUD.traverseLR(sorted_L1);
	render_buffer.mapHUD.clear();

	rmNormal();

	// Restore projection
	Device.mProject = Pold;
	Device.mFullTransform = FTold;
	RCache.set_xform_project(Device.mProject);
}

// thx to K.D.
// HUD emissive render
void R_dsgraph_structure::r_dsgraph_render_hud_emissive(DsGraphBuffer& render_buffer)
{
	if (!render_buffer.mapHUDEmissive.size())
		return;

	// Change projection
	Fmatrix Pold = Device.mProject;
	Fmatrix FTold = Device.mFullTransform;

	Device.mProject.build_projection(deg2rad(psHUD_FOV * hudFovRuntime_K * Device.fFOV), Device.fASPECT, VIEWPORT_NEAR_HUD * viewPortNearK, g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.mFullTransform.mul(Device.mProject, Device.mView);
	RCache.set_xform_project(Device.mProject);

	// Rendering
	rmNear();

	render_buffer.mapHUDEmissive.traverseLR(sorted_L1);
	render_buffer.mapHUDEmissive.clear();

	rmNormal();

	// Restore projection
	Device.mProject = Pold;
	Device.mFullTransform = FTold;
	RCache.set_xform_project(Device.mProject);
}

// HUD sorted render
void R_dsgraph_structure::r_dsgraph_render_hud_sorted(DsGraphBuffer& render_buffer)
{
	if (!render_buffer.mapHUDSorted.size())
		return;

	// Change projection
	Fmatrix Pold = Device.mProject;
	Fmatrix FTold = Device.mFullTransform;

	Device.mProject.build_projection(deg2rad(psHUD_FOV * hudFovRuntime_K * Device.fFOV), Device.fASPECT, VIEWPORT_NEAR_HUD * viewPortNearK, g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.mFullTransform.mul(Device.mProject, Device.mView);
	RCache.set_xform_project(Device.mProject);

	// Rendering
	rmNear();

	render_buffer.mapHUDSorted.traverseRL(sorted_L1);
	render_buffer.mapHUDSorted.clear();

	rmNormal();

	// Restore projection
	Device.mProject = Pold;
	Device.mFullTransform = FTold;
	RCache.set_xform_project(Device.mProject);
}

void R_dsgraph_structure::r_dsgraph_render_hud_ui(DsGraphBuffer& render_buffer)
{
	VERIFY(g_hud && g_hud->RenderActiveItemUIQuery());

	// Change projection
	Fmatrix Pold = Device.mProject;
	Fmatrix FTold = Device.mFullTransform;

	Device.mProject.build_projection(deg2rad(psHUD_FOV * hudFovRuntime_K * Device.fFOV), Device.fASPECT, VIEWPORT_NEAR_HUD, g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.mFullTransform.mul(Device.mProject, Device.mView);
	RCache.set_xform_project(Device.mProject);

	// Targets, use accumulator for temporary storage
	const ref_rt rt_null;

	RCache.set_RT(0, 1);
	RCache.set_RT(0, 2);

	if (!RImplementation.o.dx10_msaa)
	{
		if (RImplementation.o.albedo_wo)
			RImplementation.Target->u_setrt(RImplementation.Target->rt_Accumulator, rt_null, rt_null, HW.pBaseZB);
		else
			RImplementation.Target->u_setrt(RImplementation.Target->rt_Color, rt_null, rt_null, HW.pBaseZB);
	}
	else
	{
		if (RImplementation.o.albedo_wo)
			RImplementation.Target->u_setrt(RImplementation.Target->rt_Accumulator, rt_null, rt_null, RImplementation.Target->rt_MSAADepth->pZRT);
		else
			RImplementation.Target->u_setrt(RImplementation.Target->rt_Color, rt_null, rt_null, RImplementation.Target->rt_MSAADepth->pZRT);
	}

	rmNear();

	g_hud->RenderActiveItemUI();

	rmNormal();

	// Restore projection
	Device.mProject = Pold;
	Device.mFullTransform = FTold;
	RCache.set_xform_project(Device.mProject);
}

// strict-sorted render
void R_dsgraph_structure::r_dsgraph_render_sorted(DsGraphBuffer& render_buffer)
{
	if (!render_buffer.mapSorted.size())
		return;

	// Sorted (back to front)
	render_buffer.mapSorted.traverseRL(sorted_L1);
	render_buffer.mapSorted.clear();
}

// strict-sorted render
void R_dsgraph_structure::r_dsgraph_render_emissive(DsGraphBuffer& render_buffer)
{
	if (!render_buffer.mapEmissive.size() && !render_buffer.mapHUDEmissive.size())
		return;

	// Sorted (back to front)
	render_buffer.mapEmissive.traverseLR(sorted_L1);
	render_buffer.mapEmissive.clear();
}

// strict-sorted render
void R_dsgraph_structure::r_dsgraph_render_wmarks(DsGraphBuffer& render_buffer)
{
	if (!render_buffer.mapWmark.size())
		return;

	// Sorted (back to front)
	render_buffer.mapWmark.traverseLR(sorted_L1);
	render_buffer.mapWmark.clear();
}

// strict-sorted render
void R_dsgraph_structure::r_dsgraph_render_distort(DsGraphBuffer& render_buffer)
{
	if (!render_buffer.mapDistort.size())
		return;

	// Sorted (back to front)
	render_buffer.mapDistort.traverseRL(sorted_L1);
	render_buffer.mapDistort.clear();
}