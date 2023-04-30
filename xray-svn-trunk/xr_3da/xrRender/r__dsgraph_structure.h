#pragma once

#include "../../xr_3da/render.h"
#include "../../xrcdb/ispatial.h"
#include "r__dsgraph_types.h"
#include "r__sector.h"

// feedback	for receiving visuals
class R_feedback
{
public:
	virtual void rfeedback_static(dxRender_Visual*	V) = 0;
};

extern size_t dsGraphBuffersIDCounter_;

// Buffer to store all dsgraph building variables and ensure each bulder call is only using local variables to create rendering geom list
class DsGraphBuffer : public IRenderBuffer
{

public:
	DsGraphBuffer()
	{
		phaseType_				= 0;
		phaseMask[0]			= true;
		phaseMask[1]			= false;
		phaseMask[2]			= false;

		useHOM_					= true;

		testMeCallBack_			= nullptr;
		indexOfVisualWeLookFor_ = 0;

		staticVisualsCounter_	= 0;

		for (int i = 0; i < SHADER_PASSES_MAX; ++i)
		{
			mapNormalPasses[0][i].destroy();
			mapNormalPasses[1][i].destroy();
			mapMatrixPasses[0][i].destroy();
			mapMatrixPasses[1][i].destroy();
		}

		mapSorted.destroy();
		mapHUD.destroy();
		mapHUDEmissive.destroy();
		mapHUDSorted.destroy();
		mapLOD.destroy();
		mapDistort.destroy();
		mapWmark.destroy();
		mapEmissive.destroy();

		alreadyCalculated_ = false;

		dsGraphBuffersIDCounter_++;

		visMarkerId_	= dsGraphBuffersIDCounter_;
		visMarker_		= 0;

		distortedGeom_	= false;
		portalTraverserMark = 0;

		dynCnt			= 0;
		staticCnt		= 0;

		debugName		= "debug_ds_graph";
	};

	virtual ~DsGraphBuffer() { };

	u32						phaseType_;			// usually Normal(regular) or Shadow Map
	bool					phaseMask[3];		//0 - regulgar geom, 1 - translucent geom, 2 - wallmarks
	bool					useHOM_;
	bool					distortedGeom_;

	u32						portalTraverserOpt;

	xr_vector<ISpatial*>		renderables_;		// Temp pool for storing picked raw sector-checked dyn geometry
	xr_vector<ISpatial*>		renderables2_;		// Temp pool for storing picked raw unsorted dyn geometry

	R_feedback*				testMeCallBack_;			// feedback for geometry, which we wanna test with occ
	u32						indexOfVisualWeLookFor_;	// when we reach the index of this value - we do a testMeCallBack_ to assign the visual for tests

	u32						staticVisualsCounter_;

	u32						GetStaticVisualCount()				{ return staticVisualsCounter_; }
	void					ResetStaticVisualCount()			{ staticVisualsCounter_ = 0; }

	size_t					visMarkerId_;		// An id of visability structure, for ussage in checking "already added to vis structure" marker
	u32						visMarker_;			// A marker, which gets ++ each time we start to create visibles structure in this buffer. Ususaly ones per frame

	xr_vector<IRender_Sector*> sectorS;
	u32						portalTraverserMark;

	// Statistics
	u32						dynCnt;
	u32						staticCnt;
	shared_str				debugName;

	// Dynamic scene graph
	R_dsgraph::mapNormalPasses_T								mapNormalPasses[2];	// 2==(priority/2)
	R_dsgraph::mapMatrixPasses_T								mapMatrixPasses[2];
	R_dsgraph::mapSorted_T										mapSorted;
	R_dsgraph::mapHUD_T											mapHUD;
	R_dsgraph::mapHUD_T											mapHUDEmissive;
	R_dsgraph::mapHUD_T											mapHUDSorted;
	R_dsgraph::mapLOD_T											mapLOD;
	R_dsgraph::mapSorted_T										mapDistort;
	R_dsgraph::mapSorted_T										mapWmark;
	R_dsgraph::mapSorted_T										mapEmissive;

	AccessLock protectReadyCheck_;
	bool alreadyCalculated_;

	// Multithreading protection check
	bool	IsCalculated			()				{ protectReadyCheck_.Enter();  bool res = alreadyCalculated_; protectReadyCheck_.Leave(); return res; }
	void	SetCalculated			(bool val)		{ protectReadyCheck_.Enter();  alreadyCalculated_ = val; protectReadyCheck_.Leave(); }
};


class R_dsgraph_structure : public IRender_interface, public pureFrame, public pureFrameEnd
{
public:
	u32															render_phase;

	AccessLock													protectMTUnsafeRendering_;

	xr_vector<R_dsgraph::_LodItem,render_alloc<R_dsgraph::_LodItem> >	lstLODs;
	xr_vector<int,render_alloc<int> >									lstLODgroups;
	xr_vector<ISpatial*>												lstRenderables;
	xr_vector<ISpatial*>												lstSpatial;

	BOOL																b_loaded;

public:

	R_dsgraph_structure	()
	{
		b_loaded			= FALSE;
	};

	void r_dsgraph_destroy()
	{
		lstLODs.clear			();
		lstLODgroups.clear		();
		lstRenderables.clear	();
		lstSpatial.clear		();
	}

	void		r_dsgraph_insert_dynamic						(dxRender_Visual *pVisual, Fvector Center, IRenderBuffer& render_buffer);
	void		r_dsgraph_insert_static							(dxRender_Visual *pVisual, IRenderBuffer& render_buffer);

	// Draw geometry
	void		r_dsgraph_render_graph							(u32 _priority, DsGraphBuffer& render_buffer, bool _clear=true);
	void		r_dsgraph_render_graph_N						(u32 _priority, DsGraphBuffer& render_buffer, bool _clear=true);
	void		r_dsgraph_render_graph_M						(u32 _priority, DsGraphBuffer& render_buffer, bool _clear=true);

	void		r_dsgraph_render_hud							(DsGraphBuffer& render_buffer);
	void		r_dsgraph_render_hud_emissive					(DsGraphBuffer& render_buffer);
	void		r_dsgraph_render_hud_sorted						(DsGraphBuffer& render_buffer);
	void		r_dsgraph_render_hud_ui							(DsGraphBuffer& render_buffer);
	void		r_dsgraph_render_lods							(bool _setup_zb, bool _clear, DsGraphBuffer& render_buffer);
	void		r_dsgraph_render_sorted							(DsGraphBuffer& render_buffer);
	void		r_dsgraph_render_emissive						(DsGraphBuffer& render_buffer);
	void		r_dsgraph_render_wmarks							(DsGraphBuffer& render_buffer);
	void		r_dsgraph_render_distort						(DsGraphBuffer& render_buffer);

public:
	virtual u32	memory_usage()
	{
		return 0;//(g_render_lua_allocator.get_allocated_size());
	}
};
