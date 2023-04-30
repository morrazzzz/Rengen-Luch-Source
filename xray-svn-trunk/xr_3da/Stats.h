// Stats.h: interface for the CStats class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STATS_H__4C8D1860_0EE2_11D4_B4E3_4854E82A090D__INCLUDED_)
#define AFX_STATS_H__4C8D1860_0EE2_11D4_B4E3_4854E82A090D__INCLUDED_
#pragma once

// remove it when fixed
//#define LA_SHADERS_DEBUG

class ENGINE_API CGameFont;
#ifdef LA_SHADERS_DEBUG
class CConstantsDebug;
#endif

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/StatsRender.h"
#include "pure.h"

#define MEASURE_UPDATES
#define MEASURE_MT
#define MEASURE_ON_FRAME

DECLARE_MESSAGE(Stats);

struct ViewPortRenderingStats
{
	ViewPortRenderingStats();

	//for CRender::Render() timers
	float		preRender_;
	float		preCalc_;
	float		GPU_Sync_Point;
	float		mainGeomVis_;
	float		mainGeomDraw_;
	float		complexStage1_;
	float		detailsDraw_;
	float		hudItemsWorldUI_;
	float		wallmarksDraw_;
	float		MSAAEdges_;
	float		wetSurfacesDraw_;
	float		sunSmDraw_;
	float		emissiveGeom_;
	float		lightsNormalTime_;
	float		lightsPendingTime_;
	float		combinerAnd2ndGeom_;
	float		postCalc_;
	float		GPU_Occ_Wait;
	float		RenderMtWait;
	float		RenderTotalTime;

	u32			vpId;
};

class ENGINE_API CStatsPhysics
{
public:
	CStatTimer	ph_collision;		// collision
	CStatTimer	ph_core;			// integrate
	CStatTimer	Physics;			// movement+collision
};

class ENGINE_API CStats: 
	public pureRender,
	public CStatsPhysics
{
public:
	CGameFont*	GCS_StatsFont_;
	CGameFont*	FPSFont_;
	CGameFont*	secondBlockFont_;

	float		fDeviceMeasuredFPS;		// Fps measured in device.cpp
	float		fMem_calls			;
	u32			dwMem_calls			;
	u32			dwSND_Played,dwSND_Allocated;	// Play/Alloc
	float		fShedulerLoad		;

	CStatTimer	EngineTOTAL;			// 
	CStatTimer	Sheduler;				// 
	CStatTimer	UpdateClient;			// 
	u32			UpdateClient_updated;	//
	u32			UpdateClient_actual_list;		//
	u32			UpdateClient_active;	//
	u32			UpdateClient_total;		//
	u32			Particles_starting;	// starting
	u32			Particles_active;	// active
	u32			Particles_destroy;	// destroying
//	CStatTimer	Physics;			// movement+collision
//	CStatTimer	ph_collision;		// collision
//	CStatTimer	ph_core;			// collision
	CStatTimer	AI_Think;			// thinking
	CStatTimer	AI_Range;			// query: range
	CStatTimer	AI_Path;			// query: path
	CStatTimer	AI_Node;			// query: node
	CStatTimer	AI_Vis;				// visibility detection - total
	CStatTimer	AI_Vis_Query;		// visibility detection - portal traversal and frustum culling
	CStatTimer	AI_Vis_RayTests;	// visibility detection - ray casting

	CStatTimer	RenderTOTAL;		// 
	CStatTimer	RenderTOTAL_Real;	
	CStatTimer	RenderCALC;			// portal traversal, frustum culling, entities "renderable_Render"
	CStatTimer	RenderCALC_HOM;		// HOM rendering
	CStatTimer	Animation;			// skeleton calculation
	CStatTimer	RenderDUMP;			// actual primitive rendering
	CStatTimer	RenderDUMP_Wait;	// ...waiting something back (queries results, etc.)
	CStatTimer	RenderDUMP_Wait_S;	// ...frame-limit sync
	CStatTimer	RenderDUMP_RT;		// ...render-targets
	CStatTimer	RenderDUMP_SKIN;	// ...skinning
	CStatTimer	RenderDUMP_HUD;		// ...hud rendering
	CStatTimer	RenderDUMP_Glows;	// ...glows vis-testing,sorting,render
	CStatTimer	RenderDUMP_Lights;	// ...d-lights building/rendering
	CStatTimer	RenderDUMP_WM;		// ...wallmark sorting, rendering
	u32			RenderDUMP_WMS_Count;// ...number of static wallmark
	u32			RenderDUMP_WMD_Count;// ...number of dynamic wallmark
	u32			RenderDUMP_WMT_Count;// ...number of wallmark tri
	CStatTimer	RenderDUMP_DT_VIS;	// ...details visibility detection
	CStatTimer	RenderDUMP_DT_Render;// ...details rendering
	CStatTimer	RenderDUMP_DT_Cache;// ...details slot cache access
	u32			RenderDUMP_DT_Count;// ...number of DT-elements
	CStatTimer	RenderDUMP_Pcalc;	// ...projectors	building
	CStatTimer	RenderDUMP_Scalc;	// ...shadows		building
	CStatTimer	RenderDUMP_Srender;	// ...shadows		render
	
	CStatTimer	Sound;				// total time taken by sound subsystem (accurate only in single-threaded mode)
	CStatTimer	Input;				// total time taken by input subsystem (accurate only in single-threaded mode)
	CStatTimer	clRAY;				// total: ray-testing
	CStatTimer	clBOX;				// total: box query
	CStatTimer	clFRUSTUM;			// total: frustum query
	
	CStatTimer	netClient1;
	CStatTimer	netClient2;
	CStatTimer	netServer;
	CStatTimer	netClientCompressor;
	CStatTimer	netServerCompressor;
	

	
	CStatTimer	TEST0;				// debug counter
	CStatTimer	TEST1;				// debug counter
	CStatTimer	TEST2;				// debug counter
	CStatTimer	TEST3;				// debug counter

	//for displaying engine times
	float		thread1Time_;
	float		thread2Time_;
	float		thread3Time_;
	float		thread4Time_;
	float		thread5Time_;
	float		mainThreadTime_;

	int			mainThreadWaitAux_1_;
	int			mainThreadWaitAux_2_;
	int			mainThreadWaitAux_3_;
	int			mainThreadWaitAux_4_;
	int			mainThreadWaitAux_5_;

	u32			auxThread1Objects_;
	u32			auxThread2Objects_;
	u32			auxThread3Objects_;
	u32			auxThread4Objects_;
	u32			auxThread5Objects_;

	AccessLock	protectIndThread1_Stats;
	AccessLock	protectIndThread2_Stats;
	AccessLock	protectIndThread3_Stats;
	AccessLock	protectResThread1_Stats;

	float		indThread1_Time_;
	float		indThread2_Time_;
	float		indThread3_Time_;
	float		resThread1_Time_;

	u32			indThread1_Objects_;
	u32			indThread2_Objects_;
	u32			indThread3_Objects_;
	u32			resThread1_Objects_;

	ViewPortRenderingStats viewPortStats1; // main
	ViewPortRenderingStats viewPortStats2;

	//Occlusion Debug
	u32			occ_pool_size;
	u32			occ_used_size;
	u32			occ_freed_ids_size;

	u16			occ_Begin_calls;
	u16			occ_End_calls;
	u16			occ_Get_calls;


	// For Measuring UpdateCL functions
#ifdef	MEASURE_UPDATES
	float		updateCL_Object_;
	float		updateCL_BaseMonster_;
	float		updateCL_CustomMonster_;
	float		updateCL_ScriptEntity_;
	float		updateCL_ScriptObject_;
	float		updateCL_AIStalker_;
	float		updateCL_AITrader_;
	float		updateCL_VariousMonsters_;
	float		updateCL_Actor_;
	float		updateCL_GameObject_;
	float		updateCL_VariousItems_;
	float		updateCL_VariousPhysics_;
	float		updateCL_HangingLamp_;
	float		updateCL_Projector_;
	float		updateCL_Car_;
	float		updateCL_CustomZone_;
	float		updateCL_Helicopter_;

	// For Measuring schedule_Update functions

	float		scheduler_Object_;
	float		scheduler_VisionClient_;
	float		scheduler_PSInstance_;
	float		scheduler_AgentManager_;
	float		scheduler_BaseMonster_;
	float		scheduler_CustomMonster_;
	float		scheduler_ScriptBinder_;
	float		scheduler_ScriptEntity_;
	float		scheduler_ScriptObject_;
	float		scheduler_AIStalker_;
	float		scheduler_AITrader_;
	float		scheduler_AICrow_;
	float		scheduler_VariousMonsters_;
	float		scheduler_ScriptBinderObjectWrapper_;
	float		scheduler_ScriptParticlesCustom_;
	float		scheduler_Actor_;
	float		scheduler_Entity_;
	float		scheduler_EntityAlive_;
	float		scheduler_GameObject_;
	float		scheduler_HangingLamp_;
	float		scheduler_VariousItems_;
	float		scheduler_VariousPhysics_;
	float		scheduler_Car_;
	float		scheduler_CustomZone_;
	float		scheduler_LevelChanger_;
	float		scheduler_ScriptZone_;
	float		scheduler_VariousAnomalies_;
	float		scheduler_ParticleObject_;
#endif

#ifdef MEASURE_MT
	//For measuring MT code
	float		mtPhysicsTime_;
	float		mtSoundTime_;
	float		mtNetworkTime_;
	float		mtParticlesTime_;

	float		mtHOM_;
	float		mtDetails_;

	float		mtLevelPathTime_;
	float		mtDetailPathTime_;
	float		mtMapTime_;
	float		mtObjectHandlerTime_;
	float		mtSoundPlayerTime_;
	float		mtAiVisionTime_;
	float		mtAIMiscTime_;
	float		mtBulletsTime_;
	float		mtLUA_GCTime_;
	float		mtLevelSoundsTime_;
	float		mtALifeTime_;
	float		mtIKinematicsTime_;
	float		mtLTrackTime_;
	float		mtVisStructTime_;
	float		mtMeshBones_;

	float		mtPhysicsTimeCopy_;
	float		mtSoundTimeCopy_;
	float		mtNetworkTimeCopy_;
	float		mtParticlesTimeCopy_;

	float		mtHOMCopy_;
	float		mtDetailsCopy_;

	float		mtLevelPathTimeCopy_;
	float		mtDetailPathTimeCopy_;
	float		mtMapTimeCopy_;
	float		mtObjectHandlerTimeCopy_;
	float		mtSoundPlayerTimeCopy_;
	float		mtAiVisionTimeCopy_;
	float		mtAIMiscTimeCopy_;
	float		mtBulletsTimeCopy_;
	float		mtLUA_GCTimeCopy_;
	float		mtLevelSoundsTimeCopy_;
	float		mtALifeTimeCopy_;
	float		mtIKinematicsTimeCopy_;
	float		mtLTrackTimeCopy_;
	float		mtVisStructTimeCopy_;
	float		mtMeshBonesCopy_;

	float		mtRenderDelayPrior0_;
	float		mtRenderDelayPrior1_;
	float		mtRenderDelayLights_;
	float		mtRenderDelaySun_;
	float		mtRenderDelayRain_;
	float		mtRenderDelayBones_;
	float		mtRenderDelayDetails_;
	float		mtRenderDelayHom_;

	float		mtRenderDelayPrior0Copy_;
	float		mtRenderDelayPrior1Copy_;
	float		mtRenderDelayLightsCopy_;
	float		mtRenderDelaySunCopy_;
	float		mtRenderDelayRainCopy_;
	float		mtRenderDelayBonesCopy_;
	float		mtRenderDelayDetailsCopy_;
	float		mtRenderDelayHomCopy_;

#endif

#ifdef MEASURE_ON_FRAME

	float		onframe_Render_;
	float		onframe_App_;
	float		onframe_Console_;
	float		onframe_Input_;
	float		onframe_UISeq_;
	float		onframe_UIDragItem_;
	float		onframe_MainMenu_;
	float		onframe_UIGameCustom_;
	float		onframe_GamePersistant_;
	float		onframe_IGamePersistent_;
	float		onframe_Level_;
	float		onframe_ILevel_;
#endif

	bool			NeedToShow();
	bool			NeedToGatherSpecificInfo();

	void			Show();

	virtual void 	OnRender		();
	void			OnDeviceCreate	();
	void			OnDeviceDestroy	();

	void			GatherMeasurments();

	// retuns u64 of RAM used by our process in BYTES. (equivalent of "working set" in windows task manager)
	u64				GetTotalRAMConsumption();

	xr_vector		<shared_str>	errors;
	CRegistrator	<pureStats>		seqStats;

					CStats			();
					~CStats			();

	IC CGameFont*	Font			(){return GCS_StatsFont_;}

private:
	int				iFPS; // For statistics internal mechanics

	float			UpdateFPSCounterSkip;
	float			UpdateFPSMinute;
	float			fCounterTotalMinute;
	int				iTotalMinute;
	int				iAvrageMinute;
	FactoryPtr<IStatsRender>	m_pRender;

	void			InitializeMeasuringValuesUCL();
	void			InitializeMeasuringValuesUSC();
	void			InitializeMeasuringValuesMT();
	void			InitializeMeasuringValuesOnFrame();

	void			SecondBlockDrawRectAndSetFontPos();

	void			ShowUpdateCLTimes();
	void			ShowScUpdateTimes();
	void			ShowMultithredingTimes();
	void			ShowMTRenderDelays();
	void			ShowOnFrameTimes();

	void			RenderSecondBlock();
};

#endif 
