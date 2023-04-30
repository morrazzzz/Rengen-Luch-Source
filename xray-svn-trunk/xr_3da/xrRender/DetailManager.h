
#pragma once

#include "../../xrCore/xrpool.h"
#include "detailformat.h"
#include "detailmodel.h"
#include "Light.h"
#include "DetailManagerCommon.h"

class ECORE_API CDetailManager
{
public:
	int								dither[16][16];

	SSwingValue						swing_desc[2];
	SSwingValue						swing_current; 
	float							m_time_rot_1;
	float							m_time_rot_2;
	float							m_time_pos;
	float							m_global_time_old;
public:
	IReader*						dtFS;
	DetailHeader					dtH;
	DetailSlot*						dtSlots;		// note: pointer into VFS
	DetailSlot						DS_empty;

public:
	DetailVec						objects;
	vis_list						m_visibles[3];	// 0=still, 1=Wave1, 2=Wave2
	vis_list						m_visibles2[3];	// 0=still, 1=Wave1, 2=Wave2 // Second VP1
	vis_list						m_visibles_sh_map[3];
	vis_list						m_visibles_sun_shadows[3];
	vis_list						m_visibles_sun_shadows2[3]; // Second VP1

	xr_vector<CLightSource*>		lightsToCheck_;

	u32								detailsToRenderCount_;
	u32								detailsToRenderCountSVP1_;
	u32								detailsWithSunShadowsCount_;
	u32								detailsWithSunShadowsCountSPV1_;
	u32								potentialDetailsWithShadowsCount_;

	xrXRC							xrc;

	CacheSlot1**					cache_level1;
	Slot***							cache; // grid-cache itself
	svector<Slot*,dm_max_cache_size>cache_task; // non-unpacked slots
	Slot*							cache_pool; // just memory for slots

	int								cache_cx;
	int								cache_cz;

	PSS								poolSI;										// pool из которого выдел€ютс€ SlotItem

	void							UpdateVisibleM();
	void							UpdateVisibles(); // for actual grass
	void							UpdateVisiblesSVP(); // Second vp grass
	void							UpdateVisibleShadowsRaw(); // for grass, which is in the range of ps_details_with_shadows_dist and can have shadows
	void							UpdateVisibleShadowsSun(); // for sun shadows
	void							UpdateVisibleShadowsSunSVP(); // for sun shadows second vp
	void							UpdateVisibleShadowsLights(); // for lights, with grass shadows
	bool							UpdateUsefullLight(CLightSource* light);

	void							UpdateVisibleS();
public:

	// Hardware processor
	ref_geom						hw_Geom;
	u32								hw_BatchSize;

	ID3DVertexBuffer*				hw_VB;
	ID3DIndexBuffer*				hw_IB;

	ref_constant					hwc_consts;
	ref_constant					hwc_wave;
	ref_constant					hwc_wind;
	ref_constant					hwc_array;
	ref_constant					hwc_s_consts;
	ref_constant					hwc_s_xform;
	ref_constant					hwc_s_array;
	void							hw_Load			();
	void							hw_Load_Geom	();
	void							hw_Load_Shaders	();
	void							hw_Unload		();
	void							hw_Render		(vis_list (&details_to_render)[3]);

	void							hw_Render_dump	(vis_list& details_to_render, const Fvector4 &consts, const Fvector4 &wave, const Fvector4 &wind, u32 lod_id);

public:
	// get unpacked slot
	DetailSlot&						QueryDB			(int sx, int sz);
    
	void							cache_Initialize();
	void							cache_Update	(int sx, int sz, Fvector& view, int limit);
	void							cache_Task		(int gx, int gz, Slot* D);
	Slot*							cache_Query		(int sx, int sz);
	void							cache_Decompress(Slot* D);
	BOOL							cache_Validate	();
    // cache grid to world
	int								cg2w_X			(int x)			{ return cache_cx - dm_size + x; }
	int								cg2w_Z			(int z)			{ return cache_cz - dm_size + (dm_cache_line - 1 - z); }
    // world to cache grid 
	int								w2cg_X			(int x)			{ return x - cache_cx + dm_size; }
	int								w2cg_Z			(int z)			{ return cache_cz - dm_size+(dm_cache_line - 1 - z); }

	float							ssaDISCARD_;

	void							Load			();
	void							Unload			();
	void							Render			(vis_list (&details_to_render)[3]);

	/// MT stuff
	AccessLock						MT;

	bool							alreadySentToAuxThread_;
	u32								frameStartedMTCalc_;

	volatile u32					m_frame_calc;

	void	__stdcall				MT_CALC			();

	ICF	bool						MT_SYNC			()
	{
		if (m_frame_calc == CurrentFrame())
			return true;

		return false;
	}

	void							CheckDetails();

	CDetailManager					();
	virtual ~CDetailManager			();
};
