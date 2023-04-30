#include "stdafx.h"
#pragma hdrstop

#include "xrRender_console.h"
#include "dxRenderDeviceRender.h"
#include "../x_ray.h"

u32 ps_Preset = 2;
xr_token qpreset_token [] = 
{
	{ "Minimum",					0 },
	{ "Low",						1 },
	{ "Default",					2 },
	{ "High",						3 },
	{ "Extreme",					4 },
	{ 0,							0 }
};

//Shadow map size; Render can't handle less, than 1536, refer to Light_Render_Direct_ComputeXFS.cpp and r2_types.h
u32	ps_r_smap_size = 1536; //default
xr_token qsmap_size_token[] = 
{
	{ "SMap_Low",		1536 },
	{ "SMap_Default",	2048 },
	{ "SMap_High",		2560 },
	{ "SMap_Extreme",	3072 },
	{ "SMap_Maximum",	4096 },
	{ 0, 0 }
};

u32 ps_r_ssao_mode = 1;
xr_token qssao_mode_token [ ] = 
{
	{ "st_opt_off",					0 },
	{ "ui_mm_ssao",					1 },
	{ "ui_mm_hdao",					2 },
	{ "ui_mm_hbao",					3 },
	{ "ui_mm_ssdo",					4 },
	{ 0,							0 }
};

u32 ps_r_sun_shafts = 2;
xr_token qsun_shafts_token [] =
{
	{ "st_opt_off",					0 },
	{ "st_opt_low",					1 },
	{ "st_opt_medium",				2 },
	{ "st_opt_high",				3 },
	{ 0,							0 }
};

u32 ps_r_ssao = 3;
xr_token qssao_token[] =
{
	{ "st_opt_off",					0 },
	{ "st_opt_low",					1 },
	{ "st_opt_medium",				2 },
	{ "st_opt_high",				3 },
	{ "st_opt_ultra",				4 },

	{ 0,							0 }
};

u32	ps_r_sun_quality = 1;
xr_token qsun_quality_token[] =
{
	{ "st_opt_low", 0 },
	{ "st_opt_medium", 1 },
	{ "st_opt_high", 2 },
	{ "st_opt_ultra", 3 },
	{ "st_opt_extreme", 4 },

	{ 0, 0 }
};

u32 ps_r3_msaa = 0;
xr_token qmsaa_token[] =
{
	{ "st_opt_off",					0 },
	{ "2x",							1 },
	{ "4x",							2 },
//	{ "8x",							3 },
	{ 0,							0 }
};

u32 ps_r3_msaa_atest = 0;
xr_token qmsaa__atest_token[] =
{
	{ "st_opt_off",					0 },
	{ "st_opt_atest_msaa_dx10_0",	1 },
	{ "st_opt_atest_msaa_dx10_1",	2 },
	{ 0,							0 }
};

u32	ps_r3_minmax_sm	= 3;
xr_token qminmax_sm_token[] =
{
	{ "off",						0 },
	{ "on",							1 },
	{ "auto",						2 },
	{ "autodetect",					3 },
	{ 0,							0 }
};

u32 optTessQuality_ = 0;
xr_token qtess_quality_token[] =
{
	{ "st_tess_low",			0 },
	{ "st_tess_med",			1 },
	{ "st_tess_optimum",		2 },
	{ "st_tess_overneeded",		3 },

	{ 0, 0 }
};

u32 ps_r2_tonemap_wcorr = 3;
xr_token qbloom_type_token[] =
{
	{ "st_opt_off",				1 },
	{ "st_opt_hable",			2 },
	{ "st_opt_reinhard",		3 },

	{ 0, 0 }
};

u32 ps_r_optimize_static = 1;
xr_token q_optimize_static[] =
{
	{ "st_optimize_off",		0 },
	{ "st_optimize_low",		1 },
	{ "st_optimize_med",		2 },
	{ "st_optimize_high",		3 },

	{ 0, 0 }
};

u32 ps_r_optimize_dynamic = 0;
xr_token q_optimize_dynamic[] =
{
	{ "st_optimize_off",		0 },
	{ "st_optimize_low",		1 },
	{ "st_optimize_med",		2 },
	{ "st_optimize_high",		3 },

	{ 0, 0 }
};

u32 ps_r_aa_mode = 1;
xr_token qaa_mode_token[] =
{
	{ "st_opt_off",					0 },
	{ "ui_mm_fxaa",					1 },
	{ "ui_mm_smaa",					2 },
	{ 0,							0 }
};

u32 ps_r_aa = 3;
xr_token qaa_token[] =
{
	{ "st_aa_off",					0 },
	{ "st_aa_low",					1 },
	{ "st_aa_medium",				2 },
	{ "st_aa_high",					3 },
	{ "st_aa_ultra",				4 },
	{ 0,							0 }
};

// Common
extern int			psSkeletonUpdate;
extern float		r__dtex_range;

int			ps_r__LightSleepFrames		= 10;

float		ps_r__Detail_l_ambient		= 0.9f;
float		ps_r__Detail_l_aniso		= 0.25f;
float		ps_r__Detail_density		= 0.3f;
float		ps_r__Detail_rainbow_hemi	= 0.75f;
float		ps_r__Details_sun_sh_dist	= 16;
float		ps_details_with_shadows_dist= 50.f;

float		ps_r__Tree_SBC				= 1.5f;	// scale bias correct

float		ps_r__WallmarkTTL			= 50.f;
float		ps_r__WallmarkSHIFT			= 0.0001f;
float		ps_r__WallmarkSHIFT_V		= 0.0001f;
float		ps_r__SWallmarkDiscard		= 0.5f;
float		ps_r__DWallmarkDiscard		= 10.f;

// Base factor values
float		ps_r__GLOD_ssa_start		= 256.f;
float		ps_r__GLOD_ssa_end			= 64.f;
float		ps_r__ssaDISCARD			= 3.5f;
float		ps_r__ssaHZBvsTEX			= 96.f;

// Distance factor values
float		ps_r__geomLodSpriteDistF_	= 0.75f;
float		ps_r__geomDiscardDistF_		= 0.75f;
float		ps_r__geomLodDistF_			= 0.75f;
float		ps_r__geomNTextureDistF_	= 0.75f;
float		ps_r__geomDTextureDistF_	= 0.75f;

int			ps_r__tf_Anisotropic		= 8;

float		ps_r1_pps_u					= 0.f;
float		ps_r1_pps_v					= 0.f;

int			ps_GlowsPerFrame			= 16;

// R2
float		ps_r2_ssaLOD_A				= 64.f;
float		ps_r2_ssaLOD_B				= 48.f;

extern int	rsDVB_Size;
extern int	rsDIB_Size;

//thx to K.D.
int			ps_r__Detail_radius			= 50;
u32			dm_size						= 24;
u32 		dm_cache1_line				= 12;	//dm_size*2/dm_cache1_count
u32			dm_cache_line				= 49;	//dm_size+1+dm_size
u32			dm_cache_size				= 2401;	//dm_cache_line*dm_cache_line
float		dm_fade						= 47.5;	//float(2*dm_size)-.5f;
u32			dm_current_size				= 24;
u32 		dm_current_cache1_line		= 12;	//dm_current_size*2/dm_cache1_count
u32			dm_current_cache_line		= 49;	//dm_current_size+1+dm_current_size
u32			dm_current_cache_size		= 2401;	//dm_current_cache_line*dm_current_cache_line
float		dm_current_fade				= 47.5;	//float(2*dm_current_size)-.5f;

// R2-specific - gr1ph here for lost alpha presets
Flags32 ps_r2_ls_flags = 
{
	R2FLAG_SUN 
	| R2FLAG_EXP_DONT_TEST_UNSHADOWED 
	| R3FLAG_DYN_WET_SURF
	//| R3FLAG_VOLUMETRIC_SMOKE
	//| R3FLAG_MSAA
	//| R3FLAG_MSAA_OPT
	|R3FLAG_GBUFFER_OPT
	|R2FLAG_DETAIL_BUMP
	|R2FLAG_DOF
	|R2FLAG_SOFT_PARTICLES
	|R2FLAG_SOFT_WATER
	|R2FLAG_STEEP_PARALLAX
	|R2FLAG_TONEMAP
	//|R2FLAG_VOLUMETRIC_LIGHTS
	|R_FLAG_SHADERS_CACHE
	|R2FLAG_GPU_OCC_OPTIMIZATION
};

Flags32	ps_r2_ls_flags_ext = 
{
	R2FLAGEXT_SSAO_HALF_DATA | R2FLAGEXT_ENABLE_TESSELLATION | R2FLAGEXT_SUN_FLARES | R2FLAGEXT_DOF_WEATHER | R2FLAGEXT_GLOWS_ENABLE
};

Flags32 ps_r_mt_flags =
{
	R_FLAG_MT_CALC_HOM_DETAILS | R_FLAG_MT_CALC_SUN_VIS_STRUCT | R_FLAG_MT_CALC_LGHTS_VIS_STRUCT | R_FLAG_MT_CALC_PRIOR0_VIS_STRUCT | R_FLAG_MT_CALC_PRIOR1_VIS_STRUCT | R_FLAG_MT_CALC_RAIN_VIS_STRUCT
	| R_FLAG_MT_BONES_PRECALC
};

Flags32 ps_r_misc_flags =
{
	R_MISC_USE_HOM_PRIOR_0 | R_MISC_USE_HOM_FORWARD | R_MISC_USE_HOM_LIGHT | R_MISC_USE_HOM_SUN | R_MISC_USE_SECTORS_FOR_STATIC
};

float		ps_r2_df_parallax_h			= 0.02f;
float		ps_r2_df_parallax_range		= 75.f;

float		ps_r2_steep_parallax_h				= 0.013f;
float		ps_r2_steep_parallax_distance		= 12.0f;
float		ps_r2_steep_parallax_samples		= 25.0f;
float		ps_r2_steep_parallax_samples_min	= 5.0f;

float		ps_r2_tonemap_middlegray	= 1.f;
float		ps_r2_tonemap_adaptation	= 1.f;
float		ps_r2_tonemap_low_lum		= 0.0001f;
float		ps_r2_tonemap_amount		= 2.f;
float		ps_r2_tonemap_white			= 1.f;

float		ps_r2_ls_bloom_kernel_g		= 3.f;	// was 3.3f
float		ps_r2_ls_bloom_kernel_b		= .5f;	// was 0.7f	
float		ps_r2_ls_bloom_speed		= 100.f;// was 10.f
float		ps_r2_ls_bloom_kernel_scale	= 0.7f;	// was 0.5f
float		ps_r2_ls_bloom_threshold	= 0.00001f;	// was 0.3f

Fvector		ps_r2_aa_barier				= {0.800000, 0.100000, 0.000000}; // { .8f, .1f, 0};
Fvector		ps_r2_aa_weight				= { 0.250000, 0.250000, 0.000000};
float		ps_r2_aa_kernel				= 0.5f;

float		ps_r2_mblur					= 0.2f;		// .5f

int			ps_r2_GI_depth				= 1;		// 1..5
int			ps_r2_GI_photons			= 16;		// 8..64
float		ps_r2_GI_clip				= EPS_L;
float		ps_r2_GI_refl				= .9f;

float		ps_r2_ls_depth_scale		= 1.00001f;
float		ps_r2_ls_depth_bias			= -0.0003f;
float		ps_r2_ls_squality			= 1.0f;

float		ps_r2_sun_tsm_bias			= -0.01f;
float		ps_r2_sun_near				= 20.f;

float		ps_r2_sun_depth_far_scale	= 1.00000f;
float		ps_r2_sun_depth_far_bias	= -0.00002f;
float		ps_r2_sun_depth_near_scale	= 1.0000f;
float		ps_r2_sun_depth_near_bias	= 0.00001f;
float		ps_r2_sun_lumscale			= 1.0f;
float		ps_r2_sun_lumscale_hemi		= 0.65f;
float		ps_r2_sun_lumscale_amb		= 0.25f;

float		ps_r2_sun_shadows_near_casc = 24;
float		ps_r2_sun_shadows_midl_casc = 64;
float		ps_r2_sun_shadows_far_casc	= 192;

float		ps_r2_gmaterial				= 2.2f;
BOOL		ps_r_high_optimize_shad		= 1;

float		ps_r2_dhemi_sky_scale		= 0.08f;
float		ps_r2_dhemi_light_scale     = 0.2f;
float		ps_r2_dhemi_light_flow      = 0.1f;
int			ps_r2_dhemi_count			= 5;

BOOL		ps_light_details_shadow			= FALSE;
float		ps_light_ds_max_dist_from_light = 10.f;
float		ps_light_ds_max_dist_from_cam	= 32.f;
int			ps_light_ds_max_lights			= 25;

int			ps_r2_wait_sleep			= 0;
float		maxWaitForOccCalc_			= 2.f;
BOOL		ps_r_cpu_wait_gpu			= 1;

float		ps_r2_lt_smooth				= 1.f;
float		ps_r2_slight_fade			= 0.5f;
BOOL		r__dynamicLights_			= TRUE;

//	x - min (0), y - focus (1.4), z - max (100)
Fvector3	ps_r2_dof					= Fvector3().set(-1.25f, 1.4f, 600.f);
float		ps_r2_dof_sky				= 30.0f;
float		ps_r2_dof_kernel_size		= 5.0f;

float		ps_r3_dyn_wet_surf_near		= 10.f;
float		ps_r3_dyn_wet_surf_far		= 30.f;
int			ps_r3_dyn_wet_surf_sm_res	= 256;

//water reflectrions
BOOL		ps_r3_water_refl_obj			= TRUE;
BOOL		ps_r3_water_refl_moon			= TRUE;
float		ps_r3_water_refl_env_power		= 0.8f;
float		ps_r3_water_refl_intensity		= 0.5f;
float		ps_r3_water_refl_range			= 1000.f;
float		ps_r3_water_refl_obj_itter		= 64;

//other
BOOL		ps_r3_adv_wet_hud				= TRUE;
BOOL		ps_r__debug_ssao				= FALSE;

//rain drops
BOOL		ps_r3_hud_rain_drops			= TRUE;
BOOL		ps_r3_hud_visor_effect			= TRUE;
BOOL		ps_r3_hud_visor_shadowing		= TRUE;

float		ps_r2_gloss_factor				= 4.f;

float		ps_r2_particle_distance			= 75.0f;

// Number of back buffers in swap chain: from 1 to 3
u32 ps_r3_backbuffers_count = 2;
xr_token qbbuffer_token [] =
{
	{ "2",							2 },
	{ "3",							3 },
	{ "4",							4 },
	{ 0,							0 }
};

// Test float exported to shaders for development
float		ps_r__test_exp_to_shaders_1		= 1.0f;
float		ps_r__test_exp_to_shaders_2		= 1.0f;
float		ps_r__test_exp_to_shaders_3		= 1.0f;
float		ps_r__test_exp_to_shaders_4		= 1.0f;

// SS sun shafts
BOOL		ps_r3_use_ss_sunshafts				= 0;
BOOL		ps_r3_use_new_bloom = 0;
BOOL		ps_r3_use_wet_reflections = 0;

float		ps_r2_prop_ss_radius = 1.56f;
float		ps_r2_prop_ss_sample_step_phase0 = 0.09f;
float		ps_r2_prop_ss_sample_step_phase1 = 0.07f;
float		ps_r2_prop_ss_blend = 0.066f;
float		ps_r2_prop_ss_intensity = 1.0f;

//Bloom
float ps_r2_bloom_treshold = 0;
float ps_r2_bloom_amount = 0;

extern xr_vector<shared_str> missingTexturesList_;
extern BOOL getMissingTextures_;


#include "../../xr_3da/xr_ioconsole.h"
#include "../../xr_3da/xr_ioc_cmd.h"
#include "../xrRenderDX10/StateManager/dx10SamplerStateCache.h"


//-----------------------------------------------------------------------
class CCC_tf_Aniso		: public CCC_Integer
{
public:
	void	apply	()	{
		if (0 == HW.pDevice)
			return;

		int	val = *value;	clamp(val,1,16);

		SSManager.SetMaxAnisotropy(val);

	}
	CCC_tf_Aniso(LPCSTR N, int*	v) : CCC_Integer(N, v, 1, 16)		{ };

	virtual void Execute	(LPCSTR args)
	{
		CCC_Integer::Execute	(args);
		apply					();
	}

	virtual void	Status	(TStatus& S)
	{	
		CCC_Integer::Status		(S);
		apply					();
	}
};

//thx to K.D.
class CCC_detail_radius		: public CCC_Integer
{
public:
	void	apply	()	{
		dm_current_size				= iFloor((float)ps_r__Detail_radius/4)*2;
		dm_current_cache1_line		= dm_current_size*2/4;		// assuming cache1_count = 4
		dm_current_cache_line		= dm_current_size+1+dm_current_size;
		dm_current_cache_size		= dm_current_cache_line*dm_current_cache_line;
		dm_current_fade				= float(2*dm_current_size)-.5f;
	}
	CCC_detail_radius(LPCSTR N, int* V, int _min=0, int _max=999) : CCC_Integer(N, V, _min, _max)		{ };
	virtual void Execute	(LPCSTR args)
	{
		CCC_Integer::Execute	(args);
		apply					();
	}
	virtual void	Status	(TStatus& S)
	{	
		CCC_Integer::Status		(S);
	}
};


class CCC_R2GM		: public CCC_Float
{
public:
	CCC_R2GM(LPCSTR N, float*	v) : CCC_Float(N, v, 0.f, 4.f) { *v = 0; };
	virtual void	Execute	(LPCSTR args)
	{
		if (0==xr_strcmp(args,"on"))	{
			ps_r2_ls_flags.set	(R2FLAG_GLOBALMATERIAL,TRUE);
		} else if (0==xr_strcmp(args,"off"))	{
			ps_r2_ls_flags.set	(R2FLAG_GLOBALMATERIAL,FALSE);
		} else {
			CCC_Float::Execute	(args);
			if (ps_r2_ls_flags.test(R2FLAG_GLOBALMATERIAL))	{
				static LPCSTR	name[4]	=	{ "oren", "blin", "phong", "metal" };
				float	mid		= *value	;
				int		m0		= iFloor(mid)	% 4;
				int		m1		= (m0+1)		% 4;
				float	frc		= mid - float(iFloor(mid));
				Msg		("* material set to [%s]-[%s], with lerp of [%f]",name[m0],name[m1],frc);
			}
		}
	}
};

class CCC_Screenshot : public IConsole_Command
{
public:
	CCC_Screenshot(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {

		string_path	name;	name[0]=0;
		sscanf		(args,"%s",	name);
		LPCSTR		image	= xr_strlen(name)?name:0;
		::Render->Screenshot(IRender_interface::SM_NORMAL,image);
	}
};

class CCC_ModelPoolStat : public IConsole_Command
{
public:
	CCC_ModelPoolStat(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		RImplementation.Models->dump();
	}
};

class	CCC_SSAO_Mode		: public CCC_Token
{
public:
	CCC_SSAO_Mode(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N,V,T)	{}	;

	virtual void	Execute	(LPCSTR args)	{
		CCC_Token::Execute	(args);
				
		switch	(*value)
		{
			case 0:
			{
				ps_r_ssao = 0;
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 0);
				break;
			}
			case 1:
			{
				if (ps_r_ssao==0)
				{
					ps_r_ssao = 1;
				}
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HALF_DATA, 0);
				break;
			}
			case 2:
			{
				if (ps_r_ssao==0)
				{
					ps_r_ssao = 1;
				}
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 1);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_OPT_DATA, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HALF_DATA, 0);
				break;
			}
			case 3:
			{
				if (ps_r_ssao==0)
				{
					ps_r_ssao = 1;
				}
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 1);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_OPT_DATA, 1);
				break;
			}
			case 4:
			{
				if (ps_r_ssao == 0)
				{
					ps_r_ssao = 1;
				}
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_SSDO, 1);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HBAO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_HDAO, 0);
				ps_r2_ls_flags_ext.set(R2FLAGEXT_SSAO_OPT_DATA, 1);
				break;
			}
		}
	}
};

class	CCC_AA_Mode : public CCC_Token
{
public:
	CCC_AA_Mode(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T) {};

	virtual void	Execute(LPCSTR args) {
		CCC_Token::Execute(args);

		switch (*value)
		{
		case 0:
		{
			ps_r_aa = 0;
			ps_r2_ls_flags_ext.set(R2FLAGEXT_AA_FXAA, 0);
			ps_r2_ls_flags_ext.set(R2FLAGEXT_AA_SMAA, 0);
			break;
		}
		case 1:
		{

			ps_r_aa = 0;
			ps_r2_ls_flags_ext.set(R2FLAGEXT_AA_FXAA, 1);
			ps_r2_ls_flags_ext.set(R2FLAGEXT_AA_SMAA, 0);
			break;
		}
		case 2:
		{
			if (ps_r_aa == 0)
			{
				ps_r_aa = 1;
			}
			ps_r2_ls_flags_ext.set(R2FLAGEXT_AA_FXAA, 0);
			ps_r2_ls_flags_ext.set(R2FLAGEXT_AA_SMAA, 1);
			break;
		}
		}
	}
};

//-----------------------------------------------------------------------
class	CCC_Preset		: public CCC_Token
{
public:
	CCC_Preset(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N,V,T)	{}	;

	virtual void	Execute	(LPCSTR args)	{
		CCC_Token::Execute	(args);
		string_path		_cfg;
		string_path		cmd;
		
		switch	(*value)	{
			case 0:		xr_strcpy(_cfg, "rspec_minimum.ltx");	break;
			case 1:		xr_strcpy(_cfg, "rspec_low.ltx");		break;
			case 2:		xr_strcpy(_cfg, "rspec_default.ltx");	break;
			case 3:		xr_strcpy(_cfg, "rspec_high.ltx");		break;
			case 4:		xr_strcpy(_cfg, "rspec_extreme.ltx");	break;
		}
		FS.update_path			(_cfg,"$game_config$",_cfg);
		strconcat				(sizeof(cmd),cmd,"cfg_load", " ", _cfg);
		Console->Execute		(cmd);
	}
};


class CCC_memory_stats : public IConsole_Command
{
protected	:

public		:

	CCC_memory_stats(LPCSTR N) :	IConsole_Command(N)	{ bEmptyArgsHandled = true; };

	virtual void	Execute	(LPCSTR args)
	{
		u32 m_base = 0;
		u32 c_base = 0;
		u32 m_lmaps = 0; 
		u32 c_lmaps = 0;

		dxRenderDeviceRender::Instance().ResourcesGetMemoryUsage( m_base, c_base, m_lmaps, c_lmaps );

		Msg		("memory usage  mb \t \t video    \t managed      \t system \n" );

		float vb_video		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_vertex][D3DPOOL_DEFAULT]/1024/1024;
		float vb_managed	= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_vertex][D3DPOOL_MANAGED]/1024/1024;
		float vb_system		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_vertex][D3DPOOL_SYSTEMMEM]/1024/1024;
		Msg		("vertex buffer      \t \t %f \t %f \t %f ",	vb_video, vb_managed, vb_system);

		float ib_video		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_index][D3DPOOL_DEFAULT]/1024/1024; 
		float ib_managed	= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_index][D3DPOOL_MANAGED]/1024/1024; 
		float ib_system		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_index][D3DPOOL_SYSTEMMEM]/1024/1024; 
		Msg		("index buffer      \t \t %f \t %f \t %f ",	ib_video, ib_managed, ib_system);
		
		float textures_managed = (float)(m_base+m_lmaps)/1024/1024;
		Msg		("textures          \t \t %f \t %f \t %f ",	0.f, textures_managed, 0.f);

		float rt_video		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_rtarget][D3DPOOL_DEFAULT]/1024/1024;
		float rt_managed	= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_rtarget][D3DPOOL_MANAGED]/1024/1024;
		float rt_system		= (float)HW.stats_manager.memory_usage_summary[enum_stats_buffer_type_rtarget][D3DPOOL_SYSTEMMEM]/1024/1024;
		Msg		("R-Targets         \t \t %f \t %f \t %f ",	rt_video, rt_managed, rt_system);									

		Msg		("\nTotal             \t \t %f \t %f \t %f ",	vb_video+ib_video+rt_video,
																textures_managed + vb_managed+ib_managed+rt_managed,
																vb_system+ib_system+rt_system);
	}

};

struct DumpVisualsForPrefetching : public IConsole_Command {
	DumpVisualsForPrefetching(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args) {
		RImplementation.Models->ReportVisualsForPrefetching();
	}
};

class CCC_DofFar : public CCC_Float
{
public:
	CCC_DofFar(LPCSTR N, float* V, float _min=0.0f, float _max=10000.0f) 
		: CCC_Float( N, V, _min, _max){}

	virtual void Execute(LPCSTR args) 
	{
		float v = float(atof(args));

		if (v<ps_r2_dof.y+0.1f)
		{
			char	pBuf[256];
			_snprintf( pBuf, sizeof(pBuf)/sizeof(pBuf[0]), "float value greater or equal to r2_dof_focus+0.1");
			Msg("~ Invalid syntax in call to '%s'",cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r2_dof_focus");
		}
		else
		{
			CCC_Float::Execute(args);
			if(g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r2_dof);
		}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void	Save	(IWriter *F)	{;}
};

class CCC_DofNear : public CCC_Float
{
public:
	CCC_DofNear(LPCSTR N, float* V, float _min=0.0f, float _max=10000.0f) 
		: CCC_Float( N, V, _min, _max){}

	virtual void Execute(LPCSTR args) 
	{
		float v = float(atof(args));

		if (v>ps_r2_dof.y-0.1f)
		{
			char	pBuf[256];
			_snprintf( pBuf, sizeof(pBuf)/sizeof(pBuf[0]), "float value less or equal to r2_dof_focus-0.1");
			Msg("~ Invalid syntax in call to '%s'",cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r2_dof_focus");
		}
		else
		{
			CCC_Float::Execute(args);
			if(g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r2_dof);
		}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void	Save	(IWriter *F)	{;}
};

class CCC_DofFocus : public CCC_Float
{
public:
	CCC_DofFocus(LPCSTR N, float* V, float _min=0.0f, float _max=10000.0f) 
		: CCC_Float( N, V, _min, _max){}

	virtual void Execute(LPCSTR args) 
	{
		float v = float(atof(args));

		if (v>ps_r2_dof.z-0.1f)
		{
			char	pBuf[256];
			_snprintf( pBuf, sizeof(pBuf)/sizeof(pBuf[0]), "float value less or equal to r2_dof_far-0.1");
			Msg("~ Invalid syntax in call to '%s'",cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r2_dof_far");
		}
		else if (v<ps_r2_dof.x+0.1f)
		{
			char	pBuf[256];
			_snprintf( pBuf, sizeof(pBuf)/sizeof(pBuf[0]), "float value greater or equal to r2_dof_far-0.1");
			Msg("~ Invalid syntax in call to '%s'",cName);
			Msg("~ Valid arguments: %s", pBuf);
			Console->Execute("r2_dof_near");
		}
		else{
			CCC_Float::Execute(args);
			if(g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r2_dof);
			}
	}

	//	CCC_Dof should save all data as well as load from config
	virtual void	Save	(IWriter *F)	{;}
};

class CCC_Dof : public CCC_Vector3
{
public:
	CCC_Dof(LPCSTR N, Fvector* V, const Fvector _min, const Fvector _max) : 
	  CCC_Vector3(N, V, _min, _max) {;}

	virtual void	Execute	(LPCSTR args)
	{
		Fvector v;
		if (3!=sscanf(args,"%f,%f,%f",&v.x,&v.y,&v.z))	
			InvalidSyntax(); 
		else if ( (v.x > v.y-0.1f) || (v.z < v.y+0.1f))
		{
			InvalidSyntax();
			Msg("x <= y - 0.1");
			Msg("y <= z - 0.1");
		}
		else
		{
			CCC_Vector3::Execute(args);
			if(g_pGamePersistent)
				g_pGamePersistent->SetBaseDof(ps_r2_dof);
		}
	}
	virtual void	Status	(TStatus& S)
	{	
		xr_sprintf	(S,"%f,%f,%f",value->x,value->y,value->z);
	}
	virtual void	Info	(TInfo& I)
	{	
		xr_sprintf(I,"vector3 in range [%f,%f,%f]-[%f,%f,%f]",min.x,min.y,min.z,max.x,max.y,max.z);
	}

};

class CCC_DumpResources : public IConsole_Command
{
public:
	CCC_DumpResources(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		RImplementation.Models->dump();
		dxRenderDeviceRender::Instance().Resources->Dump(false);
	}
};

class CCC_MissingTextures : public IConsole_Command
{
public:
	CCC_MissingTextures(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Msg("^ Dumping registered texture absences");

		for (u32 i = 0; i < missingTexturesList_.size(); i++)
		{
			Msg("%s", missingTexturesList_[i].c_str() ? missingTexturesList_[i].c_str() : "");
		}
	}
};

class CCC_SwingFactorsDetailsManager : public CCC_FloatArray
{
private:
	int id;
	float data[5];
public:
	CCC_SwingFactorsDetailsManager(LPCSTR S, int _id, float _min = 0, float _max = 1) : CCC_FloatArray(S, data, 5, _min, _max), id(_id) { };
	virtual void Execute(LPCSTR args) 
	{
		CCC_FloatArray::Execute(args);
		RImplementation.Details->swing_desc[id].rot1 = values[0];
		RImplementation.Details->swing_desc[id].rot2 = values[1];
		RImplementation.Details->swing_desc[id].amp1 = values[2];
		RImplementation.Details->swing_desc[id].amp2 = values[3];
		RImplementation.Details->swing_desc[id].speed = values[4];
	}
};

class CCC_R_MT_VIS_SWITCH : public IConsole_Command
{
public:
	CCC_R_MT_VIS_SWITCH(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = FALSE;
	}
	virtual void Execute(LPCSTR args)
	{
		string256 str;
		str[0] = 0;
		sscanf(args, "%s", str);

		if (!xr_strcmp(str, "1"))
		{
			ps_r_mt_flags.set(R_FLAG_MT_CALC_LGHTS_VIS_STRUCT, 1);
			ps_r_mt_flags.set(R_FLAG_MT_CALC_PRIOR0_VIS_STRUCT, 1);
			ps_r_mt_flags.set(R_FLAG_MT_CALC_PRIOR1_VIS_STRUCT, 1);
			ps_r_mt_flags.set(R_FLAG_MT_CALC_RAIN_VIS_STRUCT, 1);
			ps_r_mt_flags.set(R_FLAG_MT_CALC_SUN_VIS_STRUCT, 1);
		}
		else if(!xr_strcmp(str, "0"))
		{
			ps_r_mt_flags.set(R_FLAG_MT_CALC_LGHTS_VIS_STRUCT, 0);
			ps_r_mt_flags.set(R_FLAG_MT_CALC_PRIOR0_VIS_STRUCT, 0);
			ps_r_mt_flags.set(R_FLAG_MT_CALC_PRIOR1_VIS_STRUCT, 0);
			ps_r_mt_flags.set(R_FLAG_MT_CALC_RAIN_VIS_STRUCT, 0);
			ps_r_mt_flags.set(R_FLAG_MT_CALC_SUN_VIS_STRUCT, 0);
		}
	};
	virtual void Info(TInfo& I)
	{
		xr_strcpy(I, "--- 1 = on, 0 = off");
	}
};

struct CCC_XrRenderSwitchAllMT : public IConsole_Command {
	CCC_XrRenderSwitchAllMT(LPCSTR N) : IConsole_Command(N) {};

	virtual void Execute(LPCSTR args)
	{
		string256 value;
		sscanf(args, "%s", value);

		int res = std::stoi(value);

		if (res == 0 || res == 1)
		{
			if (res == 0)
			{
				Console->Execute("mt_vis_structure 0");
				ps_r_mt_flags.set(R_FLAG_MT_CALC_HOM_DETAILS, 0);
				ps_r_mt_flags.set(R_FLAG_MT_BONES_PRECALC, 0);
			}
			else
			{
				Console->Execute("mt_vis_structure 1");
				ps_r_mt_flags.set(R_FLAG_MT_CALC_HOM_DETAILS, 1);
				ps_r_mt_flags.set(R_FLAG_MT_BONES_PRECALC, 1);
			}
		}
		else
			Msg("^ Valid arguments are 0 or 1");
	}
};

class CCC_RM_Test_Ram_Capacity : public IConsole_Command
{
public:
	CCC_RM_Test_Ram_Capacity(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		if (args && args[0])
		{
			string128 amount;

			_GetItem(args, 0, amount);
			int megabytes = atoi(amount);

			dxRenderDeviceRender::Instance().Resources->TestRAM(megabytes);
		}
		else // default
			dxRenderDeviceRender::Instance().Resources->TestRAM();
	}
};

//	Allow real-time fog config reload
#ifdef	DEBUG

#include "../xrRenderDX10/3DFluid/dx103DFluidManager.h"

class CCC_Fog_Reload : public IConsole_Command
{
public:
	CCC_Fog_Reload(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) 
	{
		FluidManager.UpdateProfiles();
	}
};
#endif


// --- Not valuable static and dynamic geometry cut off

float		o_optimize_static_l1_dist = O_S_L1_D_MED;
float		o_optimize_static_l1_size = O_S_L1_S_MED;
float		o_optimize_static_l2_dist = O_S_L2_D_MED;
float		o_optimize_static_l2_size = O_S_L2_S_MED;
float		o_optimize_static_l3_dist = O_S_L3_D_MED;
float		o_optimize_static_l3_size = O_S_L3_S_MED;
float		o_optimize_static_l4_dist = O_S_L4_D_MED;
float		o_optimize_static_l4_size = O_S_L4_S_MED;
float		o_optimize_static_l5_dist = O_S_L5_D_MED;
float		o_optimize_static_l5_size = O_S_L5_S_MED;

class	CCC_OS_Token : public CCC_Token
{
public:
	CCC_OS_Token(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T)	{};

	virtual void Execute(LPCSTR args)
	{
		CCC_Token::Execute(args);

		ps_r_optimize_static = *value;

		switch (*value)
		{
		case 0:
		{
			break;
		}
		case 1:
		{
			o_optimize_static_l1_dist = O_S_L1_D_LOW;
			o_optimize_static_l1_size = O_S_L1_S_LOW;
			o_optimize_static_l2_dist = O_S_L2_D_LOW;
			o_optimize_static_l2_size = O_S_L2_S_LOW;
			o_optimize_static_l3_dist = O_S_L3_D_LOW;
			o_optimize_static_l3_size = O_S_L3_S_LOW;
			o_optimize_static_l4_dist = O_S_L4_D_LOW;
			o_optimize_static_l4_size = O_S_L4_S_LOW;
			o_optimize_static_l5_dist = O_S_L5_D_LOW;
			o_optimize_static_l5_size = O_S_L5_S_LOW;

			break;
		}
		case 2:
		{
			o_optimize_static_l1_dist = O_S_L1_D_MED;
			o_optimize_static_l1_size = O_S_L1_S_MED;
			o_optimize_static_l2_dist = O_S_L2_D_MED;
			o_optimize_static_l2_size = O_S_L2_S_MED;
			o_optimize_static_l3_dist = O_S_L3_D_MED;
			o_optimize_static_l3_size = O_S_L3_S_MED;
			o_optimize_static_l4_dist = O_S_L4_D_MED;
			o_optimize_static_l4_size = O_S_L4_S_MED;
			o_optimize_static_l5_dist = O_S_L5_D_MED;
			o_optimize_static_l5_size = O_S_L5_S_MED;

			break;
		}
		case 3:
		{
			o_optimize_static_l1_dist = O_S_L1_D_HII;
			o_optimize_static_l1_size = O_S_L1_S_HII;
			o_optimize_static_l2_dist = O_S_L2_D_HII;
			o_optimize_static_l2_size = O_S_L2_S_HII;
			o_optimize_static_l3_dist = O_S_L3_D_HII;
			o_optimize_static_l3_size = O_S_L3_S_HII;
			o_optimize_static_l4_dist = O_S_L4_D_HII;
			o_optimize_static_l4_size = O_S_L4_S_HII;
			o_optimize_static_l5_dist = O_S_L5_D_HII;
			o_optimize_static_l5_size = O_S_L5_S_HII;

			break;
		}
		}
	}
};


float		o_optimize_dynamic_l1_dist = O_D_L1_D_MED;
float		o_optimize_dynamic_l1_size = O_D_L1_S_MED;
float		o_optimize_dynamic_l2_dist = O_D_L2_D_MED;
float		o_optimize_dynamic_l2_size = O_D_L2_S_MED;
float		o_optimize_dynamic_l3_dist = O_D_L3_D_MED;
float		o_optimize_dynamic_l3_size = O_D_L3_S_MED;

class	CCC_OD_Token : public CCC_Token
{
public:
	CCC_OD_Token(LPCSTR N, u32* V, xr_token* T) : CCC_Token(N, V, T)	{};

	virtual void Execute(LPCSTR args)
	{
		CCC_Token::Execute(args);

		ps_r_optimize_dynamic = *value;

		switch (*value)
		{
		case 0:
		{
			break;
		}
		case 1:
		{
			o_optimize_dynamic_l1_dist = O_D_L1_D_LOW;
			o_optimize_dynamic_l1_size = O_D_L1_S_LOW;
			o_optimize_dynamic_l2_dist = O_D_L2_D_LOW;
			o_optimize_dynamic_l2_size = O_D_L2_S_LOW;
			o_optimize_dynamic_l3_dist = O_D_L3_D_LOW;
			o_optimize_dynamic_l3_size = O_D_L3_S_LOW;

			break;
		}
		case 2:
		{
			o_optimize_dynamic_l1_dist = O_D_L1_D_MED;
			o_optimize_dynamic_l1_size = O_D_L1_S_MED;
			o_optimize_dynamic_l2_dist = O_D_L2_D_MED;
			o_optimize_dynamic_l2_size = O_D_L2_S_MED;
			o_optimize_dynamic_l3_dist = O_D_L3_D_MED;
			o_optimize_dynamic_l3_size = O_D_L3_S_MED;

			break;
		}
		case 3:
		{
			o_optimize_dynamic_l1_dist = O_D_L1_D_HII;
			o_optimize_dynamic_l1_size = O_D_L1_S_HII;
			o_optimize_dynamic_l2_dist = O_D_L2_D_HII;
			o_optimize_dynamic_l2_size = O_D_L2_S_HII;
			o_optimize_dynamic_l3_dist = O_D_L3_D_HII;
			o_optimize_dynamic_l3_size = O_D_L3_S_HII;

			break;
		}
		}
	}
};


//-----------------------------------------------------------------------
void xrRender_initconsole()
{
	CMD3(CCC_Preset,	"_preset",				&ps_Preset,	qpreset_token);

	CMD4(CCC_Integer,	"rs_skeleton_update",	&psSkeletonUpdate,		2,		128);


	CMD4(CCC_Integer,			"monitor_missing_textures", &getMissingTextures_, 0,	1);
	CMD1(CCC_MissingTextures,	"dump_missing_textures");

	CMD4(CCC_Float,		"r__dtex_range",		&r__dtex_range,		5,		175);

	CMD1(CCC_Screenshot, "screenshot");

	CMD4(CCC_Float,		"r__ssa_glod_start",	&ps_r__GLOD_ssa_start,		128,	512);
	CMD4(CCC_Float,		"r__ssa_glod_end",		&ps_r__GLOD_ssa_end,		16,		96);

	CMD4(CCC_Float,		"r__wallmark_ttl",		&ps_r__WallmarkTTL,			1.0f,	10.f * 60.f);
	CMD4(CCC_Float,		"r__s_wallmark_discard",&ps_r__SWallmarkDiscard,	0.01f,	10.f);
	CMD4(CCC_Float,		"r__d_wallmark_discard",&ps_r__DWallmarkDiscard,	1.f,	50.f);

	Fvector	tw_min, tw_max;

	CMD4(CCC_Float,		"r__lod_sprite_dist_f",		&ps_r__geomLodSpriteDistF_,		0.1f,	 3.0f);
	CMD4(CCC_Float,		"r__geom_quality_dist_f",	&ps_r__geomLodDistF_,			0.1f,	 1.0f);
	CMD4(CCC_Float,		"r__geom_discard_dist_f",	&ps_r__geomDiscardDistF_,		0.1f,	 1.0f);
	CMD4(CCC_Float,		"r__dtexture_dist_f",		&ps_r__geomDTextureDistF_,		0.1f,	 3.0f);
	CMD4(CCC_Float,		"r__ntexture_dist_f",		&ps_r__geomNTextureDistF_,		0.1f,	 3.0f);

	CMD4(CCC_detail_radius,	"r__detail_radius",		&ps_r__Detail_radius,		45,		150);
	CMD4(CCC_Float,			"r__detail_density",	&ps_r__Detail_density,		.18f,	0.45f);
	CMD4(CCC_Float,			"r__detail_l_ambient",	&ps_r__Detail_l_ambient,	.5f,	.95f);
	CMD4(CCC_Float,			"r__detail_l_aniso",	&ps_r__Detail_l_aniso,		.1f,	.5f);

	CMD4(CCC_Float,			"r__details_with_shadows_dist",		&ps_details_with_shadows_dist,		16.f,	128.f);

	CMD2(CCC_tf_Aniso,	"r__tf_aniso",			&ps_r__tf_Anisotropic);
	CMD4(CCC_Integer,	"r__glows_per_frame",	&ps_GlowsPerFrame,			2,		32);
	CMD3(CCC_Mask,		"r__glows_enable",		&ps_r2_ls_flags_ext,		R2FLAGEXT_GLOWS_ENABLE);

	CMD3(CCC_Mask,		"r__shaders_cache",		&ps_r2_ls_flags,			R_FLAG_SHADERS_CACHE);

	CMD4(CCC_Float,		"r1_pps_u",				&ps_r1_pps_u,				-1.f,	+1.f	);
	CMD4(CCC_Float,		"r1_pps_v",				&ps_r1_pps_v,				-1.f,	+1.f	);
	CMD3(CCC_Mask,		"r__wireframe",			&ps_r2_ls_flags_ext,		R2FLAGEXT_WIREFRAME);

	CMD4(CCC_Float,		"r2_ssa_lod_a",			&ps_r2_ssaLOD_A,			16,		96);
	CMD4(CCC_Float,		"r2_ssa_lod_b",			&ps_r2_ssaLOD_B,			32,		64);

//	CMD4(CCC_Integer,	"r_vb_size",			&rsDVB_Size,		32,		4096);
//	CMD4(CCC_Integer,	"r_ib_size",			&rsDIB_Size,		32,		4096);

	// R2-specific
	CMD2(CCC_R2GM,		"r2em",					&ps_r2_gmaterial);

	CMD3(CCC_Mask,		"r2_tonemap",			&ps_r2_ls_flags,			R2FLAG_TONEMAP);
	// Яркость относительно lowlum параметра
	CMD4(CCC_Float,		"r2_tonemap_middlegray",&ps_r2_tonemap_middlegray,	0.0f,		2.0f);
	// Cкорость адаптация зрения
	CMD4(CCC_Float,		"r2_tonemap_adaptation",&ps_r2_tonemap_adaptation,	0.01f,		10.0f);
	// Контраст между темным и ярким пикселем. 1.0 - контраст нулевой, HDR не видно 
	CMD4(CCC_Float,		"r2_tonemap_lowlum",	&ps_r2_tonemap_low_lum,		0.0001f,	1.0f);
	// Насколько сильно будет проявляться адаптация зрения относительно оригинального изображения
	CMD4(CCC_Float,		"r2_tonemap_amount",	&ps_r2_tonemap_amount,		0.0000f,	1.0f);
	CMD4(CCC_Float,		"r2_tonemap_white",		&ps_r2_tonemap_white,		1.f,		3.0f);

	CMD4(CCC_Float,		"r2_ls_bloom_kernel_scale",		&ps_r2_ls_bloom_kernel_scale,	0.5f,	2.f);
	CMD4(CCC_Float,		"r2_ls_bloom_kernel_g",			&ps_r2_ls_bloom_kernel_g,		1.f,	7.f);
	CMD4(CCC_Float,		"r2_ls_bloom_kernel_b",			&ps_r2_ls_bloom_kernel_b,		0.01f,	1.f);
	CMD4(CCC_Float,		"r2_ls_bloom_threshold",		&ps_r2_ls_bloom_threshold,		0.f,	1.f);
	CMD4(CCC_Float,		"r2_ls_bloom_speed",			&ps_r2_ls_bloom_speed,			0.f,	100.f);
	CMD3(CCC_Mask,		"r2_ls_bloom_fast",				&ps_r2_ls_flags,				R2FLAG_FASTBLOOM);

	CMD4(CCC_Float,		"r2_ls_squality",		&ps_r2_ls_squality,			.5f,	1.f);
	CMD4(CCC_Float,		"r2_particle_distance",	&ps_r2_particle_distance,	35.0f,	1000.f);

	CMD3(CCC_Mask,		"r2_allow_r1_lights",	&ps_r2_ls_flags,			R2FLAG_R1LIGHTS);
	CMD4(CCC_Integer,	"r__dynamic_lights",	&r__dynamicLights_,			0,		1);

	CMD4(CCC_Integer,	"r__light_details_shadows",			&ps_light_details_shadow,			0,		1);
	CMD4(CCC_Float,		"r__light_ds_max_dist_from_light",	&ps_light_ds_max_dist_from_light,	3.f,	50.f);
	CMD4(CCC_Float,		"r__light_ds_max_dist_from_cam",	&ps_light_ds_max_dist_from_cam,		16.f,	128.f);
	CMD4(CCC_Integer,	"r__light_ds_max_lights",			&ps_light_ds_max_lights,			16,		64);

	CMD4(CCC_Float,		"r2_gloss_factor",		&ps_r2_gloss_factor,		.0f,	10.f);

	CMD3(CCC_Mask,		"r2_use_nvdbt",			&ps_r2_ls_flags,			R2FLAG_USE_NVDBT);

	CMD3(CCC_Mask,		"mt_hom_and_details",	&ps_r_mt_flags,				R_FLAG_MT_CALC_HOM_DETAILS);
	CMD3(CCC_Mask,		"mt_sun_vis_struct",	&ps_r_mt_flags,				R_FLAG_MT_CALC_SUN_VIS_STRUCT);
	CMD3(CCC_Mask,		"mt_lights_vis_struct",	&ps_r_mt_flags,				R_FLAG_MT_CALC_LGHTS_VIS_STRUCT);
	CMD3(CCC_Mask,		"mt_rain_vis_struct",	&ps_r_mt_flags,				R_FLAG_MT_CALC_RAIN_VIS_STRUCT);
	CMD3(CCC_Mask,		"mt_prior0_vis_struct",	&ps_r_mt_flags,				R_FLAG_MT_CALC_PRIOR0_VIS_STRUCT);
	CMD3(CCC_Mask,		"mt_prior1_vis_struct",	&ps_r_mt_flags,				R_FLAG_MT_CALC_PRIOR1_VIS_STRUCT);
	CMD3(CCC_Mask,		"mt_mesh_bones_precalc",&ps_r_mt_flags,				R_FLAG_MT_BONES_PRECALC);

	CMD1(CCC_R_MT_VIS_SWITCH, "mt_vis_structure");
	CMD1(CCC_XrRenderSwitchAllMT, "mt_xrrender");

	if (CApplication::isDeveloperMode)
		CMD3(CCC_Mask,		"r2_sun",				&ps_r2_ls_flags,			R2FLAG_SUN);

	CMD3(CCC_Mask,		"r2_exp_donttest_uns",	&ps_r2_ls_flags,			R2FLAG_EXP_DONT_TEST_UNSHADOWED);
	CMD3(CCC_Mask,		"r2_exp_donttest_shad",	&ps_r2_ls_flags,			R2FLAG_EXP_DONT_TEST_SHADOWED);

	CMD3(CCC_Mask,		"r2_sun_details_shadows",		&ps_r2_ls_flags,				R2FLAG_DETAILS_SUN_SHADOWS);
	CMD4(CCC_Float,		"r2_sun_details_shadows_dist",	&ps_r__Details_sun_sh_dist,		5.f,	32.f);
	
	CMD4(CCC_Float,		"r2_sun_tsm_bias",		&ps_r2_sun_tsm_bias,		-0.5,	+0.5);
	CMD4(CCC_Float,		"r2_sun_near",			&ps_r2_sun_near,			1.f,	150.f);

	CMD4(CCC_Float,		"r2_sun_depth_far_scale",	&ps_r2_sun_depth_far_scale,		0.5,	1.5);
	CMD4(CCC_Float,		"r2_sun_depth_far_bias",	&ps_r2_sun_depth_far_bias,		-0.5,	+0.5);
	CMD4(CCC_Float,		"r2_sun_depth_near_scale",	&ps_r2_sun_depth_near_scale,	0.5,	1.5);
	CMD4(CCC_Float,		"r2_sun_depth_near_bias",	&ps_r2_sun_depth_near_bias,		-0.5,	+0.5);

	CMD4(CCC_Float,		"r2_sun_lumscale",		&ps_r2_sun_lumscale,		0.0,	+5.0);
	CMD4(CCC_Float,		"r2_sun_lumscale_hemi",	&ps_r2_sun_lumscale_hemi,	0.0,	+2.0);
	CMD4(CCC_Float,		"r2_sun_lumscale_amb",	&ps_r2_sun_lumscale_amb,	0.0,	+3.0);

	CMD4(CCC_Float,		"r2_sun_shadows_near_casc",		&ps_r2_sun_shadows_near_casc,		8.f,	32.f);
	CMD4(CCC_Float,		"r2_sun_shadows_midl_casc",		&ps_r2_sun_shadows_midl_casc,		32.f,	128.f);
	CMD4(CCC_Float,		"r2_sun_shadows_far_casc",		&ps_r2_sun_shadows_far_casc,		128.f,	1024.f);

	CMD3(CCC_Mask,		"r2_aa",				&ps_r2_ls_flags,			R2FLAG_AA);
	CMD4(CCC_Float,		"r2_aa_kernel",			&ps_r2_aa_kernel,			0.3f,	0.7f);
	CMD3(CCC_Mask,		"r2_mblur_enable",		&ps_r2_ls_flags,			R2FLAG_MBLUR);
	CMD4(CCC_Float,		"r2_mblur",				&ps_r2_mblur,				0.15f,	0.7f);

	CMD4(CCC_Integer,	"r3_new_bloom",			&ps_r3_use_new_bloom,		0, 1);
	CMD4(CCC_Integer,	"r3_wet_reflections",    &ps_r3_use_wet_reflections, 0, 1);

	CMD3(CCC_Mask,		"r2_gi",				&ps_r2_ls_flags,			R2FLAG_GI);
	CMD4(CCC_Float,		"r2_gi_clip",			&ps_r2_GI_clip,				EPS,	0.1f);
	CMD4(CCC_Integer,	"r2_gi_depth",			&ps_r2_GI_depth,			1,		5);
	CMD4(CCC_Integer,	"r2_gi_photons",		&ps_r2_GI_photons,			8,		256);
	CMD4(CCC_Float,		"r2_gi_refl",			&ps_r2_GI_refl,				EPS_L,	0.99f);

	CMD4(CCC_Integer,	"r2_wait_sleep",				&ps_r2_wait_sleep,			0,		1);
	CMD4(CCC_Float,		"r_max_wait_gpu_occ_calc_ms",	&maxWaitForOccCalc_,		0.f,	10.f);
	CMD4(CCC_Integer,	"r_cpu_wait_gpu",				&ps_r_cpu_wait_gpu,			0,		1);
	CMD3(CCC_Mask,		"r_optimize_light_gpu_occ_calc",&ps_r2_ls_flags,	R2FLAG_GPU_OCC_OPTIMIZATION);

	CMD4(CCC_Integer,	"r2_dhemi_count",		&ps_r2_dhemi_count,			4,		25);
	CMD4(CCC_Float,		"r2_dhemi_sky_scale",	&ps_r2_dhemi_sky_scale,		0.0f,	100.f);
	CMD4(CCC_Float,		"r2_dhemi_light_scale",	&ps_r2_dhemi_light_scale,	0,		100.f);
	CMD4(CCC_Float,		"r2_dhemi_light_flow",	&ps_r2_dhemi_light_flow,	0,		1.f);
	CMD4(CCC_Float,		"r2_dhemi_smooth",		&ps_r2_lt_smooth,			0.f,	10.f);

	CMD3(CCC_Mask,		"rs_hom_depth_draw",			&ps_r2_ls_flags_ext,		R_FLAGEXT_HOM_DEPTH_DRAW);
	CMD3(CCC_Mask,		"r__use_hom_for_main",			&ps_r_misc_flags,			R_MISC_USE_HOM_PRIOR_0);
	CMD3(CCC_Mask,		"r__use_hom_for_forward",		&ps_r_misc_flags,			R_MISC_USE_HOM_FORWARD);
	CMD3(CCC_Mask,		"r__use_hom_for_lights_sh",		&ps_r_misc_flags,			R_MISC_USE_HOM_LIGHT);
	CMD3(CCC_Mask,		"r__use_hom_for_sun_sh",		&ps_r_misc_flags,			R_MISC_USE_HOM_SUN);

	if (CApplication::isDeveloperMode)
		CMD3(CCC_Mask,		"r__use_sector_for_static",		&ps_r_misc_flags,			R_MISC_USE_SECTORS_FOR_STATIC);
	
	CMD3(CCC_Mask,		"r2_shadow_cascede_zcul",&ps_r2_ls_flags_ext,		R2FLAGEXT_SUN_ZCULLING);

	CMD4(CCC_Float,		"r2_ls_depth_scale",	&ps_r2_ls_depth_scale,		0.5,	1.5);
	CMD4(CCC_Float,		"r2_ls_depth_bias",		&ps_r2_ls_depth_bias,		-0.5,	+0.5);

	CMD4(CCC_Float,		"r2_parallax_h",		&ps_r2_df_parallax_h,		.0f,	0.5f);
	CMD4(CCC_Float,		"r2_parallax_range",	&ps_r2_df_parallax_range,	0.0f,	175.0f);

	//Separate values for steep parallax
	CMD4(CCC_Float,		"r2_steep_parallax_h",				&ps_r2_steep_parallax_h,				.005f,	0.05f);
	CMD4(CCC_Float,		"r2_steep_parallax_distance",		&ps_r2_steep_parallax_distance,			4.0f,	100.0f);
	CMD4(CCC_Float,		"r2_steep_parallax_samples",		&ps_r2_steep_parallax_samples,			5.0f,	100.0f);
	CMD4(CCC_Float,		"r2_steep_parallax_samples_min",	&ps_r2_steep_parallax_samples_min,		5.0f,	100.0f);

	CMD4(CCC_Float,		"r2_slight_fade",		&ps_r2_slight_fade,			.2f,	1.f);

	tw_min.set			(0, 0, 0);	tw_max.set	(1, 1, 1);
	CMD4(CCC_Vector3,	"r2_aa_break",			&ps_r2_aa_barier,			tw_min, tw_max);

	tw_min.set			(0, 0, 0);	tw_max.set	(1, 1, 1);
	CMD4(CCC_Vector3,	"r2_aa_weight",			&ps_r2_aa_weight,			tw_min, tw_max);

	//	Igor: Depth of field
	tw_min.set(-10000, -10000,0);
	tw_max.set(10000, 10000, 10000);

	CMD4( CCC_Dof,		"r2_dof",						&ps_r2_dof,					tw_min,		tw_max);
	CMD4( CCC_DofNear,	"r2_dof_near",					&ps_r2_dof.x,				tw_min.x,	tw_max.x);
	CMD4( CCC_DofFocus,	"r2_dof_focus",					&ps_r2_dof.y,				tw_min.y,	tw_max.y);
	CMD4( CCC_DofFar,	"r2_dof_far",					&ps_r2_dof.z,				tw_min.z,	tw_max.z);

	CMD4(CCC_Float,		"r2_dof_kernel",				&ps_r2_dof_kernel_size,		.0f,		10.f);
	CMD4(CCC_Float,		"r2_dof_sky",					&ps_r2_dof_sky,				-10000.f,	10000.f);
	CMD3(CCC_Mask,		"r2_dof_enable",				&ps_r2_ls_flags,			R2FLAG_DOF);
	CMD3(CCC_Mask,		"r2_dof_weather",				&ps_r2_ls_flags_ext,		R2FLAGEXT_DOF_WEATHER); // skyloader: use dof values from weather or console
	
	CMD3(CCC_Mask,		"r2_volumetric_lights",			&ps_r2_ls_flags,			R2FLAG_VOLUMETRIC_LIGHTS); //skyloader: not used in LA
	CMD3(CCC_Token,		"r2_sun_shafts",				&ps_r_sun_shafts,			qsun_shafts_token);
	CMD3(CCC_Token,		"r2_smap_size",					&ps_r_smap_size,			qsmap_size_token);
	CMD3(CCC_SSAO_Mode,	"r2_ssao_mode",					&ps_r_ssao_mode,			qssao_mode_token);
	CMD3(CCC_Token,		"r2_ssao",						&ps_r_ssao,					qssao_token);
	CMD3(CCC_Mask,		"r2_ssao_blur",                 &ps_r2_ls_flags_ext,		R2FLAGEXT_SSAO_BLUR);
	CMD3(CCC_Mask,		"r2_ssao_opt_data",				&ps_r2_ls_flags_ext,		R2FLAGEXT_SSAO_OPT_DATA);
	CMD3(CCC_Mask,		"r2_ssao_half_data",			&ps_r2_ls_flags_ext,		R2FLAGEXT_SSAO_HALF_DATA);
	CMD3(CCC_Mask,		"r2_ssao_hbao",					&ps_r2_ls_flags_ext,		R2FLAGEXT_SSAO_HBAO);
	CMD3(CCC_Mask,		"r2_ssao_hdao",					&ps_r2_ls_flags_ext,		R2FLAGEXT_SSAO_HDAO);
	CMD3(CCC_Mask,		"r2_ssao_ssdo",					&ps_r2_ls_flags_ext,		R2FLAGEXT_SSAO_SSDO);
	
	CMD3(CCC_Token,		"r2_smaa",						&ps_r_aa, qaa_token);
	CMD3(CCC_AA_Mode,	"r2_aa_mode",					&ps_r_aa_mode, qaa_mode_token);

	CMD3(CCC_Mask,		"r4_enable_tessellation",		&ps_r2_ls_flags_ext,		R2FLAGEXT_ENABLE_TESSELLATION);
	CMD3(CCC_Mask,		"r4_tess_wireframe",			&ps_r2_ls_flags_ext,		R2FLAGEXT_TESS_WIREFRAME);
	CMD3(CCC_Token,		"r4_tess_quality",				&optTessQuality_,			qtess_quality_token);

	CMD3(CCC_Mask,		"r2_steep_parallax",			&ps_r2_ls_flags,			R2FLAG_STEEP_PARALLAX);
	CMD3(CCC_Mask,		"r2_detail_bump",				&ps_r2_ls_flags,			R2FLAG_DETAIL_BUMP);

	CMD3(CCC_Token,		"r2_sun_quality",				&ps_r_sun_quality,			qsun_quality_token);

	CMD3(CCC_Mask,		"r2_sun_flares",				&ps_r2_ls_flags_ext,		R2FLAGEXT_SUN_FLARES);

	CMD3(CCC_Mask,		"r2_soft_water",				&ps_r2_ls_flags,			R2FLAG_SOFT_WATER);
	CMD3(CCC_Mask,		"r2_soft_particles",			&ps_r2_ls_flags,			R2FLAG_SOFT_PARTICLES);

	CMD3(CCC_Token,		"r3_msaa",						&ps_r3_msaa,				qmsaa_token);
	//CMD3(CCC_Mask,		"r3_msaa_opt",					&ps_r2_ls_flags,			R3FLAG_MSAA_OPT);
	CMD3(CCC_Mask,		"r3_gbuffer_opt",				&ps_r2_ls_flags,			R3FLAG_GBUFFER_OPT);
	CMD3(CCC_Mask,		"r3_use_dx10_1",				&ps_r2_ls_flags,			(u32)R3FLAG_USE_DX10_1);
	CMD3(CCC_Token,		"r3_msaa_alphatest",			&ps_r3_msaa_atest,			qmsaa__atest_token);
	CMD3(CCC_Token,		"r3_minmax_sm",					&ps_r3_minmax_sm,			qminmax_sm_token);

	CMD3(CCC_Mask,		"r3_dynamic_wet_surfaces",		&ps_r2_ls_flags,			R3FLAG_DYN_WET_SURF);

	CMD4(CCC_Float,		"r3_dynamic_wet_surfaces_near",		&ps_r3_dyn_wet_surf_near,	10,		70);
	CMD4(CCC_Float,		"r3_dynamic_wet_surfaces_far",		&ps_r3_dyn_wet_surf_far,	30,		100);
	CMD4(CCC_Integer,	"r3_dynamic_wet_surfaces_sm_res",	&ps_r3_dyn_wet_surf_sm_res,	64,		2048);

	CMD3(CCC_Token,		"r3_swap_chain_buffers_count",	&ps_r3_backbuffers_count,	qbbuffer_token);
	CMD3(CCC_Token,		"r2_tonemap_wcorr",				&ps_r2_tonemap_wcorr,		qbloom_type_token);

	CMD3(CCC_Mask,		"r2_gloss_build_2218",			&ps_r2_ls_flags_ext,		R2FLAGEXT_GLOSS_BUILD_2218);
	CMD3(CCC_Mask,		"r2_color_boost",			    &ps_r2_ls_flags_ext,		R2FLAGEXT_COLOR_BOOST);

	CMD3(CCC_Mask,		"r3_bokeh",						&ps_r2_ls_flags_ext,			R2FLAGEXT_BOKEH);

	//water reflectrions
	CMD4(CCC_Integer,	"r3_water_refl_obj",			&ps_r3_water_refl_obj,				0,		1);
	CMD4(CCC_Integer,	"r3_water_refl_moon",			&ps_r3_water_refl_moon,				0,		1);
	CMD4(CCC_Float,		"r3_water_refl_env_power",		&ps_r3_water_refl_env_power,		0.1f,	3.0f);
	CMD4(CCC_Float,		"r3_water_refl_intensity",		&ps_r3_water_refl_intensity,		0.1f,	10.0f);
	CMD4(CCC_Float,		"r3_water_refl_range",			&ps_r3_water_refl_range,			400.f,	1000.f);
	CMD4(CCC_Float,		"r3_water_refl_obj_itter",		&ps_r3_water_refl_obj_itter,		16.f,	128.f);

	// other hud effects
	CMD4(CCC_Integer,	"r3_adv_wet_hud",				&ps_r3_adv_wet_hud,				0,		1);

	if (CApplication::isDeveloperMode)
	{
		CMD4(CCC_Integer,	"r3_hud_rain_drops",			&ps_r3_hud_rain_drops,			0,		1);
		CMD4(CCC_Integer,	"r3_hud_visor_effect",			&ps_r3_hud_visor_effect,		0,		1);
		CMD4(CCC_Integer,	"r3_hud_visor_shadowing",		&ps_r3_hud_visor_shadowing,		0,		1);
	}

	// ss sunshafts
	CMD4(CCC_Integer,	"r3_use_ss_sunshafts",				&ps_r3_use_ss_sunshafts,			0, 1);
	CMD4(CCC_Float,		"r3_sunshafts_SampleStep_Phase1",	&ps_r2_prop_ss_sample_step_phase0,	0.01f, 0.2f);
	CMD4(CCC_Float,		"r3_sunshafts_SampleStep_Phase2",	&ps_r2_prop_ss_sample_step_phase1,	0.01f, 0.2f);
	CMD4(CCC_Float,		"r3_sunshafts_Radius",				&ps_r2_prop_ss_radius,				0.5f, 4.0f);
	CMD4(CCC_Float,		"r3_sunshafts_Intensity",			&ps_r2_prop_ss_intensity,			0.0f, 2.0f);
	CMD4(CCC_Float,		"r3_sunshafts_Blend",				&ps_r2_prop_ss_blend,				0.01f, 1.0f);

	CMD4(CCC_Float,		"bloom_treshold",					&ps_r2_bloom_treshold,				0.0f, 1.f);
	CMD4(CCC_Float,		"bloom_amount",						&ps_r2_bloom_amount,				0.0f, 1.f);

	CMD3(CCC_OS_Token,	"r__optimize_static_geom",		&ps_r_optimize_static,			q_optimize_static);
	CMD3(CCC_OD_Token,	"r__optimize_dynamic_geom",		&ps_r_optimize_dynamic,			q_optimize_dynamic);
	CMD4(CCC_Integer,	"r__high_optimize_shadows",		&ps_r_high_optimize_shad,		0, 1);
	
//	CMD3(CCC_Mask,		"r3_volumetric_smoke",			&ps_r2_ls_flags,			R3FLAG_VOLUMETRIC_SMOKE); //skyloader: not used in the game, plus eats many fps in game :)
	
	CMD1(CCC_memory_stats,	"render_memory_stats");
	CMD1(DumpVisualsForPrefetching, "dump_visuals_for_prefetching"); //Prints the list of visuals, which caused stutterings during game

	// test
	CMD4(CCC_Float,		"r_developer_float_1",				&ps_r__test_exp_to_shaders_1, -10000000, 10000000);
	CMD4(CCC_Float,		"r_developer_float_2",				&ps_r__test_exp_to_shaders_2, -10000000, 10000000);
	CMD4(CCC_Float,		"r_developer_float_3",				&ps_r__test_exp_to_shaders_3, -10000000, 10000000);
	CMD4(CCC_Float,		"r_developer_float_4",				&ps_r__test_exp_to_shaders_4, -10000000, 10000000);

	CMD4(CCC_Integer,	"r__debug_ambient_occ",				&ps_r__debug_ssao,			0, 1);

	if (CApplication::isDeveloperMode)
	{
		CMD1(CCC_RM_Test_Ram_Capacity, "r__fill_ram");
	}

#ifdef DEBUG
	CMD4(CCC_Integer,	"r__lsleep_frames",		&ps_r__LightSleepFrames,	4,		30);
	CMD4(CCC_Float,		"r__wallmark_shift_pp",	&ps_r__WallmarkSHIFT,		0.0f,	1.f);
	CMD4(CCC_Float,		"r__wallmark_shift_v",	&ps_r__WallmarkSHIFT_V,		0.0f,	1.f);

	CMD1(CCC_Fog_Reload,"r3_fog_reload");

	CMD1(CCC_ModelPoolStat, "stat_models");
	CMD1(CCC_DumpResources,	"dump_resources");
#endif
}

void xrRender_apply_tf()
{
	Console->Execute	("r__tf_aniso"	);
}


