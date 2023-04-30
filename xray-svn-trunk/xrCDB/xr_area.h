#ifndef __XR_AREA_H__
#define __XR_AREA_H__

#include "xr_collide_defs.h"

class 	ISpatial;
class 	ICollisionForm;
class 	CObject;

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/ObjectSpaceRender.h"
#include "xrXRC.h"

#include "xrcdb.h"

#define MEASUR_XR_AREA

#ifdef MEASUR_XR_AREA
#define STATIC_ADD(CS, WHERE, VAL) \
{\
	CS.Enter();\
	WHERE += VAL;\
	CS.Leave();\
}

#define DYN_ADD(CS, WHERE, VAL) \
{\
	CS.Enter();\
	WHERE += VAL;\
	CS.Leave();\
}
#else
#define STATIC_ADD(CS, WHERE, VAL)\
{\
}
#define DYN_ADD(CS, WHERE, VAL)\
{\
}

#endif

struct hdrCFORM;

class XRCDB_API CObjectSpace
{
private:
	// Runtime stuff. Note: Several samples of each are for MT perfomance

	AccessLock							StaticLock_1;
	AccessLock							StaticLock_2;
	AccessLock							StaticLock_3;

	AccessLock							DynLock_1;
	AccessLock							DynLock_2;

	AccessLock							getNearestLock_1;
	AccessLock							getNearestLock_2;
	AccessLock							getNearestLock_3;

	CDB::MODEL							Static;
	Fbox								m_BoundingVolume;

	xrXRC								xrc_1;				// MT: dangerous, should appear only inside StaticLock_1 cr section
	xrXRC								xrc_2;				// MT: dangerous, should appear only inside StaticLock_2 cr section
	xrXRC								xrc_3;				// MT: dangerous, should appear only inside StaticLock_3 cr section

	collide::rq_results					r_temp_1_dyn;		// MT: dangerous, should appear only inside DynLock_1 cr section
	collide::rq_results					r_temp_2_dyn;		// MT: dangerous, should appear only inside DynLock_2 cr section
	collide::rq_results					r_temp_1_st;		// MT: dangerous, should appear only inside StaticLock_1 cr section
	collide::rq_results					r_temp_2_st;		// MT: dangerous, should appear only inside StaticLock_2 cr section
	collide::rq_results					r_temp_3_st;		// MT: dangerous, should appear only inside StaticLock_3 cr section

	xr_vector<ISpatial*>				r_spatial_1;			// MT: dangerous
	xr_vector<ISpatial*>				r_spatial_2;			// MT: dangerous

	xr_vector<ISpatial*>				r_get_nearest_pool_1;	// MT: dangerous
	xr_vector<ISpatial*>				r_get_nearest_pool_2;	// MT: dangerous
	xr_vector<ISpatial*>				r_get_nearest_pool_3;	// MT: dangerous

#ifdef MEASUR_XR_AREA
	float								staticCalcTime_;
	float								dynCalcTime_;

	float								mainThreadStaticDelayTime_;
	float								otherThreadsStaticDelayTime_;

	float								mainThreadDynDelayTime_;
	float								otherThreadsDynDelayTime_;

	AccessLock							measureSProtect_;
	AccessLock							measureDProtect_;

	AccessLock							measureMainThreadSDelayProtect_;
	AccessLock							measureOtherThreadsSDelayProtect_;

	AccessLock							measureMainThreadDDelayProtect_;
	AccessLock							measureOtherThreadsDDelayProtect_;
#endif

public:

#ifdef DEBUG
	FactoryPtr<IObjectSpaceRender>		*m_pRender;
	//ref_shader							sh_debug;
	//clQueryCollision					q_debug;			// MT: dangerous
	//xr_vector<std::pair<Fsphere,u32> >	dbg_S;				// MT: dangerous
#endif

private:
	void								EnterXRC			(AccessLock*& lock, xrXRC*& xrc, collide::rq_results*& rq_res);
	void								EnterDYN			(AccessLock*& lock, collide::rq_results*& rq_res, xr_vector<ISpatial*>*& spatial_pool);

	void								EnterGetNearest		(AccessLock*& lock, xr_vector<ISpatial*>*& pool_to_use);

	BOOL								_RayTest			( const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::ray_cache* cache, CObject* ignore_object, CObject*& collided_obj);
	BOOL								_RayPick			( const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::rq_result& R, CObject* ignore_object );
	BOOL								_RayQuery2			( collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback* cb, LPVOID user_data, collide::test_callback* tb, CObject* ignore_object);
public:
										CObjectSpace		( );
										~CObjectSpace		( );

	void								Load				(  CDB::build_callback build_callback  );
	void								Load				(   LPCSTR path, LPCSTR fname, CDB::build_callback build_callback  );
	void								Load				(  IReader* R, CDB::build_callback build_callback  );
	void								Create				(  Fvector*	verts, CDB::TRI* tris, const hdrCFORM &H, CDB::build_callback build_callback  );

	// ::Cast a ray from 'start' towards 'dir' that hits objects of type 'tgt', but ignores 'ignore_object'. Ray is restricted to 'range'. Hitted static obj info is stored in 'cache'; Occluded/No
	BOOL								RayTest				( const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::ray_cache* cache, CObject* ignore_object);

	// ::Cast a ray from 'start' towards 'dir' that hits objects of type 'tgt', but ignores 'ignore_object'. Ray is restricted to 'range'. Hitted static obj info is stored in 'cache'. Pointer to hitted dyn object is stored in 'collided_obj'; Occluded/No
	BOOL								RayTestD			( const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::ray_cache* cache, CObject* ignore_object, CObject*& collided_obj);

	// Game raypick (nearest) - returns object and addititional params
	BOOL								RayPick				( const Fvector &start, const Fvector &dir, float range, collide::rq_target tgt, collide::rq_result& R, CObject* ignore_object );

	// General collision query
	BOOL								RayQuery			( collide::rq_results& dest, const collide::ray_defs& rq, collide::rq_callback* cb, LPVOID user_data, collide::test_callback* tb, CObject* ignore_object);
	BOOL								RayQuery			( collide::rq_results& dest, ICollisionForm* target, const collide::ray_defs& rq);

	bool								BoxQuery			( Fvector const & 		box_center, 
															  Fvector const & 		box_z_axis,
															  Fvector const & 		box_y_axis,
															  Fvector const	& 		box_sizes,
															  xr_vector<Fvector> *	out_tris );

	int									GetNearest			( xr_vector<CObject*>&	q_nearest, ICollisionForm *obj, float range );
	int									GetNearest			( xr_vector<CObject*>&	q_nearest, const Fvector &point, float range, CObject* ignore_object );
	int									GetNearest			( xr_vector<ISpatial*>& q_spatial, xr_vector<CObject*>&	q_nearest, const Fvector &point, float range, CObject* ignore_object );

	CDB::TRI*							GetStaticTris		() { return Static.get_tris();	}
	Fvector*							GetStaticVerts		() { return Static.get_verts(); }
	CDB::MODEL*							GetStaticModel		() { return &Static;			}

	const Fbox&							GetBoundingVolume	() { return m_BoundingVolume;}

#ifdef MEASUR_XR_AREA
	float								GetCalcTimeStatic()					{ measureSProtect_.Enter(); float res = staticCalcTime_; measureSProtect_.Leave(); return res; };
	void								SetCalcTimeStatic		(float v)	{ measureSProtect_.Enter(); staticCalcTime_ = v; measureSProtect_.Leave(); };

	float								GetCalcTimeDyn()					{ measureDProtect_.Enter(); float res = dynCalcTime_; measureDProtect_.Leave(); return res; };
	void								SetCalcTimeDyn			(float v)	{ measureDProtect_.Enter(); dynCalcTime_ = v; measureDProtect_.Leave(); };

	float								GetMainThreadStaticDelay()			{ measureMainThreadSDelayProtect_.Enter(); float res = mainThreadStaticDelayTime_; measureMainThreadSDelayProtect_.Leave(); return res; };
	void								SetMainThreadStaticDelay(float v)	{ measureMainThreadSDelayProtect_.Enter(); mainThreadStaticDelayTime_ = v; measureMainThreadSDelayProtect_.Leave(); };

	float								GetSThreadsStaticDelay	()			{ measureOtherThreadsSDelayProtect_.Enter(); float res = otherThreadsStaticDelayTime_; measureOtherThreadsSDelayProtect_.Leave(); return res; };
	void								SetSThreadsStaticDelay	(float v)	{ measureOtherThreadsSDelayProtect_.Enter(); otherThreadsStaticDelayTime_ = v; measureOtherThreadsSDelayProtect_.Leave(); };

	float								GetMainThreadDynDelay	()			{ measureMainThreadDDelayProtect_.Enter(); float res = mainThreadDynDelayTime_; measureMainThreadDDelayProtect_.Leave(); return res; };
	void								SetMainThreadDynDelay	(float v)	{ measureMainThreadDDelayProtect_.Enter(); mainThreadDynDelayTime_ = v; measureMainThreadDDelayProtect_.Leave(); };

	float								GetSThreadsDynDelay		()			{ measureOtherThreadsDDelayProtect_.Enter(); float res = otherThreadsDynDelayTime_; measureOtherThreadsDDelayProtect_.Leave(); return res; };
	void								SetSThreadsDynDelay		(float v)	{ measureOtherThreadsDDelayProtect_.Enter(); otherThreadsDynDelayTime_ = v; measureOtherThreadsDDelayProtect_.Leave(); };
#endif

#ifdef DEBUG
	void								dbgRender			();
	//ref_shader							dbgGetShader		()	{ return sh_debug;	}
#endif
};

#endif
