// LightTrack.cpp: implementation of the CROS_impl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LightTrack.h"
#include "../../include/xrRender/RenderVisual.h"
#include "../../xr_3da/xr_object.h"

#include "../../xr_3da/igame_persistent.h"
#include "../../xr_3da/environment.h"

ENGINE_API BOOL mt_LTracking_;

CROS_impl::CROS_impl()
{
	approximate.set		( 0, 0, 0 );
	frame_updated		= 0;
	frame_smoothed		= 0;
	aiLumNextFrame_		= 0;
	shadow_recv_frame	= u32(-1);
	shadow_recv_slot	= -1;

	result_count		= 0;
	result_iterator		= 0;
	result_frame		= u32(-1);
	result_sun			= 0;

	hemi_value			= 0.5f;
	hemi_smooth			= 0.5f;
	hemi_sky_tests_coef = 0.5f;

	sun_value			= 0.2f;
	sun_smooth			= 0.2f;

	needed_for_ai		= false;

	ai_luminocity		= 0.1f;

	last_position.set( 0.0f, 0.0f, 0.0f );
	ticks_to_update		= 0;
	sky_rays_uptodate	= 0;

	MODE				= IRender_ObjectSpecific::TRACE_ALL;

	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		hemi_cube[i] = 0.1f;
	}

	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		hemi_cube_smooth[i] = 0.1f;
	}
}

CROS_impl::~CROS_impl()
{
	R_ASSERT(!engineState.test(FRAME_PROCESING));

	Device.RemoveFromAuxthread5Pool(fastdelegate::FastDelegate0<>(this, &CROS_impl::perform_ros_update));
}

void CROS_impl::add_to_pool(CLightSource* source)
{
	// Search
	for (xr_vector<Item>::iterator I = tracking_items.begin(); I != tracking_items.end(); I++)
	{
		if (source == I->source)
		{
			I->frame_touched = CurrentFrame();

			return;
		}
	}

	// Register _new_
	tracking_items.push_back(Item());
	Item& L = tracking_items.back();

	L.frame_touched = CurrentFrame();
	L.source			= source;
	L.cache.verts[0].set(0,0,0);
	L.cache.verts[1].set(0,0,0);
	L.cache.verts[2].set(0,0,0);
	L.test				= 0.f;
	L.energy			= 0.f;
}


IC bool	pred_energy(const CROS_impl::Light& L1, const CROS_impl::Light& L2)
{
	return L1.energy > L2.energy;
}

#pragma warning(push)
#pragma warning(disable:4305)

const float		hdir		[lt_hemisamples][3] = 
{
	{-0.26287,	0.52573,	0.80902	},
	{0.27639,	0.44721,	0.85065	},
	{-0.95106,	0.00000,	0.30902	},
	{-0.95106,	0.00000,	-0.30902},
	{0.58779,	0.00000,	-0.80902},
	{0.58779,	0.00000,	0.80902	},

	{-0.00000,	0.00000,	1.00000	},
	{0.52573,	0.85065,	0.00000	},
	{-0.26287,	0.52573,	-0.80902},
	{-0.42533,	0.85065,	0.30902	},
	{0.95106,	0.00000,	0.30902	},
	{0.95106,	0.00000,	-0.30902},
	

	{0.00000,	1.00000,	0.00000	},
	{-0.58779,	0.00000,	0.80902	},
	{-0.72361,	0.44721,	0.52573	},
	{-0.72361,	0.44721,	-0.52573},
	{-0.58779,	0.00000,	-0.80902},
	{0.16246,	0.85065,	-0.50000},

	{0.89443,	0.44721,	0.00000	},
	{-0.85065,	0.52573,	-0.00000},
	{0.16246,	0.85065,	0.50000	},
	{0.68819,	0.52573,	-0.50000},
	{0.27639,	0.44721,	-0.85065},
	{0.00000,	0.00000,	-1.00000},

	{-0.42533,	0.85065,	-0.30902},
	{0.68819,	0.52573,	0.50000	},
};

#pragma warning(pop)

inline void CROS_impl::accum_hemi(float* hemi_cube, Fvector3& dir, float scale)
{
	if (dir.x>0)
		hemi_cube[CUBE_FACE_POS_X] +=  dir.x * scale;
	else
		hemi_cube[CUBE_FACE_NEG_X] -=  dir.x * scale;	//	dir.x <= 0

	if (dir.y>0)
		hemi_cube[CUBE_FACE_POS_Y] +=  dir.y * scale;
	else
		hemi_cube[CUBE_FACE_NEG_Y] -=  dir.y * scale;	//	dir.y <= 0

	if (dir.z>0)
		hemi_cube[CUBE_FACE_POS_Z] +=  dir.z * scale;
	else
		hemi_cube[CUBE_FACE_NEG_Z] -=  dir.z * scale;	//	dir.z <= 0
}


void CROS_impl::update_ros(IRenderable* O)
{
	if (!O || !this || g_loading_events.size())
	{
		// since light update is sent to mt while rendering - it can happen, that
		// its processed in the next frame. And if the level is destoryed - secondary 
		// thread will process a nulled object and crash

		Msg("^ Safely aborting light tracking calculations");

		return;
	}

	owner_ = O;

	if (mt_LTracking_)
	{
		Device.AddToAuxThread_Pool(5, fastdelegate::FastDelegate0<> (this, &CROS_impl::perform_ros_update));
	}
	else
	{
		perform_ros_update();
	}
}

void CROS_impl::perform_ros_update()
{
	if (frame_updated == CurrentFrame())
		return;

	if (!owner_)
		return;

	IRenderable* O = owner_;

	CObject* _object = dynamic_cast<CObject*>(O);

	VERIFY(_object);
	VERIFY(&O->renderable);
	VERIFY(O->renderable.visual);

	frame_updated = CurrentFrame();

#ifdef MEASURE_MT
	CTimer T; T.Start();
#endif

	VERIFY(dynamic_cast<CROS_impl*>(O->renderable_ROS()));

	// select sample, randomize position inside object
	vis_data &vis = O->renderable.visual->getVisData();
	Fvector	position;
	O->renderable.xform.transform_tiny	(position,vis.sphere.P);

	position.y += .3f * vis.sphere.R;

	Fvector	direction;
	direction.random_dir();

	//function call order is important at least for r1
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		hemi_cube[i] = 0;
	}

	bool bFirstTime = (0 == result_count);

	//Main calculating funcs
	calc_sun_value(position, _object);
	calc_sky_hemi_value(position, _object);
	prepare_lights(position, O);


	// Process ambient lighting and approximate average lighting
	// Process our lights to find average luminescences
	CEnvDescriptor&	desc = *g_pGamePersistent->Environment().CurrentEnv;

	Fvector	accum	= { desc.ambient.x,		desc.ambient.y,		desc.ambient.z };
	Fvector	hemi	= { desc.hemi_color.x,	desc.hemi_color.y,	desc.hemi_color.z };
	Fvector sun_	= { desc.sun_color.x,	desc.sun_color.y,	desc.sun_color.z };

	if (needed_for_ai)
	{
		update_ai_luminocity(O);
	}

	if (MODE & IRender_ObjectSpecific::TRACE_HEMI)
		hemi.mul(hemi_smooth);
	else 
		hemi.mul(.2f);

	accum.add(hemi);

	if (MODE & IRender_ObjectSpecific::TRACE_SUN)
		sun_.mul(sun_smooth);
	else
		sun_.mul(.2f);

	accum.add(sun_);

	if (MODE & IRender_ObjectSpecific::TRACE_LIGHTS)
	{
		Fvector lacc = { 0, 0, 0 };

		float hemi_cube_light[NUM_FACES] = { 0, 0, 0, 0, 0, 0 };

		for (u32 lit = 0; lit<tracking_lights.size(); lit++)
		{
			CLightSource* L = tracking_lights[lit].source;
			float d = L->position.distance_to(position);

			float a = (1 / (L->attenuation0 + L->attenuation1 * d + L->attenuation2 * d * d) - d * L->falloff) * (L->flags.bStatic ? 1.f : 2.f);
			a = (a > 0) ? a : 0.0f;

			Fvector3 dir;

			dir.sub(L->position, position);
			dir.normalize_safe();
			
			//multiply intensity on attenuation and accumulate result in hemi cube face
			float lights_color_sum = (tracking_lights[lit].color.r + tracking_lights[lit].color.g + tracking_lights[lit].color.b) / 3.0f;
			float koef = lights_color_sum * a * ps_r2_dhemi_light_scale;
			
			accum_hemi(hemi_cube_light, dir, koef);

			lacc.x += tracking_lights[lit].color.r * a;
			lacc.y += tracking_lights[lit].color.g * a;
			lacc.z += tracking_lights[lit].color.b * a;
		}

		const float	minHemiValue = 1/255.f;

		float	hemi_light = (lacc.x + lacc.y + lacc.z)/3.0f * ps_r2_dhemi_light_scale;

		hemi_value += hemi_light;
		hemi_value = std::max(hemi_value, minHemiValue);

		for (size_t i = 0; i < NUM_FACES; ++i)
		{
			hemi_cube[i] += hemi_cube_light[i] * (1 - ps_r2_dhemi_light_flow) + ps_r2_dhemi_light_flow * hemi_cube_light[(i + NUM_FACES / 2) % NUM_FACES];
			hemi_cube[i] = std::max(hemi_cube[i], minHemiValue);
		}

		accum.add(lacc);
	}
	else
		accum.set(.1f, .1f, .1f);

	//clamp(hemi_value, 0.0f, 1.0f); //Possibly can change hemi value

	if (bFirstTime)
	{
		protectHemiSetGet_.Enter();

		hemi_smooth	= hemi_value;

		protectHemiSetGet_.Leave();

		protectHemiCubeSetGet_.Enter();

		CopyMemory(hemi_cube_smooth, hemi_cube, NUM_FACES*sizeof(float));

		protectHemiCubeSetGet_.Leave();
	}

	update_ros_smooth();

	protectApproximateSetGet_.Enter();
	approximate = accum; // Assigning value to the "Main" variable
	protectApproximateSetGet_.Leave();


#ifdef MEASURE_MT
	Device.Statistic->mtLTrackTime_ += T.GetElapsed_sec();
#endif
}


//	Update ticks settings
static const s32 s_iUTFirstTimeMin = 1;
static const s32 s_iUTFirstTimeMax = 1;
static const s32 s_iUTPosChangedMin = 3;
static const s32 s_iUTPosChangedMax = 6;
static const s32 s_iUTIdleMin = 1000;
static const s32 s_iUTIdleMax = 2000;


void CROS_impl::smart_update_ros(IRenderable* O)
{
	if (!O)
		return;

	if (0 == O->renderable.visual)
		return;

	--ticks_to_update;

	// Acquire current position
	Fvector	position;

	VERIFY(dynamic_cast<CROS_impl*>	(O->renderable_ROS()));

	vis_data &vis = O->renderable.visual->getVisData();
	O->renderable.xform.transform_tiny(position, vis.sphere.P);

	if ( ticks_to_update <= 0)
	{
		update_ros(O);

		last_position = position;

		if (result_count < lt_hemisamples)
			ticks_to_update = ::Random.randI(s_iUTFirstTimeMin, s_iUTFirstTimeMax + 1);
		else if (sky_rays_uptodate < lt_hemisamples)
			ticks_to_update = ::Random.randI(s_iUTPosChangedMin, s_iUTPosChangedMax + 1);
		else
			ticks_to_update = ::Random.randI(s_iUTIdleMin, s_iUTIdleMax + 1);
	}
	else
	{
		if (!last_position.similar(position, 0.15f))
		{
			sky_rays_uptodate = 0;

			update_ros(O);

			last_position = position;

			if (result_count < lt_hemisamples)
				ticks_to_update = ::Random.randI(s_iUTFirstTimeMin, s_iUTFirstTimeMax + 1);
			else
				ticks_to_update = ::Random.randI(s_iUTPosChangedMin, s_iUTPosChangedMax + 1);
		}
	}
}

extern float ps_r2_lt_smooth;

// hemi & sun: update and smooth
void CROS_impl::update_ros_smooth(IRenderable* O)
{
	if (frame_smoothed == CurrentFrame())
		return;

	frame_smoothed = CurrentFrame();

	//smart_update_ros(O);

	float l_f = TimeDelta() * ps_r2_lt_smooth;

	clamp(l_f, 0.f, 1.f);
	float l_i = 1.f - l_f;

	protectHemiSetGet_.Enter();
	hemi_smooth = hemi_value * l_f + hemi_smooth * l_i;
	protectHemiSetGet_.Leave();

	protectSunSetGet_.Enter();
	sun_smooth = sun_value * l_f + sun_smooth * l_i;
	protectSunSetGet_.Leave();

	protectHemiCubeSetGet_.Enter();

	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		hemi_cube_smooth[i] = hemi_cube[i]*l_f + hemi_cube_smooth[i]*l_i;
	}

	for (u8 i = 0; i < 6; i++)
		VERIFY(hemi_cube[i] < 1.f);

	for (u8 i = 0; i < 6; i++)
		VERIFY(hemi_cube_smooth[i] < 1.f);

	protectHemiCubeSetGet_.Leave();
}


void CROS_impl::calc_sun_value(Fvector& position, CObject* _object)
{
	CLightSource* sun = (CLightSource*)RImplementation.Lights.sun_adapted._get();

	if	(MODE & IRender_ObjectSpecific::TRACE_SUN)
	{
		if  (--result_sun < 0)
		{
			result_sun += ::Random.randI(lt_hemisamples / 4, lt_hemisamples / 2);
			Fvector	direction;
			direction.set(sun->direction).invert().normalize();

			sun_value = !(g_pGameLevel->ObjectSpace.RayTest(position, direction, 500.f, collide::rqtBoth, &cache_sun, _object)) ? 1.f : 0.f;
		}
	}
}


void CROS_impl::calc_sky_hemi_value(Fvector& position, CObject* _object)
{
	// hemi-tracing
	if	(MODE & IRender_ObjectSpecific::TRACE_HEMI)	
	{
	
		sky_rays_uptodate += ps_r2_dhemi_count;
		sky_rays_uptodate = _min(sky_rays_uptodate, lt_hemisamples);

		for (u32 it=0; it<(u32)ps_r2_dhemi_count; it++) // five samples per one frame
		{
			u32	sample = 0;

			if (result_count < lt_hemisamples)
			{
				sample = result_count;
				result_count++;
			}
			else
			{
				sample=(result_iterator%lt_hemisamples);
				result_iterator++;
			}

			// take sample
			Fvector	direction;
			direction.set(hdir[sample][0],hdir[sample][1],hdir[sample][2]).normalize();

			//.result[sample] = !g_pGameLevel->ObjectSpace.RayTest(position,direction,50.f,collide::rqtBoth,&cache[sample],_object);

			result_[sample] = !g_pGameLevel->ObjectSpace.RayTest(position,direction,50.f,collide::rqtStatic,&cache[sample],_object);
			//	Msg				("%d:-- %s",sample,result[sample]?"true":"false");
		}
	}

	// hemi & sun: update and smooth

	int _pass = 0;

	for (int it = 0; it < result_count; it++)
	{
		if (result_[it])
			_pass++;
	}

	hemi_value = float(_pass)/float(result_count ? result_count : 1);
	hemi_sky_tests_coef = hemi_value;

	hemi_value *= ps_r2_dhemi_sky_scale;

	for (int it = 0; it < result_count; it++)
	{
		if (result_[it])
		{
			accum_hemi(hemi_cube, Fvector3().set(hdir[it][0], hdir[it][1], hdir[it][2]), ps_r2_dhemi_sky_scale);
		}
	}
}


void CROS_impl::prepare_lights(Fvector& position, IRenderable* O)
{
	CObject* _object = dynamic_cast<CObject*>(O);

	float dt = TimeDelta();

	vis_data &vis = O->renderable.visual->getVisData();
	float radius;
	radius = vis.sphere.R;

	// light-tracing

	BOOL bTraceLights = MODE & IRender_ObjectSpecific::TRACE_LIGHTS;

	if ((!O->renderable_ShadowGenerate()) && (!O->renderable_ShadowReceive()))
		bTraceLights = FALSE;

	if(bTraceLights)
	{
		// Select nearest lights
		Fvector bb_size	= {radius,radius,radius};

		g_SpatialSpace->q_box(RImplementation.lstSpatial, 0, STYPE_LIGHTSOURCEHEMI, position, bb_size);

		for (u32 o_it=0; o_it<RImplementation.lstSpatial.size(); o_it++)
		{
			ISpatial* spatial = RImplementation.lstSpatial[o_it];
			CLightSource* source = (CLightSource*)(spatial->dcast_Light());

			VERIFY (source); // sanity check

			float R	= radius+source->range;

			if (position.distance_to(source->position) < R && source->flags.bStatic)
			{
				add_to_pool(source);
			}
		}

		// Trace visibility
		tracking_lights.clear();

		for (s32 id = 0; id < s32(tracking_items.size()); id++)
		{
			// remove untouched lights
			xr_vector<CROS_impl::Item>::iterator I = tracking_items.begin() + id;

			if (I->frame_touched != CurrentFrame())
			{
				tracking_items.erase(I); 
				id--; 
				continue; 
			}

			// Trace visibility
			Fvector P, D;
			float amount = 0;
			CLightSource* xrL = I->source;
			Fvector& LP = xrL->position;

			P = position;

			// point/spot
			float f = D.sub(P,LP).magnitude();
			if (g_pGameLevel->ObjectSpace.RayTest(LP, D.div(f), f, collide::rqtStatic, &I->cache, _object))
				amount -= lt_dec;
			else
				amount += lt_inc;

			I->test += amount * dt;
			clamp(I->test, -.5f, 1.f);

			I->energy =	.9f * I->energy + .1f * I->test;

			float E	= I->energy * xrL->color.intensity();

			if (E > EPS)
			{
				// Select light
				tracking_lights.push_back(CROS_impl::Light());
				CROS_impl::Light& L = tracking_lights.back();

				L.source = xrL;
				L.color.mul_rgb(xrL->color,I->energy / 2);
				L.energy = I->energy / 2;

				if (!xrL->flags.bStatic)
				{ 
					L.color.mul_rgb(.5f);
					L.energy *= .5f; 
				}
			}
		}

		// Sort lights by importance - important for R1-shadows
		std::sort(tracking_lights.begin(), tracking_lights.end(), pred_energy);

	}
}
