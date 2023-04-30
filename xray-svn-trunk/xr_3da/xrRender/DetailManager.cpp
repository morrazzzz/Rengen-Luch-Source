
#include "stdafx.h"
#pragma hdrstop

#include "DetailManager.h"

#include "../../xr_3da/igame_persistent.h"
#include "../../xr_3da/environment.h"
#include <xmmintrin.h>

extern float r_ssaDISCARD_VP1;

const float dbgOffset			= 0.f;
const int	dbgItems			= 128;

//--------------------------------------------------- Decompression
static int magic4x4[4][4] =
{
 	{ 0, 14,  3, 13},
	{11,  5,  8,  6},
	{12,  2, 15,  1},
	{ 7,  9,  4, 10}
};

void bwdithermap	(int levels, int magic[16][16])
{
	/* Get size of each step */
    float N = 255.0f / (levels - 1);

	/*
	* Expand 4x4 dither pattern to 16x16.  4x4 leaves obvious patterning,
	* and doesn't give us full intensity range (only 17 sublevels).
	*
	* magicfact is (N - 1)/16 so that we get numbers in the matrix from 0 to
	* N - 1: mod N gives numbers in 0 to N - 1, don't ever want all
	* pixels incremented to the next level (this is reserved for the
	* pixel value with mod N == 0 at the next level).
	*/

	float magicfact = (N - 1) / 16;

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				for (int l = 0; l < 4; l++)
					magic[4 * k + i][4 * l + j] = (int)(0.5 + magic4x4[i][j] * magicfact + (magic4x4[k][l] / 16.) * magicfact);
}
//--------------------------------------------------- Decompression

void SSwingValue::lerp(const SSwingValue& A, const SSwingValue& B, float f)
{
	float fi	= 1.f - f;
	amp1		= fi * A.amp1  + f * B.amp1;
	amp2		= fi * A.amp2  + f * B.amp2;
	rot1		= fi * A.rot1  + f * B.rot1;
	rot2		= fi * A.rot2  + f * B.rot2;
	speed		= fi * A.speed + f * B.speed;
}

void SSwingValue::set(const CEnvDescriptor::EnvSwingValue& A)
{
	amp1		= A.amp1;
	amp2		= A.amp2;
	rot1		= A.rot1;
	rot2		= A.rot2;
	speed		= A.speed;
}

CDetailManager::CDetailManager()
{
	dtFS 		= 0;
	dtSlots		= 0;

	hw_Geom		= 0;
	hw_BatchSize= 0;
	hw_VB		= 0;
	hw_IB		= 0;
	m_time_rot_1 = 0;
	m_time_rot_2 = 0;
	m_time_pos	= 0;
	m_global_time_old = 0;

	alreadySentToAuxThread_ = false;
	m_frame_calc = 0;
	frameStartedMTCalc_ = 0;

	detailsToRenderCount_				= 0;
	detailsWithSunShadowsCount_			= 0;
	potentialDetailsWithShadowsCount_	= 0;

	// thanks to K.D.
	dm_size			= dm_current_size;
	dm_cache_line	= dm_current_cache_line;
	dm_cache1_line	= dm_current_cache1_line;
	dm_cache_size	= dm_current_cache_size;
	dm_fade			= dm_current_fade;

	cache_level1	= (CacheSlot1**)Memory.mem_alloc(dm_cache1_line*sizeof(CacheSlot1*)

	#ifdef DEBUG
		,"CDetailManager::cache_level1"
	#endif

		);
	for (u32 i = 0; i < dm_cache1_line; ++i)
	{
		cache_level1[i]	= (CacheSlot1*)Memory.mem_alloc(dm_cache1_line*sizeof(CacheSlot1)

	#ifdef DEBUG
		,"CDetailManager::cache_level1 "+i
	#endif

		);
		for (u32 j = 0; j < dm_cache1_line; ++j)
			new (&(cache_level1[i][j])) CacheSlot1();
	}

	cache	= (Slot***)Memory.mem_alloc(dm_cache_line*sizeof(Slot**)

	#ifdef DEBUG
		,"CDetailManager::cache"
	#endif

	);
	for (u32 i = 0; i < dm_cache_line; ++i)
		cache[i] = (Slot**)Memory.mem_alloc(dm_cache_line*sizeof(Slot*)

	#ifdef DEBUG
		,"CDetailManager::cache "+i
	#endif		

		);

	cache_pool	= (Slot *)Memory.mem_alloc(dm_cache_size*sizeof(Slot)

	#ifdef DEBUG
		,"CDetailManager::cache_pool"
	#endif

		);

	for (u32 i = 0; i < dm_cache_size; ++i)
		new (&(cache_pool[i])) Slot();

	ssaDISCARD_			= r_ssaDISCARD_VP1;
}

CDetailManager::~CDetailManager()
{
	for (u32 i = 0; i < dm_cache_size; ++i)
		cache_pool[i].~Slot();

	Memory.mem_free(cache_pool);

	for (u32 i = 0; i < dm_cache_line; ++i)
		Memory.mem_free(cache[i]);

	Memory.mem_free(cache);
	
	for (u32 i = 0; i < dm_cache1_line; ++i)
	{
		for (u32 j = 0; j < dm_cache1_line; ++j)
			cache_level1[i][j].~CacheSlot1();

		Memory.mem_free(cache_level1[i]);
	}

	Memory.mem_free(cache_level1);
}

void CDetailManager::Load()
{
	// Open file stream
	if (!FS.exist("$level$", "level.details"))
	{
		dtFS = NULL;

		return;
	}

	string_path			fn;
	FS.update_path		(fn, "$level$", "level.details");

	dtFS				= FS.r_open(fn);

	// Header
	dtFS->r_chunk_safe	(0, &dtH, sizeof(dtH));

	R_ASSERT			(dtH.version == DETAIL_VERSION);

	u32 m_count			= dtH.object_count;

	// Models
	IReader* m_fs		= dtFS->open_chunk(1);

	for (u32 m_id = 0; m_id < m_count; m_id++)
	{
		CDetail*		dt	= xr_new <CDetail>();
		IReader* S			= m_fs->open_chunk(m_id);
		dt->Load			(S);
		objects.push_back	(dt);
		S->close			();
	}

	m_fs->close();

	// Get pointer to database (slots)
	IReader* m_slots	= dtFS->open_chunk(2);
	dtSlots				= (DetailSlot*)m_slots->pointer();

	m_slots->close();

	// Initialize 'vis' and 'cache'
	for (u32 i = 0; i < 3; ++i)
	{ 
		m_visibles[i].resize(objects.size());
		m_visibles2[i].resize(objects.size());
		m_visibles_sh_map[i].resize(objects.size());
		m_visibles_sun_shadows[i].resize(objects.size());
		m_visibles_sun_shadows2[i].resize(objects.size());
	}

	cache_Initialize();

	// Make dither matrix
	bwdithermap		(2, dither);

	// Hardware specific optimizations
	hw_Load();

	// swing desc
	// normal
	swing_desc[0].amp1	= pSettings->r_float("details", "swing_normal_amp1");
	swing_desc[0].amp2	= pSettings->r_float("details", "swing_normal_amp2");
	swing_desc[0].rot1	= pSettings->r_float("details", "swing_normal_rot1");
	swing_desc[0].rot2	= pSettings->r_float("details", "swing_normal_rot2");
	swing_desc[0].speed	= pSettings->r_float("details", "swing_normal_speed");

	// fast
	swing_desc[1].amp1	= pSettings->r_float("details", "swing_fast_amp1");
	swing_desc[1].amp2	= pSettings->r_float("details", "swing_fast_amp2");
	swing_desc[1].rot1	= pSettings->r_float("details", "swing_fast_rot1");
	swing_desc[1].rot2	= pSettings->r_float("details", "swing_fast_rot2");
	swing_desc[1].speed	= pSettings->r_float("details", "swing_fast_speed");
}

void CDetailManager::Unload()
{
	// Need to syncronize with aux thread, to avoid further aux thread crashes
	Device.IndAuxProcessingProtection_1.Enter();
	Device.IndAuxPoolProtection_1.Enter();

	xr_vector<fastdelegate::FastDelegate0<> >::iterator I = std::find(
		Device.independableAuxThreadPool_1_.begin(),
		Device.independableAuxThreadPool_1_.end(),
		fastdelegate::FastDelegate0<>(this, &CDetailManager::MT_CALC)
		);

	if (I != Device.independableAuxThreadPool_1_.end())
		Device.independableAuxThreadPool_1_.erase(I);

	Device.IndAuxPoolProtection_1.Leave();
	Device.IndAuxProcessingProtection_1.Leave();

	hw_Unload();

	for (DetailIt it=objects.begin(); it!=objects.end(); it++)
	{
		(*it)->Unload();

		xr_delete(*it);
    }

	objects.clear();

	m_visibles[0].clear	();
	m_visibles[1].clear	();
	m_visibles[2].clear	();
	m_visibles2[0].clear();
	m_visibles2[1].clear();
	m_visibles2[2].clear();
	m_visibles_sh_map[0].clear();
	m_visibles_sh_map[1].clear();
	m_visibles_sh_map[2].clear();
	m_visibles_sun_shadows[0].clear();
	m_visibles_sun_shadows[1].clear();
	m_visibles_sun_shadows[2].clear();
	m_visibles_sun_shadows2[0].clear();
	m_visibles_sun_shadows2[1].clear();
	m_visibles_sun_shadows2[2].clear();

	FS.r_close(dtFS);
}

void CDetailManager::UpdateVisibleM()
{
	RDEVICE.Statistic->RenderDUMP_DT_VIS.Begin();

	// Order is necessary
	UpdateVisibles();

	UpdateVisiblesSVP();

	if (ps_r2_ls_flags.test(R2FLAG_DETAILS_SUN_SHADOWS) || ps_light_details_shadow)
		UpdateVisibleShadowsRaw();

	if (ps_r2_ls_flags.test(R2FLAG_DETAILS_SUN_SHADOWS))
	{
		UpdateVisibleShadowsSun();

		UpdateVisibleShadowsSunSVP(); // uses main vp sun grass, but cuts of invisible
	}

	if (ps_light_details_shadow)
		UpdateVisibleShadowsLights();

	//Msg("all %u, pot %u, sun %u, svp %u, spvsun %u", detailsToRenderCount_, potentialDetailsWithShadowsCount_, detailsWithSunShadowsCount_, detailsToRenderCountSVP1_, detailsWithSunShadowsCountSPV1_);

	RDEVICE.Statistic->RenderDUMP_DT_VIS.End();
}

void CDetailManager::UpdateVisibles()
{
	Fvector EYE = Device.vpSavedView1.GetCameraPosition_saved_MT();

	CFrustum view_frust;
	Fmatrix fulltransform = Device.vpSavedView1.GetFullTransform_saved_MT();
	view_frust.CreateFromMatrix(fulltransform, FRUSTUM_P_LRTB + FRUSTUM_P_FAR);

	float fade_limit = dm_fade;	fade_limit = fade_limit*fade_limit;
	float fade_start = 1.f;		fade_start = fade_start*fade_start;

	float fade_range = fade_limit - fade_start;

	//clear 'vis'
	for (u8 i = 0; i != 3; i++)
	{
		vis_list& list = m_visibles[i];
		for (u32 j = 0; j != list.size(); j++)
			list[j].clear_not_free();

		vis_list& list4 = m_visibles2[i];
		for (u32 j = 0; j != list4.size(); j++)
			list4[j].clear_not_free();

		vis_list& list2 = m_visibles_sh_map[i];
		for (u32 k = 0; k != list2.size(); k++)
			list2[k].clear_not_free();

		vis_list& list3 = m_visibles_sun_shadows[i];
		for (u32 l = 0; l != list3.size(); l++)
			list3[l].clear_not_free();

		vis_list& list5 = m_visibles_sun_shadows2[i];
		for (u32 l = 0; l != list5.size(); l++)
			list5[l].clear_not_free();
	}

	// Initialize 'vis' and 'cache'
	// Collect objects for rendering

	u32 frame = CurrentFrameMT();

	for (u32 _mz = 0; _mz < dm_cache1_line; _mz++)
	{
		for (u32 _mx = 0; _mx < dm_cache1_line; _mx++)
		{
			CacheSlot1& MS = cache_level1[_mz][_mx];

			if (MS.empty)
			{
				continue;
			}

			u32 mask = 0xff;
			u32 res = view_frust.testSAABB(MS.vis.sphere.P, MS.vis.sphere.R, MS.vis.box.data(), mask);

			if (fcvNone == res)
			{
				continue;	// invisible-view frustum
			}

			// test slots

			u32 dwCC = dm_cache1_count * dm_cache1_count;

			for (u32 _i = 0; _i < dwCC; _i++)
			{
				Slot* PS = *MS.slots[_i];
				Slot& S = *PS;

				// if slot empty - continue
				if (S.empty)
					continue;

				// if upper test = fcvPartial - test inner slots
				if (fcvPartial == res)
				{
					u32 _mask = mask;
					u32 _res = view_frust.testSAABB(S.vis.sphere.P, S.vis.sphere.R, S.vis.box.data(), _mask);

					if (fcvNone == _res)
						continue;	// invisible-view frustum
				}

				if (!RImplementation.HOM.hom_visible(S.vis))
					continue;	// invisible-occlusion

				// Add to visibility structures
				if (frame > S.frame)
				{
					// Calc fade factor	(per slot)
					float dist_sq = EYE.distance_to_sqr(S.vis.sphere.P);

					if (dist_sq > fade_limit) // if don't even fit into highest quality range - dont try for lower quality too
						continue;

					float alpha = (dist_sq < fade_start) ? 0.f : (dist_sq - fade_start) / fade_range;
					float alpha_i = 1.f - alpha;
					float dist_sq_rcp = 1.f / dist_sq;

					S.frame = frame + Random.randI(15, 30);

					for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
					{
						SlotPart& sp = S.G[sp_id];

						if (sp.id == DetailSlot::ID_Empty)
							continue;

						sp.r_items[0].clear_not_free();
						sp.r_items[1].clear_not_free();
						sp.r_items[2].clear_not_free();

						float R = objects[sp.id]->bv_sphere.R;
						float Rq_drcp = R * R * dist_sq_rcp; // reordered expression for 'ssa' calc

						SlotItem **siIT = &(*sp.items.begin()), **siEND = &(*sp.items.end());

						for (; siIT != siEND; siIT++)
						{
							if (*siIT == NULL)
								continue;

							SlotItem& Item = *(*siIT);

							float scale = Item.scale_calculated = Item.scale * alpha_i;
							float ssa = scale*scale*Rq_drcp;

							if (ssa < ssaDISCARD_)
								continue;

							u32 vis_id = 0;

							if (ssa > 16 * ssaDISCARD_)
								vis_id = Item.vis_ID;

							sp.r_items[vis_id].push_back(*siIT);

							Item.parent_slot = PS;
						}
					}
				}

				for (int sp_id = 0; sp_id < dm_obj_in_slot; sp_id++)
				{
					SlotPart& sp = S.G[sp_id];

					if (sp.id == DetailSlot::ID_Empty)
						continue;

					if (!sp.r_items[0].empty())
					{
						m_visibles[0][sp.id].push_back(&sp.r_items[0]);
					}
					if (!sp.r_items[1].empty())
					{
						m_visibles[1][sp.id].push_back(&sp.r_items[1]);
					}
					if (!sp.r_items[2].empty())
					{
						m_visibles[2][sp.id].push_back(&sp.r_items[2]);
					}
				}
			}
		}
	}
}

void CDetailManager::UpdateVisibleShadowsRaw()
{
	Fvector EYE = Device.vpSavedView1.GetCameraPosition_saved_MT();

	detailsToRenderCount_ = 0;
	potentialDetailsWithShadowsCount_ = 0;

	// Determine the raw pool of grass that is potentialy going to cast shadows(to not render shadows for all rendered grass)
	for (u8 j = 0; j < 3; j++) // itterate waves
	{
		for (u8 k = 0; k < m_visibles[j].size(); k++) // itterate vectors of object types of each wave
		{
			for (u16 l = 0; l < m_visibles[j][k].size(); l++) // itterate vector of vectors of slots parts
			{
				auto slot_items_vector = m_visibles[j][k][l];

				//Check the first slot item parrent distance to camera

				SlotItem& slot_item = *slot_items_vector->at(0);

				Slot* parrent_slot = (Slot*)slot_item.parent_slot;

				detailsToRenderCount_ += m_visibles[j][k][l]->size();

				if (parrent_slot)
				{
					float distnce_to_viewport = EYE.distance_to(parrent_slot->vis.sphere.P);

					if (distnce_to_viewport < ps_details_with_shadows_dist)
					{
						m_visibles_sh_map[j][k].push_back(m_visibles[j][k][l]);

						potentialDetailsWithShadowsCount_ += m_visibles[j][k][l]->size();
					}
				}
			}
		}
	}
}

void CDetailManager::UpdateVisiblesSVP()
{
	CFrustum second_vp_frust;
	Fmatrix full = Device.vpSavedView2.GetShrinkedFullTransform_saved_MT();

	second_vp_frust.CreateFromMatrix(full, FRUSTUM_P_ALL &(~FRUSTUM_P_NEAR));
	
	detailsToRenderCountSVP1_ = 0;

	for (u8 j = 0; j < 3; j++) // itterate waves
	{
		for (u8 k = 0; k < m_visibles[j].size(); k++) // itterate vectors of object types of each wave
		{
			for (u16 l = 0; l < m_visibles[j][k].size(); l++) // itterate vector of vectors of slots parts
			{
				auto slot_items_vector = m_visibles[j][k][l];

				SlotItem& slot_item = *slot_items_vector->at(0);

				Slot* parrent_slot = (Slot*)slot_item.parent_slot;

				if (parrent_slot)
				{
					Fvector who_to_check_p = parrent_slot->vis.sphere.P;
					float who_to_check_r = parrent_slot->vis.sphere.R;

					u32	tmask = second_vp_frust.getMask();

					if (second_vp_frust.testSphere(who_to_check_p, who_to_check_r, tmask) == fcvNone)
						continue;

					detailsToRenderCountSVP1_ += m_visibles[j][k][l]->size();

					m_visibles2[j][k].push_back(m_visibles[j][k][l]);
				}
			}
		}
	}
}

void CDetailManager::UpdateVisibleShadowsSunSVP()
{
	CFrustum second_vp_frust;
	Fmatrix full = Device.vpSavedView2.GetShrinkedFullTransform_saved_MT();

	second_vp_frust.CreateFromMatrix(full, FRUSTUM_P_ALL &(~FRUSTUM_P_NEAR));

	detailsWithSunShadowsCountSPV1_ = 0;

	// Using the main VP sun grass pool. So basicly if spv will look different direction - we will not get much grass shadows there

	for (u8 j = 0; j < 3; j++) // itterate waves
	{
		for (u8 k = 0; k < m_visibles_sun_shadows[j].size(); k++) // itterate vectors of object types of each wave
		{
			for (u16 l = 0; l < m_visibles_sun_shadows[j][k].size(); l++) // itterate vector of vectors of slots parts
			{
				auto slot_items_vector = m_visibles_sun_shadows[j][k][l];

				SlotItem& slot_item = *slot_items_vector->at(0);

				Slot* parrent_slot = (Slot*)slot_item.parent_slot;

				if (parrent_slot)
				{
					Fvector who_to_check_p = parrent_slot->vis.sphere.P;
					float who_to_check_r = parrent_slot->vis.sphere.R;

					u32	tmask = second_vp_frust.getMask();

					if (second_vp_frust.testSphere(who_to_check_p, who_to_check_r, tmask) == fcvNone)
						continue;

					detailsWithSunShadowsCountSPV1_ += m_visibles_sun_shadows[j][k][l]->size();

					m_visibles_sun_shadows2[j][k].push_back(m_visibles_sun_shadows[j][k][l]);
				}
			}
		}
	}
}

void CDetailManager::UpdateVisibleShadowsSun()
{
	Fvector EYE = Device.vpSavedView1.GetCameraPosition_saved_MT();

	detailsWithSunShadowsCount_ = 0;

	// Determine the grass for sun shadows. Itterate raw pool of potential grass for shadows and find those, that are wihin needed radious
	for (u8 j = 0; j < 3; j++) // itterate waves
	{
		for (u8 k = 0; k < m_visibles_sh_map[j].size(); k++) // itterate vectors of object types of each wave
		{
			for (u16 l = 0; l < m_visibles_sh_map[j][k].size(); l++) // itterate vector of vectors of slots parts
			{
				auto slot_items_vector = m_visibles_sh_map[j][k][l];

				//Check the first slot item parrent distance to camera

				SlotItem& slot_item = *slot_items_vector->at(0);

				Slot* parrent_slot = (Slot*)slot_item.parent_slot;

				if (parrent_slot)
				{
					float distnce_to_viewport = EYE.distance_to(parrent_slot->vis.sphere.P);

					if (distnce_to_viewport < ps_r__Details_sun_sh_dist && distnce_to_viewport < ps_r2_sun_shadows_near_casc) // don't exeed first ss cascade dist
					{
						m_visibles_sun_shadows[j][k].push_back(m_visibles_sh_map[j][k][l]);

						detailsWithSunShadowsCount_ += m_visibles_sh_map[j][k][l]->size();
					}
				}
			}
		}
	}
}

void CDetailManager::UpdateVisibleShadowsLights() // This funtion must run after Lights deletion stage is done
{
	Fvector EYE = Device.vpSavedView1.GetCameraPosition_saved_MT();

	R_ASSERT(RImplementation.CanCalcMTDetails());

#ifndef DEBUG
	for (u32 y = 0; y < lightsToCheck_.size(); y++)
	{
		R_ASSERT(lightsToCheck_[y]);

		if (lightsToCheck_[y])
			R_ASSERT(lightsToCheck_[y]->lightDsGraphBuffer_);
	}
#endif

	CLightSource* last_omni_owning_light = nullptr;

	// Process
	u16 light_with_ds = 0;

	for (u16 j = 0; j < lightsToCheck_.size(); j++)
	{
		CLightSource* light = lightsToCheck_[j];

		if (light)
		{
			float distnce_to_viewport = EYE.distance_to(light->position);

			if (distnce_to_viewport < ps_light_ds_max_dist_from_cam)
			{
				// Prepare

				for (u8 k = 0; k < 3; k++) // itterate waves
				{
					vis_list& list = light->m_shadowed_details[k];

					if (list.size() <= 0) // if not reserved yet
						list.resize(objects.size());

					// Clear
					for (u32 l = 0; l != list.size(); l++)
						list[l].clear_not_free();
				}

				light->detailWithShadowsCount_ = 0;
				bool usefull = false;

				bool finish_omni_light_anyway = light->omniParent_ && light->omniParent_ == last_omni_owning_light; // need to finish omni parts anyway

				if (light_with_ds < ps_light_ds_max_lights || finish_omni_light_anyway)
					usefull = UpdateUsefullLight(light); // determine grass for rendering shadows

				light->usefullForDetailsShadows_ = usefull;

				if (usefull)
				{
					//Msg("%u P %f %f %f %f", j, distnce_to_viewport, VPUSH(light->position));
					light_with_ds++;

					if (light->omniParent_)
						last_omni_owning_light = light->omniParent_;
				}
			}
			else
			{
				for (u8 k = 0; k < 3; k++) // free memory, if light doesn't render grass shadows
					light->m_shadowed_details[k].clear();

				light->usefullForDetailsShadows_ = false; // disable light from trying to render gs
			}
		}
	}
}

bool CDetailManager::UpdateUsefullLight(CLightSource* light)
{
	bool any_detail_found = false;

	CFrustum light_frustum;
	Fmatrix lm = light->X.S.combine_mt_copy; // mt unsafe
	light_frustum.CreateFromMatrix(lm, FRUSTUM_P_ALL &(~FRUSTUM_P_NEAR));

	// Determine the grass for this light shadows. Itterate raw pool of potential grass for shadows and find those, that are wihin needed radious
	for (u8 j = 0; j < 3; j++) // itterate waves
	{
		for (u8 k = 0; k < m_visibles_sh_map[j].size(); k++) // itterate vectors of object types of each wave
		{
			for (u16 l = 0; l < m_visibles_sh_map[j][k].size(); l++) // itterate vector of vectors of slots parts
			{
				auto slot_items_vector = m_visibles_sh_map[j][k][l];

				//Check the first slot item parrent distance to camera

				SlotItem& slot_item = *slot_items_vector->at(0);

				Slot* parrent_slot = (Slot*)slot_item.parent_slot;

				if (parrent_slot)
				{
					float distnce_to_light = light->position.distance_to(parrent_slot->vis.sphere.P);

					if (distnce_to_light < light->range && distnce_to_light < ps_light_ds_max_dist_from_light) // check distance first
					{
						// now check if grass sphere is inside light frustum
						if (true)
						{
							Fvector who_to_check_p = parrent_slot->vis.sphere.P;
							float who_to_check_r = parrent_slot->vis.sphere.R;

							u32	tmask = light_frustum.getMask();

							if (light_frustum.testSphere(who_to_check_p, who_to_check_r, tmask) == fcvNone)
								continue;
						}

						// add

						light->m_shadowed_details[j][k].push_back(m_visibles_sh_map[j][k][l]);

						any_detail_found = true;
						light->detailWithShadowsCount_ += (u16)m_visibles_sh_map[j][k][l]->size();
					}
				}
			}
		}
	}

	return any_detail_found;
}


void CDetailManager::Render(vis_list(&details_to_render)[3])
{
	if (!dtFS)
		return;

	if (!psDeviceFlags.is(rsDetails))
		return;

	RDEVICE.Statistic->RenderDUMP_DT_Render.Begin();

	float factor			= g_pGamePersistent->Environment().wind_strength_factor;

	swing_desc[0].set		(g_pGamePersistent->Environment().Current[0]->m_cSwingDesc[0]);
	swing_desc[1].set		(g_pGamePersistent->Environment().Current[0]->m_cSwingDesc[1]);

	swing_current.lerp		(swing_desc[0],swing_desc[1],factor);

	RCache.set_CullMode		(CULL_CCW);
	RCache.set_xform_world	(Fidentity);

	hw_Render(details_to_render);

	RDEVICE.Statistic->RenderDUMP_DT_Render.End	();
}

void CDetailManager::CheckDetails()
{
	if (!dtFS)
		return;

	if (!psDeviceFlags.is(rsDetails))
		return;

	// MT
	CTimer dull_waitng_timer; dull_waitng_timer.Start();

	MT.Enter(); // wait for aux thread, if it is already started to calculate and is still calculating

	bool secondary_thread_calculated = MT_SYNC(); // check if aux calculated visabilty

	MT.Leave();

	if (!secondary_thread_calculated) // if aux thread did not even start to calculate up to this point - do it now in main thread
	{
		for (u32 i = 0; i < lightsToCheck_.size(); i++)
			lightsToCheck_[i]->X.S.combine_mt_copy = lightsToCheck_[i]->X.S.combine;

		MT_CALC();
	}

	float dull_time = dull_waitng_timer.GetElapsed_sec() * 1000.f;

	RImplementation.vpStats->RenderMtWait += dull_time;

#ifdef MEASURE_MT
	Device.Statistic->mtRenderDelayDetails_ += dull_time;
#endif
	
	//if (ps_r_mt_flags.test(R_FLAG_MT_CALC_HOM_DETAILS) && dull_time > 0.5f)
	//	R_ASSERT2(false, make_string("Not enought time for grass visability calculation %f", dull_time));
}

void __stdcall CDetailManager::MT_CALC()
{
	if (!RImplementation.Details || !dtFS || !psDeviceFlags.is(rsDetails))
	{ 
		alreadySentToAuxThread_ = false; 

		return; 
	};

#ifdef MEASURE_MT
	CTimer measure_mt; measure_mt.Start();
#endif

	MT.Enter();

	u32 ready_frame = CurrentFrameMT();

	if (ps_r_mt_flags.test(R_FLAG_MT_CALC_HOM_DETAILS))
		ready_frame = frameStartedMTCalc_ + 1;

	if (m_frame_calc < ready_frame && !g_pGamePersistent->SceneRenderingBlocked())
	{
		R_ASSERT(RImplementation.CanCalcMTDetails());

		Fvector EYE = Device.vpSavedView1.GetCameraPosition_saved_MT();
		
		int s_x = iFloor(EYE.x / dm_slot_size + .5f);
		int s_z = iFloor(EYE.z / dm_slot_size + .5f);

		RDEVICE.Statistic->RenderDUMP_DT_Cache.Begin();

		cache_Update(s_x, s_z, EYE, dm_max_decompress);

		RDEVICE.Statistic->RenderDUMP_DT_Cache.End();

		UpdateVisibleM();

		m_frame_calc = ready_frame;
	}

	MT.Leave();

	alreadySentToAuxThread_ = false;

#if defined (MEASURE_MT)
	Device.Statistic->mtDetails_ += measure_mt.GetElapsed_sec();
#endif
}
