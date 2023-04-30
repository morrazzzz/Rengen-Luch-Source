// Rain.h: interface for the CRain class.
//
//////////////////////////////////////////////////////////////////////

#ifndef RainH
#define RainH
#pragma once


#include "../xrcdb/xr_collide_defs.h"

//refs
class ENGINE_API IRender_DetailModel;

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/RainRender.h"
// gr1ph start
#define RAIN_MANAGER_LTX "rain_manager"

struct ENGINE_API SRainParams
{
	public:
		int		max_desired_items;
		float	source_radius, source_offset;
		float	max_distance, sink_offset, drop_length;
		float	drop_width, drop_angle, drop_max_angle;
		float	drop_max_wind_vel, drop_speed_min, drop_speed_max;
		int		max_particles, particles_cache;
		float	particles_time;
		u32		dwReferences;
	public:
				SRainParams();		
};

//
class ENGINE_API CEffect_Rain
{
	friend class dxRainRender;
private:
	struct	Item
	{
		Fvector			P;
		Fvector			Phit;
		Fvector			D;
		float			fSpeed;
		u32				dwTime_Life;
		u32				dwTime_Hit;
		u32				uv_set;
		void			invalidate	()
		{
			dwTime_Life	= 0;
		}
	};
	struct	Particle
	{
		Particle		*next,*prev;
		Fmatrix			mXForm;
		Fsphere			bounds;
		float			time;
	};
	enum	States
	{
		stIdle		= 0,
		stWorking
	};
private:
	// Visualization	(rain) and (drops)
	FactoryPtr<IRainRender>	m_pRender;
	
	// Data and logic
	xr_vector<Item>					items;
	States							state;

	// Particles
	xr_vector<Particle>				particle_pool;
	Particle*						particle_active;
	Particle*						particle_idle;

	// Sounds
	ref_sound						snd_Ambient;
	ref_sound						snd_Wind;
	bool							bWindWorking;

	// Utilities
	void							p_create		();
	void							p_destroy		();

	void							p_remove		(Particle* P, Particle* &LST);
	void							p_insert		(Particle* P, Particle* &LST);
	int								p_size			(Particle* LST);
	Particle*						p_allocate		();
	void							p_free			(Particle* P);

	// Some methods
	void							Born			(Item& dest, float radius);
	void							Hit				(Fvector& pos);
	BOOL							RayPick			(const Fvector& s, const Fvector& d, float& range, collide::rq_target tgt);
	void							RenewItem		(Item& dest, float height, BOOL bHit);
public:
									CEffect_Rain	();
									~CEffect_Rain	();

	void							Render			();
	void							OnFrame			();

	void							InvalidateState	()		{ state = stIdle; };
};

#endif //RainH
