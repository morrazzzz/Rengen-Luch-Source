
#pragma once

#include "../../xrCore/xrpool.h"
#include "detailformat.h"
#include "detailmodel.h"

const int	dm_max_decompress = 7;

//thx to K.D.
const u32		dm_max_cache_size = 62001; // assuming max dm_size = 124
extern u32		dm_size;
extern u32 		dm_cache1_line;
extern u32		dm_cache_line;
extern u32		dm_cache_size;
extern float	dm_fade;
extern u32		dm_current_size;//				= iFloor((float)ps_r__Detail_radius/4)*2;				//!
extern u32 		dm_current_cache1_line;//		= dm_current_size*2/dm_cache1_count;		//! dm_current_size*2 must be div dm_cache1_count
extern u32		dm_current_cache_line;//		= dm_current_size+1+dm_current_size;
extern u32		dm_current_cache_size;//		= dm_current_cache_line*dm_current_cache_line;
extern float	dm_current_fade;//				= float(2*dm_current_size)-.5f;

const int 		dm_cache1_count = 4;
const int		dm_max_objects = 64;
const int		dm_obj_in_slot = 4;
const float		dm_slot_size = DETAIL_SLOT_SIZE;

	struct SlotItem // один кустик
	{
		float						scale;
		float						scale_calculated;
		Fmatrix						mRotY;
		u32							vis_ID;				// индекс в visibility списке он же тип [не качается, качается1, качается2]
		float						c_hemi;
		float						c_sun;

		void*						parent_slot;

		SlotItem()					{ parent_slot = nullptr; };
	};

	DEFINE_VECTOR(SlotItem*, SlotItemVec, SlotItemVecIt);

	struct SlotPart
	{
		u32							id;					// ID модельки
		SlotItemVec					items;              // список кустиков
		SlotItemVec					r_items[3];         // список кустиков for render
	};

	enum SlotType
	{
		stReady						= 0,				// Ready to use
		stPending,										// Pending for decompression

		stFORCEDWORD				= 0xffffffff
	};

	struct Slot // распакованый слот размером DETAIL_SLOT_SIZE
	{
		struct
		{
			u32						empty	:1;
			u32						type	:1;
			u32						frame	:30;
		};

		int							sx,sz;				// координаты слота X x Y
		vis_data					vis;				// 
		SlotPart					G[dm_obj_in_slot];	// 

									Slot()				{ frame = 0; empty = 1; type = stReady; sx = sz = 0; vis.clear_vis_data(); }
	};

	typedef	xr_vector<xr_vector <SlotItemVec* > >	vis_list;

	struct CacheSlot1
	{
		u32							empty;
		vis_data 					vis;

		Slot** 						slots[dm_cache1_count * dm_cache1_count];

		CacheSlot1()				{ empty = 1; vis.clear_vis_data(); }
	};

	typedef	svector<CDetail*, dm_max_objects>	DetailVec;
	typedef	DetailVec::iterator					DetailIt;
	typedef	poolSS<SlotItem, 4096>				PSS;

	// swing values
	struct SSwingValue
	{
		float						rot1;
		float						rot2;
		float						amp1;
		float						amp2;
		float						speed;
		void						lerp	(const SSwingValue& v1, const SSwingValue& v2, float factor);

		void						set		(const CEnvDescriptor::EnvSwingValue& A);

	};

