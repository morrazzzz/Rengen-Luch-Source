#include "stdafx.h"
#include "ISpatial.h"
#include "frustum.h"

extern Fvector	c_spatial_offset[8];

class	walker
{
public:
	u32				mask;
	CFrustum*		F;
	ISpatial_DB*	space;
public:
	walker					(ISpatial_DB*	_space, u32 _mask, const CFrustum* _F)
	{
		mask	= _mask;
		F		= (CFrustum*)_F;
		space	= _space;
	}
	void		walk(const ISpatial_NODE* N, const Fvector& n_C, const float n_R, u32 fmask, xr_vector<ISpatial*>& R)
	{
		// box
		float	n_vR	=		2*n_R;
		Fbox	BB;		BB.set	(n_C.x-n_vR, n_C.y-n_vR, n_C.z-n_vR, n_C.x+n_vR, n_C.y+n_vR, n_C.z+n_vR);
		if		(fcvNone==F->testAABB(BB.data(),fmask))	return;

		// test items
		xr_vector<ISpatial*>::const_iterator _it = N->items.begin();
		xr_vector<ISpatial*>::const_iterator _end = N->items.end();
		for (; _it!=_end; _it++)
		{
			ISpatial*		S	= *_it;
			if (0==(S->spatial.s_type&mask))	continue;

			Fvector&		sC		= S->spatial.sphere.P;
			float			sR		= S->spatial.sphere.R;
			u32				tmask	= fmask;
			if (fcvNone==F->testSphere(sC,sR,tmask))	continue;

			R.push_back	(S);
		}

		// recurse
		float	c_R		= n_R/2;
		for (u32 octant=0; octant<8; octant++)
		{
			if (0==N->children[octant])	continue;
			Fvector		c_C;			c_C.mad	(n_C,c_spatial_offset[octant],c_R);
			walk						(N->children[octant],c_C,c_R,fmask, R);
		}
	}
};

void	ISpatial_DB::q_frustum		(xr_vector<ISpatial*>& R, u32 _o, u32 _mask, const CFrustum& _frustum)	
{
	AccessLock* lock_to_leave = nullptr;

	// Enter one of the free locks. When the spatial db is being modified, all locks are grabed and this test will be waiting for modification to finish
	// but it will not wait for other tests to finish, and will be performed without a delay, since testing functions dont modify spatial db, and several threads can do it simmultaniously
	if (spatialDBcs1_.TryEnter())
		lock_to_leave = &spatialDBcs1_;
	else if (spatialDBcs2_.TryEnter())
		lock_to_leave = &spatialDBcs2_;
	else
	{
		spatialDBcs3_.Enter();

		lock_to_leave = &spatialDBcs3_;
	}

	R_ASSERT(lock_to_leave);

	R.clear_not_free();

	walker				W(this, _mask, &_frustum); W.walk(m_root_node, m_spatial_db_center, m_spatial_db_bounds, _frustum.getMask(), R);
	
	lock_to_leave->Leave();
}
