#ifndef xrRender_consoleH
#define xrRender_consoleH
#pragma once

// Common
extern ECORE_API	u32			ps_r_sun_shafts;	//=	0;
extern ECORE_API	xr_token	qsun_shafts_token[];
extern ECORE_API	u32			ps_r_ssao;			//	=	0;
extern ECORE_API	xr_token	qssao_token[];
extern ECORE_API	u32			ps_r_smap_size;
extern ECORE_API	xr_token	qsmap_size_token[];
extern ECORE_API	u32			ps_r_ssao_mode;
extern ECORE_API	xr_token	qssao_mode_token[];
extern ECORE_API	u32			ps_r_sun_quality;	//	=	0;
extern ECORE_API	xr_token	qsun_quality_token[];

extern ECORE_API	u32			ps_r3_msaa;	//	=	0;
extern ECORE_API	xr_token	qmsaa_token[];

extern ECORE_API	u32			ps_r3_msaa_atest; //=	0;
extern ECORE_API	xr_token	qmsaa__atest_token[];

extern ECORE_API	u32			ps_r3_minmax_sm;//	=	0;
extern ECORE_API	xr_token	qminmax_sm_token[];

extern ECORE_API	u32			ps_r_aa_mode;
extern ECORE_API	xr_token	qaa_mode_token[];

extern ECORE_API	u32			ps_r_aa;
extern ECORE_API	xr_token	qaa_token[];

extern ECORE_API	int			ps_r__LightSleepFrames;

extern ECORE_API	float		ps_r__Detail_l_ambient;
extern ECORE_API	float		ps_r__Detail_l_aniso;
extern ECORE_API	float		ps_r__Detail_density;
extern ECORE_API	int			ps_r__Detail_radius;
extern ECORE_API	float		ps_r__Details_sun_sh_dist; // shuold not exeed ps_r2_sun_shadowns_near for optimization
extern ECORE_API	float		ps_details_with_shadows_dist;

extern ECORE_API	float		ps_r__Tree_SBC;		// scale bias correct

extern ECORE_API	float		ps_r__WallmarkTTL;
extern ECORE_API	float		ps_r__WallmarkSHIFT;
extern ECORE_API	float		ps_r__WallmarkSHIFT_V;
extern ECORE_API	float		ps_r__SWallmarkDiscard;
extern ECORE_API	float		ps_r__DWallmarkDiscard;

extern ECORE_API	float		ps_r__GLOD_ssa_start;
extern ECORE_API	float		ps_r__GLOD_ssa_end;
extern ECORE_API	float		ps_r__ssaDISCARD;
extern ECORE_API	float		ps_r__ssaHZBvsTEX;

extern ECORE_API	float		ps_r__geomLodSpriteDistF_;
extern ECORE_API	float		ps_r__geomDiscardDistF_;
extern ECORE_API	float		ps_r__geomLodDistF_;
extern ECORE_API	float		ps_r__geomNTextureDistF_;
extern ECORE_API	float		ps_r__geomDTextureDistF_;

// Cut-off not valuable static geometry
extern ECORE_API	u32			ps_r_optimize_static;
// Cut-off not valuable dyn geometry
extern ECORE_API	u32			ps_r_optimize_dynamic;
// Maximum geometry cut-off for sun shadows
extern ECORE_API	BOOL		ps_r_high_optimize_shad;

extern ECORE_API	int			ps_r__tf_Anisotropic;
extern ECORE_API	int			ps_GlowsPerFrame;

extern ECORE_API	float		ps_r1_pps_u;
extern ECORE_API	float		ps_r1_pps_v;
// R2
extern ECORE_API	float		ps_r2_ssaLOD_A;
extern ECORE_API	float		ps_r2_ssaLOD_B;

// R2-specific
extern ECORE_API Flags32		ps_r2_ls_flags;
extern ECORE_API Flags32		ps_r2_ls_flags_ext;
extern ECORE_API Flags32		ps_r_mt_flags;
extern ECORE_API Flags32		ps_r_misc_flags;

extern ECORE_API float			ps_r2_df_parallax_h;		// r2-only
extern ECORE_API float			ps_r2_df_parallax_range;	// r2-only
extern ECORE_API float			ps_r2_steep_parallax_h;
extern ECORE_API float			ps_r2_steep_parallax_distance;
extern ECORE_API float			ps_r2_steep_parallax_samples;
extern ECORE_API float			ps_r2_steep_parallax_samples_min;
extern ECORE_API float			ps_r2_gmaterial;			// r2-only
extern ECORE_API float			ps_r2_tonemap_middlegray;	// r2-only
extern ECORE_API float			ps_r2_tonemap_adaptation;	// r2-only
extern ECORE_API float			ps_r2_tonemap_low_lum;		// r2-only
extern ECORE_API float			ps_r2_tonemap_amount;		// r2-only
extern ECORE_API float			ps_r2_tonemap_white;		// r2-only
extern ECORE_API float			ps_r2_ls_bloom_kernel_scale;// r2-only	// gauss
extern ECORE_API float			ps_r2_ls_bloom_kernel_g;	// r2-only	// gauss
extern ECORE_API float			ps_r2_ls_bloom_kernel_b;	// r2-only	// bilinear
extern ECORE_API float			ps_r2_ls_bloom_threshold;	// r2-only
extern ECORE_API float			ps_r2_ls_bloom_speed;		// r2-only

extern ECORE_API Fvector		ps_r2_aa_barier;			// r2-only
extern ECORE_API Fvector		ps_r2_aa_weight;			// r2-only
extern ECORE_API float			ps_r2_aa_kernel;			// r2-only
extern ECORE_API float			ps_r2_mblur;				// .5f
extern ECORE_API int			ps_r2_GI_depth;				// 1..5
extern ECORE_API int			ps_r2_GI_photons;			// 8..256
extern ECORE_API float			ps_r2_GI_clip;				// EPS
extern ECORE_API float			ps_r2_GI_refl;				// .9f
extern ECORE_API float			ps_r2_ls_depth_scale;		// 1.0f
extern ECORE_API float			ps_r2_ls_depth_bias;		// -0.0001f
extern ECORE_API float			ps_r2_ls_squality;			// 1.0f
extern ECORE_API float			ps_r2_sun_near;				// 10.0f

extern ECORE_API float			ps_r2_sun_tsm_bias;			// 0.0001f
extern ECORE_API float			ps_r2_sun_depth_far_scale;	// 1.00001f
extern ECORE_API float			ps_r2_sun_depth_far_bias;	// -0.0001f
extern ECORE_API float			ps_r2_sun_depth_near_scale;	// 1.00001f
extern ECORE_API float			ps_r2_sun_depth_near_bias;	// -0.0001f
extern ECORE_API float			ps_r2_sun_lumscale;			// 1.0f
extern ECORE_API float			ps_r2_sun_lumscale_hemi;	// 1.0f
extern ECORE_API float			ps_r2_sun_lumscale_amb;		// 1.0f

extern ECORE_API float			ps_r2_sun_shadows_near_casc;
extern ECORE_API float			ps_r2_sun_shadows_midl_casc;
extern ECORE_API float			ps_r2_sun_shadows_far_casc;

extern ENGINE_API BOOL			r2_sun_static;
extern ENGINE_API BOOL			r2_advanced_pp;	// advanced post process and effects

extern ECORE_API float			ps_r2_particle_distance;

extern ECORE_API float			ps_r2_dhemi_sky_scale;		// 1.5f
extern ECORE_API float			ps_r2_dhemi_light_scale;	// 1.f
extern ECORE_API float			ps_r2_dhemi_light_flow;		// .1f
extern ECORE_API int			ps_r2_dhemi_count;			// 5
extern ECORE_API float			ps_r2_slight_fade;			// 1.f

extern ECORE_API BOOL			ps_light_details_shadow;
extern ECORE_API float			ps_light_ds_max_dist_from_light;
extern ECORE_API float			ps_light_ds_max_dist_from_cam;
extern ECORE_API int			ps_light_ds_max_lights;

extern ECORE_API int			ps_r2_wait_sleep;
extern ECORE_API BOOL			ps_r_cpu_wait_gpu;

//	x - min (0), y - focus (1.4), z - max (100)
extern ECORE_API Fvector3		ps_r2_dof;
extern ECORE_API float			ps_r2_dof_sky;				//	distance to sky
extern ECORE_API float			ps_r2_dof_kernel_size;		//	7.0f

extern ECORE_API float			ps_r3_dyn_wet_surf_near;	// 10.0f
extern ECORE_API float			ps_r3_dyn_wet_surf_far;		// 30.0f
extern ECORE_API int			ps_r3_dyn_wet_surf_sm_res;	// 256
extern ECORE_API u32			ps_r3_backbuffers_count;	// 2
extern ECORE_API xr_token		qbbuffer_token[];
extern ECORE_API u32			ps_r2_tonemap_wcorr;

//water reflectrions
extern ECORE_API BOOL			ps_r3_water_refl_obj;
extern ECORE_API BOOL			ps_r3_water_refl_moon;
extern ECORE_API float			ps_r3_water_refl_env_power;
extern ECORE_API float			ps_r3_water_refl_intensity;
extern ECORE_API float			ps_r3_water_refl_range;
extern ECORE_API float			ps_r3_water_refl_obj_itter;

//other
extern ECORE_API BOOL			ps_r3_adv_wet_hud;
extern ECORE_API BOOL			ps_r__debug_ssao;

// hud rain drops and visor effect
extern ECORE_API BOOL			ps_r3_hud_rain_drops;
extern ECORE_API BOOL			ps_r3_hud_visor_effect;
extern ECORE_API BOOL			ps_r3_hud_visor_shadowing;
extern ECORE_API BOOL			ps_r3_use_new_bloom;
extern ECORE_API BOOL			ps_r3_use_wet_reflections;

// Test float exported to shaders for development
extern ECORE_API float			ps_r__test_exp_to_shaders_1;
extern ECORE_API float			ps_r__test_exp_to_shaders_2;
extern ECORE_API float			ps_r__test_exp_to_shaders_3;
extern ECORE_API float			ps_r__test_exp_to_shaders_4;

// SS sun shafts
extern ECORE_API BOOL			ps_r3_use_ss_sunshafts;

extern ECORE_API float			ps_r2_prop_ss_sample_step_phase0;
extern ECORE_API float			ps_r2_prop_ss_sample_step_phase1;

extern ECORE_API float			ps_r2_prop_ss_radius;
extern ECORE_API float			ps_r2_prop_ss_intensity;
extern ECORE_API float			ps_r2_prop_ss_blend;

extern ECORE_API float			ps_r2_bloom_treshold;
extern ECORE_API float			ps_r2_bloom_amount;
extern ECORE_API BOOL			ps_r3_ssgi;

// These values are changed, when geometry optimization option is changed
extern ECORE_API float			o_optimize_static_l1_dist;
extern ECORE_API float			o_optimize_static_l1_size;
extern ECORE_API float			o_optimize_static_l2_dist;
extern ECORE_API float			o_optimize_static_l2_size;
extern ECORE_API float			o_optimize_static_l3_dist;
extern ECORE_API float			o_optimize_static_l3_size;
extern ECORE_API float			o_optimize_static_l4_dist;
extern ECORE_API float			o_optimize_static_l4_size;
extern ECORE_API float			o_optimize_static_l5_dist;
extern ECORE_API float			o_optimize_static_l5_size;

extern ECORE_API float			o_optimize_dynamic_l1_dist;
extern ECORE_API float			o_optimize_dynamic_l1_size;
extern ECORE_API float			o_optimize_dynamic_l2_dist;
extern ECORE_API float			o_optimize_dynamic_l2_size;
extern ECORE_API float			o_optimize_dynamic_l3_dist;
extern ECORE_API float			o_optimize_dynamic_l3_size;

enum
{
	R2FLAG_SUN					= (1<<0),
	R2FLAG_DETAILS_SUN_SHADOWS	= (1<<3),
	R2FLAG_TONEMAP				= (1<<4),
	R2FLAG_AA					= (1<<5),
	R2FLAG_GI					= (1<<6),
	R2FLAG_FASTBLOOM			= (1<<7),
	R2FLAG_GLOBALMATERIAL		= (1<<8),
	R2FLAG_GPU_OCC_OPTIMIZATION	= (1<<9),
	R2FLAG_R1LIGHTS				= (1<<10),

	R2FLAG_MBLUR			= (1<<12),
	
	R2FLAG_EXP_DONT_TEST_UNSHADOWED			= (1<<14),
	R2FLAG_EXP_DONT_TEST_SHADOWED			= (1<<15),

	R2FLAG_USE_NVDBT			= (1<<16),

	R2FLAG_SOFT_WATER			= (1<<19),	//	Igor: need restart
	R2FLAG_SOFT_PARTICLES		= (1<<20),	//	Igor: need restart
	R2FLAG_VOLUMETRIC_LIGHTS	= (1<<21),
	R2FLAG_STEEP_PARALLAX		= (1<<22),
	R2FLAG_DOF					= (1<<23),

	R2FLAG_DETAIL_BUMP			= (1<<25),

	R3FLAG_DYN_WET_SURF			= (1<<26),
	R_FLAG_SHADERS_CACHE		= (1<<27),

	R3FLAG_MSAA_HYBRID			= (1<<28),
	R3FLAG_MSAA_OPT				= (1<<29),
	R3FLAG_GBUFFER_OPT			= (1<<30),
	R3FLAG_USE_DX10_1			= (1<<31),
	//R3FLAG_MSAA_ALPHATEST		= (1<<31),
};

enum
{
	R2FLAGEXT_SSAO_BLUR				= (1<<0),
	R2FLAGEXT_SSAO_OPT_DATA			= (1<<1),
	R2FLAGEXT_SSAO_HALF_DATA		= (1<<2),
	R2FLAGEXT_SSAO_HBAO				= (1<<3),
	R2FLAGEXT_SSAO_HDAO				= (1<<4),
	R2FLAGEXT_ENABLE_TESSELLATION	= (1<<5),
	R2FLAGEXT_TESS_WIREFRAME		= (1<<6),
	R_FLAGEXT_HOM_DEPTH_DRAW		= (1<<7),
	R2FLAGEXT_SUN_ZCULLING			= (1<<8),
	R2FLAGEXT_SUN_FLARES			= (1<<10),
	R2FLAGEXT_DOF_WEATHER			= (1<<11),
	R2FLAGEXT_GLOWS_ENABLE			= (1<<12),
	R2FLAGEXT_WIREFRAME				= (1<<13),
	R2FLAGEXT_GLOSS_BUILD_2218		= (1<<15),
	R2FLAGEXT_COLOR_BOOST			= (1<<16),
	R2FLAGEXT_BOKEH					= (1<<17),
	R2FLAGEXT_SSAO_SSDO				= (1 << 18),
	R2FLAGEXT_AA_FXAA				= (1 << 19),
	R2FLAGEXT_AA_SMAA				= (1 << 20),
};

enum
{
	R_FLAG_MT_CALC_HOM_DETAILS			= (1 << 0),

	R_FLAG_MT_CALC_SUN_VIS_STRUCT		= (1 << 1),
	R_FLAG_MT_CALC_RAIN_VIS_STRUCT		= (1 << 2),
	R_FLAG_MT_CALC_LGHTS_VIS_STRUCT		= (1 << 3),
	R_FLAG_MT_CALC_PRIOR0_VIS_STRUCT	= (1 << 4),
	R_FLAG_MT_CALC_PRIOR1_VIS_STRUCT	= (1 << 5),
	R_FLAG_MT_BONES_PRECALC				= (1 << 6),
};

enum
{
	R_MISC_USE_HOM_PRIOR_0				= (1 << 0),
	R_MISC_USE_HOM_FORWARD				= (1 << 1),
	R_MISC_USE_HOM_LIGHT				= (1 << 2),
	R_MISC_USE_HOM_SUN					= (1 << 3),
	R_MISC_USE_SECTORS_FOR_STATIC		= (1 << 4)
};

// Static geometry optimization

#define O_S_L1_S_LOW	10.f // geometry 3d volume size
#define O_S_L1_D_LOW	150.f // distance, after which it is not rendered
#define O_S_L2_S_LOW	100.f
#define O_S_L2_D_LOW	200.f
#define O_S_L3_S_LOW	500.f
#define O_S_L3_D_LOW	250.f
#define O_S_L4_S_LOW	2500.f
#define O_S_L4_D_LOW	350.f
#define O_S_L5_S_LOW	7000.f
#define O_S_L5_D_LOW	400.f

#define O_S_L1_S_MED	25.f
#define O_S_L1_D_MED	50.f
#define O_S_L2_S_MED	200.f
#define O_S_L2_D_MED	150.f
#define O_S_L3_S_MED	1000.f
#define O_S_L3_D_MED	200.f
#define O_S_L4_S_MED	2500.f
#define O_S_L4_D_MED	300.f
#define O_S_L5_S_MED	7000.f
#define O_S_L5_D_MED	400.f

#define O_S_L1_S_HII	50.f
#define O_S_L1_D_HII	50.f
#define O_S_L2_S_HII	400.f
#define O_S_L2_D_HII	150.f
#define O_S_L3_S_HII	1500.f
#define O_S_L3_D_HII	200.f
#define O_S_L4_S_HII	5000.f
#define O_S_L4_D_HII	300.f
#define O_S_L5_S_HII	20000.f
#define O_S_L5_D_HII	350.f

#define O_S_L1_S_ULT	80.f
#define O_S_L1_D_ULT	40.f
#define O_S_L2_S_ULT	600.f
#define O_S_L2_D_ULT	100.f
#define O_S_L3_S_ULT	2500.f
#define O_S_L3_D_ULT	120.f
#define O_S_L4_S_ULT	5000.f
#define O_S_L4_D_ULT	140.f
#define O_S_L5_S_ULT	20000.f
#define O_S_L5_D_ULT	200.f

// Dyn geometry optimization

#define O_D_L1_S_LOW	1.f // geometry 3d volume size
#define O_D_L1_D_LOW	80.f // distance, after which it is not rendered
#define O_D_L2_S_LOW	3.f
#define O_D_L2_D_LOW	150.f
#define O_D_L3_S_LOW	4000.f
#define O_D_L3_D_LOW	250.f

#define O_D_L1_S_MED	1.f
#define O_D_L1_D_MED	40.f
#define O_D_L2_S_MED	4.f
#define O_D_L2_D_MED	100.f
#define O_D_L3_S_MED	4000.f
#define O_D_L3_D_MED	200.f

#define O_D_L1_S_HII	1.4f
#define O_D_L1_D_HII	30.f
#define O_D_L2_S_HII	4.f
#define O_D_L2_D_HII	80.f
#define O_D_L3_S_HII	4000.f
#define O_D_L3_D_HII	150.f

#define O_D_L1_S_ULT	2.0f
#define O_D_L1_D_ULT	30.f
#define O_D_L2_S_ULT	8.f
#define O_D_L2_D_ULT	50.f
#define O_D_L3_S_ULT	4000.f
#define O_D_L3_D_ULT	110.f

extern void						xrRender_initconsole	();
extern BOOL						xrRender_test_hw		();
extern void						xrRender_apply_tf		();

#endif
