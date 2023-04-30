#include "stdafx.h"

#include "fhierrarhyvisual.h"
#include "SkeletonCustom.h"
#include "../../xr_3da/fmesh.h"
#include "../../xr_3da/irenderable.h"

#include "flod.h"
#include "particlegroup.h"
#include "FTreeVisual.h"

using	namespace R_dsgraph;

size_t dsGraphBuffersIDCounter_ = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene graph actual insertion and sorting ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

float		r_ssaDISCARD;
float		r_ssaLOD_A;
float		r_ssaLOD_B;
float		r_ssaGLOD_start;
float		r_ssaGLOD_end;
float		r_ssaHZBvsTEX;

ICF	float CalcSSA(float& distSQ, Fvector& C, dxRender_Visual* V)
{
	float R = V->vis.sphere.R + 0;
	distSQ = Device.currentVpSavedView->GetCameraPosition_saved().distance_to_sqr(C) + EPS;
	return	R / distSQ;
}

ICF	float CalcSSA(float& distSQ, Fvector& C, float R)
{
	distSQ = Device.currentVpSavedView->GetCameraPosition_saved().distance_to_sqr(C) + EPS;
	return	R / distSQ;
}


void R_dsgraph_structure::r_dsgraph_insert_dynamic(dxRender_Visual* pVisual, Fvector Center, IRenderBuffer& render_buffer)
{
	DsGraphBuffer& dsgraph_buffer = (DsGraphBuffer&)render_buffer;

	if (dsgraph_buffer.setRenderable_->renderable.needToDelete) // We don't want to render objects that are ready to delete
		return;

	pVisual->protectVisMarker_.Enter();

	auto it = pVisual->vis.vis_marker.find(dsgraph_buffer.visMarkerId_); // find our buffer mark

	// if dont have our buffer mark - create it (ususally first scene frames)
	if (it == pVisual->vis.vis_marker.end())
#pragma todo("Can overflow if many short-life sources spam its mark. May be need to make a erase when light gets destroyed")
		pVisual->vis.vis_marker.insert(mk_pair(dsgraph_buffer.visMarkerId_, dsgraph_buffer.visMarker_));
	else if (it->second == dsgraph_buffer.visMarker_) // Avoid adding same geometry several times in the same ds graph buffer
	{
		pVisual->protectVisMarker_.Leave();

		return;
	}

	pVisual->protectVisMarker_.Leave();

	it->second = dsgraph_buffer.visMarker_;

	float distSQ;
	float SSA =	CalcSSA(distSQ, Center, pVisual);

	if (SSA <= r_ssaDISCARD)
		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY(pVisual->shader._get());

	ShaderElement* sh_d = &*pVisual->shader->E[4];

	if (dsgraph_buffer.distortedGeom_ && sh_d && sh_d->flags.bDistort && dsgraph_buffer.phaseMask[sh_d->flags.iPriority / 2])
	{
		mapSorted_Node* N = dsgraph_buffer.mapDistort.insertInAnyWay(distSQ);

		N->val.ssa = SSA;
		N->val.pObject = dsgraph_buffer.setRenderable_;
		N->val.pVisual = pVisual;
		N->val.Matrix = dsgraph_buffer.renderingMatrix_;
		N->val.se = sh_d; // 4=L_special
	}

	// Select shader
	ShaderElement* sh = RImplementation.rimp_select_sh_dynamic(pVisual, distSQ, dsgraph_buffer);

	if (!sh)
		return;

	if (!dsgraph_buffer.phaseMask[sh->flags.iPriority / 2])
		return;

	// Create common node
	// NOTE: Invisible elements exist only in R1
	_MatrixItem item = { SSA, dsgraph_buffer.setRenderable_, pVisual, dsgraph_buffer.renderingMatrix_ };

	// HUD rendering
	if (dsgraph_buffer.isForHud_)
	{
		if (sh->flags.bStrictB2F)	
		{
			if (sh->flags.bEmissive)
			{
				mapSorted_Node* N = dsgraph_buffer.mapHUDEmissive.insertInAnyWay(distSQ);
				N->val.ssa = SSA;
				N->val.pObject = dsgraph_buffer.setRenderable_;
				N->val.pVisual = pVisual;
				N->val.Matrix = dsgraph_buffer.renderingMatrix_;
				N->val.se = &*pVisual->shader->E[4];		// 4=L_special
			}

			mapSorted_Node* N = dsgraph_buffer.mapHUDSorted.insertInAnyWay(distSQ);
			N->val.ssa = SSA;
			N->val.pObject = dsgraph_buffer.setRenderable_;
			N->val.pVisual = pVisual;
			N->val.Matrix = dsgraph_buffer.renderingMatrix_;
			N->val.se = sh;

			return;
		} 
		else 
		{
			mapHUD_Node* N			= dsgraph_buffer.mapHUD.insertInAnyWay		(distSQ);
			N->val.ssa				= SSA;
			N->val.pObject			= dsgraph_buffer.setRenderable_;
			N->val.pVisual			= pVisual;
			N->val.Matrix			= dsgraph_buffer.renderingMatrix_;
			N->val.se				= sh;

			if (sh->flags.bEmissive) 
			{
				mapSorted_Node* s_node		= dsgraph_buffer.mapHUDEmissive.insertInAnyWay	(distSQ);
				s_node->val.ssa				= SSA;
				s_node->val.pObject			= dsgraph_buffer.setRenderable_;
				s_node->val.pVisual			= pVisual;
				s_node->val.Matrix			= dsgraph_buffer.renderingMatrix_;
				s_node->val.se				= &*pVisual->shader->E[4];		// 4=L_special
			}

			return;
		}
	}

	// Shadows registering
	if (dsgraph_buffer.allowInvisibles_)
		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		mapSorted_Node* N		= dsgraph_buffer.mapSorted.insertInAnyWay	(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= dsgraph_buffer.setRenderable_;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= dsgraph_buffer.renderingMatrix_;
		N->val.se				= sh;

		return;
	}

	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive)
	{
		mapSorted_Node* N		= dsgraph_buffer.mapEmissive.insertInAnyWay(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= dsgraph_buffer.setRenderable_;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= dsgraph_buffer.renderingMatrix_;
		N->val.se				= &*pVisual->shader->E[4];		// 4=L_special
	}

	if (sh->flags.bWmark && dsgraph_buffer.phaseMask[2])
	{
		mapSorted_Node* N		= dsgraph_buffer.mapWmark.insertInAnyWay(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= dsgraph_buffer.setRenderable_;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= dsgraph_buffer.renderingMatrix_;
		N->val.se				= sh;		

		return;
	}

	dsgraph_buffer.dynCnt++;

	for (u32 iPass = 0; iPass<sh->passes.size(); ++iPass)
	{
		// the most common node
		SPass& pass	= *sh->passes[iPass];
		mapMatrix_T& map = dsgraph_buffer.mapMatrixPasses[sh->flags.iPriority / 2][iPass];
		
		mapMatrixVS::TNode* Nvs = map.insert(&*pass.vs);
		mapMatrixGS::TNode* Ngs = Nvs->val.insert(pass.gs->gs);
		mapMatrixPS::TNode* Nps = Ngs->val.insert(pass.ps->ps);

		Nps->val.hs = pass.hs->sh;
		Nps->val.ds = pass.ds->sh;
		mapMatrixCS::TNode* Ncs		= Nps->val.mapCS.insert	(pass.constants._get());

		mapMatrixStates::TNode*		Nstate	= Ncs->val.insert	(pass.state->state);
		mapMatrixTextures::TNode*	Ntex	= Nstate->val.insert(pass.T._get());
		mapMatrixItems&				items	= Ntex->val;

		items.push_back(item);
	}
}


void R_dsgraph_structure::r_dsgraph_insert_static(dxRender_Visual *pVisual, IRenderBuffer& render_buffer)
{
	DsGraphBuffer& dsgraph_buffer = (DsGraphBuffer&)render_buffer;

	pVisual->protectVisMarker_.Enter();

	auto it = pVisual->vis.vis_marker.find(dsgraph_buffer.visMarkerId_);  // find our buffer mark

	// if dont have our buffer mark - create it(ususally first scene frames)
	if (it == pVisual->vis.vis_marker.end())
		pVisual->vis.vis_marker.insert(mk_pair(dsgraph_buffer.visMarkerId_, dsgraph_buffer.visMarker_));
	else if (it->second == dsgraph_buffer.visMarker_) // Avoid adding same geometry several times in the same ds graph buffer
	{
		pVisual->protectVisMarker_.Leave();

		return;
	}

	pVisual->protectVisMarker_.Leave();

	it->second = dsgraph_buffer.visMarker_;

	float distSQ;
	float SSA =	CalcSSA (distSQ, pVisual->vis.sphere.P, pVisual);

	if (SSA <= r_ssaDISCARD)
		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY(pVisual->shader._get());

	ShaderElement* sh_d = &*pVisual->shader->E[4];

	if (dsgraph_buffer.distortedGeom_ && sh_d && sh_d->flags.bDistort && dsgraph_buffer.phaseMask[sh_d->flags.iPriority / 2])
	{
		mapSorted_Node* N = dsgraph_buffer.mapDistort.insertInAnyWay(distSQ);

		N->val.ssa				= SSA;
		N->val.pObject			= NULL;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= Fidentity;
		N->val.se				= &*pVisual->shader->E[4];		// 4=L_special
	}

	// Select shader
	ShaderElement* sh = RImplementation.rimp_select_sh_static(pVisual, distSQ, dsgraph_buffer);

	if (!sh)
		return;

	if (!dsgraph_buffer.phaseMask[sh->flags.iPriority / 2])
		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)
	{
		mapSorted_Node* N = dsgraph_buffer.mapSorted.insertInAnyWay(distSQ);

		N->val.pObject				= NULL;
		N->val.pVisual				= pVisual;
		N->val.Matrix				= Fidentity;
		N->val.se					= sh;

		return;
	}

	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive)
	{
		mapSorted_Node* N = dsgraph_buffer.mapEmissive.insertInAnyWay(distSQ);

		N->val.ssa				= SSA;
		N->val.pObject			= NULL;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= Fidentity;
		N->val.se				= &*pVisual->shader->E[4];		// 4=L_special
	}

	if (sh->flags.bWmark && dsgraph_buffer.phaseMask[2])
	{
		mapSorted_Node* N = dsgraph_buffer.mapWmark.insertInAnyWay(distSQ);

		N->val.ssa				= SSA;
		N->val.pObject			= NULL;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= Fidentity;
		N->val.se				= sh;

		return;
	}

	if (dsgraph_buffer.testMeCallBack_ && dsgraph_buffer.staticVisualsCounter_ == dsgraph_buffer.indexOfVisualWeLookFor_)
	{
		dsgraph_buffer.testMeCallBack_->rfeedback_static(pVisual);

		dsgraph_buffer.indexOfVisualWeLookFor_ = 0;
		dsgraph_buffer.testMeCallBack_ = nullptr;
	}

	//if (&dsgraph_buffer == RImplementation.mainRenderPrior0CalculatingDsBuffer_ && dsgraph_buffer.staticCnt > devfloat4)
	//	return;

	dsgraph_buffer.staticVisualsCounter_++; // this one is not a statistic
	dsgraph_buffer.staticCnt++;

	for (u32 iPass = 0; iPass < sh->passes.size(); ++iPass)
	{
		SPass& pass = *sh->passes[iPass];
		mapNormal_T& map = dsgraph_buffer.mapNormalPasses[sh->flags.iPriority / 2][iPass];

		mapNormalVS::TNode* Nvs		= map.insert(&*pass.vs);
		mapNormalGS::TNode* Ngs		= Nvs->val.insert(pass.gs->gs);
		mapNormalPS::TNode* Nps		= Ngs->val.insert(pass.ps->ps);

		Nps->val.hs = pass.hs->sh;
		Nps->val.ds = pass.ds->sh;

		mapNormalCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());

		mapNormalStates::TNode*		Nstate	= Ncs->val.insert	(pass.state->state);
		mapNormalTextures::TNode*	Ntex	= Nstate->val.insert(pass.T._get());
		mapNormalItems&				items	= Ntex->val;
		_NormalItem					item	= {SSA,pVisual};

		items.push_back(item);
	}
}


// Cut off Dynamic geometry depending of size of geometry and distance to camera and current geometry optimization settings
IC bool IsValuableToRenderDyn(dxRender_Visual *pVisual, Fmatrix& transform_matrix, bool sm)
{
	if (ps_r_optimize_dynamic >= 1 || (sm && ps_r_high_optimize_shad))
	{
		float sphere_volume = pVisual->getVisData().sphere.volume();

		Fvector	Tpos;
		transform_matrix.transform_tiny(Tpos, pVisual->vis.sphere.P);
		
		float adjusted_distane = Device.currentVpSavedView->GetDistFromCameraMT(Tpos);

		if (sm && ps_r_high_optimize_shad) // Highest cut off for shadow map
		{
			if (adjusted_distane > ps_r2_sun_shadows_far_casc) // don't need geometry behind the farest sun shadow cascade
				return false;

			if ((sphere_volume < O_D_L1_S_ULT) && adjusted_distane > O_D_L1_D_ULT)
				return false;
			else if ((sphere_volume < O_D_L2_S_ULT) && adjusted_distane > O_D_L2_D_ULT)
				return false;
			else if ((sphere_volume < O_D_L3_S_ULT) && adjusted_distane > O_D_L3_D_ULT)
				return false;
			else
				return true;
		}
		else if (ps_r_optimize_dynamic >= 1)
		{
			if ((sphere_volume < o_optimize_dynamic_l1_size) && adjusted_distane > o_optimize_dynamic_l1_dist)
				return false;
			else if ((sphere_volume < o_optimize_dynamic_l2_size) && adjusted_distane > o_optimize_dynamic_l2_dist)
				return false;
			else if ((sphere_volume < o_optimize_dynamic_l3_size) && adjusted_distane > o_optimize_dynamic_l3_dist)
				return false;
			else
				return true;
		}
	}

	return true;
}

// Cut off Static geometry depending of size of geometry and distance to camera and current geometry optimization settings
IC bool IsValuableToRender(dxRender_Visual *pVisual, bool sm)
{
	if (ps_r_optimize_static >= 1 || (sm && ps_r_high_optimize_shad))
	{
		float sphere_volume = pVisual->getVisData().sphere.volume();

		float adjusted_distane = Device.currentVpSavedView->GetDistFromCameraMT(pVisual->vis.sphere.P);

		if (sm && ps_r_high_optimize_shad) // Highest cut off for shadow map
		{
			if (sphere_volume < 50000.f && adjusted_distane > ps_r2_sun_shadows_far_casc) // don't need geometry behind the farest sun shadow cascade
				return false;

			if ((sphere_volume < O_S_L1_S_ULT) && adjusted_distane > O_S_L1_D_ULT)
				return false;
			else if ((sphere_volume < O_S_L2_S_ULT) && adjusted_distane > O_S_L2_D_ULT)
				return false;
			else if ((sphere_volume < O_S_L3_S_ULT) && adjusted_distane > O_S_L3_D_ULT)
				return false;
			else if ((sphere_volume < O_S_L4_S_ULT) && adjusted_distane > O_S_L4_D_ULT)
				return false;
			else if ((sphere_volume < O_S_L5_S_ULT) && adjusted_distane > O_S_L5_D_ULT)
				return false;
			else
				return true;
		}
		else if (ps_r_optimize_static >= 1)
		{
			if ((sphere_volume < o_optimize_static_l1_size) && adjusted_distane > o_optimize_static_l1_dist)
				return false;
			else if ((sphere_volume < o_optimize_static_l2_size) && adjusted_distane > o_optimize_static_l2_dist)
				return false;
			else if ((sphere_volume < o_optimize_static_l3_size) && adjusted_distane > o_optimize_static_l3_dist)
				return false;
			else if ((sphere_volume < o_optimize_static_l4_size) && adjusted_distane > o_optimize_static_l4_dist)
				return false;
			else if ((sphere_volume < o_optimize_static_l5_size) && adjusted_distane > o_optimize_static_l5_dist)
				return false;
			else
				return true;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRender::add_leafs_Dynamic(dxRender_Visual *pVisual, IRenderBuffer& render_buffer, bool ignore_optimization)
{
	if (!pVisual)
		return;

	DsGraphBuffer& dsgraph_buffer = (DsGraphBuffer&)render_buffer;

	if (!ignore_optimization && !IsValuableToRenderDyn(pVisual, dsgraph_buffer.renderingMatrix_, dsgraph_buffer.phaseType_ == PHASE_SMAP))
		return;

	// Visual is 100% visible - simply add it
	xr_vector<dxRender_Visual*>::iterator I, E;	// it may be useful for 'hierrarhy' visual

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		if (dsgraph_buffer.phaseType_ == PHASE_SMAP) // don't add particles for shadow maps, since they are not drawn anyway
			return;

		// Add all children, doesn't perform any tests
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;

		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& pg_item = *i_it;

			if (pg_item._effect)
				add_leafs_Dynamic(pg_item._effect, render_buffer, ignore_optimization);

			for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_related.begin(); pit != pg_item._children_related.end(); pit++)
				add_leafs_Dynamic(*pit, render_buffer, ignore_optimization);

			for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_free.begin(); pit != pg_item._children_free.end(); pit++)
				add_leafs_Dynamic(*pit, render_buffer, ignore_optimization);
		}
	}
	return;
	case MT_HIERRARHY:
	{
		// Add all children, doesn't perform any tests
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;

		I = pV->children.begin();
		E = pV->children.end();

		for (; I != E; I++)
			add_leafs_Dynamic(*I, render_buffer, ignore_optimization);
	}
	return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		// Add all children, doesn't perform any tests
		CKinematics * pV = (CKinematics*)pVisual;
		BOOL _use_lod = FALSE;

		if (pV->m_lod)
		{
			Fvector Tpos;
			float D;

			dsgraph_buffer.renderingMatrix_.transform_tiny(Tpos, pV->vis.sphere.P);

			float ssa = CalcSSA(D, Tpos, pV->vis.sphere.R / 2.f);	// assume dynamics never consume full sphere

			if (ssa < r_ssaLOD_A)
				_use_lod = TRUE;
		}

		if (_use_lod)
		{
			add_leafs_Dynamic(pV->m_lod, render_buffer, ignore_optimization);
		}
		else
		{
			portectTempKinematicsVec_.Enter();

			objectsToCalcBones_.push_back(pV);

			if (dsgraph_buffer.phaseType_ == PHASE_NORMAL) // Wallmarked kinematics is only needed, when object is truly seen
				actualViewPortBufferNow->objectsToCalcWallMarks_.push_back(pV);

			portectTempKinematicsVec_.Leave();

			I = pV->children.begin();
			E = pV->children.end();

			for (; I != E; I++)
				add_leafs_Dynamic(*I, render_buffer, ignore_optimization);
		}
	}
	return;
	default:
	{
		// General type of visual
		// Calculate distance to it's center
		Fvector Tpos;

		dsgraph_buffer.renderingMatrix_.transform_tiny(Tpos, pVisual->vis.sphere.P);
		r_dsgraph_insert_dynamic(pVisual, Tpos, render_buffer);
	}
	return;
	}
}

void CRender::add_leafs_Static(dxRender_Visual *pVisual, IRenderBuffer& render_buffer)
{
	DsGraphBuffer& dsgraph_buffer = (DsGraphBuffer&)render_buffer;
	EFC_Visible	VIS_V = fcvNone;
	vis_data& vis = pVisual->vis;

	// Always test view frustum
	u32 planes_v = dsgraph_buffer.GetViewFrustum()->getMask();

	VIS_V = dsgraph_buffer.GetViewFrustum()->testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes_v);

	if (VIS_V == fcvNone)
		return;

	if (dsgraph_buffer.useHOM_ && !HOM.hom_visible(vis))
		return;

	if (!pVisual->ignoreGeometryDrawOptimization_ && !IsValuableToRender(pVisual, dsgraph_buffer.phaseType_ == PHASE_SMAP))
		return;

	// Visual is 100% visible - simply add it
	xr_vector<dxRender_Visual*>::iterator I, E;	// it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		if (dsgraph_buffer.phaseType_ == PHASE_SMAP) // don't add particles for shadow maps, since they are not drawn anyway
			return;

		// Add all children, doesn't perform any tests
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;
		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& pg_item = *i_it;

			if (pg_item._effect)
				add_leafs_Dynamic(pg_item._effect, render_buffer);

			for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_related.begin(); pit != pg_item._children_related.end(); pit++)
				add_leafs_Dynamic(*pit, render_buffer);

			for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_free.begin(); pit != pg_item._children_free.end(); pit++)
				add_leafs_Dynamic(*pit, render_buffer);
		}
	}
	return;
	case MT_HIERRARHY:
	{
		// Add all children, doesn't perform any tests
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;

		I = pV->children.begin();
		E = pV->children.end();

		for (; I != E; I++)
			add_leafs_Static(*I, render_buffer);
	}
	return;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		// Add all children, doesn't perform any tests
		CKinematics* pV = (CKinematics*)pVisual;

		portectTempKinematicsVec_.Enter();

		objectsToCalcBones_.push_back(pV);

		portectTempKinematicsVec_.Leave();

		I = pV->children.begin();
		E = pV->children.end();

		for (; I != E; I++)
			add_leafs_Static(*I, render_buffer);
	}
	return;
	case MT_LOD:
	{
		FLOD* pV = (FLOD*)pVisual;

		float D;
		float ssa = CalcSSA(D, pV->vis.sphere.P, pV);

		ssa *= pV->lod_factor;

		if (ssa<r_ssaLOD_A)
		{
			if (ssa<r_ssaDISCARD)
				return;

			mapLOD_Node* N = dsgraph_buffer.mapLOD.insertInAnyWay(D);

			N->val.ssa = ssa;
			N->val.pVisual = pVisual;
		}

		if (ssa > r_ssaLOD_B || dsgraph_buffer.phaseType_ == PHASE_SMAP)
		{
			// Add all children, doesn't perform any tests
			I = pV->children.begin();
			E = pV->children.end();

			for (; I != E; I++)
				add_leafs_Static(*I, render_buffer);
		}
	}
	return;
	case MT_TREE_PM:
	case MT_TREE_ST:
	{
		// General type of visual
		r_dsgraph_insert_static(pVisual, render_buffer);
	}
	return;
	default:
	{
		// General type of visual
		r_dsgraph_insert_static(pVisual, render_buffer);
	}
	return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CRender::add_Dynamic(dxRender_Visual *pVisual, u32 planes, IRenderBuffer& render_buffer)
{
	DsGraphBuffer& dsgraph_buffer = (DsGraphBuffer&)render_buffer;

	if (!pVisual->ignoreGeometryDrawOptimization_ && !IsValuableToRenderDyn(pVisual, dsgraph_buffer.renderingMatrix_, dsgraph_buffer.phaseType_ == PHASE_SMAP))
		return FALSE;

	// Check frustum visibility and calculate distance to visual's center
	Fvector	Tpos;	// transformed position
	EFC_Visible	VIS;

	dsgraph_buffer.renderingMatrix_.transform_tiny(Tpos, pVisual->vis.sphere.P);
	VIS = dsgraph_buffer.GetViewFrustum()->testSphere(Tpos, pVisual->vis.sphere.R, planes);

	if (fcvNone == VIS)
		return FALSE;

	// If we get here visual is visible or partially visible
	xr_vector<dxRender_Visual*>::iterator I, E;	// it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		if (dsgraph_buffer.phaseType_ == PHASE_SMAP) // don't add particles for shadow maps, since they are not drawn anyway
			return FALSE;

		// Add all children, doesn't perform any tests
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;

		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& pg_item = *i_it;

			if (fcvPartial == VIS)
			{
				if (pg_item._effect)
					add_Dynamic(pg_item._effect, planes, render_buffer);

				for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_related.begin(); pit != pg_item._children_related.end(); pit++)
					add_Dynamic(*pit, planes, render_buffer);

				for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_free.begin(); pit != pg_item._children_free.end(); pit++)
					add_Dynamic(*pit, planes, render_buffer);
			}
			else
			{
				if (pg_item._effect)
					add_leafs_Dynamic(pg_item._effect, render_buffer);

				for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_related.begin(); pit != pg_item._children_related.end(); pit++)
					add_leafs_Dynamic(*pit, render_buffer);

				for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_free.begin(); pit != pg_item._children_free.end(); pit++)
					add_leafs_Dynamic(*pit, render_buffer);
			}
		}
	}
	break;
	case MT_HIERRARHY:
	{
		// Add all children
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;

		I = pV->children.begin();
		E = pV->children.end();

		if (fcvPartial == VIS)
		{
			for (; I != E; I++)
				add_Dynamic(*I, planes, render_buffer);
		}
		else
		{
			for (; I != E; I++)
				add_leafs_Dynamic(*I, render_buffer);
		}
	}
	break;
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		// Add all children, doesn't perform any tests
		CKinematics* pV = (CKinematics*)pVisual;
		BOOL _use_lod = FALSE;

		if (pV->m_lod)
		{
			Fvector pos;
			float D;
			dsgraph_buffer.renderingMatrix_.transform_tiny(pos, pV->vis.sphere.P);

			float ssa = CalcSSA(D, pos, pV->vis.sphere.R / 2.f); // assume dynamics never consume full sphere

			if (ssa<r_ssaLOD_A)
				_use_lod = TRUE;
		}

		if (_use_lod)
		{
			add_leafs_Dynamic(pV->m_lod, render_buffer);
		}
		else
		{
			portectTempKinematicsVec_.Enter();

			objectsToCalcBones_.push_back(pV);

			if (dsgraph_buffer.phaseType_ == PHASE_NORMAL) // Wallmarked kinematics is only needed, when object is truly seen
				actualViewPortBufferNow->objectsToCalcWallMarks_.push_back(pV);

			portectTempKinematicsVec_.Leave();

			I = pV->children.begin();
			E = pV->children.end();

			for (; I != E; I++)
				add_leafs_Dynamic(*I, render_buffer);
		}
	}
	break;
	default:
	{
		// General type of visual
		r_dsgraph_insert_dynamic(pVisual, Tpos, render_buffer);
	}
	break;
	}
	return TRUE;
}

void CRender::add_Static(dxRender_Visual *pVisual, IRenderBuffer& render_buffer, xr_vector<CFrustum>* sector_frustums, u64 mask)
{
	// Check frustum visibility and calculate distance to visual's center
	EFC_Visible	VIS_V = fcvNone, VIS_S = fcvNone;
	vis_data& vis = pVisual->vis;
	DsGraphBuffer& dsgraph_buffer = (DsGraphBuffer&)render_buffer;

	// Always test view frustum
	u32 planes_v = dsgraph_buffer.GetViewFrustum()->getMask();

	VIS_V = dsgraph_buffer.GetViewFrustum()->testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes_v);

	if (fcvNone == VIS_V)
		return;

	u64 frustums_mask = mask;

	// Test sector frustums, if prompted
	if (sector_frustums && sector_frustums->size())
	{
		u64 loop_mask = 0;

		for (size_t i = 0; i < sector_frustums->size(); ++i)
		{
			loop_mask = u64(1) << i;

			if (mask & loop_mask) // basicly, this is to avoid testing non touching frustums in recursive calls to avoid slowdowns
			{
				u32 planes_s = sector_frustums->at(i).getMask();

				EFC_Visible res = sector_frustums->at(i).testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes_s);
				VIS_S = (EFC_Visible)_max((u32)VIS_S, (u32)res); // save only partitial or fully and the greatest

				if(res == fcvNone) // if not visible in this frustum - dont use it for recursive calls then
					frustums_mask &= ~loop_mask;
			}
		}

		if (VIS_S == fcvNone)
			return;
	}

	EFC_Visible visability = VIS_S; // use only sector frustum result, since view frustum test is just an additional check(otherwise breaks recursive calls)

	if (dsgraph_buffer.useHOM_ && !HOM.hom_visible(vis))
		return;

	if (!pVisual->ignoreGeometryDrawOptimization_ && !IsValuableToRender(pVisual, dsgraph_buffer.phaseType_ == PHASE_SMAP))
		return;

	// If we get here visual is visible or partially visible
	xr_vector<dxRender_Visual*>::iterator I, E;	// it may be usefull for 'hierrarhy' visuals

	switch (pVisual->Type) 
	{
	case MT_PARTICLE_GROUP:
	{
		if (dsgraph_buffer.phaseType_ == PHASE_SMAP) // don't add particles for shadow maps, since they are not drawn anyway
			return;

		// Add all children, doesn't perform any tests
		PS::CParticleGroup* pG = (PS::CParticleGroup*)pVisual;

		for (PS::CParticleGroup::SItemVecIt i_it = pG->items.begin(); i_it != pG->items.end(); i_it++)
		{
			PS::CParticleGroup::SItem& pg_item = *i_it;

			if (visability == fcvPartial)
			{
				if (pg_item._effect)
					add_Dynamic(pg_item._effect, planes_v, render_buffer);

				for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_related.begin(); pit != pg_item._children_related.end(); pit++)
					add_Dynamic(*pit, planes_v, render_buffer);

				for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_free.begin(); pit != pg_item._children_free.end(); pit++)
					add_Dynamic(*pit, planes_v, render_buffer);
			}
			else
			{
				if (pg_item._effect)
					add_leafs_Dynamic(pg_item._effect, render_buffer);

				for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_related.begin(); pit != pg_item._children_related.end(); pit++)
					add_leafs_Dynamic(*pit, render_buffer);

				for (xr_vector<dxRender_Visual*>::iterator pit = pg_item._children_free.begin(); pit != pg_item._children_free.end(); pit++)
					add_leafs_Dynamic(*pit, render_buffer);
			}
		}
	}
	break;
	case MT_HIERRARHY:
	{
		// Add all children
		FHierrarhyVisual* pV = (FHierrarhyVisual*)pVisual;

		I = pV->children.begin();
		E = pV->children.end();

		if (visability == fcvPartial)
		{
			for (; I != E; I++)
				add_Static(*I, render_buffer, sector_frustums, frustums_mask);
		}
		else
		{
			for (; I != E; I++)
				add_leafs_Static(*I, render_buffer);
		}
	}
	break;

	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		// Add all children, doesn't perform any tests
		CKinematics * pV = (CKinematics*)pVisual;

		portectTempKinematicsVec_.Enter();

		objectsToCalcBones_.push_back(pV);

		portectTempKinematicsVec_.Leave();

		I = pV->children.begin();
		E = pV->children.end();

		if (visability == fcvPartial)
		{
			for (; I != E; I++)
				add_Static(*I, render_buffer, sector_frustums, frustums_mask);
		}
		else
		{
			for (; I != E; I++)
				add_leafs_Static(*I, render_buffer);
		}
	}
	break;

	case MT_LOD:
	{
		FLOD* pV = (FLOD*)pVisual;
		float D;
		float ssa = CalcSSA(D, pV->vis.sphere.P, pV);

		ssa *= pV->lod_factor;

		if (ssa < r_ssaLOD_A && dsgraph_buffer.phaseType_ != PHASE_SMAP) // no lods in sm
		{
			if (ssa < r_ssaDISCARD)
				return;

			mapLOD_Node* N = dsgraph_buffer.mapLOD.insertInAnyWay(D);

			N->val.ssa = ssa;
			N->val.pVisual = pVisual;
		}

		if (ssa > r_ssaLOD_B || dsgraph_buffer.phaseType_ == PHASE_SMAP)
		{
			// Add all children, perform tests
			I = pV->children.begin();
			E = pV->children.end();

			for (; I != E; I++)
				add_leafs_Static(*I, render_buffer);
		}
	}
	break;
	case MT_TREE_ST:
	case MT_TREE_PM:
	{
		// General type of visual
		r_dsgraph_insert_static(pVisual, render_buffer);
	}
	return;
	default:
	{
		// General type of visual
		r_dsgraph_insert_static(pVisual, render_buffer);
	}
	break;
	}
}
