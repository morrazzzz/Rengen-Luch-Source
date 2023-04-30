
#pragma once

#include "../xrCore/xrPool.h"

#include "xr_collide_defs.h"

/*
Requirements:
0. Generic
	* O(1) insertion
		- radius completely determines	"level"
		- position completely detemines "node"
	* O(1) removal
	* 
1. Rendering
	* Should live inside spatial DB
	* Should have at least "bounding-sphere" or "bounding-box"
	* Should have pointer to "sector" it lives in
	* Approximate traversal order relative to point ("camera")
2. Spatial queries
	* Should live inside spatial DB
	* Should have at least "bounding-sphere" or "bounding-box"
*/

const float						c_spatial_min	= 8.f;
//////////////////////////////////////////////////////////////////////////
enum
{
	STYPE_RENDERABLE			= (1<<0),
	STYPE_LIGHTSOURCE			= (1<<1),
	STYPE_COLLIDEABLE			= (1<<2),
	STYPE_VISIBLEFORAI			= (1<<3),
	STYPE_REACTTOSOUND			= (1<<4),
	STYPE_PHYSIC				= (1<<5),
	STYPE_OBSTACLE				= (1<<6),
	STYPE_SHAPE					= (1<<7),
	STYPE_LIGHTSOURCEHEMI		= (1<<8),
};

enum SPACIAL_MT_BEHAVIOR
{
	eS_MT_CONDITION_1 = (1 << 0),
	eS_MT_CONDITION_2 = (1 << 1),
	eS_MT_CONDITION_3 = (1 << 2),
	eS_MT_CONDITION_4 = (1 << 3),
	eS_MT_CONDITION_5 = (1 << 4)
};

// Comment: 
//		ordinal objects			- renderable?, collideable?, visibleforAI?
//		physical-decorations	- renderable, collideable
//		lights					- lightsource
//		particles(temp-objects)	- renderable
//		glow					- renderable
//		sound					- ???

class 				ISpatial_NODE;
class 				IRender_Sector;
class 				ISpatial_DB;
namespace Feel { class Sound; }
class 				IRenderable;
class 				IRender_Light;

class XRCDB_API ISpatial
{
private:
	AccessLock					spatialUpdateProtect;
	std::atomic<u32>			spatialSectorUpdFrame; // Atomic, because its used in mt

protected:
	Flags32						spacialMTCond;
public:

	struct	_spatial
	{
		u32						s_type;
		Fsphere					sphere;
		Fvector					node_center;	// Cached node center for TBV optimization
		float					node_radius;	// Cached node bounds for TBV optimization
		ISpatial_NODE*			node_ptr;		// Cached parent node for "empty-members" optimization
		IRender_Sector*			current_sector;
		ISpatial_DB*			space;			// allow different spaces

		std::atomic<bool>		needSectorUpd;	// previously was a flag STYPEFLAG_INVALIDSECTOR // Atomic, because its used in mt

		_spatial() : s_type(0)	{}				// safe way to ensure type is zero before any contstructors takes place
	}							spatial;
public:
	BOOL						spatial_inside		();
				bool			spatial_updatesector_internal(u32 cur_frame);
				// optimized, so no need to call 'update' and then 'get', which would cause double critical section use
				bool			spatial_updatesector_internal_res(u32 cur_frame, IRender_Sector*& res_sector);
public:
				void 			spatial_move		();
				void _stdcall  	spatial_move_call	();

				void 			spatial_register		();
				void _stdcall  	spatial_register_call	();

				void 			spatial_unregister		();
				void _stdcall  	spatial_unregister_call	();

	virtual		Fvector			spatial_sector_point()	{ return spatial.sphere.P; }

	ICF			bool			spatial_updatesector(u32 cur_frame)
	{
		return spatial_updatesector_internal(cur_frame);
	};

	ICF			bool			spatial_updatesector_and_get_res(u32 cur_frame, IRender_Sector*& res_sector)
	{
		return spatial_updatesector_internal_res(cur_frame, res_sector);
	};

	IC IRender_Sector*			GetCurrentSector	() { spatialUpdateProtect.Enter(); IRender_Sector* res = spatial.current_sector;  spatialUpdateProtect.Leave(); return res; }

	virtual		CObject*		dcast_CObject		()	{ return 0;	}
	virtual		Feel::Sound*	dcast_FeelSound		()	{ return 0;	}
	virtual		IRenderable*	dcast_Renderable	()	{ return 0;	}
	virtual		IRender_Light*	dcast_Light			()	{ return 0; }

	void		SetMTCond							(SPACIAL_MT_BEHAVIOR mt_cond, bool value) { spacialMTCond.set(mt_cond, value); };

				ISpatial		(ISpatial_DB* space	);
	virtual		~ISpatial		();

protected:
	virtual		void			spatial_move_intern			();
	virtual		void			spatial_register_intern		();
	virtual		void			spatial_unregister_intern	();
};

//////////////////////////////////////////////////////////////////////////
//class ISpatial_NODE;
class 	ISpatial_NODE
{
public:
	typedef	_W64 unsigned		ptrt;
public:
	ISpatial_NODE*				parent;					// parent node for "empty-members" optimization
	ISpatial_NODE*				children		[8];	// children nodes
	xr_vector<ISpatial*>		items;					// own items
public:
	void						_init			(ISpatial_NODE* _parent);
	void						_remove			(ISpatial*		_S);
	void						_insert			(ISpatial*		_S);
	BOOL						_empty			()						
	{
		return items.empty() &&
			children[0] == nullptr && children[1] == nullptr &&
			children[2] == nullptr && children[3] == nullptr &&
			children[4] == nullptr && children[5] == nullptr &&
			children[6] == nullptr && children[7] == nullptr;
	}
};
////////////






//template <class T, int granularity>
//class	poolSS;
#ifndef	DLL_API
#	define DLL_API					__declspec(dllimport)
#endif // #ifndef	DLL_API

//////////////////////////////////////////////////////////////////////////
class XRCDB_API	ISpatial_DB
{
private:
	// We need to have several locks, to allow several threads perform spatial testing without a delay, but, when the spatial db is
	// being modified - no testing will be performed, since all the locks will be grabed by modifying thread. In this case, testings will
	// be delayed until modification is done
	AccessLock						spatialDBcs1_;
	AccessLock						spatialDBcs2_;
	AccessLock						spatialDBcs3_;

	poolSS< ISpatial_NODE, 128 >	allocator;

	xr_vector<ISpatial_NODE*>		allocator_pool;
	ISpatial*						rt_insert_object;
public:
	ISpatial_NODE*					m_root_node;
	Fvector							m_spatial_db_center;
	float							m_spatial_db_bounds;
	u32								stat_nodes;
	u32								stat_objects;
	CStatTimer						stat_insert;
	CStatTimer						stat_remove;
private:
	IC u32							_octant			(u32 x, u32 y, u32 z)			{	return z*4 + y*2 + x;	}
	IC u32							_octant			(Fvector& base, Fvector& rel)
	{
		u32 o	= 0;
		if (rel.x > base.x) o+=1;
		if (rel.y > base.y) o+=2;
		if (rel.z > base.z) o+=4;
		return	o;
	}

	ISpatial_NODE*					_node_create	();
	void 							_node_destroy	(ISpatial_NODE* &P);

	void							_insert			(ISpatial_NODE* N, Fvector& n_center, float n_radius);
	void							_remove			(ISpatial_NODE* N, ISpatial_NODE* N_sub);
public:
	ISpatial_DB();
	~ISpatial_DB();

	// managing
	void							initialize		(Fbox& BB);
	//void							destroy			();
	void							insert_spatial	(ISpatial* S);
	void							remove_spatial	(ISpatial* S);
	void							update			(u32 nodes=8);
	BOOL							verify			();

public:
	enum
	{
		O_ONLYFIRST		= (1<<0),
		O_ONLYNEAREST	= (1<<1),
		O_ORDERED		= (1<<2),
		O_force_u32		= u32(-1)
	};

	// query
	void							q_ray			(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_and, const Fvector&		_start,  const Fvector&	_dir, float _range);
	void							q_box			(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or,  const Fvector&		_center, const Fvector& _size);
	void							q_sphere		(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or,  const Fvector&		_center, const float _radius);
	void							q_frustum		(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or,  const CFrustum&	_frustum);

	static void						OnFrameEnd			();
};

XRCDB_API extern ISpatial_DB*		g_SpatialSpace			;
XRCDB_API extern ISpatial_DB*		g_SpatialSpacePhysic	;
