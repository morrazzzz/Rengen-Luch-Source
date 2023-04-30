#include "stdafx.h"
#pragma hdrstop

#ifndef _EDITOR
    #include "render.h"
#endif

#include "Environment.h"
#include "xr_efflensflare.h"
#include "rain.h"
#include "thunderbolt.h"
#include "xrHemisphere.h"
#include "perlin.h"

#include "xr_input.h"

#ifndef _EDITOR
	#include "IGame_Level.h"
#endif

#include "../xrcore/xrCore.h"

#include "../Include/xrRender/EnvironmentRender.h"
#include "../Include/xrRender/LensFlareRender.h"
#include "../Include/xrRender/RainRender.h"
#include "../Include/xrRender/ThunderboltRender.h"

#include "igame_persistent.h"

ENGINE_API	float			psVisDistance	= 1.f;
ENGINE_API	float			psFogDistance	= 1.f;
static const float			MAX_NOISE_FREQ	= 0.03f;

#if 0
	#define WEATHER_LOGGING
#endif


// real WEATHER->WFX transition time
#define WFX_TRANS_TIME		5.f

// Soft Transition Base time
#define STR_BASE_TIME		2.0f

CEnvironment::CEnvironment	() :
	CurrentEnv				(0)
{
	bNeed_re_create_env = FALSE;
	bWFX					= false;
	Current[0]				= 0;
	Current[1]				= 0;
    CurrentWeather			= 0;
    CurrentWeatherName		= 0;
	eff_Rain				= 0;
    eff_LensFlare 			= 0;
    eff_Thunderbolt			= 0;
	OnDeviceCreate			();

#ifdef _EDITOR
	ed_from_time			= 0.f;
	ed_to_time				= DAY_LENGTH;
#endif

#ifndef _EDITOR
	m_paused				= false;
#endif

	environmentTime_		= 0.f;
	environmentTimeFactor_  = 12.f;

	WFXStartTime_			= 0.f;

	wind_strength_factor	= 0.f;
	wind_gust_factor		= 0.f;

	wind_blast_strength	= 0.f;
	wind_blast_direction.set(1.f,0.f,0.f);

	wind_blast_strength_start_value	= 0.f;
	wind_blast_strength_stop_value	= 0.f;

	// fill clouds hemi verts & faces 
	const Fvector* verts;
	CloudsVerts.resize		(xrHemisphereVertices(2,verts));
	CopyMemory				(&CloudsVerts.front(),verts,CloudsVerts.size()*sizeof(Fvector));
	const u16* indices;
	CloudsIndices.resize	(xrHemisphereIndices(2,indices));
	CopyMemory				(&CloudsIndices.front(),indices,CloudsIndices.size()*sizeof(u16));

	// perlin noise
	PerlinNoise1D			= xr_new <CPerlinNoise1D>(Random.randI(0,0xFFFF));
	PerlinNoise1D->SetOctaves(2);
	PerlinNoise1D->SetAmplitude(0.66666f);

    // params
	p_var_alt		= deg2rad(pSettings->r_float					( "thunderbolt_common", "altitude" ));  
	p_var_long		= deg2rad(pSettings->r_float					( "thunderbolt_common", "delta_longitude" ));
	p_min_dist		= _min(.95f, pSettings->r_float					( "thunderbolt_common", "min_dist_factor" ));
	p_tilt			= deg2rad(pSettings->r_float					( "thunderbolt_common", "tilt" ));
	p_second_prop	= pSettings->r_float							( "thunderbolt_common", "second_propability" );

	clamp(p_second_prop, 0.f, 1.f);

	p_sky_color		= pSettings->r_float							( "thunderbolt_common", "sky_color" );
	p_sun_color		= pSettings->r_float							( "thunderbolt_common", "sun_color" );
	p_fog_color		= pSettings->r_float							( "thunderbolt_common", "fog_color" );


	isSoftTransition_		= false;
	softTransTarget_		= nullptr;
	softTransTargetWName_	= nullptr;

	softTransSequence_.push_back(xr_new <CEnvDescriptor>());
	softTransSequence_.push_back(xr_new <CEnvDescriptor>());
	softTransSequence_.push_back(xr_new <CEnvDescriptor>());
}


CEnvironment::~CEnvironment	()
{
	xr_delete				(PerlinNoise1D);
	OnDeviceDestroy			();

	destroy_mixer			();
}


void CEnvironment::Invalidate()
{
	bWFX					= false;
	isSoftTransition_		= false;

	// remove textures from memory
	if (Current[0] && Current[0]->m_pDescriptor)
		Current[0]->m_pDescriptor->OnDeviceDestroy();
	if (Current[1] && Current[1]->m_pDescriptor)
		Current[1]->m_pDescriptor->OnDeviceDestroy();

	Current[0]				= 0;
	Current[1]				= 0;
	if (eff_LensFlare)		eff_LensFlare->Invalidate();
}

float CEnvironment::TimeDiff(float prev, float cur)
{
	if (prev>cur)
		return (DAY_LENGTH-prev) + cur;
	else
		return cur-prev;
}

float CEnvironment::TimeWeight(float val, float min_t, float max_t)
{
	float weight = 0.f;
	float length = TimeDiff(min_t, max_t);

	if (!fis_zero(length, EPS))
	{
		if (min_t > max_t)
		{
			if ((val >= min_t) || (val <= max_t))
				weight = TimeDiff(min_t, val) / length;
		}
		else
		{
			if ((val>=min_t) && (val<=max_t))
				weight = TimeDiff(min_t, val) / length;
		}

		clamp		(weight, 0.f, 1.f);
	}

	return weight;
}

void CEnvironment::ChangeEnvironmentTime(float game_time)
{
	environmentTime_ = NormalizeTime(environmentTime_ + game_time);
}

void CEnvironment::SetEnvTime(u64 game_time)
{

#ifndef _EDITOR
	if (m_paused)
	{
		return;
	}
#endif

	float converted = float(s64(game_time % (24 * 60 * 60 * 1000))) / 1000.f;

	if (bWFX)
		wfx_time -= TimeDiff(environmentTime_, converted);

	environmentTime_ = converted;
}

float CEnvironment::NormalizeTime(float tm)
{
	if (tm < 0.f)
		return tm + DAY_LENGTH;
	else if (tm > DAY_LENGTH)
		return tm - DAY_LENGTH;
	else
		return tm;
}


void CEnvironment::SetWeather(shared_str name, bool forced)
{
	if (name.size())
	{
        EnvsMapIt it = WeatherCycles.find(name);

		if (it == WeatherCycles.end())
		{
			Msg("! Invalid weather name: %s", name.c_str());

			return;
		}

        R_ASSERT3(it != WeatherCycles.end(), "Invalid weather name.", *name);

		CurrentWeatherName = it->first;

		if (forced)
		{
			Invalidate();
		}

		if (!bWFX)
		{
			CurrentWeather		= &it->second;
			CurrentWeatherName	= it->first;
		}

		if (forced)
		{
			SelectEnvs(environmentTime_);
		}

#ifdef WEATHER_LOGGING
		Msg("@ Starting Cycle: %s [%s]", *name, forced ? "forced" : "deferred");
#endif

    }
	else
	{

#ifndef _EDITOR
		FATAL("! Empty weather name");
#endif

    }
}


bool CEnvironment::SetWeatherFX(shared_str name, float game_time)
{
	if (bWFX)
		return false; // does not support overlays

	if (name.size())
	{
		if (game_time < 0.f) // if time is not specified - use Environment time
			game_time = environmentTime_;

#ifdef WEATHER_LOGGING
		Msg("@ Starting WFX as at %f Game Time", game_time);
#endif

		EnvsMapIt it		= WeatherFXs.find(name);

		R_ASSERT3			(it != WeatherFXs.end(), "Invalid weather effect name.", *name);

		prevWeather_		= CurrentWeather;
		prevWeatherName_	= CurrentWeatherName;
		WFXStartTime_		= game_time; // for save/load
		R_ASSERT(prevWeather_);

#ifdef WEATHER_LOGGING
		Msg("@ Regular weather: Current[0] ex.t. = %f, Current[1] ex.t. = %f", Current[0]->exec_time, Current[1]->exec_time);
#endif

		// Various set ups for correct weather transition

		CurrentWeather		= &it->second;
		CurrentWeatherName	= it->first;

		float rewind_tm		= WFX_TRANS_TIME * environmentTimeFactor_;
		float start_tm		= game_time + rewind_tm;

		float current_length; // the time dif. betwean regular weather's current 0 and current 1

		if (Current[0]->exec_time > Current[1]->exec_time)
			current_length = (DAY_LENGTH - Current[0]->exec_time) + Current[1]->exec_time;
		else
			current_length = Current[1]->exec_time - Current[0]->exec_time;


		std::sort(CurrentWeather->begin(), CurrentWeather->end(), sort_env_etl_pred); // raw sorting of WFX cycle, before we modify exec time for them

		// These first two sections are needed to prevent weather change glitch
		CEnvDescriptor* first					= CurrentWeather->at(0); // Prev. weather's current 0 with modified exec. time
		CEnvDescriptor* second					= CurrentWeather->at(1); // Prev. weather's current 1 with modified exec. time

		CEnvDescriptor* just_before_last_one	= CurrentWeather->at(CurrentWeather->size() - 2); // The section on which WFX is technicaly finished
		CEnvDescriptor* last_one				= CurrentWeather->at(CurrentWeather->size() - 1); // This section will be the one, to which WFX will be transformed at the end


		first->copy(*Current[0]);
		first->exec_time = NormalizeTime(game_time - ((rewind_tm / (Current[1]->exec_time - game_time)) * current_length - rewind_tm));

		second->copy(*Current[1]);
		second->exec_time = NormalizeTime(start_tm);

		for (EnvIt t_it = CurrentWeather->begin() + 2; t_it != CurrentWeather->end() - 1; t_it++)
			(*t_it)->exec_time = NormalizeTime(start_tm + (*t_it)->exec_time_loaded);

		SelectEnv(prevWeather_, WFX_end_desc, just_before_last_one->exec_time);		// Select weather section that is gonna be after WFX is played
		last_one->copy(*WFX_end_desc);												// Copy selected section to the ending iterrator of WFX
		last_one->exec_time = WFX_end_desc->exec_time;


		wfx_time			= TimeDiff(game_time, last_one->exec_time);
		bWFX				= true;

		std::sort(CurrentWeather->begin(), CurrentWeather->end(), sort_env_pred); // final sort of wfx cycle

		// Apply copy of prev weather with modified exec time for smoth transition
		Current[0] = first;
		Current[1] = second;

#ifdef WEATHER_LOGGING
		Msg("@ Starting WFX cycle: '%s' - %3.2f sec; WFX sections = %u", *name, wfx_time, CurrentWeather->size());

		u32 i = 0;
		for (EnvIt l_it = CurrentWeather->begin(); l_it != CurrentWeather->end(); l_it++, ++i)
		{
			CEnvDescriptor* desc = *l_it;
			Msg("@ Section # %u: %s; Exec. Time %3.2f", i, desc->m_identifier.c_str(), desc->exec_time);
		}
#endif

	}
	else
	{

#ifndef _EDITOR
		FATAL("! Empty weather effect name");
#endif

	}

	return true;
}


bool CEnvironment::StartWeatherFXFromTime(shared_str name, float time)
{
	if(!SetWeatherFX(name))				
		return false;

	for (EnvIt it = CurrentWeather->begin(); it != CurrentWeather->end(); it++)
		(*it)->exec_time = NormalizeTime((*it)->exec_time - wfx_time + time);

	wfx_time = time;
	return true;
}

void CEnvironment::StopWFX()
{
	R_ASSERT(CurrentWeatherName.size());

	bWFX = false;

#ifdef WEATHER_LOGGING
	Msg("@ WFX cycle ended. Weather: '%s' Desc: '%s'/'%s' GameTime: %3.2f", CurrentWeatherName.c_str(), Current[0]->m_identifier.c_str(), Current[1]->m_identifier.c_str(), environmentTime_);
#endif
}


void CEnvironment::SoftTransition(shared_str name, float transition_time)
{

#ifdef WEATHER_LOGGING
	Msg("@ Creating Soft Transition Cycle: %s over %3.2fs", name.c_str(), transition_time);
#endif

	if (name.size())
	{
		EnvsMapIt transit_to = WeatherCycles.find(name);

		if (transit_to == WeatherCycles.end())
		{
			Msg("! Invalid weather name: %s", name.c_str());
			return;
		}

		R_ASSERT3(transit_to != WeatherCycles.end(), "Invalid weather name.", *name);

		softTransTarget_		= &transit_to->second;
		softTransTargetWName_	= transit_to->first;


		CurrentWeather = &softTransSequence_;

		CEnvDescriptor* first	= CurrentWeather->at(0); // Prev. weather's current 0 with modified exec. time
		CEnvDescriptor* second	= CurrentWeather->at(1); // Prev. weather's current 1 with modified exec. time
		CEnvDescriptor* third	= CurrentWeather->at(2); // a target weather's "previous hour" section.(for target weather Current[0])


		float rewind_tm = STR_BASE_TIME * environmentTimeFactor_;
		float start_tm = environmentTime_ + rewind_tm;
		float current_length;

		if (Current[0]->exec_time > Current[1]->exec_time)
			current_length = (DAY_LENGTH - Current[0]->exec_time) + Current[1]->exec_time;
		else
			current_length = Current[1]->exec_time - Current[0]->exec_time;


		// Create the soft transition sections
		first->copy(*Current[0]);
		first->exec_time = NormalizeTime(environmentTime_ - ((rewind_tm / (Current[1]->exec_time - environmentTime_)) * current_length - rewind_tm));

		second->copy(*Current[1]);
		second->exec_time = NormalizeTime(start_tm);

		CEnvDescriptor* temp;
		FindWeatherFor_Before(softTransTarget_, temp, environmentTime_);

		third->copy(*temp);
		third->exec_time = second->exec_time + transition_time;

		// Apply copy of prev weather with modified exec time for smoth transition
		Current[0] = first;
		Current[1] = second;

		isSoftTransition_ = true;

#ifdef WEATHER_LOGGING
		Msg("@ Starting Soft Transition Cycle to %s over %3.2f sec", *name, transition_time);

		u32 i = 0;
		for (EnvIt l_it = CurrentWeather->begin(); l_it != CurrentWeather->end(); l_it++, ++i)
		{
			CEnvDescriptor* desc = *l_it;
			Msg("@ Section # %u: %s; Exec. Time %3.2f", i, desc->m_identifier.c_str(), desc->exec_time);
		}

#endif

	}
}


void CEnvironment::OnSoftTransFinished(float gt)
{
	R_ASSERT(CurrentWeatherName.size());

	isSoftTransition_ = false;
	SetWeather(softTransTargetWName_);

	SelectEnv(CurrentWeather, Current[1], gt);
	Current[1]->m_pDescriptor->OnDeviceCreate(*Current[1]); // create textures in memory

#ifdef WEATHER_LOGGING
	Msg("@ Returning back Current[0] original exec. time");
#endif

	CEnvDescriptor* original_from_env_vector;
	SetEnvDesc(Current[0]->m_identifier.c_str(), original_from_env_vector);

	Current[0]->exec_time = original_from_env_vector->exec_time; // return back the original execution time, to avoid lerp speed ups

#ifdef WEATHER_LOGGING
	Msg("@ Soft trans finnished");
#endif
}

IC bool lb_env_pred(const CEnvDescriptor* x, float val)
{
#ifdef WEATHER_LOGGING
	Msg("@ Predicate: name = %s; exec_time = %f, val = %f", x->m_identifier.c_str(), x->exec_time, val);
#endif
	return x->exec_time < val;
}

void CEnvironment::SelectEnv(EnvVec* envs, CEnvDescriptor*& e, float gt)
{
	EnvIt env = std::lower_bound(envs->begin(), envs->end(), gt, lb_env_pred);

	if (env == envs->end())
	{
		e = envs->front();
	}
	else
	{
		e = *env;
	}
}

void CEnvironment::FindWeatherFor_Before(EnvVec* envs, CEnvDescriptor*& e, float gt)
{
	// Find the first one that is not less than Game Time, but more then others
	EnvIt env = std::lower_bound(envs->begin(), envs->end(), gt, lb_env_pred);

	if (env == envs->end())
	{
		env = envs->begin();
	}

	CEnvDescriptor* temp = *env;

	// Find The last one, that is less than Game Time
	if (temp == envs->front())
		temp = envs->back();
	else
		temp = *(env - 1);


	e = temp;
}

void CEnvironment::SetEnvDesc(LPCSTR weather_section, CEnvDescriptor*& e)
{
	EnvsMapIt _I, _E;
	_I = WeatherCycles.begin();
	_E = WeatherCycles.end();

	for (; _I != _E; _I++)
	{
		for (EnvIt it = _I->second.begin(); it != _I->second.end(); it++)
		{
			if (!xr_strcmp((*it)->m_identifier.c_str(), weather_section))
			{
				e = (*it);
				return;
			}
		}
	}

	Msg("!Could not set env descriptor from section %s", weather_section);
}


void CEnvironment::SelectEnvs(EnvVec* envs, CEnvDescriptor*& e0, CEnvDescriptor*& e1, float gt)
{
	EnvIt env = std::lower_bound(envs->begin(), envs->end(), gt, lb_env_pred);

	if (env==envs->end())
	{
		e0 = *(envs->end()-1);
		e1 = envs->front();
	}
	else
	{
		e1 = *env;

		if (env==envs->begin())
			e0 = *(envs->end()-1);
		else
			e0 = *(env-1);
	}
}


void CEnvironment::SelectEnvs(float gt)
{
	R_ASSERT(CurrentWeather);

	if ((Current[0] == Current[1]) && (Current[0] == 0)) // first or forced start
	{
		R_ASSERT(!bWFX);

		SelectEnvs(CurrentWeather, Current[0], Current[1], gt);

		// create textures in memory
		Current[0]->m_pDescriptor->OnDeviceCreate(*Current[0]);
		Current[1]->m_pDescriptor->OnDeviceCreate(*Current[1]);

		Msg("@ Init. Weather Current[0] = %s, Current[1] = %s", Current[0]->m_identifier.c_str(), Current[1]->m_identifier.c_str());
    }
	else
	{
		bool bSelect = false;

		if (Current[0]->exec_time > Current[1]->exec_time) //this is for the time betwean 23:00 and 00:01 (af_clear_23_00 and af_clear_00_00)
		{ 
			bSelect	= (gt > Current[1]->exec_time) && (gt < Current[0]->exec_time);
		}
		else //for regular time range(betwean 00:01 and 23:59)
		{
			bSelect = (gt > Current[1]->exec_time);
		}

		//if game time met the desired time for weather selection, set the "past section" to "next section" and select a new "next section"
		if (bSelect)
		{
			if (g_pGamePersistent->IsWeatherScriptLoaded())
			{
				if (Current[0] != Current[1]) // in cases like situation if it is a first selection after loading and loaded weather was soft transition weather
					Current[0]->m_pDescriptor->OnDeviceDestroy(); // remove textures from memory

				Current[0] = Current[1];

				if (bWFX) // Stop WFX if its time
				{
					if (Current[0] == CurrentWeather->at(CurrentWeather->size() - 2))
					{
						StopWFX();
					}
				}

				if (isSoftTransition_) // Finnish Soft Transition if its time
				{
					if (Current[0] == CurrentWeather->at(CurrentWeather->size() - 1))
					{
						OnSoftTransFinished(gt);
						return;
					}

				}

				if (!bWFX && !isSoftTransition_) // Dont involve script while playing WFX cycle or IF doing soft transition
					g_pGamePersistent->SelectWeatherCycle();

				SelectEnv(CurrentWeather, Current[1], gt);

				Current[1]->m_pDescriptor->OnDeviceCreate(*Current[1]); // create textures in memory

#ifdef WEATHER_LOGGING
				Msg("@ Selected Weather: '%s'; Current[0] '%s', Exec Time: %3.2f; Current[1] '%s', Exec Time: %3.2f; GT: %3.2f", CurrentWeatherName.c_str(), Current[0]->m_identifier.c_str(), Current[0]->exec_time, Current[1]->m_identifier.c_str(), Current[1]->exec_time, environmentTime_);
#endif

				R_ASSERT(Current[0] != Current[1]);
			}
		}
    }
}


void CEnvironment::lerp		(float& current_weight)
{
	SelectEnvs(environmentTime_);

	R_ASSERT(Current[0] && Current[1]);

	current_weight = TimeWeight(environmentTime_, Current[0]->exec_time, Current[1]->exec_time);

	// modifiers
	CEnvModifier			EM;
	EM.far_plane			= 0;
	EM.fog_color.set		( 0,0,0 );
	EM.fog_density			= 0;
	EM.ambient.set			( 0,0,0 );
	EM.sky_color.set		( 0,0,0 );
	EM.hemi_color.set		( 0,0,0 );
	EM.use_flags.zero		();

	Fvector	view			= Device.vCameraPosition;
	float mpower			= 0;

	for (xr_vector<CEnvModifier>::iterator mit = Modifiers.begin(); mit != Modifiers.end(); mit++)
		mpower				+= EM.sum(*mit, view);

	// final lerp
	CurrentEnv->lerp		(this, *Current[0], *Current[1], current_weight, EM, mpower);
}


void CEnvironment::OnFrame()
{
	if (!g_pGameLevel)
		return;

	float current_weight;
	lerp(current_weight);

	PerlinNoise1D->SetFrequency		(wind_gust_factor * MAX_NOISE_FREQ);
	wind_strength_factor = clampr(PerlinNoise1D->GetContinious(EngineTime()) + 0.5f, 0.f, 1.f);

    shared_str l_id						= (current_weight < 0.5f) ? Current[0]->lens_flare_id : Current[1]->lens_flare_id;
	eff_LensFlare->OnFrame				(l_id);
	shared_str t_id						= (current_weight < 0.5f) ? Current[0]->tb_id : Current[1]->tb_id;
    eff_Thunderbolt->OnFrame			(t_id, CurrentEnv->bolt_period, CurrentEnv->bolt_duration);
	eff_Rain->OnFrame					();

	// Environment params (setting)
	m_pRender->OnFrame(*this);
}


void CEnvironment::create_mixer ()
{
	R_ASSERT(!CurrentEnv);

	CurrentEnv = xr_new <CEnvDescriptorMixer>("00:00:00");
}

void CEnvironment::destroy_mixer()
{
	xr_delete				(CurrentEnv);
}

SThunderboltDesc* CEnvironment::thunderbolt_description			(const CInifile* config, shared_str const& section)
{
	SThunderboltDesc*		result = xr_new <SThunderboltDesc>();
	result->load			(config, section);
	return					(result);
}

SThunderboltCollection* CEnvironment::thunderbolt_collection	(const CInifile* pIni, LPCSTR section)
{
	SThunderboltCollection*	result = xr_new <SThunderboltCollection>();
	result->load			(pIni, section);
	return					(result);
}

SThunderboltCollection* CEnvironment::thunderbolt_collection	(xr_vector<SThunderboltCollection*>& collection,  shared_str const& id)
{
	typedef xr_vector<SThunderboltCollection*>	Container;
	Container::iterator		i = collection.begin();
	Container::iterator		e = collection.end();
	for ( ; i != e; ++i)
		if ((*i)->section == id)
			return			(*i);

	NODEFAULT;
#ifdef DEBUG
	return					(0);
#endif // #ifdef DEBUG
}

CLensFlareDescriptor* CEnvironment::add_flare					(xr_vector<CLensFlareDescriptor*>& collection, shared_str const& id)
{
	typedef xr_vector<CLensFlareDescriptor*>	Flares;

	Flares::const_iterator	i = collection.begin();
	Flares::const_iterator	e = collection.end();
	for ( ; i != e; ++i) {
		if ((*i)->section == id)
			return			(*i);
	}

	CLensFlareDescriptor*	result = xr_new <CLensFlareDescriptor>();
	result->load			(pSettings, id.c_str());
	collection.push_back	(result);	
	return					(result);
}


#define WEATHER_CHUNK_DATA 0x0009 // Other chunks are defined in alife_space.h

void CEnvironment::SaveWeather(IWriter	&memory_stream)
{
	Msg("# Saving engine weather");

	memory_stream.open_chunk(WEATHER_CHUNK_DATA);

	if (!isSoftTransition_)
	{
		memory_stream.w_stringZ(Current[0] ? Current[0]->m_identifier : "null");
		memory_stream.w_stringZ(Current[1] ? Current[1]->m_identifier : "null");
		memory_stream.w_stringZ(CurrentWeatherName.c_str() ? CurrentWeatherName : "null");
	}
	else // if it happened that player saved during soft transition cycle - Save forcasted weather names
	{
		memory_stream.w_stringZ(Current[0] ? softTransSequence_.at(2)->m_identifier : "null");
		memory_stream.w_stringZ(Current[1] ? softTransSequence_.at(2)->m_identifier : "null"); // This is ok. after loading - engine will choose correct Current[1]
		memory_stream.w_stringZ(softTransTargetWName_.c_str() ? softTransTargetWName_ : "null");
	}

	//Save WFX
	memory_stream.w_bool(bWFX);
	if (bWFX)
	{
		memory_stream.w_stringZ(CurrentWeatherName.c_str() ? CurrentWeatherName : "null");
		memory_stream.w_stringZ(prevWeatherName_.c_str() ? prevWeatherName_ : "null");

		memory_stream.w_stringZ(CurrentWeather->at(0) ? CurrentWeather->at(0)->m_identifier : "null");
		memory_stream.w_stringZ(CurrentWeather->at(1) ? CurrentWeather->at(1)->m_identifier : "null");

		memory_stream.w_float(WFXStartTime_);

#ifdef WEATHER_LOGGING
		Msg("# WFX Saved: %s %s %s %s %f", CurrentWeatherName.c_str(), prevWeatherName_.c_str(), CurrentWeather->at(0)->m_identifier.c_str(), CurrentWeather->at(1)->m_identifier.c_str(), WFXStartTime_);
#endif
	}

	memory_stream.close_chunk();
}

void CEnvironment::LoadWeather(IReader &file_stream)
{
	Msg("# Loading engine weather");

	Invalidate();

	R_ASSERT2(file_stream.find_chunk(WEATHER_CHUNK_DATA), "Can't find chunk WEATHER_CHUNK_DATA!");

	shared_str weather_before, weather_after, weather_cycle;

	file_stream.r_stringZ(weather_before);
	file_stream.r_stringZ(weather_after);
	file_stream.r_stringZ(weather_cycle);

#ifdef WEATHER_LOGGING
	Msg("@ Weather: Current[0] = %s, Current[1] = %s, CurrentWeatherName = %s", weather_before.c_str(), weather_after.c_str(), weather_cycle.c_str());
#endif

	if (xr_strcmp("null", weather_before))
	{
		SetEnvDesc(weather_before.c_str(), Current[0]);
		Current[0]->m_pDescriptor->OnDeviceCreate(*Current[0]); // create textures in memory
	}

	if (xr_strcmp("null", weather_after))
	{
		SetEnvDesc(weather_after.c_str(), Current[1]);
		Current[1]->m_pDescriptor->OnDeviceCreate(*Current[1]); // create textures in memory
	}

	if (xr_strcmp("null", weather_cycle))
		SetWeather(weather_cycle, false);


	// Load WFX
	bWFX = file_stream.r_bool();

	// To avoid saving every WFX weather section, Lets simmulate the Weather parametres to be as they were at the beginig of WFX cycle,
	// and then just run SetWeatherFX with the time shifted to WFX creation time. The function will create all WFX section as when it's called regulary
	if (bWFX)
	{
		Msg("@ Loading engine weather: WFX");

		bWFX = false;

		shared_str wfx_name, prev_weather_name, wfx_begin_name, wfx_end_name;
		float game_time_at_wfx_start;

		file_stream.r_stringZ(wfx_name);
		file_stream.r_stringZ(prev_weather_name);
		file_stream.r_stringZ(wfx_begin_name);
		file_stream.r_stringZ(wfx_end_name);
		game_time_at_wfx_start = file_stream.r_float();

#ifdef WEATHER_LOGGING
		Msg("@ WFX Loaded: %s %s %s %s %f", wfx_name.c_str(), prev_weather_name.c_str(), wfx_begin_name.c_str(), wfx_end_name.c_str(), game_time_at_wfx_start);
#endif

		SetWeather(prev_weather_name, false);

#ifdef WEATHER_LOGGING
		Msg("@ Weather is Set: %s", CurrentWeatherName.c_str());
#endif

		SetEnvDesc(wfx_begin_name.c_str(), Current[0]);
		SetEnvDesc(wfx_end_name.c_str(), Current[1]);

		// create textures in memory
		Current[0]->m_pDescriptor->OnDeviceCreate(*Current[0]);
		Current[1]->m_pDescriptor->OnDeviceCreate(*Current[1]);

#ifdef WEATHER_LOGGING
		Msg("@ Current[0] %s, Current[1] %s", Current[0]->m_identifier.c_str(), Current[1]->m_identifier.c_str());

		Msg("@ Launching WFX %s", wfx_name.c_str());
#endif

		SetWeatherFX(wfx_name, game_time_at_wfx_start);
	}
}
