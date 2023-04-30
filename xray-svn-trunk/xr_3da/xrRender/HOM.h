// HOM.h: interface for the CHOM class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "../../xr_3da/IGame_Persistent.h"

class occTri;

class CHOM  
#ifdef DEBUG
	: public pureRender
#endif
{
private:
	xrXRC					xrc;
	CDB::MODEL*				m_pModel;
	occTri*					m_pTris;
	BOOL					bEnabled;
	Fmatrix					m_xform;
	Fmatrix					m_xform_01;
#ifdef DEBUG
	u32						tris_in_frame_visible;
	u32						tris_in_frame;
#endif

	AccessLock				MT;
	volatile u32			MT_frame_rendered;

	void					Render_DB	(CFrustum&	base);
public:
	bool					alreadySentToAuxThread_;

	void					Load		();
	void					Unload		();
	void					Render		(CFrustum&	base);

	void					occlude		(Fbox2&		space) { }
	void					DisableHOM	();
	void					EnableHOM	();

	void					CheckHOM	();

	void	__stdcall		MT_RENDER	();

	ICF	bool				MT_SYNC		()
	{ 
		if (MT_frame_rendered == CurrentFrame())
			return true;

		return false;
	}
	// MT buggy, but ok. Use it in mt environment, where there a lot of hom calls since its Fast!
	BOOL					hom_visible		(vis_data& vis);
	// Very slow, since localy copies vis_data, but needed for some dyn scene tests
	BOOL					hom_visible		(IRenderable* renderable);

	BOOL					hom_visible		(const Fbox3& B);
	BOOL					hom_visible		(const sPoly& P);
	BOOL					hom_visible		(const Fbox2& B, float depth);	// viewport-space (0..1)

	CHOM	();
	~CHOM	();

#ifdef DEBUG
	virtual void			OnRender	();
			void			stats		();
#endif
};
