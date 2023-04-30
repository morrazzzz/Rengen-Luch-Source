#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#include "xr_efflensflare.h"
#include "thunderbolt.h"
#include "rain.h"

#include "IGame_Level.h"
#include "xrGame/object_broker.h"
#include "xrGame/LevelGameDef.h"

#include "securom_api.h"
#include "IGame_Persistent.h"

void CEnvDescriptor::EnvSwingValue::lerp(const EnvSwingValue& A, const EnvSwingValue& B, float f)
{
	float fi	= 1.f-f;
	amp1		= fi*A.amp1  + f*B.amp1;
	amp2		= fi*A.amp2  + f*B.amp2;
	rot1		= fi*A.rot1  + f*B.rot1;
	rot2		= fi*A.rot2  + f*B.rot2;
	speed		= fi*A.speed + f*B.speed;
}

void CEnvModifier::load	(IReader* fs, u32 version)
{
	version = 0;

	use_flags.one();
	fs->r_fvector3	(position);

	radius			= fs->r_float();

	power			= fs->r_float();

	far_plane		= fs->r_float();

	fs->r_fvector3	(fog_color);
	fog_density		= fs->r_float();

	fs->r_fvector3	(ambient);
	fs->r_fvector3	(sky_color);
	fs->r_fvector3	(hemi_color);

	use_flags.assign(fs->r_u16());

	isBox_ = fs->r_u8();
	fs->r_fvector3(boxBounds_);

	boundingBox_.set(position, position); boundingBox_.grow(boxBounds_);
}

float	CEnvModifier::sum	(CEnvModifier& M, Fvector3& view)
{
	float _dist_sq = view.distance_to_sqr(M.position);

	if (isBox_)
	{
		Fvector	self_cr;
		M.boundingBox_.getcenter(self_cr);

		if (!M.boundingBox_.contains(view))
			return 0;
	}
	else
	{
		if (_dist_sq >= (M.radius * M.radius))
			return 0;
	}

	float _att = 1 - _sqrt(_dist_sq) / M.isBox_ ? (M.boxBounds_.x + M.boxBounds_.y + M.boxBounds_.z) / 3 : M.radius;
	float _power = M.power * _att;
	

	if(M.use_flags.test(eViewDist))
	{
		far_plane			+=	M.far_plane*_power;
		use_flags.set		(eViewDist, TRUE);
	}
	if(M.use_flags.test(eFogColor))
	{
		fog_color.mad		(M.fog_color,_power);
		use_flags.set		(eFogColor, TRUE);
	}
	if(M.use_flags.test(eFogDensity))
	{
		fog_density			+=	M.fog_density*_power;
		use_flags.set		(eFogDensity, TRUE);
	}

	if(M.use_flags.test(eAmbientColor))
	{
		ambient.mad			(M.ambient,_power);
		use_flags.set		(eAmbientColor, TRUE);
	}

	if(M.use_flags.test(eSkyColor))
	{
		sky_color.mad		(M.sky_color,_power);
		use_flags.set		(eSkyColor, TRUE);
	}

	if(M.use_flags.test(eHemiColor))
	{
		hemi_color.mad		(M.hemi_color,_power);
		use_flags.set		(eHemiColor, TRUE);
	}
	
	return				_power;
}

//-----------------------------------------------------------------------------
// Environment ambient
//-----------------------------------------------------------------------------
void CEnvAmbient::load(const shared_str& sect)
{
	section				= sect;
	string_path			tmp;
	// sounds
	if (pSettings->line_exist(sect,"sounds")){
		Fvector2 t		= pSettings->r_fvector2	(sect,"sound_period");
		sound_period.set(iFloor(t.x*1000.f),iFloor(t.y*1000.f));
		sound_dist		= pSettings->r_fvector2	(sect,"sound_dist"); if (sound_dist[0]>sound_dist[1]) std::swap(sound_dist[0],sound_dist[1]);
		LPCSTR snds		= pSettings->r_string	(sect,"sounds");
		u32 cnt			= _GetItemCount(snds);
		if (cnt){
			sounds.resize(cnt);
			for (u32 k=0; k<cnt; ++k)
				sounds[k].create(_GetItem(snds,k,tmp),st_Effect,sg_SourceType);
		}
	}
	// effects
	if (pSettings->line_exist(sect,"effects")){
		Fvector2 t		= pSettings->r_fvector2	(sect,"effect_period");
		effect_period.set(iFloor(t.x*1000.f),iFloor(t.y*1000.f));
		LPCSTR effs		= pSettings->r_string	(sect,"effects");
		u32 cnt			= _GetItemCount(effs);
		if (cnt){
			effects.resize(cnt);
			for (u32 k=0; k<cnt; ++k){
				_GetItem(effs,k,tmp);
				effects[k].life_time		= iFloor(pSettings->r_float(tmp,"life_time")*1000.f);
				effects[k].particles		= pSettings->r_string	(tmp,"particles");		VERIFY(effects[k].particles.size());
				effects[k].offset			= pSettings->r_fvector3	(tmp,"offset");
				effects[k].wind_gust_factor	= pSettings->r_float	(tmp,"wind_gust_factor");
				if (pSettings->line_exist(tmp,"sound"))
					effects[k].sound.create	(pSettings->r_string(tmp,"sound"),st_Effect,sg_SourceType);
			}
		}
	}
	//VERIFY(!sounds.empty() || !effects.empty());
}

//-----------------------------------------------------------------------------
// Environment descriptor
//-----------------------------------------------------------------------------
CEnvDescriptor::CEnvDescriptor	() :
	m_identifier		(NULL)
{
	exec_time			= 0.0f;
	exec_time_loaded	= 0.0f;
	
	clouds_color.set	(1,1,1,1);
	sky_color.set		(1,1,1);
	sky_rotation		= 0.0f;

	far_plane			= 400.0f;;

	fog_color.set		(1,1,1);
	fog_density			= 0.0f;
	fog_distance		= 400.0f;

	rain_density		= 0.0f;
	rain_color.set		(0,0,0);

	bolt_period			= 0.0f;
	bolt_duration		= 0.0f;

	wind_velocity		= 0.0f;
	clouds_velocity_0	= 0.0f;
	clouds_velocity_1	= 0.0f;
	wind_direction		= 0.0f;
	wind_volume		= 0.0f;
    
	ambient.set			(0,0,0);
	hemi_color.set		(1,1,1,1);
	sun_color.set		(1,1,1);
	sun_dir.set			(0,-1,0);

	dof_value.set		(-1.25f, 1.4f, 600.f);
	dof_kernel			= 5.0f;
	dof_sky				= 30.0f;

    lens_flare_id		= "";
	tb_id				= "";

	m_fSunShaftsIntensity	= 0;
	m_fWaterIntensity		= 1;

	m_fTreeAmplitude		= 0.005f;
	m_fTreeSpeed			= 1.00f;
	m_fTreeRotation			= 10.0f;
	m_fTreeWave.set			(.1f, .01f, .11f);
    
	env_ambient			= NULL;
}

#define	C_CHECK(C)	if (C.x<0 || C.x>2 || C.y<0 || C.y>2 || C.z<0 || C.z>2)	{ Msg("! Invalid '%s' in env-section '%s'",#C,m_identifier.c_str());}
void CEnvDescriptor::load	(CEnvironment& environment, LPCSTR exec_tm, LPCSTR S)
{
	m_identifier				= S;

	Ivector3 tm				={0,0,0};
	sscanf					(exec_tm,"%d:%d:%d",&tm.x,&tm.y,&tm.z);
	R_ASSERT3				((tm.x>=0)&&(tm.x<24)&&(tm.y>=0)&&(tm.y<60)&&(tm.z>=0)&&(tm.z<60),"Incorrect weather time",m_identifier.c_str());
	exec_time				= tm.x*3600.f+tm.y*60.f+tm.z;
	exec_time_loaded		= exec_time;
	string_path				st,st_env;
	xr_strcpy				(st,pSettings->r_string	(m_identifier.c_str(),"sky_texture"));
	strconcat				(sizeof(st_env),st_env,st,"#small"		);
	sky_texture_name		= st;
	sky_texture_env_name	= st_env;
	clouds_texture_name		= pSettings->r_string	(m_identifier.c_str(),"clouds_texture");

	clouds_color			= pSettings->r_fvector4	(m_identifier.c_str(),"clouds_color");

	sky_color				= pSettings->r_fvector3	(m_identifier.c_str(),"sky_color");		
	
	if (pSettings->line_exist(m_identifier.c_str(),"sky_rotation"))	sky_rotation	= deg2rad(pSettings->r_float(m_identifier.c_str(),"sky_rotation"));
	else											sky_rotation	= 0;
	far_plane				= pSettings->r_float	(m_identifier.c_str(),"far_plane");
	fog_color				= pSettings->r_fvector3	(m_identifier.c_str(),"fog_color");
	fog_density				= pSettings->r_float	(m_identifier.c_str(),"fog_density");
	fog_distance			= pSettings->r_float	(m_identifier.c_str(),"fog_distance");
	rain_density			= pSettings->r_float	(m_identifier.c_str(),"rain_density");		clamp(rain_density,0.f,4.f);
	if (rain_density >= 0.01) rain_density += Random.randF(0.0f, 1.4f);// probability for random rain density

	rain_color				= pSettings->r_fvector3	(m_identifier.c_str(),"rain_color");            
	wind_velocity			= pSettings->r_float	(m_identifier.c_str(),"wind_velocity");
	clouds_velocity_0			= pSettings->line_exist(m_identifier.c_str(),"clouds_velocity_0")?pSettings->r_float	(m_identifier.c_str(),"clouds_velocity_0"):0.003f;
	clouds_velocity_1			= pSettings->line_exist(m_identifier.c_str(),"clouds_velocity_1")?pSettings->r_float	(m_identifier.c_str(),"clouds_velocity_1"):0.05f;
	wind_direction			= deg2rad(pSettings->r_float(m_identifier.c_str(),"wind_direction"));
	wind_volume				= pSettings->line_exist(m_identifier.c_str(),"wind_sound_volume") ? pSettings->r_float(m_identifier.c_str(),"wind_sound_volume"):0.0f;
	ambient					= pSettings->r_fvector3	(m_identifier.c_str(),"ambient");
	hemi_color				= pSettings->r_fvector4	(m_identifier.c_str(),"hemi_color");
	sun_color				= pSettings->r_fvector3	(m_identifier.c_str(),"sun_color");
	Fvector2 sund			= pSettings->r_fvector2	(m_identifier.c_str(),"sun_dir");	sun_dir.setHP	(deg2rad(sund.y),deg2rad(sund.x));
	//VERIFY2					(sun_dir.y < 0, "Invalid sun direction settings while loading");

	lens_flare_id			= environment.eff_LensFlare->AppendDef(environment, pSettings->r_string(m_identifier.c_str(),"flares"));
	tb_id					= environment.eff_Thunderbolt->AppendDef(environment, pSettings, pSettings->r_string(m_identifier.c_str(),"thunderbolt"));
	bolt_period				= (tb_id.size())?pSettings->r_float	(m_identifier.c_str(),"bolt_period"):0.f;
	bolt_duration			= (tb_id.size())?pSettings->r_float	(m_identifier.c_str(),"bolt_duration"):0.f;
	env_ambient				= pSettings->line_exist(m_identifier.c_str(),"env_ambient")?environment.AppendEnvAmb	(pSettings->r_string(m_identifier.c_str(),"env_ambient")):0;

	if (pSettings->line_exist(m_identifier.c_str(),"sun_shafts_intensity"))
		m_fSunShaftsIntensity = pSettings->r_float(m_identifier.c_str(),"sun_shafts_intensity");

	if (pSettings->line_exist(m_identifier.c_str(),"water_intensity"))
		m_fWaterIntensity = pSettings->r_float(m_identifier.c_str(),"water_intensity");

	m_fTreeAmplitude			= pSettings->line_exist(S, "trees_amplitude") ? pSettings->r_float(S, "trees_amplitude") : 0.005f;
	m_fTreeSpeed				= pSettings->line_exist(S, "trees_speed") ? pSettings->r_float(S, "trees_speed") : 1.00f;
	m_fTreeRotation				= pSettings->line_exist(S, "trees_rotation") ? pSettings->r_float(S, "trees_rotation") : 10.0f;

	if (pSettings->line_exist(S, "trees_wave"))
		m_fTreeWave			= pSettings->r_fvector3	(S,"trees_wave");
	else
		m_fTreeWave.set			(.1f, .01f, .11f);

	sun_lumscale = pSettings->line_exist(S, "sun_lumscale") ? pSettings->r_float(S, "sun_lumscale") : 1.f;
	dof_value = pSettings->line_exist(S, "dof") ? pSettings->r_fvector3(m_identifier.c_str(), "dof") : Fvector3().set(-1.25f, 1.4f, 600.f);
	dof_kernel = pSettings->line_exist(S, "dof_kernel") ? pSettings->r_float(S, "dof_kernel") : 7.0f;
	dof_sky = pSettings->line_exist(S, "dof_sky") ? pSettings->r_float(S, "dof_sky") : 30.0f;
	
	// swing desc
	// normal
	m_cSwingDesc[0].amp1	= pSettings->line_exist(S, "swing_normal_amp1") ? pSettings->r_float(S,"swing_normal_amp1") : pSettings->r_float("details","swing_normal_amp1");
	m_cSwingDesc[0].amp2	= pSettings->line_exist(S, "swing_normal_amp2") ? pSettings->r_float(S,"swing_normal_amp2") : pSettings->r_float("details","swing_normal_amp2");
	m_cSwingDesc[0].rot1	= pSettings->line_exist(S, "swing_normal_rot1") ? pSettings->r_float(S,"swing_normal_rot1") : pSettings->r_float("details","swing_normal_rot1");
	m_cSwingDesc[0].rot2	= pSettings->line_exist(S, "swing_normal_rot2") ? pSettings->r_float(S,"swing_normal_rot2") : pSettings->r_float("details","swing_normal_rot2");
	m_cSwingDesc[0].speed	= pSettings->line_exist(S, "swing_normal_speed") ? pSettings->r_float(S,"swing_normal_speed") : pSettings->r_float("details","swing_normal_speed");
	// fast
	m_cSwingDesc[1].amp1	= pSettings->line_exist(S, "swing_fast_amp1") ? pSettings->r_float(S,"swing_fast_amp1") : pSettings->r_float("details","swing_fast_amp1");
	m_cSwingDesc[1].amp2	= pSettings->line_exist(S, "swing_fast_amp2") ? pSettings->r_float(S,"swing_fast_amp2") : pSettings->r_float("details","swing_fast_amp2");
	m_cSwingDesc[1].rot1	= pSettings->line_exist(S, "swing_fast_rot1") ? pSettings->r_float(S,"swing_fast_rot1") : pSettings->r_float("details","swing_fast_rot1");
	m_cSwingDesc[1].rot2	= pSettings->line_exist(S, "swing_fast_rot2") ? pSettings->r_float(S,"swing_fast_rot2") : pSettings->r_float("details","swing_fast_rot2");
	m_cSwingDesc[1].speed	= pSettings->line_exist(S, "swing_fast_speed") ? pSettings->r_float(S,"swing_fast_speed") : pSettings->r_float("details","swing_fast_speed");

	C_CHECK					(clouds_color);
	C_CHECK					(sky_color	);
	C_CHECK					(fog_color	);
	C_CHECK					(rain_color	);
	C_CHECK					(ambient	);
	C_CHECK					(hemi_color	);
	C_CHECK					(sun_color	);
	on_device_create		();
}

void CEnvDescriptor::on_device_create	()
{
	//m_pDescriptor->OnDeviceCreate(*this);
	/*
	if (sky_texture_name.size())	
		sky_texture.create		(sky_texture_name.c_str());

	if (sky_texture_env_name.size())
		sky_texture_env.create	(sky_texture_env_name.c_str());

	if (clouds_texture_name.size())	
		clouds_texture.create	(clouds_texture_name.c_str());
		*/
}

void CEnvDescriptor::on_device_destroy	()
{
	m_pDescriptor->OnDeviceDestroy();
	/*
	sky_texture.destroy		();
	sky_texture_env.destroy	();
	clouds_texture.destroy	();
	*/
}

//-----------------------------------------------------------------------------
// Environment Mixer
//-----------------------------------------------------------------------------
CEnvDescriptorMixer::CEnvDescriptorMixer(shared_str const& identifier) :
	CEnvDescriptor	()
{
}

void CEnvDescriptorMixer::destroy()
{
	m_pDescriptorMixer->Destroy();
	/*
	sky_r_textures.clear		();
	sky_r_textures_env.clear	();
	clouds_r_textures.clear		();
	*/

	//	Reuse existing code
	on_device_destroy();
/*
	sky_texture.destroy			();
	sky_texture_env.destroy		();
	clouds_texture.destroy		();
	*/
}

void CEnvDescriptorMixer::clear	()
{
	m_pDescriptorMixer->Clear();
	/*
	std::pair<u32,ref_texture>	zero = mk_pair(u32(0),ref_texture(0));
	sky_r_textures.clear		();
	sky_r_textures.push_back	(zero);
	sky_r_textures.push_back	(zero);
	sky_r_textures.push_back	(zero);

	sky_r_textures_env.clear	();
	sky_r_textures_env.push_back(zero);
	sky_r_textures_env.push_back(zero);
	sky_r_textures_env.push_back(zero);

	clouds_r_textures.clear		();
	clouds_r_textures.push_back	(zero);
	clouds_r_textures.push_back	(zero);
	clouds_r_textures.push_back	(zero);
	*/
}

#define NO_RAIN_TO_RAINY_TIEM_OFFSET 0.4f

void CEnvDescriptorMixer::lerp	(CEnvironment* , CEnvDescriptor& A, CEnvDescriptor& B, float f, CEnvModifier& Mdf, float modifier_power)
{
	float	modif_power		=	1.f/(modifier_power+1);	// the environment itself
	float	fi				=	1-f;

	m_pDescriptorMixer->lerp(&*A.m_pDescriptor, &*B.m_pDescriptor);

	weight					=	f;

	// rain specific time weght, for making smoother rain and thunder
	float rain_f = f;
	{
		if (A.rain_density <= 0.01f) // if the previous desc did not have the rain at all - make a delayed rain begining
			if (f < NO_RAIN_TO_RAINY_TIEM_OFFSET) // dont start rain and rain effects, untill we are close to the half of the cycle time
				rain_f = 0.f;
			else // if we are close to the half of the cycle time - start lerping rain
				rain_f = (f - NO_RAIN_TO_RAINY_TIEM_OFFSET) / (1.f - NO_RAIN_TO_RAINY_TIEM_OFFSET); // get the 0.0 - 1.0 value from offseted weight for the rest of the time, when we need the rain and rain effects enabled

		else if (B.rain_density <= 0.01f) // if next weather is not rainy at all - then DO rain only untill half of the cycle time is past
			if (f < 1.f - NO_RAIN_TO_RAINY_TIEM_OFFSET) // dont start rain and rain effects, untill we are at least close to the half of the cycle
				rain_f = f / (1.f - NO_RAIN_TO_RAINY_TIEM_OFFSET); // get the 0.0 - 1.0 value from offseted weight
			else // if we passed half of the time - stop rain and effects
				rain_f = 1.f;
	}

	float rain_fi = 1.f - rain_f;

	R_ASSERT2(rain_f >= 0.f && rain_f <= 1.f, make_string("rain_f = %f", rain_f));

	clouds_color.lerp		(A.clouds_color,B.clouds_color,f);

	sky_rotation			=	(fi*A.sky_rotation + f*B.sky_rotation);

//.	far_plane				=	(fi*A.far_plane + f*B.far_plane + Mdf.far_plane)*psVisDistance*modif_power;
	if(Mdf.use_flags.test(eViewDist))
		far_plane				=	(fi*A.far_plane + f*B.far_plane + Mdf.far_plane)*psVisDistance*modif_power;
	else
		far_plane				=	(fi*A.far_plane + f*B.far_plane)*psVisDistance;
	
//.	fog_color.lerp			(A.fog_color,B.fog_color,f).add(Mdf.fog_color).mul(modif_power);
	fog_color.lerp			(A.fog_color,B.fog_color,f);
	if(Mdf.use_flags.test(eFogColor))
		fog_color.add(Mdf.fog_color).mul(modif_power);

//.	fog_density				=	(fi*A.fog_density + f*B.fog_density + Mdf.fog_density)*modif_power;
	fog_density				=	(fi*A.fog_density + f*B.fog_density);
	if(Mdf.use_flags.test(eFogDensity))
	{
		fog_density			+= Mdf.fog_density;
		fog_density			*= modif_power;
	}

	// fog distance K shold not be greater than view distance K
	float fog_dist_K = psFogDistance <= psVisDistance ? psFogDistance : psVisDistance;

	fog_distance			= (fi*A.fog_distance + f*B.fog_distance)*fog_dist_K;
	fog_near				=	(1.0f - fog_density)*0.85f * fog_distance;
	fog_far					=	0.99f * fog_distance;

	clamp(far_plane, fog_distance + 100.f, far_plane); // far plane should always be further than fog distance, to avoid glitching of objects

	rain_density			=	rain_fi*A.rain_density + rain_f*B.rain_density;
	rain_color.lerp			(A.rain_color,B.rain_color,rain_f);
	bolt_period				=	rain_fi*A.bolt_period + rain_f*B.bolt_period;
	bolt_duration			=	rain_fi*A.bolt_duration + rain_f*B.bolt_duration;

	// wind
	wind_velocity			=	fi*A.wind_velocity + f*B.wind_velocity;
	clouds_velocity_0			=	fi*A.clouds_velocity_0 + f*B.clouds_velocity_0;
	clouds_velocity_1			=	fi*A.clouds_velocity_1 + f*B.clouds_velocity_1;
	wind_direction			=	fi*A.wind_direction + f*B.wind_direction;
	wind_volume			=	fi*A.wind_volume + f*B.wind_volume;

	m_fSunShaftsIntensity	=	fi*A.m_fSunShaftsIntensity + f*B.m_fSunShaftsIntensity;
	m_fWaterIntensity		=	fi*A.m_fWaterIntensity + f*B.m_fWaterIntensity;

	//trees
	m_fTreeAmplitude		=	fi*A.m_fTreeAmplitude + f*B.m_fTreeAmplitude;
	m_fTreeSpeed			=	fi*A.m_fTreeSpeed + f*B.m_fTreeSpeed;
	m_fTreeRotation			=	fi*A.m_fTreeRotation + f*B.m_fTreeRotation;
	m_fTreeWave.lerp			(A.m_fTreeWave,B.m_fTreeWave,f);

	// colors
//.	sky_color.lerp			(A.sky_color,B.sky_color,f).add(Mdf.sky_color).mul(modif_power);
	sky_color.lerp			(A.sky_color,B.sky_color,f);
	if(Mdf.use_flags.test(eSkyColor))
		sky_color.add(Mdf.sky_color).mul(modif_power);

//.	ambient.lerp			(A.ambient,B.ambient,f).add(Mdf.ambient).mul(modif_power);
	ambient.lerp			(A.ambient,B.ambient,f);
	if(Mdf.use_flags.test(eAmbientColor))
		ambient.add(Mdf.ambient).mul(modif_power);

	hemi_color.lerp			(A.hemi_color,B.hemi_color,f);

	if(Mdf.use_flags.test(eHemiColor))
	{
		hemi_color.x			+= Mdf.hemi_color.x;
		hemi_color.y			+= Mdf.hemi_color.y; 
		hemi_color.z			+= Mdf.hemi_color.z;
		hemi_color.x			*= modif_power;
		hemi_color.y			*= modif_power;
		hemi_color.z			*= modif_power;
	}

	sun_color.lerp			(A.sun_color,B.sun_color,f);

	sun_lumscale			= fi*A.sun_lumscale + f*B.sun_lumscale;
	dof_value.lerp			(A.dof_value,B.dof_value,f);
	dof_kernel				= fi*A.dof_kernel + f*B.dof_kernel;
	dof_sky					= fi*A.dof_sky + f*B.dof_sky;

	m_cSwingDesc[0].lerp			(A.m_cSwingDesc[0],B.m_cSwingDesc[0],f);
	m_cSwingDesc[1].lerp			(A.m_cSwingDesc[1],B.m_cSwingDesc[1],f);

	R_ASSERT				( _valid(A.sun_dir) );
	R_ASSERT				( _valid(B.sun_dir) );
	sun_dir.lerp			(A.sun_dir,B.sun_dir,f).normalize();
	R_ASSERT				( _valid(sun_dir) );

	//VERIFY2					(sun_dir.y<0,"Invalid sun direction settings while lerp")
	;}

//-----------------------------------------------------------------------------
// Environment IO
//-----------------------------------------------------------------------------
CEnvAmbient* CEnvironment::AppendEnvAmb		(const shared_str& sect)
{
	for (EnvAmbVecIt it=Ambients.begin(); it!=Ambients.end(); it++)
		if ((*it)->name().equal(sect)) return *it;
	Ambients.push_back		(xr_new <CEnvAmbient>());
	Ambients.back()->load	(sect);
	return Ambients.back();
}

void	CEnvironment::mods_load			()
{
	Modifiers.clear_and_free			();
	string_path							path;
	if (FS.exist(path,"$level$","level.env_mod"))	
	{
		IReader*	fs	= FS.r_open		(path);
		u32			id	= 0;
		u32 ver		= 0;
		u32 sz;

		while( 0!=(sz=fs->find_chunk(id)) )	
		{
			if(id==0 && sz==sizeof(u32))
			{
				ver				= fs->r_u32();
			}else
			{
				CEnvModifier		E;
				E.load(fs, ver);
				Modifiers.push_back	(E);
			}
			id					++;
		}
		FS.r_close	(fs);
	}
}

void	CEnvironment::mods_unload		()
{
	Modifiers.clear_and_free			();
}

CEnvDescriptor* CEnvironment::create_descriptor	(LPCSTR exec_tm, LPCSTR S)
{
	CEnvDescriptor*	result = xr_new <CEnvDescriptor>();
	result->load(*this, exec_tm, S);

	return			(result);
}

void CEnvironment::load_weathers		()
{
	if (!WeatherCycles.empty())
		return;

	LPCSTR first_weather=0;
	int weather_count	= pSettings->line_count("weathers");
	for (int w_idx=0; w_idx<weather_count; w_idx++)
	{
		LPCSTR weather, sect_w;
		if (pSettings->r_line("weathers",w_idx,&weather,&sect_w))
		{
			if (0==first_weather) first_weather=weather;
			int env_count	= pSettings->line_count(sect_w);
			LPCSTR exec_tm, sect_e;
			for (int env_idx=0; env_idx<env_count; env_idx++)
			{
				if (pSettings->r_line(sect_w,env_idx,&exec_tm,&sect_e))
				{

					CEnvDescriptor*			object = create_descriptor(exec_tm, sect_e);
					WeatherCycles[weather].push_back			(object);
				}
			}
		}

		if (CurrentWeatherName == 0)
		{
			CurrentWeatherName = weather;
			Msg("@ Default Weather = %s", CurrentWeatherName.c_str());
		}
	}

	// sorting weather envs
	EnvsMapIt _I=WeatherCycles.begin();
	EnvsMapIt _E=WeatherCycles.end();
	for (; _I!=_E; _I++){
		R_ASSERT3	(_I->second.size()>1,"Environment in weather must >=2",*_I->first);
		std::sort(_I->second.begin(),_I->second.end(),sort_env_etl_pred);
	}
	R_ASSERT2	(!WeatherCycles.empty(),"Empty weathers.");
	SetWeather	((*WeatherCycles.begin()).first.c_str());
}

void CEnvironment::load_weather_effects	()
{
	if (!WeatherFXs.empty())
		return;

	int line_count	= pSettings->line_count("weather_effects");
	for (int w_idx=0; w_idx<line_count; w_idx++)
	{
		LPCSTR weather, sect_w;
		if (pSettings->r_line("weather_effects",w_idx,&weather,&sect_w))
		{
			EnvVec& env		= WeatherFXs[weather];
			env.push_back	(xr_new <CEnvDescriptor>()); env.back()->exec_time_loaded = 0;
			int env_count	= pSettings->line_count(sect_w);
			LPCSTR exec_tm, sect_e;
			for (int env_idx=0; env_idx<env_count; env_idx++)
			{
				if (pSettings->r_line(sect_w,env_idx,&exec_tm,&sect_e))
					env.push_back	(create_descriptor(exec_tm, sect_e));
			}
			env.push_back	(xr_new <CEnvDescriptor>());
			env.back()->exec_time_loaded = DAY_LENGTH;
		}
	}

	// sorting weather envs
	EnvsMapIt _I=WeatherFXs.begin();
	EnvsMapIt _E=WeatherFXs.end();
	for (; _I!=_E; _I++){
		R_ASSERT3	(_I->second.size()>1,"Environment in weather must >=2",*_I->first);
		std::sort	(_I->second.begin(),_I->second.end(),sort_env_etl_pred);
	}
}

void CEnvironment::load		()
{
	if (!CurrentEnv)
		create_mixer		();

	m_pRender->OnLoad();
	//tonemap					= Device.Resources->_CreateTexture("$user$tonemap");	//. hack
	if (!eff_Rain)    		eff_Rain 		= xr_new <CEffect_Rain>();
	if (!eff_LensFlare)		eff_LensFlare 	= xr_new <CLensFlare>();
	if (!eff_Thunderbolt)	eff_Thunderbolt	= xr_new <CEffect_Thunderbolt>();
	
	load_weathers			();
	load_weather_effects	();
}

void CEnvironment::unload	()
{
	EnvsMapIt _I,_E;
	// clear weathers
	_I		= WeatherCycles.begin();
	_E		= WeatherCycles.end();
	for (; _I!=_E; _I++){
		for (EnvIt it=_I->second.begin(); it!=_I->second.end(); it++)
			xr_delete	(*it);
	}

	WeatherCycles.clear		();
	// clear weather effect
	_I		= WeatherFXs.begin();
	_E		= WeatherFXs.end();
	for (; _I!=_E; _I++){
		for (EnvIt it=_I->second.begin(); it!=_I->second.end(); it++)
			xr_delete	(*it);
	}
	WeatherFXs.clear		();
	// clear ambient
	for (EnvAmbVecIt it=Ambients.begin(); it!=Ambients.end(); it++)
		xr_delete		(*it);
	Ambients.clear		();
	// misc
	xr_delete			(eff_Rain);
	xr_delete			(eff_LensFlare);
	xr_delete			(eff_Thunderbolt);
	CurrentWeather		= 0;
	CurrentWeatherName	= 0;
	CurrentEnv->clear	();
	Invalidate			();

	m_pRender->OnUnload	();
//	tonemap				= 0;
}
