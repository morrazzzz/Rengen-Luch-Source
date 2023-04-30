#include "stdafx.h"
#include "xr_area.h"
#include "ispatial.h"
#include "../xr_3da/xr_collide_form.h"
#include "../xr_3da/xr_object.h"
#include "../xr_3da/cl_intersect.h"
#include "d3d9types.h"

#ifdef	DEBUG
static BOOL _cdb_bDebug = false;
extern XRCDB_API BOOL *cdb_bDebug = &_cdb_bDebug;
bool bDebug()
{
	return !!(*cdb_bDebug);
}
#endif
using namespace	collide;


// tatarinrafa: Оптимизированно для использования в многопотчной среде. Цель - позволить хотя бы трем потокам получать результаты со скоростью,
// не сильно отличающейся от скорости при получении результата для одного потока. 
// Теперь имеет три хранилища xrc_1 xrc_2 и xrc_3, что дает возможность получать результаты по статическим объектам сразу трем потокам, без особых
// задержек из-за захваченного мутекса. 

#define MAX_TRIES 100
#define MAX_XRC_COUNT 3
#define MAX_DYN_COUNT 2

void CObjectSpace::EnterXRC(AccessLock*& lock, xrXRC*& xrc, collide::rq_results*& rq_res)
{
	CTimer TD; TD.Start();

	for (u8 i = 0; i < MAX_TRIES; i++)
	{
		bool leave = false;

		for (u8 y = 0; y < MAX_XRC_COUNT; y++)
		{
			if (y == 0 && StaticLock_1.TryEnter())
			{
				lock = &StaticLock_1;
				xrc = &xrc_1;
				rq_res = &r_temp_1_st;

				leave = true;

				break;
			}
			else if (y == 1 && StaticLock_2.TryEnter())
			{
				lock = &StaticLock_2;
				xrc = &xrc_2;
				rq_res = &r_temp_2_st;

				leave = true;

				break;
			}
			else if (y == 2 && StaticLock_3.TryEnter())
			{
				lock = &StaticLock_3;
				xrc = &xrc_3;
				rq_res = &r_temp_3_st;

				leave = true;

				break;
			}
			else if (i >= MAX_TRIES - 1) // if tried enough times, and still all xrcs are buisy, enter into last xrc and use it, when its not buisy
			{
				StaticLock_3.Enter();
				lock = &StaticLock_3;
				xrc = &xrc_3;
				rq_res = &r_temp_3_st;

				leave = true;

				break;
			}
		}

		if (leave)
			break;
	}

	R_ASSERT(lock); R_ASSERT(xrc); R_ASSERT(rq_res);

#ifdef MEASUR_XR_AREA
	// Measure delays in entering critical sections
	if (GetCurrentThreadId() == Core.mainThreadID)
	{
		STATIC_ADD(measureMainThreadSDelayProtect_, mainThreadStaticDelayTime_, TD.GetElapsed_sec());
	}
	else
	{
		STATIC_ADD(measureOtherThreadsSDelayProtect_, otherThreadsStaticDelayTime_, TD.GetElapsed_sec());
	}
#endif
}


void CObjectSpace::EnterDYN(AccessLock*& lock, collide::rq_results*& rq_res, xr_vector<ISpatial*>*& spatial_pool)
{
	CTimer TD; TD.Start();

	for (u8 i = 0; i < MAX_TRIES; i++)
	{
		bool leave = false;

		for (u8 y = 0; y < MAX_DYN_COUNT; y++)
		{
			if (y == 0 && DynLock_1.TryEnter())
			{
				lock = &DynLock_1;
				rq_res = &r_temp_1_dyn;
				spatial_pool = &r_spatial_1;

				leave = true;

				break;
			}
			else if (y == 1 && DynLock_2.TryEnter())
			{
				lock = &DynLock_2;
				rq_res = &r_temp_2_dyn;
				spatial_pool = &r_spatial_2;

				leave = true;

				break;
			}
			else if (i >= MAX_TRIES - 1) // if tried enough times, and still all xrcs are buisy, enter into last xrc and use it, when its not buisy
			{
				DynLock_2.Enter();
				lock = &DynLock_2;
				rq_res = &r_temp_2_dyn;
				spatial_pool = &r_spatial_2;

				leave = true;

				break;
			}
		}

		if (leave)
			break;
	}

	R_ASSERT(lock); R_ASSERT(rq_res); R_ASSERT(spatial_pool);

#ifdef MEASUR_XR_AREA
	// Measure delays in entering critical sections
	if (GetCurrentThreadId() == Core.mainThreadID)
	{
		DYN_ADD(measureMainThreadDDelayProtect_, mainThreadDynDelayTime_, TD.GetElapsed_sec());
	}
	else
	{
		DYN_ADD(measureOtherThreadsDDelayProtect_, otherThreadsDynDelayTime_, TD.GetElapsed_sec());
	}
#endif
}


//--------------------------------------------------------------------------------
// RayTests
//--------------------------------------------------------------------------------


BOOL CObjectSpace::RayTest	( const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::ray_cache* cache, CObject* ignore_object)
{
	CObject* temp = nullptr;

	BOOL _ret = _RayTest(start, dir, range, tgt, cache, ignore_object, temp);

	return _ret;
}


BOOL CObjectSpace::RayTestD(const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::ray_cache* cache, CObject* ignore_object, CObject*& collided_obj)
{
	BOOL _ret = _RayTest(start, dir, range, tgt, cache, ignore_object, collided_obj);

	return _ret;
}


BOOL CObjectSpace::_RayTest(const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::ray_cache* cache, CObject* ignore_object, CObject*& collided_obj)
{
	VERIFY(_abs(dir.magnitude() - 1)<EPS);

	// dynamic test
	if (tgt & rqtDyn)
	{
		CTimer T; T.Start();
		
		AccessLock* lock_to_leave = nullptr;
		xr_vector<ISpatial*>* spatial_pool = nullptr;
		collide::rq_results* rq_res_to_use = nullptr;

		EnterDYN(lock_to_leave, rq_res_to_use, spatial_pool); // Get the non-buisy xrc, its mutex and rq res storage

		R_ASSERT(lock_to_leave); R_ASSERT(spatial_pool); R_ASSERT(rq_res_to_use);

		collide::ray_defs Q(start, dir, range, CDB::OPT_ONLYFIRST, tgt);

		spatial_pool->clear();
		rq_res_to_use->r_clear();

		u32	d_flags = STYPE_COLLIDEABLE | ((tgt&rqtObstacle) ? STYPE_OBSTACLE : 0) | ((tgt & rqtShape) ? STYPE_SHAPE : 0);

		// traverse object database
		g_SpatialSpace->q_ray(*spatial_pool, 0, d_flags, start, dir, range);

		// Determine visibility for dynamic part of scene
		for (u32 o_it = 0; o_it<spatial_pool->size(); o_it++)
		{
			ISpatial*	spatial = spatial_pool->at(o_it);
			CObject*	collidable = spatial->dcast_CObject();

			if (collidable && (collidable != ignore_object))
			{
				ECollisionFormType tp = collidable->collidable.model->Type();

				if ((tgt & (rqtObject | rqtObstacle)) && (tp == cftObject) && collidable->collidable.model->_RayQuery(Q, *rq_res_to_use))
				{
					collided_obj = collidable;

					lock_to_leave->Leave();

					DYN_ADD(measureDProtect_, dynCalcTime_, T.GetElapsed_sec());

					return TRUE;
				}

				if ((tgt & rqtShape) && (tp == cftShape) && collidable->collidable.model->_RayQuery(Q, *rq_res_to_use))
				{
					collided_obj = collidable;

					lock_to_leave->Leave();

					DYN_ADD(measureDProtect_, dynCalcTime_, T.GetElapsed_sec());

					return TRUE;
				}
			}
		}

		lock_to_leave->Leave();

		DYN_ADD(measureDProtect_, dynCalcTime_, T.GetElapsed_sec());
	}

	// static test
	if (tgt & rqtStatic)
	{
		CTimer T; T.Start();

		AccessLock* lock_to_leave = nullptr;
		xrXRC* xrc_to_use = nullptr;
		collide::rq_results* rq_res_to_use = nullptr;

		EnterXRC(lock_to_leave, xrc_to_use, rq_res_to_use); // Get the non-buisy xrc, its mutex and rq res storage

		R_ASSERT(xrc_to_use); R_ASSERT(lock_to_leave); R_ASSERT(rq_res_to_use);

		{
			xrc_to_use->ray_options(CDB::OPT_ONLYFIRST);

			// If we get here - test static model
			if (cache)
			{
				// 0. similar query???
				if (cache->similar(start, dir, range))
				{
					lock_to_leave->Leave();

					STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec());

					return cache->result;
				}

				// 1. Check cached polygon
				float _u, _v, _range;

				if (CDB::TestRayTri(start, dir, cache->verts, _u, _v, _range, false))
				{
					if (_range > 0 && _range < range) { lock_to_leave->Leave(); STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec()); return TRUE; }
				}


				// 2. Polygon doesn't pick - real database query
				xrc_to_use->ray_query(&Static, start, dir, range);

				if (0 == xrc_to_use->r_count())
				{
					cache->set(start, dir, range, FALSE);

					lock_to_leave->Leave();

					STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec());

					return FALSE;
				}
				else
				{
					// cache polygon
					cache->set(start, dir, range, TRUE);

					CDB::RESULT*	R = xrc_to_use->r_begin();
					CDB::TRI&		Tr = Static.get_tris()[R->id];
					Fvector*		V = Static.get_verts();

					cache->verts[0].set(V[Tr.verts[0]]);
					cache->verts[1].set(V[Tr.verts[1]]);
					cache->verts[2].set(V[Tr.verts[2]]);

					lock_to_leave->Leave();

					STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec());

					return TRUE;
				}
			}
			else
			{
				xrc_to_use->ray_query(&Static, start, dir, range);

				lock_to_leave->Leave();

				STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec());

				return xrc_to_use->r_count();
			}

			lock_to_leave->Leave();

			STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec());
		}
	}

	return FALSE;
}


//--------------------------------------------------------------------------------
// RayPicks
//--------------------------------------------------------------------------------


BOOL CObjectSpace::RayPick	( const Fvector &start, const Fvector &dir, float range, rq_target tgt, rq_result& R, CObject* ignore_object)
{
	BOOL	_res	= _RayPick(start,dir,range,tgt,R,ignore_object);

	return	_res;
}

BOOL CObjectSpace::_RayPick	( const Fvector &start, const Fvector &dir, float range, rq_target tgt, rq_result& R, CObject* ignore_object)
{
	R.O	= 0;
	R.range = range;
	R.element = -1;

	if (tgt&rqtStatic)
	{ 
		CTimer T; T.Start();

		AccessLock* lock_to_leave = nullptr;
		xrXRC* xrc_to_use = nullptr;
		collide::rq_results* rq_res_to_use = nullptr;

		EnterXRC(lock_to_leave, xrc_to_use, rq_res_to_use); // Get the non-buisy xrc, its mutex and rq res storage

		R_ASSERT(xrc_to_use); R_ASSERT(lock_to_leave); R_ASSERT(rq_res_to_use);

		{
			xrc_to_use->ray_options(CDB::OPT_ONLYNEAREST | CDB::OPT_CULL);
			xrc_to_use->ray_query(&Static, start, dir, range);

			if (xrc_to_use->r_count())
			{
				R.set_if_less(xrc_to_use->r_begin());
			}

			lock_to_leave->Leave();

			STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec());
		}
	}


	if (tgt&rqtDyn)
	{ 
		CTimer T; T.Start();

		AccessLock* lock_to_leave = nullptr;
		xr_vector<ISpatial*>* spatial_pool = nullptr;
		collide::rq_results* rq_res_to_use = nullptr;

		EnterDYN(lock_to_leave, rq_res_to_use, spatial_pool); // Get the non-buisy xrc, its mutex and rq res storage

		R_ASSERT(lock_to_leave); R_ASSERT(spatial_pool); R_ASSERT(rq_res_to_use);

		rq_res_to_use->r_clear();
		spatial_pool->clear();

		collide::ray_defs Q		(start,dir,R.range,CDB::OPT_ONLYNEAREST|CDB::OPT_CULL,tgt);

		// traverse object database
		u32 d_flags = STYPE_COLLIDEABLE|((tgt&rqtObstacle)?STYPE_OBSTACLE:0)|((tgt&rqtShape)?STYPE_SHAPE:0);

		g_SpatialSpace->q_ray(*spatial_pool, 0, d_flags, start, dir, range);

		// Determine visibility for dynamic part of scene

		for (u32 o_it = 0; o_it<spatial_pool->size(); o_it++)
		{
			ISpatial*	spatial = spatial_pool->at(o_it);
			CObject*	collidable		= spatial->dcast_CObject();

			if			(0==collidable)				continue;
			if			(collidable==ignore_object)	continue;

			ECollisionFormType tp		= collidable->collidable.model->Type();

			if (((tgt&(rqtObject|rqtObstacle))&&(tp==cftObject))||((tgt&rqtShape)&&(tp==cftShape)))
			{
				u32		C	= D3DCOLOR_XRGB	(64,64,64);
				Q.range		= R.range;

				if (collidable->collidable.model->_RayQuery(Q, *rq_res_to_use))
				{
					C				= D3DCOLOR_XRGB(128,128,196);
					R.set_if_less(rq_res_to_use->r_begin());
				}
#ifdef DEBUG
				if (bDebug()){
					Fsphere	S;		S.P = spatial->spatial.sphere.P; S.R = spatial->spatial.sphere.R;
					(*m_pRender)->dbgAddSphere(S,C);
					//dbg_S.push_back	(mk_pair(S,C));
				}
#endif
			}
		}

		lock_to_leave->Leave();

		DYN_ADD(measureDProtect_, dynCalcTime_, T.GetElapsed_sec());
	}

	return (R.element>=0);
}


//--------------------------------------------------------------------------------
// RayQueries
//--------------------------------------------------------------------------------


BOOL CObjectSpace::RayQuery		(collide::rq_results& dest, const collide::ray_defs& R, collide::rq_callback* CB, LPVOID user_data, collide::test_callback* tb, CObject* ignore_object)
{
	BOOL						_res = _RayQuery2(dest,R,CB,user_data,tb,ignore_object);

	return						(_res);
}

BOOL CObjectSpace::_RayQuery2	(collide::rq_results& r_dest, const collide::ray_defs& R, collide::rq_callback* CB, LPVOID user_data, collide::test_callback* tb, CObject* ignore_object)
{
	// initialize query
	r_dest.r_clear();

	rq_target s_mask = rqtStatic;
	rq_target d_mask = rq_target(((R.tgt&rqtObject) ? rqtObject : rqtNone) | ((R.tgt&rqtObstacle) ? rqtObstacle : rqtNone) | ((R.tgt&rqtShape) ? rqtShape : rqtNone));

	u32	d_flags = STYPE_COLLIDEABLE|((R.tgt&rqtObstacle)?STYPE_OBSTACLE:0)|((R.tgt&rqtShape)?STYPE_SHAPE:0);

	// Test static
	if (R.tgt&s_mask)
	{ 
		CTimer T; T.Start();

		AccessLock* lock_to_leave = nullptr;
		xrXRC* xrc_to_use = nullptr;
		collide::rq_results* rq_res_to_use = nullptr;

		EnterXRC(lock_to_leave, xrc_to_use, rq_res_to_use); // Get the non-buisy xrc, its mutex and rq res storage

		R_ASSERT(xrc_to_use); R_ASSERT(lock_to_leave); R_ASSERT(rq_res_to_use);

		{
			rq_res_to_use->r_clear();

			xrc_to_use->ray_options(R.flags);
			xrc_to_use->ray_query(&Static, R.start, R.dir, R.range);

			if (xrc_to_use->r_count())
			{
				CDB::RESULT* _I = xrc_to_use->r_begin();
				CDB::RESULT* _E = xrc_to_use->r_end();
				for (; _I != _E; _I++)
					rq_res_to_use->append_result(rq_result().set(0, _I->range, _I->id));
			}

			if (R.tgt&d_mask)	// if need dyn too, then temporary copy it to result pool, so that its not lost because of MT
				r_dest = *rq_res_to_use;
			else				// Or sort and find the result in static pool only
			{
				if (rq_res_to_use->r_count())
				{
					rq_res_to_use->r_sort();

					collide::rq_result* _I = rq_res_to_use->r_begin();
					collide::rq_result* _E = rq_res_to_use->r_end();

					for (; _I != _E; _I++)
					{
						r_dest.append_result(*_I);
						if (!(CB ? CB(*_I, user_data) : TRUE))						{ lock_to_leave->Leave(); STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec()); return r_dest.r_count(); }
						if (R.flags&(CDB::OPT_ONLYNEAREST | CDB::OPT_ONLYFIRST))	{ lock_to_leave->Leave(); STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec()); return r_dest.r_count(); }
					}
				}
			}

			lock_to_leave->Leave();

			STATIC_ADD(measureSProtect_, staticCalcTime_, T.GetElapsed_sec());
		}
	}

	// Test dynamic
	if (R.tgt&d_mask)
	{ 
		CTimer T; T.Start();

		AccessLock* lock_to_leave = nullptr;
		xr_vector<ISpatial*>* spatial_pool = nullptr;
		collide::rq_results* rq_res_to_use = nullptr;

		EnterDYN(lock_to_leave, rq_res_to_use, spatial_pool); // Get the non-buisy xrc, its mutex and rq res storage

		R_ASSERT(lock_to_leave); R_ASSERT(spatial_pool); R_ASSERT(rq_res_to_use);

		rq_res_to_use->r_clear();

		// Get the results from static test
		*rq_res_to_use = r_dest;

		r_dest.r_clear();

		spatial_pool->clear_not_free();

		// Traverse object database
		g_SpatialSpace->q_ray(*spatial_pool, 0, d_flags, R.start, R.dir, R.range);

		for (u32 o_it = 0; o_it<spatial_pool->size(); o_it++)
		{
			CObject*	collidable = spatial_pool->at(o_it)->dcast_CObject();
			if			(0==collidable)				continue;
			if			(collidable==ignore_object)	continue;
			ICollisionForm*	cform		= collidable->collidable.model;
			ECollisionFormType tp		= collidable->collidable.model->Type();

			if (((R.tgt&(rqtObject|rqtObstacle))&&(tp==cftObject))||((R.tgt&rqtShape)&&(tp==cftShape)))
			{
				if (tb&&!tb(R,collidable,user_data))continue;
				cform->_RayQuery(R, *rq_res_to_use);
			}
		}

		if (rq_res_to_use->r_count())
		{
			rq_res_to_use->r_sort();

			collide::rq_result* _I = rq_res_to_use->r_begin();
			collide::rq_result* _E = rq_res_to_use->r_end();

			for (; _I != _E; _I++)
			{
				r_dest.append_result(*_I);
				if (!(CB ? CB(*_I, user_data) : TRUE))						{ lock_to_leave->Leave(); DYN_ADD(measureDProtect_, dynCalcTime_, T.GetElapsed_sec()); return r_dest.r_count(); }
				if (R.flags&(CDB::OPT_ONLYNEAREST | CDB::OPT_ONLYFIRST))	{ lock_to_leave->Leave(); DYN_ADD(measureDProtect_, dynCalcTime_, T.GetElapsed_sec()); return r_dest.r_count(); }
			}
		}

		lock_to_leave->Leave();

		DYN_ADD(measureDProtect_, dynCalcTime_, T.GetElapsed_sec());
	}



	return r_dest.r_count();
}

BOOL CObjectSpace::RayQuery	(collide::rq_results& r_dest, ICollisionForm* target, const collide::ray_defs& R)
{
	VERIFY					(target);

	r_dest.r_clear			();

	return target->_RayQuery(R,r_dest);
}
