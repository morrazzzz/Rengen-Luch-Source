#include "stdafx.h"

#include "xr_area.h"
#include "../xr_3da/xr_object.h"
#include "../xr_3da/xrLevel.h"
#include "../xr_3da/xr_collide_form.h"

using namespace	collide;

// Class	: CObjectSpace
// Purpose	: stores space slots

CObjectSpace::CObjectSpace():
	xrc_1(), xrc_2(), xrc_3() // initialize three pools for faster mt perfomance
#ifdef DEBUG
	,m_pRender(0)
#endif
{
#ifdef DEBUG
	if( RenderFactory )	
		m_pRender = CNEW(FactoryPtr<IObjectSpaceRender>)() ;

	//sh_debug.create				("debug\\wireframe","$null");
#endif
	m_BoundingVolume.invalidate	();

#ifdef MEASUR_XR_AREA
	staticCalcTime_				= 0.f;
	dynCalcTime_				= 0.f;
	mainThreadStaticDelayTime_	= 0.f;
	otherThreadsStaticDelayTime_= 0.f;
#endif
}

CObjectSpace::~CObjectSpace()
{

#ifdef DEBUG
	//sh_debug.destroy			();
	CDELETE(m_pRender);
#endif
}

//----------------------------------------------------------------------

#define MAX_TRIES 100
#define MAX_POOLS_COUNT 3

void CObjectSpace::EnterGetNearest(AccessLock*& lock, xr_vector<ISpatial*>*& pool_to_use)
{
	CTimer TD; TD.Start();

	for (u8 i = 0; i < MAX_TRIES; i++)
	{
		bool leave = false;

		for (u8 y = 0; y < MAX_POOLS_COUNT; y++)
		{
			if (y == 0 && getNearestLock_1.TryEnter())
			{
				lock = &getNearestLock_1;
				pool_to_use = &r_get_nearest_pool_1;

				leave = true;

				break;
			}
			else if (y == 1 && getNearestLock_2.TryEnter())
			{
				lock = &getNearestLock_2;
				pool_to_use = &r_get_nearest_pool_2;

				leave = true;

				break;
			}
			else if (y == 2 && getNearestLock_3.TryEnter())
			{
				lock = &getNearestLock_3;
				pool_to_use = &r_get_nearest_pool_3;

				leave = true;

				break;
			}
			else if (i >= MAX_TRIES - 1) // if tried enough times, and still all temp pool are buisy, enter into last getnearestlock and use it, when its not buisy
			{
				getNearestLock_3.Enter();
				lock = &getNearestLock_3;
				pool_to_use = &r_get_nearest_pool_3;

				leave = true;

				break;
			}
		}

		if (leave)
			break;
	}

	R_ASSERT(lock); R_ASSERT(pool_to_use);

#ifdef MEASUR_XR_AREA
	// Measure delays in entering critical sections
	if (GetCurrentThreadId() == Core.mainThreadID)
	{
		DYN_ADD(measureMainThreadSDelayProtect_, mainThreadStaticDelayTime_, TD.GetElapsed_sec());
	}
	else
	{
		DYN_ADD(measureOtherThreadsSDelayProtect_, otherThreadsStaticDelayTime_, TD.GetElapsed_sec());
	}
#endif
}

int CObjectSpace::GetNearest(xr_vector<ISpatial*>& q_spatial, xr_vector<CObject*>& q_nearest, const Fvector &point, float range, CObject* ignore_object )
{
	q_spatial.clear_not_free();

	// Query objects
	q_nearest.clear_not_free();

	Fsphere Q; Q.set(point, range);
	Fvector B; B.set(range, range, range);

	g_SpatialSpace->q_box(q_spatial, 0, STYPE_COLLIDEABLE, point, B);

	// Iterate
	xr_vector<ISpatial*>::iterator it	= q_spatial.begin();
	xr_vector<ISpatial*>::iterator end	= q_spatial.end();

	for (; it != end; it++)
	{
		CObject* O = (*it)->dcast_CObject();

		if (!O)
			continue;

		if (O == ignore_object)
			continue;

		Fsphere mS = { O->spatial.sphere.P, O->spatial.sphere.R };

		if (Q.intersect(mS))
			q_nearest.push_back(O);
	}

	return q_nearest.size();
}

//----------------------------------------------------------------------
IC int CObjectSpace::GetNearest(xr_vector<CObject*>& q_nearest, const Fvector &point, float range, CObject* ignore_object) // Slowest, since mt protected
{
	CTimer T; T.Start();

	AccessLock* lock_to_leave = nullptr;
	xr_vector<ISpatial*>* pool_to_use = nullptr;

	EnterGetNearest(lock_to_leave, pool_to_use);

	lock_to_leave->Enter();

	int res = (GetNearest(*pool_to_use, q_nearest, point, range, ignore_object));

	lock_to_leave->Leave();

	DYN_ADD(measureDProtect_, dynCalcTime_, T.GetElapsed_sec());

	return res;
}

IC int CObjectSpace::GetNearest(xr_vector<CObject*>& q_nearest, ICollisionForm* obj, float range)
{
	CObject* O = obj->Owner();
	return GetNearest(q_nearest, O->spatial.sphere.P, range + O->spatial.sphere.R, O);
}

void CObjectSpace::Load(CDB::build_callback build_callback)
{
	Load("$level$","level.cform", build_callback);
}

void CObjectSpace::Load(LPCSTR path, LPCSTR fname, CDB::build_callback build_callback)
{
#ifdef USE_ARENA_ALLOCATOR
	Msg( "CObjectSpace::Load, g_collision_allocator.get_allocated_size() - %d", int(g_collision_allocator.get_allocated_size()/1024.0/1024) );
#endif
	IReader *F = FS.r_open(path, fname);

	R_ASSERT(F);

	Load(F, build_callback);
}

void CObjectSpace::Load(IReader* F, CDB::build_callback build_callback)
{
	hdrCFORM H;

	F->r(&H, sizeof(hdrCFORM));
	Fvector* verts = (Fvector*)F->pointer();
	CDB::TRI* tris = xr_alloc<CDB::TRI>(H.facecount); 
	u8*begin= (u8*)(verts + H.vertcount);
	for (size_t i = 0; i < H.facecount; i++)
	{
		memcpy(&tris[i], begin, CDB::TRI::GetSizeof());
		begin += CDB::TRI::GetSizeof();
	}
	Create(verts, tris, H, build_callback);
	xr_free(tris);
	FS.r_close(F);
}

void CObjectSpace::Create(Fvector* verts, CDB::TRI* tris, const hdrCFORM &H, CDB::build_callback build_callback)
{
	R_ASSERT(CFORM_CURRENT_VERSION == H.version);

	Static.build(verts, H.vertcount, tris, H.facecount, build_callback);
	m_BoundingVolume.set(H.aabb);

	g_SpatialSpace->initialize(m_BoundingVolume);
	g_SpatialSpacePhysic->initialize(m_BoundingVolume);
	//Sound->set_geometry_occ				( &Static );
	//Sound->set_handler					( _sound_event );
}

//----------------------------------------------------------------------
#ifdef DEBUG
void CObjectSpace::dbgRender()
{
	(*m_pRender)->dbgRender();
}
/*
void CObjectSpace::dbgRender()
{
	R_ASSERT(bDebug);

	RCache.set_Shader(sh_debug);
	for (u32 i=0; i<q_debug.boxes.size(); i++)
	{
		Fobb&		obb		= q_debug.boxes[i];
		Fmatrix		X,S,R;
		obb.xform_get(X);
		RCache.dbg_DrawOBB(X,obb.m_halfsize,D3DCOLOR_XRGB(255,0,0));
		S.scale		(obb.m_halfsize);
		R.mul		(X,S);
		RCache.dbg_DrawEllipse(R,D3DCOLOR_XRGB(0,0,255));
	}
	q_debug.boxes.clear();

	for (i=0; i<dbg_S.size(); i++)
	{
		std::pair<Fsphere,u32>& P = dbg_S[i];
		Fsphere&	S = P.first;
		Fmatrix		M;
		M.scale		(S.R,S.R,S.R);
		M.translate_over(S.P);
		RCache.dbg_DrawEllipse(M,P.second);
	}
	dbg_S.clear();
}
*/
#endif
