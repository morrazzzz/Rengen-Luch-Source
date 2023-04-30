// LightTrack.h: interface for the CLightTrack class.

#pragma once

const	float lt_inc		= 4.f;
const	float lt_dec		= 2.f;
const	int lt_hemisamples	= 26;

class CROS_impl : public IRender_ObjectSpecific
{
public:
	enum CubeFaces
	{
		CUBE_FACE_POS_X,
		CUBE_FACE_POS_Y,
		CUBE_FACE_POS_Z,
		CUBE_FACE_NEG_X,
		CUBE_FACE_NEG_Y,
		CUBE_FACE_NEG_Z,
		NUM_FACES
	};

	struct Item
	{
		u32					frame_touched;	// to track creation & removal
		CLightSource*		source;
		collide::ray_cache	cache;
		float				test;			// note range: (-1[no]..1[yes])
		float				energy;
	};
	struct Light
	{
		CLightSource*		source;
		float				energy;
		Fcolor				color;
	};

	// R1 Secific:
	u32						shadow_gen_frame;
	u32						shadow_recv_frame;
	int						shadow_recv_slot;

private:

	// general
	u32						MODE;
	u32						frame_updated;
	u32						frame_smoothed;

	//for ai
	u32						aiLumNextFrame_;

	xr_vector<ISpatial*>	Spatials; // For ai visability
	xr_vector<Item>			Ai_Unsorted_Items; // For ai visability
	xr_vector<Light>		Ai_Tracking_Lights; // For ai visability

	// 
	xr_vector<Item>			tracking_items;	// everything what touches
	xr_vector<Light>		tracking_lights;

	bool					result_[lt_hemisamples];
	collide::ray_cache		cache[lt_hemisamples];
	collide::ray_cache		cache_sun;
	s32						result_count;
	u32						result_iterator;
	u32						result_frame;
	s32						result_sun;

	float					ai_luminocity;
	float					hemi_cube[NUM_FACES];
	float					hemi_cube_smooth[NUM_FACES];

	float					hemi_value;
	float					hemi_smooth;
	float					hemi_sky_tests_coef; // value of Ppassed devided by Total raycast tests towards sky
	float					sun_value;
	float					sun_smooth;

	Fvector					approximate;

	Fvector					last_position;
	s32						ticks_to_update;
	s32						sky_rays_uptodate;

	void					add_to_pool(CLightSource* L);

	AccessLock				protectHemiSetGet_;
	AccessLock				protectSunSetGet_;
	AccessLock				protectApproximateSetGet_;
	AccessLock				protectAILumSetGet_;
public:
	virtual	void			force_mode			(u32 mode)	{ MODE = mode; };
	virtual IC float		get_luminocity()				{ protectApproximateSetGet_.Enter(); float result = _max(approximate.x, _max(approximate.y, approximate.z)); clamp(result, 0.f, 1.f); protectApproximateSetGet_.Leave(); return (result); };
	virtual IC float		get_luminocity_hemi()			{ return get_hemi(); };
	virtual IC float*		get_luminocity_hemi_cube()		{ return hemi_cube_smooth; };
	//for ai calculations
	virtual IC float		get_ai_luminocity()				{ protectAILumSetGet_.Enter(); float result = ai_luminocity; protectAILumSetGet_.Leave(); return result; };


	void					update_ros				(IRenderable* O);
	void					update_ros_smooth		(IRenderable* O=0);

	void _stdcall			perform_ros_update		();

	// update luminacity of an object for ai calculations
	void					update_ai_luminocity(IRenderable* O);

	
	ICF	float get_hemi()
	{
		if (frame_smoothed != CurrentFrame())
			update_ros_smooth();

		protectHemiSetGet_.Enter();

		float res = hemi_smooth;

		protectHemiSetGet_.Leave();

		return res;
	}

	ICF	float get_sun()
	{
		if (frame_smoothed != CurrentFrame())
			update_ros_smooth();

		protectSunSetGet_.Enter();

		float res = sun_smooth;

		protectSunSetGet_.Leave();

		return res;
	}

	ICF Fvector3 get_approximate()
	{
		if (frame_smoothed != CurrentFrame())
			update_ros_smooth();

		protectApproximateSetGet_.Enter();

		Fvector3 res = approximate;

		protectApproximateSetGet_.Leave();

		return res;
	}

	const float* get_hemi_cube()
	{
		if (frame_smoothed != CurrentFrame())
			update_ros_smooth();

		return hemi_cube_smooth;
	}

	CROS_impl();

	virtual ~CROS_impl();

	IC xr_vector<Light>& GetTrackingLightsPool() { return tracking_lights; };

private:
	//static inline CubeFaces get_cube_face(Fvector3& dir);
	
	//Accumulates light from direction for corresponding faces
	static inline void accum_hemi(float* hemi_cube, Fvector3& dir, float scale);

	//Calculates sun part of ambient occlusion
	void calc_sun_value(Fvector& position, CObject* _object);

	//Calculates sky part of ambient occlusion
	void calc_sky_hemi_value(Fvector& position, CObject* _object);

	//prepares static or hemisphere lights for ambient occlusion calculations
	void prepare_lights(Fvector& position, IRenderable* O);

	//	Updates only if makes a desizion that update is necessary
	void smart_update_ros(IRenderable* O);

};
