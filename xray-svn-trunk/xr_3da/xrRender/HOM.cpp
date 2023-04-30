
#include "stdafx.h"
#include "HOM.h"
#include "occRasterizer.h"
#include "../../xr_3da/GameFont.h"
#include "dxRenderDeviceRender.h"
#include "FBasicVisual.h"

void CHOM::CheckHOM()
{
	CTimer dull_waitng_timer; dull_waitng_timer.Start();

	MT.Enter(); // wait for aux thread, if it is already started to calculate and is still calculating

	bool secondary_thread_calculated = MT_SYNC();

	MT.Leave();

	if (!secondary_thread_calculated) // if aux thread did not even start to calculate up to this point - do it now in main thread
	{
		//R_ASSERT(false);

		MT_RENDER();
	}

	float dull_time = dull_waitng_timer.GetElapsed_sec() * 1000.f;

	RImplementation.vpStats->RenderMtWait += dull_time;

#ifdef MEASURE_MT
	Device.Statistic->mtRenderDelayHom_ += dull_time;
#endif

	//if (ps_r_mt_flags.test(R_FLAG_MT_CALC_HOM_DETAILS) && dull_time > 0.1f)
	//	R_ASSERT2(false, make_string("Not enought time for hom calculation %f", dull_time));
}

void __stdcall CHOM::MT_RENDER()
{
#ifdef MEASURE_MT
	CTimer measure_mt; measure_mt.Start();
#endif

	MT.Enter();

	bool b_main_menu_is_active = (g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive());

	if (MT_frame_rendered != CurrentFrame() && !b_main_menu_is_active)
	{
		CFrustum view_frust;
		Fmatrix transform = Device.vpSavedView1.GetFullTransform_saved_MT();
		view_frust.CreateFromMatrix(transform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

		EnableHOM();
		Render(view_frust);
	}

	RImplementation.HOM.alreadySentToAuxThread_ = false;

	MT.Leave();


#ifdef MEASURE_MT
	Device.Statistic->mtHOM_ += measure_mt.GetElapsed_sec();
#endif
}


CHOM::CHOM()
{
	bEnabled		= FALSE;
	m_pModel		= 0;
	m_pTris			= 0;

	alreadySentToAuxThread_ = false;
#ifdef DEBUG
	Device.seqRender.Add(this,REG_PRIORITY_LOW-1000);
#endif
}

CHOM::~CHOM()
{
#ifdef DEBUG
	Device.seqRender.Remove(this);
#endif
}

#pragma pack(push,4)
struct HOM_poly			
{
	Fvector	v1, v2, v3;
	u32 flags;
};
#pragma pack(pop)

IC float Area(Fvector& v0, Fvector& v1, Fvector& v2)
{
	float e1 = v0.distance_to(v1);
	float e2 = v0.distance_to(v2);
	float e3 = v1.distance_to(v2);
	
	float p  = (e1 + e2 + e3) / 2.f;

	return _sqrt(p * (p - e1) * (p - e2) * (p - e3));
}

void CHOM::Load()
{
	// Find and open file
	string_path fName;
	FS.update_path(fName,"$level$", "level.hom");

	VERIFY2(FS.exist(fName), "Occlusion map does not exist");
	
	if (!FS.exist(fName))
	{
		Msg("WARNING: Occlusion map '%s' not found.", fName);
		
		return;
	}

	Msg	("* Loading HOM: %s", fName);
	
	IReader* fs = FS.r_open(fName);
	IReader* S = fs->open_chunk(1);

	// Load tris and merge them
	CDB::Collector CL;

	while (!S->eof())
	{
		HOM_poly P;
		S->r(&P, sizeof(P));

		CL.add_face_packed_D(P.v1, P.v2, P.v3, P.flags, 0.01f);
	}
	
	// Determine adjacency
	xr_vector<u32> adjacency;
	CL.calc_adjacency (adjacency);

	// Create RASTER-triangles
	m_pTris = xr_alloc<occTri> (u32(CL.getTS()));

	for (u32 it = 0; it < CL.getTS(); it++)
	{
		CDB::TRI& clT = CL.getT()[it];
		occTri& rT	= m_pTris[it];
		Fvector& v0	= CL.getV()[clT.verts[0]];
		Fvector& v1	= CL.getV()[clT.verts[1]];
		Fvector& v2	= CL.getV()[clT.verts[2]];

		rT.adjacent[0] = (0xffffffff == adjacency[3 * it + 0]) ? ((occTri*) (-1)) : (m_pTris+adjacency[3 * it + 0]);
		rT.adjacent[1] = (0xffffffff == adjacency[3 * it + 1]) ? ((occTri*) (-1)) : (m_pTris+adjacency[3 * it + 1]);
		rT.adjacent[2] = (0xffffffff == adjacency[3 * it + 2]) ? ((occTri*) (-1)) : (m_pTris+adjacency[3 * it + 2]);

		rT.flags = clT.dummy;
		rT.area = Area(v0, v1, v2);

		if (rT.area < EPS_L)
		{
			Msg	("! Invalid HOM triangle (%f,%f,%f)-(%f,%f,%f)-(%f,%f,%f)", VPUSH(v0), VPUSH(v1), VPUSH(v2));
		}

		rT.plane.build(v0, v1, v2);

		rT.skip = 0;

		rT.center.add(v0, v1).add(v2).div(3.f);
	}

	// Create AABB-tree
	m_pModel = xr_new<CDB::MODEL>();
	m_pModel->build(CL.getV(), int(CL.getVS()), CL.getT(), int(CL.getTS()));

	bEnabled = TRUE;
	S->close();

	FS.r_close(fs);
}

void CHOM::Unload()
{
	xr_delete(m_pModel);
	xr_free(m_pTris);

	bEnabled = FALSE;
}

class pred_fb
{
public:
	occTri* m_pTris;
	Fvector camera;

public:
	pred_fb(occTri* _t) : m_pTris(_t)	{}
	pred_fb(occTri* _t, Fvector& _c) : m_pTris(_t), camera(_c)	{}

	ICF bool operator() (const CDB::RESULT& _1, const CDB::RESULT& _2) const
	{
		occTri&	t0 = m_pTris[_1.id];
		occTri&	t1 = m_pTris[_2.id];

		return	camera.distance_to_sqr(t0.center) < camera.distance_to_sqr(t1.center);
	}

	ICF bool operator() (const CDB::RESULT& _1)	const
	{
		occTri&	T = m_pTris[_1.id];

		return T.skip > CurrentFrame();
	}
};

void CHOM::Render_DB(CFrustum& base)
{
	//Update projection matrices on every frame to ensure valid HOM culling
	float view_dim	= occ_dim_0;

	Fmatrix	m_viewport =
	{
		view_dim / 2.f,			0.0f,					0.0f,		0.0f,
		0.0f,					-view_dim / 2.f,		0.0f,		0.0f,
		0.0f,					0.0f,					1.0f,		0.0f,
		view_dim / 2.f + 0 + 0,	view_dim / 2.f + 0 + 0,	0.0f,		1.0f
	};

	Fmatrix m_viewport_01 =
	{
		1.f / 2.f,			0.0f,				0.0f,		0.0f,
		0.0f,				-1.f / 2.f,			0.0f,		0.0f,
		0.0f,				0.0f,				1.0f,		0.0f,
		1.f / 2.f + 0 + 0,	1.f / 2.f + 0 + 0,	0.0f,		1.0f
	};

	m_xform.mul(m_viewport, Device.vpSavedView1.GetFullTransform_saved_MT());
	m_xform_01.mul(m_viewport_01, Device.vpSavedView1.GetFullTransform_saved_MT());

	R_ASSERT(m_pModel);

	if (!m_pModel)
		return;

	// Query DB
	xrc.frustum_options(0);
	xrc.frustum_query(m_pModel, base);

	if (0==xrc.r_count())
		return;

	// Prepare
	CDB::RESULT* it = xrc.r_begin();
	CDB::RESULT* end = xrc.r_end();
	
	Fvector COP = Device.vpSavedView1.GetCameraPosition_saved_MT();

	end = std::remove_if(it, end, pred_fb(m_pTris));

	std::sort(it, end, pred_fb(m_pTris, COP));

	// Build frustum with near plane only
	CFrustum clip;
	Fmatrix full_trans = Device.vpSavedView1.GetFullTransform_saved_MT();

	clip.CreateFromMatrix(full_trans, FRUSTUM_P_NEAR);

	sPoly src, dst;

	u32 _frame = CurrentFrame();

#ifdef DEBUG
	tris_in_frame				= xrc.r_count();
	tris_in_frame_visible		= 0;
#endif

	// Perfrom selection, sorting, culling
	for (; it != end; it++)
	{
		// Control skipping
		occTri& T = m_pTris[it->id];
		u32	next = _frame + ::Random.randI(3, 10);

		// Test for good occluder - should be improved :)
		if (!(T.flags || (T.plane.classify(COP) > 0)))
		{
			T.skip = next;
			continue;
		}

		// Access to triangle vertices
		CDB::TRI& t = m_pModel->get_tris()[it->id];
		Fvector* v = m_pModel->get_verts();

		src.clear();
		dst.clear();

		src.push_back	(v[t.verts[0]]);
		src.push_back	(v[t.verts[1]]);
		src.push_back	(v[t.verts[2]]);

		sPoly* P = clip.ClipPoly(src, dst);
		if (0 == P)
		{
			T.skip = next;

			continue;
		}

		// XForm and Rasterize
#ifdef DEBUG
		tris_in_frame_visible	++;
#endif
		u32 pixels = 0;
		int limit = int(P->size()) - 1;

		for (int k = 1; k < limit; k++)
		{
			m_xform.transform(T.raster[0], (*P)[0]);
			m_xform.transform(T.raster[1], (*P)[k + 0]);
			m_xform.transform(T.raster[2], (*P)[k + 1]);

			pixels += Raster.rasterize(&T);
		}

		if (0 == pixels)
		{
			T.skip = next;
			continue;
		}
	}
}

void CHOM::Render(CFrustum& base)
{
	if (!bEnabled)
		return;
	
	Device.Statistic->RenderCALC_HOM.Begin();
	Raster.clear();
	Render_DB(base);
	Raster.propagade();

	MT_frame_rendered = CurrentFrame();

	Device.Statistic->RenderCALC_HOM.End();
}

ICF	BOOL xform_b0(Fvector2& min, Fvector2& max, float& minz, const Fmatrix& X, float _x, float _y, float _z)
{
	float z = _x * X._13 + _y * X._23 + _z * X._33 + X._43;

	if (z < EPS)
		return TRUE;

	float iw = 1.f / (_x * X._14 + _y * X._24 + _z * X._34 + X._44);		
	min.x = max.x = (_x * X._11 + _y * X._21 + _z * X._31 + X._41) * iw;
	min.y = max.y = (_x * X._12 + _y * X._22 + _z * X._32 + X._42) * iw;

	minz = 0.f + z * iw;

	return FALSE;
}

ICF	BOOL xform_b1(Fvector2& min, Fvector2& max, float& minz, const Fmatrix& X, float _x, float _y, float _z)
{
	float t;
	float z = _x * X._13 + _y * X._23 + _z * X._33 + X._43;

	if (z < EPS)
		return TRUE;

	float iw = 1.f / (_x * X._14 + _y * X._24 + _z * X._34 + X._44);

	t = (_x * X._11 + _y * X._21 + _z * X._31 + X._41) * iw;

	if (t < min.x)
		min.x = t;
	else if (t > max.x)
		max.x = t;

	t = (_x * X._12 + _y * X._22 + _z * X._32 + X._42) * iw;

	if (t < min.y)
		min.y = t;
	else if (t > max.y)
		max.y = t;

	t = 0.f + z * iw;

	if (t < minz)
		minz = t;

	return FALSE;
}

IC BOOL _visible(const Fbox& B, const Fmatrix& m_xform_01)
{
	// Find min/max points of xformed-box
	Fvector2 min, max;
	float z;

	if (xform_b0(min, max, z, m_xform_01, B.min.x, B.min.y, B.min.z)) return TRUE;
	if (xform_b1(min, max, z, m_xform_01, B.min.x, B.min.y, B.max.z)) return TRUE;
	if (xform_b1(min, max, z, m_xform_01, B.max.x, B.min.y, B.max.z)) return TRUE;
	if (xform_b1(min, max, z, m_xform_01, B.max.x, B.min.y, B.min.z)) return TRUE;
	if (xform_b1(min, max, z, m_xform_01, B.min.x, B.max.y, B.min.z)) return TRUE;
	if (xform_b1(min, max, z, m_xform_01, B.min.x, B.max.y, B.max.z)) return TRUE;
	if (xform_b1(min, max, z, m_xform_01, B.max.x, B.max.y, B.max.z)) return TRUE;
	if (xform_b1(min, max, z, m_xform_01, B.max.x, B.max.y, B.min.z)) return TRUE;

	return Raster.test(min.x, min.y, max.x, max.y, z);
}

BOOL CHOM::hom_visible(const Fbox3& B)
{
	if (!bEnabled)
		return TRUE;

	if (B.contains(Device.currentVpSavedView->GetCameraPosition_saved_MT()))
		return TRUE;

	return _visible(B, m_xform_01);
}

BOOL CHOM::hom_visible(const Fbox2& B, float depth)
{
	if (!bEnabled)
		return TRUE;

	return Raster.test(B.min.x, B.min.y, B.max.x, B.max.y, depth);
}

BOOL CHOM::hom_visible(IRenderable* renderable)
{
	dxRender_Visual* visual = dynamic_cast<dxRender_Visual*>(renderable->renderable.visual);

	vis_data& vis = visual->vis;
	vis_data v_copy;

	vis.CopySelf(v_copy);
	v_copy.box = vis.box;
	v_copy.box.xform(renderable->renderable.xform);

	BOOL homvisible = hom_visible(v_copy);

	v_copy.CopySelf(vis);

	return homvisible;
}

#define MIN_TEST_DELAY 10
#define MAX_TEST_DELAY 25

BOOL CHOM::hom_visible(vis_data& vis)
{
	if (!bEnabled) // sometimes maps dont have hom data
		return TRUE;

	u32 cur_frame = CurrentFrame();
	
	// Anti thread race check
	// If same object is tested by several threads at same time - they can override each other for test variables, but atomic should protect from undefined values
	VERIFY(vis.nextHomTestframe < MAX_TEST_DELAY + 5);

	if (cur_frame < vis.nextHomTestframe)
		return vis.isVisible; // not at this time, return previous result

	BOOL result = _visible(vis.box, m_xform_01);
	u32  delay = 1; // if would be hidden - shedule to next frame

	if (result)	// visible	- delay next test
		delay = ::Random.randI(MIN_TEST_DELAY, MAX_TEST_DELAY);

	vis.nextHomTestframe = cur_frame + delay;
	vis.lastHomTestFrame = cur_frame;
	vis.isVisible = result;

	return result;
}

BOOL CHOM::hom_visible(const sPoly& P)
{
	if (!bEnabled)
		return TRUE;

	// Find min/max points of xformed-box

	Fvector2 min, max;
	float z;

	if (xform_b0(min, max, z, m_xform_01, P.front().x, P.front().y, P.front().z))
		return TRUE;

	for (u32 it = 1; it < P.size(); it++)
		if (xform_b1(min, max, z, m_xform_01, P[it].x, P[it].y, P[it].z))
			return TRUE;

	return Raster.test(min.x, min.y, max.x, max.y, z);
}

void CHOM::DisableHOM()
{
	bEnabled = FALSE;
}

void CHOM::EnableHOM()
{
	bEnabled = m_pModel ? TRUE : FALSE;
}

#ifdef DEBUG
void CHOM::OnRender	()
{
	if (RImplementation.currentViewPort != MAIN_VIEWPORT)
		return;
	
	Raster.on_dbg_render();

	if (psDeviceFlags.is(rsOcclusionDraw)){
		if (m_pModel){
			DEFINE_VECTOR		(FVF::L,LVec,LVecIt);
			static LVec	poly;	poly.resize(m_pModel->get_tris_count()*3);
			static LVec	line;	line.resize(m_pModel->get_tris_count()*6);
			for (int it=0; it<m_pModel->get_tris_count(); it++){
				CDB::TRI* T		= m_pModel->get_tris()+it;
				Fvector* verts	= m_pModel->get_verts();
				poly[it*3+0].set(*(verts+T->verts[0]),0x80FFFFFF);
				poly[it*3+1].set(*(verts+T->verts[1]),0x80FFFFFF);
				poly[it*3+2].set(*(verts+T->verts[2]),0x80FFFFFF);
				line[it*6+0].set(*(verts+T->verts[0]),0xFFFFFFFF);
				line[it*6+1].set(*(verts+T->verts[1]),0xFFFFFFFF);
				line[it*6+2].set(*(verts+T->verts[1]),0xFFFFFFFF);
				line[it*6+3].set(*(verts+T->verts[2]),0xFFFFFFFF);
				line[it*6+4].set(*(verts+T->verts[2]),0xFFFFFFFF);
				line[it*6+5].set(*(verts+T->verts[0]),0xFFFFFFFF);
			}
			RCache.set_xform_world(Fidentity);
			// draw solid
			Device.SetNearer(TRUE);
			RCache.set_Shader	(dxRenderDeviceRender::Instance().m_SelectionShader);
			RCache.dbg_Draw		(D3DPT_TRIANGLELIST,&*poly.begin(),poly.size()/3);
			Device.SetNearer(FALSE);
			// draw wire
			if (bDebug){
				RImplementation.rmNear();
			}else{
				Device.SetNearer(TRUE);
			}
			RCache.set_Shader	(dxRenderDeviceRender::Instance().m_SelectionShader);
			RCache.dbg_Draw		(D3DPT_LINELIST,&*line.begin(),line.size()/2);
			if (bDebug){
				RImplementation.rmNormal();
			}else{
				Device.SetNearer(FALSE);
			}
		}
	}
}
void CHOM::stats()
{
	if (m_pModel){
		CGameFont& F		= *Device.Statistic->Font();
		F.OutNext			(" **** HOM-occ ****");
		F.OutNext			("  visible:  %2d", tris_in_frame_visible);
		F.OutNext			("  frustum:  %2d", tris_in_frame);
		F.OutNext			("    total:  %2d", m_pModel->get_tris_count());
	}
}
#endif