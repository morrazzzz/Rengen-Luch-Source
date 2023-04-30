#include "stdafx.h"
#include "GameFont.h"
#pragma hdrstop

#include "../xrcdb/ISpatial.h"
#include "IGame_Persistent.h"
#include "render.h"

#include "ConstantDebug.h"

#include "../Include/xrRender/DrawUtils.h"

#include "../Include/xrRender/UIShader.h"
#include "../Include/xrRender/UIRender.h"

#include "IGame_Level.h"

#include "../xrCore/CPUUsage.h"

#include "StatDynGraph.h"

// stats
DECLARE_RP(Stats);

extern BOOL statisticAllowed_;

extern BOOL show_FPS_only;
extern BOOL show_engine_timers;
extern BOOL show_render_times;
extern BOOL log_render_times;
extern BOOL log_engine_times;
extern BOOL display_ram_usage;
extern BOOL display_cpu_usage;

extern BOOL show_UpdateClTimes_;
extern BOOL show_ScedulerUpdateTimes_;
extern BOOL show_MTTimes_;
extern BOOL show_OnFrameTimes_;

ENGINE_API BOOL showActorLuminocity_ = FALSE;

extern BOOL displayFrameGraph;

Fvector colorok = { 206, 180, 247 };
Fvector colorbad = { 224, 114, 119 };
u32 statscolor1 = color_rgba(206, 180, 247, 255);
u32 back_color = color_rgba(20, 20, 20, 150);
Frect back_region;

BOOL g_bDisableRedText = FALSE;

FactoryPtr<IUIShader>* back_shader;

float CPU_UsageNextUpdate_ = 0;
float tempCpuUsage_ = 0;
u32 tempCpuUsageFrames_ = 0;
float cpuUsagePrcentage_ = 0;

extern AccessLock bThreadReadyProtect_;

StatDynGraph<float>* frameTimeGraph = nullptr;

class	optimizer	{
	float	average_	;
	BOOL	enabled_	;
public:
	optimizer	()		{
		average_	= 30.f;
//		enabled_	= TRUE;
//		disable		();
		// because Engine is not exist
		enabled_	= FALSE;
	}

	BOOL	enabled	()	{ return enabled_;	}
	void	enable	()	{ if (!enabled_)	{ enabled_=TRUE;	}}
	void	disable	()	{ if (enabled_)		{ enabled_=FALSE; }}
	void	update	(float value)	{
		if (value < average_*0.7f)	{
			// 25% deviation
			enable	();
		} else {
			disable	();
		};
		average_	= 0.99f*average_ + 0.01f*value;
	};
};
static optimizer vtune;

void _draw_cam_pos(CGameFont* pFont)
{
	float sz		= pFont->GetHeight();
	pFont->SetHeightI(0.02f);
	pFont->SetColor	(0xffffffff);
	pFont->Out		(10, 600, "CAMERA POSITION:  [%3.2f,%3.2f,%3.2f]",VPUSH(Device.vCameraPosition));
	pFont->SetHeight(sz);
	pFont->OnRender	();
}


IC void DisplayWithcolor(CGameFont& targetFont, LPCSTR msg, u32 returntocolor, float value, float bad_value){
	u32 colortodisplay;
	if (value >= 0.f && value < bad_value)
	{
		float factor = value / bad_value;

		Fvector temp;
		temp.lerp(colorok, colorbad, factor);

		colortodisplay = color_rgba((u32)temp.x, (u32)temp.y, (u32)temp.z, 255);
	}
	else
	{
		colortodisplay = color_rgba((u32)colorbad.x, (u32)colorbad.y, (u32)colorbad.z, 255);
	}

	targetFont.SetColor(colortodisplay);

	//if (bad_value < 1.1f && value > 8.f)
	//	Msg("Overload in %s. Value = %f", msg, value);

	targetFont.OutNext("%fms = %s", value, msg);
	targetFont.SetColor(returntocolor);
}


void DrawRect(Frect const& r, u32 color)
{
	UIRender->PushPoint(r.x1, r.y1, 0.0f, color, 0.0f, 0.0f);
	UIRender->PushPoint(r.x2, r.y1, 0.0f, color, 1.0f, 0.0f);
	UIRender->PushPoint(r.x2, r.y2, 0.0f, color, 1.0f, 1.0f);

	UIRender->PushPoint(r.x1, r.y1, 0.0f, color, 0.0f, 0.0f);
	UIRender->PushPoint(r.x2, r.y2, 0.0f, color, 1.0f, 1.0f);
	UIRender->PushPoint(r.x1, r.y2, 0.0f, color, 0.0f, 1.0f);
}



CStats::CStats()
{
	back_shader = NULL;
	GCS_StatsFont_ = 0;

	fMem_calls = 0;
	RenderDUMP_DT_Count = 0;
	Device.seqRender.Add(this, REG_PRIORITY_LOW - 1000);

	FPSFont_ = 0;
	secondBlockFont_ = 0;

	InitializeMeasuringValuesUCL();
	InitializeMeasuringValuesUSC();
	InitializeMeasuringValuesMT();
	InitializeMeasuringValuesOnFrame();

	cpuUsageHandler_.InitMonitoring();
}

CStats::~CStats()
{
	Device.seqRender.Remove(this);

	xr_delete(GCS_StatsFont_);
	xr_delete(FPSFont_);
	xr_delete(secondBlockFont_);
}

ENGINE_API float fActor_Lum = 0.0f; //temp debug

bool CStats::NeedToShow()
{
	bool need = (psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || showActorLuminocity_
		|| show_FPS_only || show_engine_timers || show_render_times
		|| errors.size() || display_ram_usage || display_cpu_usage
		|| show_UpdateClTimes_ || show_ScedulerUpdateTimes_	|| show_MTTimes_ || show_OnFrameTimes_);

	return need;
}

bool CStats::NeedToGatherSpecificInfo()
{
	bool need = psDeviceFlags.test(rsStatistic) || show_FPS_only == TRUE || show_engine_timers == TRUE;

	return need;
}

void CStats::Show() 
{
	if (!statisticAllowed_)
		return;

	// Stop timers
	{
		EngineTOTAL.FrameEnd		();	
		Sheduler.FrameEnd			();	
		UpdateClient.FrameEnd		();	
		Physics.FrameEnd			();	
		ph_collision.FrameEnd		();
		ph_core.FrameEnd			();
		Animation.FrameEnd			();	
		AI_Think.FrameEnd			();
		AI_Range.FrameEnd			();
		AI_Path.FrameEnd			();
		AI_Node.FrameEnd			();
		AI_Vis.FrameEnd				();
		AI_Vis_Query.FrameEnd		();
		AI_Vis_RayTests.FrameEnd	();
		
		RenderTOTAL.FrameEnd		();
		RenderCALC.FrameEnd			();
		RenderCALC_HOM.FrameEnd		();
		RenderDUMP.FrameEnd			();	
		RenderDUMP_RT.FrameEnd		();
		RenderDUMP_SKIN.FrameEnd	();	
		RenderDUMP_Wait.FrameEnd	();	
		RenderDUMP_Wait_S.FrameEnd	();	
		RenderDUMP_HUD.FrameEnd		();	
		RenderDUMP_Glows.FrameEnd	();	
		RenderDUMP_Lights.FrameEnd	();	
		RenderDUMP_WM.FrameEnd		();	
		RenderDUMP_DT_VIS.FrameEnd	();	
		RenderDUMP_DT_Render.FrameEnd();	
		RenderDUMP_DT_Cache.FrameEnd();
		RenderDUMP_Pcalc.FrameEnd	();	
		RenderDUMP_Scalc.FrameEnd	();	
		RenderDUMP_Srender.FrameEnd	();	
		
		Sound.FrameEnd				();
		Input.FrameEnd				();
		clRAY.FrameEnd				();	
		clBOX.FrameEnd				();
		clFRUSTUM.FrameEnd			();
		
		netClient1.FrameEnd			();
		netClient2.FrameEnd			();
		netServer.FrameEnd			();

		netClientCompressor.FrameEnd();
		netServerCompressor.FrameEnd();
		
		TEST0.FrameEnd				();
		TEST1.FrameEnd				();
		TEST2.FrameEnd				();
		TEST3.FrameEnd				();

		g_SpatialSpace->stat_insert.FrameEnd		();
		g_SpatialSpace->stat_remove.FrameEnd		();
		g_SpatialSpacePhysic->stat_insert.FrameEnd	();
		g_SpatialSpacePhysic->stat_remove.FrameEnd	();
	}

	if (TimeDelta()<=EPS_S)
	{
		float mem_count		= float	(Memory.stat_calls);
		if (mem_count>fMem_calls)	fMem_calls	=	mem_count;
		else						fMem_calls	=	.9f*fMem_calls + .1f*mem_count;
		Memory.stat_calls	= 0;
	}

	CGameFont& F = *GCS_StatsFont_;

	float f_base_size = 0.01f;

	F.SetHeightI (f_base_size);


	if (vtune.enabled())
	{
		float sz = GCS_StatsFont_->GetHeight();
		GCS_StatsFont_->SetHeightI(0.02f);
		GCS_StatsFont_->SetColor(0xFFFF0000);
		GCS_StatsFont_->OutSet(Device.dwWidth / 2.0f + (GCS_StatsFont_->SizeOf_("--= tune =--") / 2.0f), Device.dwHeight / 2.0f);
		GCS_StatsFont_->OutNext("--= tune =--");
		GCS_StatsFont_->OnRender();
		GCS_StatsFont_->SetHeight(sz);
	};

	// Show
	{
		if (psDeviceFlags.test(rsStatistic))
		{
			static float	r_ps = 0;
			static float	b_ps = 0;
			r_ps = .99f*r_ps + .01f*(clRAY.count / clRAY.result);
			b_ps = .99f*b_ps + .01f*(clBOX.count / clBOX.result);

			CSound_stats				snd_stat;
			::Sound->statistic(&snd_stat, 0);
			F.SetColor(0xFFFFFFFF);
			GCS_StatsFont_->OutSet(0, 0);

			m_pRender->OutData1(F);

#ifdef FS_DEBUG
			F.OutNext	("mapped:      %d",			g_file_mapped_memory);
			F.OutSkip	();
#endif
			m_pRender->OutData3(F);
			F.OutSkip();

#define PPP(a) (100.f*float(a)/float(EngineTOTAL.result))
			F.OutNext("*** ENGINE:  %2.2fms", EngineTOTAL.result);
			F.OutNext("Memory:      %2.2fa", fMem_calls);
			F.OutNext("uClients:    %2.2fms, %2.1f%%, actual(%d)/active(%d)/total(%d)", UpdateClient.result, PPP(UpdateClient.result), UpdateClient_actual_list, UpdateClient_active, UpdateClient_total);
			F.OutNext("uSheduler:   %2.2fms, %2.1f%%", Sheduler.result, PPP(Sheduler.result));
			F.OutNext("uSheduler_L: %2.2fms", fShedulerLoad);
			F.OutNext("uParticles:  Qstart[%d] Qactive[%d] Qdestroy[%d]", Particles_starting, Particles_active, Particles_destroy);
			F.OutNext("spInsert:    o[%.2fms, %2.1f%%], p[%.2fms, %2.1f%%]", g_SpatialSpace->stat_insert.result, PPP(g_SpatialSpace->stat_insert.result), g_SpatialSpacePhysic->stat_insert.result, PPP(g_SpatialSpacePhysic->stat_insert.result));
			F.OutNext("spRemove:    o[%.2fms, %2.1f%%], p[%.2fms, %2.1f%%]", g_SpatialSpace->stat_remove.result, PPP(g_SpatialSpace->stat_remove.result), g_SpatialSpacePhysic->stat_remove.result, PPP(g_SpatialSpacePhysic->stat_remove.result));
			F.OutNext("Physics:     %2.2fms, %2.1f%%", Physics.result, PPP(Physics.result));
			F.OutNext("  collider:  %2.2fms", ph_collision.result);
			F.OutNext("  solver:    %2.2fms, %d", ph_core.result, ph_core.count);
			F.OutNext("aiThink:     %2.2fms, %d", AI_Think.result, AI_Think.count);
			F.OutNext("  aiRange:   %2.2fms, %d", AI_Range.result, AI_Range.count);
			F.OutNext("  aiPath:    %2.2fms, %d", AI_Path.result, AI_Path.count);
			F.OutNext("  aiNode:    %2.2fms, %d", AI_Node.result, AI_Node.count);
			F.OutNext("aiVision:    %2.2fms, %d", AI_Vis.result, AI_Vis.count);
			F.OutNext("  Query:     %2.2fms", AI_Vis_Query.result);
			F.OutNext("  RayCast:   %2.2fms", AI_Vis_RayTests.result);
			F.OutSkip();

#undef  PPP
#define PPP(a) (100.f*float(a)/float(RenderTOTAL.result))
			F.OutNext("*** RENDER:  %2.2fms", RenderTOTAL.result);
			F.OutNext("R_CALC:      %2.2fms, %2.1f%%", RenderCALC.result, PPP(RenderCALC.result));
			F.OutNext("  HOM:       %2.2fms, %d", RenderCALC_HOM.result, RenderCALC_HOM.count);
			F.OutNext("  Skeletons: %2.2fms, %d", Animation.result, Animation.count);
			F.OutNext("R_DUMP:      %2.2fms, %2.1f%%", RenderDUMP.result, PPP(RenderDUMP.result));
			F.OutNext("  Wait-L:    %2.2fms", RenderDUMP_Wait.result);
			F.OutNext("  Wait-S:    %2.2fms", RenderDUMP_Wait_S.result);
			F.OutNext("  Skinning:  %2.2fms", RenderDUMP_SKIN.result);
			F.OutNext("  DT_Vis/Cnt:%2.2fms/%d", RenderDUMP_DT_VIS.result, RenderDUMP_DT_Count);
			F.OutNext("  DT_Render: %2.2fms", RenderDUMP_DT_Render.result);
			F.OutNext("  DT_Cache:  %2.2fms", RenderDUMP_DT_Cache.result);
			F.OutNext("  Wallmarks: %2.2fms, %d/%d - %d", RenderDUMP_WM.result, RenderDUMP_WMS_Count, RenderDUMP_WMD_Count, RenderDUMP_WMT_Count);
			F.OutNext("  Glows:     %2.2fms", RenderDUMP_Glows.result);
			F.OutNext("  Lights:    %2.2fms, %d", RenderDUMP_Lights.result, RenderDUMP_Lights.count);
			F.OutNext("  RT:        %2.2fms, %d", RenderDUMP_RT.result, RenderDUMP_RT.count);
			F.OutNext("  HUD:       %2.2fms", RenderDUMP_HUD.result);
			F.OutNext("  P_calc:    %2.2fms", RenderDUMP_Pcalc.result);
			F.OutNext("  S_calc:    %2.2fms", RenderDUMP_Scalc.result);
			F.OutNext("  S_render:  %2.2fms, %d", RenderDUMP_Srender.result, RenderDUMP_Srender.count);
			F.OutSkip();
			F.OutNext("*** SOUND:   %2.2fms", Sound.result);
			F.OutNext("  TGT/SIM/E: %d/%d/%d", snd_stat._rendered, snd_stat._simulated, snd_stat._events);
			F.OutNext("  HIT/MISS:  %d/%d", snd_stat._cache_hits, snd_stat._cache_misses);
			F.OutSkip();
			F.OutNext("Input:       %2.2fms", Input.result);
			F.OutNext("clRAY:       %2.2fms, %d, %2.0fK", clRAY.result, clRAY.count, r_ps);
			F.OutNext("clBOX:       %2.2fms, %d, %2.0fK", clBOX.result, clBOX.count, b_ps);
			F.OutNext("clFRUSTUM:   %2.2fms, %d", clFRUSTUM.result, clFRUSTUM.count);
			F.OutSkip();
			F.OutNext("netClientRecv:   %2.2fms, %d", netClient1.result, netClient1.count);
			F.OutNext("netClientSend:   %2.2fms, %d", netClient2.result, netClient2.count);
			F.OutNext("netServer:   %2.2fms, %d", netServer.result, netServer.count);
			F.OutNext("netClientCompressor:   %2.2fms", netClientCompressor.result);
			F.OutNext("netServerCompressor:   %2.2fms", netServerCompressor.result);

			F.OutSkip();

			F.OutSkip();
			F.OutNext("TEST 0:      %2.2fms, %d", TEST0.result, TEST0.count);
			F.OutNext("TEST 1:      %2.2fms, %d", TEST1.result, TEST1.count);
			F.OutNext("TEST 2:      %2.2fms, %d", TEST2.result, TEST2.count);
			F.OutNext("TEST 3:      %2.2fms, %d", TEST3.result, TEST3.count);
#ifdef DEBUG_MEMORY_MANAGER
			F.OutSkip	();
			F.OutNext	("str: cmp[%3d], dock[%3d], qpc[%3d]",Memory.stat_strcmp,Memory.stat_strdock,CPU::qpc_counter);
			Memory.stat_strcmp	=	0		;
			Memory.stat_strdock	=	0		;
			CPU::qpc_counter	=	0		;
#else
			F.OutSkip();
			F.OutNext("qpc[%3d]", CPU::qpc_inquiry_counter);
			CPU::qpc_inquiry_counter = 0;
#endif
			F.OutSkip();
			m_pRender->OutData4(F);

			//////////////////////////////////////////////////////////////////////////
			// Renderer specific
			F.SetHeightI(f_base_size);
			F.OutSet(200, 0);
			Render->Statistics(&F);

			//////////////////////////////////////////////////////////////////////////
			// Game specific
			F.SetHeightI(f_base_size);

			//////////////////////////////////////////////////////////////////////////
			// process PURE STATS
			F.SetHeightI(f_base_size);
			seqStats.Process(rp_Stats);
			GCS_StatsFont_->OnRender();
#ifdef LA_SHADERS_DEBUG
			/////////////////////////////////////////////////////////////////////////
			// shaders constants debug
			F.SetHeightI						(f_base_size);
			F.OutSet							(1500,300);
			g_pConstantsDebug->OnRender			(pFont);
			pFont->OnRender						();
#endif
		}


		// The info on the right side of the screen

		bool draw_back = show_engine_timers || show_render_times;
		bool show_display = show_FPS_only || show_engine_timers || show_render_times || display_ram_usage || display_cpu_usage || showActorLuminocity_;

		float devicewidth = (float)Device.dwWidth;
		float deviceheight = (float)Device.dwHeight;

		if (draw_back) // draw only when needed
		{
			if (!back_shader) //init shader, if not already
			{
				back_shader = xr_new <FactoryPtr<IUIShader> >();
				(*back_shader)->create("hud\\default", "ui\\ui_console");
			}

			float posx, posx2;
			float fontstresh_f = 150 * (devicewidth / 1280) - 150;
			posx = (900.0f + fontstresh_f) * (devicewidth / 1280.f);
			posx2 = 1230.f * (devicewidth / 1280.f);

			back_region.set(posx, 0.0f, posx2, deviceheight);

			UIRender->SetShader(**back_shader);
			UIRender->StartPrimitive(6, IUIRender::ptTriList, IUIRender::pttTL);
			DrawRect(back_region, back_color);
			UIRender->FlushPrimitive();
		}

		if (show_display)
		{
			CGameFont& fpsFont = *FPSFont_;

			fpsFont.SetColor(statscolor1);
			fpsFont.SetAligment(CGameFont::alRight);

			float posx, posy;
			if (devicewidth / deviceheight > 1.5f)
			{
				posx = 1220.f * (devicewidth / 1280.f);
				posy = 10.f * (deviceheight / 768.f);
				fpsFont.SetHeightI(0.014);
			}
			else
			{
				posx = 980.f * (devicewidth / 1024.f);
				posy = 15.f * (deviceheight / 768.f);
				fpsFont.SetHeightI(0.014);
			}

			fpsFont.OutSet(posx, posy);

			//FPS Counter
			if (show_FPS_only)
			{
				//Delay update so that displaying of value does not refrech like crazy
				if (Device.rateControlingTimer_.GetElapsed_ms_f() - UpdateFPSCounterSkip > 100.f)
				{
					UpdateFPSCounterSkip = Device.rateControlingTimer_.GetElapsed_ms_f();
					iFPS = (int)fDeviceMeasuredFPS;
					fCounterTotalMinute += fDeviceMeasuredFPS / 10.f;
				}

				//Update Total/Minute and Avrege
				if (Device.rateControlingTimer_.GetElapsed_ms_f() - UpdateFPSMinute > 60000.f)
				{
					UpdateFPSMinute = Device.rateControlingTimer_.GetElapsed_ms_f();
					iTotalMinute = (int)fCounterTotalMinute;
					iAvrageMinute = (int)fCounterTotalMinute / 60;
					fCounterTotalMinute = 0;
				}

				fpsFont.OutNext("%i", iFPS);
				fpsFont.OutSkip();
				fpsFont.OutNext("A/MIN %i", iAvrageMinute);
				fpsFont.OutNext("T/MIN %i", iTotalMinute);
			}

			//RAM usage
			if (display_ram_usage)
			{
				fpsFont.OutSkip();
				fpsFont.OutNext("%u KB = Used RAM", GetTotalRAMConsumption() / 1024);
			}

			//CPU usage
			if (display_cpu_usage)
			{
				fpsFont.OutSkip();

				//Delay update so that displaying of value does not refrech like crazy
				tempCpuUsage_ += (float)cpuUsageHandler_.GetCurrentCPUUsage();
				tempCpuUsageFrames_++;
				float currnt_time = Device.rateControlingTimer_.GetElapsed_ms_f();

				if (CPU_UsageNextUpdate_ < currnt_time)
				{
					cpuUsagePrcentage_ = tempCpuUsage_ / tempCpuUsageFrames_;

					CPU_UsageNextUpdate_ = currnt_time + 200.f;
					tempCpuUsage_ = 0.f;
					tempCpuUsageFrames_ = 0;
				}

				fpsFont.OutNext("%u%% = CPU usage", (u32)cpuUsagePrcentage_);
			}

			//Engine timings
			if (show_engine_timers)
			{
				protectIndThread1_Stats.Enter();
				u32 ind_aux_thread_1_objects = indThread1_Objects_;
				float ind_aux_thread_1_time = indThread1_Time_;

				indThread1_Objects_ = 0;
				indThread1_Time_ = 0.f;
				protectIndThread1_Stats.Leave();

				protectIndThread2_Stats.Enter();
				u32 ind_aux_thread_2_objects = indThread2_Objects_;
				float ind_aux_thread_2_time = indThread2_Time_;

				indThread2_Objects_ = 0;
				indThread2_Time_ = 0.f;
				protectIndThread2_Stats.Leave();

				protectIndThread3_Stats.Enter();
				u32 ind_aux_thread_3_objects = indThread3_Objects_;
				float ind_aux_thread_3_time = indThread3_Time_;

				indThread3_Objects_ = 0;
				indThread3_Time_ = 0.f;
				protectIndThread3_Stats.Leave();

				protectResThread1_Stats.Enter();
				u32 res_thread_1_objects = resThread1_Objects_;
				float res_thread_1_time = resThread1_Time_;

				resThread1_Objects_ = 0;
				resThread1_Time_ = 0.f;
				protectResThread1_Stats.Leave();

				fpsFont.OutSkip();
				fpsFont.OutNext("Engine timers: Frame# %u", CurrentFrame());

				fpsFont.OutSkip();
				DisplayWithcolor(fpsFont, "Main Thread", statscolor1, mainThreadTime_, 25.f);
				bThreadReadyProtect_.Enter();
				DisplayWithcolor(fpsFont, "Aux Thread #1", statscolor1, thread1Time_, 10.f);
				DisplayWithcolor(fpsFont, "Aux Thread #2", statscolor1, thread2Time_, 10.f);
				DisplayWithcolor(fpsFont, "Aux Thread #3", statscolor1, thread3Time_, 10.f);
				DisplayWithcolor(fpsFont, "Aux Thread #4", statscolor1, thread4Time_, 10.f);
				DisplayWithcolor(fpsFont, "Aux Thread #5", statscolor1, thread5Time_, 10.f);
				bThreadReadyProtect_.Leave();
				DisplayWithcolor(fpsFont, "Ind. Aux Thread #1", statscolor1, ind_aux_thread_1_time, 10.f);
				DisplayWithcolor(fpsFont, "Ind. Aux Thread #2", statscolor1, ind_aux_thread_2_time, 10.f);
				DisplayWithcolor(fpsFont, "Ind. Aux Thread #3", statscolor1, ind_aux_thread_3_time, 10.f);
				DisplayWithcolor(fpsFont, "Res. Upload Thread #1", statscolor1, res_thread_1_time, 1.f);  // 1.f is to see how much it works

				fpsFont.OutNext("%u = Aux Thread #1 Objects", auxThread1Objects_);
				fpsFont.OutNext("%u = Aux Thread #2 Objects", auxThread2Objects_);
				fpsFont.OutNext("%u = Aux Thread #3 Objects", auxThread3Objects_);
				fpsFont.OutNext("%u = Aux Thread #4 Objects", auxThread4Objects_);
				fpsFont.OutNext("%u = Aux Thread #5 Objects", auxThread5Objects_);
				fpsFont.OutNext("%u = Ind. Aux Thread #1 Objects", ind_aux_thread_1_objects);
				fpsFont.OutNext("%u = Ind. Aux Thread #2 Objects", ind_aux_thread_2_objects);
				fpsFont.OutNext("%u = Ind. Aux Thread #3 Objects", ind_aux_thread_3_objects);
				fpsFont.OutNext("%u = Res. Upload Thread #1 Objects", res_thread_1_objects);

				fpsFont.OutNext("%i = Main Wait Aux #1 Counter", mainThreadWaitAux_1_);
				fpsFont.OutNext("%i = Main Wait Aux #2 Counter", mainThreadWaitAux_2_);
				fpsFont.OutNext("%i = Main Wait Aux #3 Counter", mainThreadWaitAux_3_);
				fpsFont.OutNext("%i = Main Wait Aux #4 Counter", mainThreadWaitAux_4_);
				fpsFont.OutNext("%i = Main Wait Aux #5 Counter", mainThreadWaitAux_5_);

				fpsFont.OutSkip();
				DisplayWithcolor(fpsFont, "Engine", statscolor1, EngineTOTAL.result, 5.f);
				DisplayWithcolor(fpsFont, "Render", statscolor1, RenderTOTAL.result, 22.f);

				fpsFont.OutSkip();
				DisplayWithcolor(fpsFont, "UpdateCL calls", statscolor1, UpdateClient.result, 4.f);
				DisplayWithcolor(fpsFont, "Scheduler calls", statscolor1, Sheduler.result, 4.f);

#ifdef MEASUR_XR_AREA
				if (g_pGameLevel)
				{
					DisplayWithcolor(fpsFont, "Area Tests  Static", statscolor1, g_pGameLevel->ObjectSpace.GetCalcTimeStatic()* 1000.f, 4.f);
					DisplayWithcolor(fpsFont, "Area Tests Dynamic", statscolor1, g_pGameLevel->ObjectSpace.GetCalcTimeDyn()* 1000.f, 4.f);

					DisplayWithcolor(fpsFont, "Area Tests SDelay Main Thread", statscolor1, g_pGameLevel->ObjectSpace.GetMainThreadStaticDelay() * 1000.f, 1.f);
					DisplayWithcolor(fpsFont, "Area Tests SDelay Sec Threads", statscolor1, g_pGameLevel->ObjectSpace.GetSThreadsStaticDelay() * 1000.f, 2.f);
					DisplayWithcolor(fpsFont, "Area Tests DDelay Main Thread", statscolor1, g_pGameLevel->ObjectSpace.GetMainThreadDynDelay() * 1000.f, 1.f);
					DisplayWithcolor(fpsFont, "Area Tests DDelay Sec Threads", statscolor1, g_pGameLevel->ObjectSpace.GetSThreadsDynDelay() * 1000.f, 2.f);

					g_pGameLevel->ObjectSpace.SetCalcTimeDyn(0.f);
					g_pGameLevel->ObjectSpace.SetCalcTimeStatic(0.f);
					g_pGameLevel->ObjectSpace.SetMainThreadStaticDelay(0.f);
					g_pGameLevel->ObjectSpace.SetSThreadsStaticDelay(0.f);
					g_pGameLevel->ObjectSpace.SetMainThreadDynDelay(0.f);
					g_pGameLevel->ObjectSpace.SetSThreadsDynDelay(0.f);
				}
#endif

				fpsFont.OutSkip();

				float total_cpu_waited_gpu_results = viewPortStats1.GPU_Sync_Point + viewPortStats1.GPU_Occ_Wait + viewPortStats2.GPU_Sync_Point + viewPortStats2.GPU_Occ_Wait;
				DisplayWithcolor(fpsFont, "Total CPU Waited GPU", statscolor1, total_cpu_waited_gpu_results, 2.f);

				//Logging
				if (log_engine_times)
				{
					Msg("#Engine timers: frame# %u", CurrentFrame());

					Msg("%fms = Main Thread", mainThreadTime_);
					bThreadReadyProtect_.Enter();
					Msg("%fms = Aux Thread #1", thread1Time_);
					Msg("%fms = Aux Thread #2", thread2Time_);
					Msg("%fms = Aux Thread #3", thread3Time_);
					Msg("%fms = Aux Thread #4", thread4Time_);
					Msg("%fms = Aux Thread #5", thread5Time_);
					bThreadReadyProtect_.Leave();
					Msg("%fms = Ind. Aux Thread #1", ind_aux_thread_1_time);
					Msg("%fms = Ind. Aux Thread #2", ind_aux_thread_2_time);
					Msg("%fms = Ind. Aux Thread #3", ind_aux_thread_3_time);
					Msg("%fms = Res. Upload Thread #1", res_thread_1_time);

					Msg("%i = Main Wait Aux #1 Counter", mainThreadWaitAux_1_);
					Msg("%i = Main Wait Aux #2 Counter", mainThreadWaitAux_2_);
					Msg("%i = Main Wait Aux #3 Counter", mainThreadWaitAux_3_);
					Msg("%i = Main Wait Aux #4 Counter", mainThreadWaitAux_4_);
					Msg("%i = Main Wait Aux #5 Counter", mainThreadWaitAux_5_);

					Msg("%u = Aux Thread #1 Objects", auxThread1Objects_);
					Msg("%u = Aux Thread #2 Objects", auxThread2Objects_);
					Msg("%u = Aux Thread #3 Objects", auxThread3Objects_);
					Msg("%u = Aux Thread #4 Objects", auxThread4Objects_);
					Msg("%u = Aux Thread #5 Objects", auxThread5Objects_);
					Msg("%u = Ind. Aux Thread #1 Objects", ind_aux_thread_1_objects);
					Msg("%u = Ind. Aux Thread #2 Objects", ind_aux_thread_2_objects);
					Msg("%u = Ind. Aux Thread #3 Objects", ind_aux_thread_3_objects);
					Msg("%u = Res. Upload Thread #1 Objects", res_thread_1_objects);

					Msg("%2.2fms = Engine", EngineTOTAL.result);
					Msg("%2.2fms = Render", RenderTOTAL.result);

					Msg("%2.2fms UpdateCL calls", UpdateClient.result);
					Msg("%2.2fms Scheduler calls", Sheduler.result);

					Msg("%fms = Total CPU Waited GPU", total_cpu_waited_gpu_results);
				}
			}

			//Render timings
			if (show_render_times)
			{
				for (u32 i = 0; i < VIEW_PORTS_CNT; ++i)
				{
					ViewPortRenderingStats& vstats = i == 0 ? viewPortStats1 : viewPortStats2;

					fpsFont.OutSkip();
					fpsFont.OutNext("Render timers(VP %u): Frame# %u", vstats.vpId, CurrentFrame());

					fpsFont.OutSkip();
					DisplayWithcolor(fpsFont, "Pre Render", statscolor1, vstats.preRender_, 0.5f);
					DisplayWithcolor(fpsFont, "GPU Sync Point", statscolor1, vstats.GPU_Sync_Point, 0.5f);
					DisplayWithcolor(fpsFont, "Pre Calc", statscolor1, vstats.preCalc_, 0.5f);
					DisplayWithcolor(fpsFont, "Main Geom Vis", statscolor1, vstats.mainGeomVis_, 3.f);
					DisplayWithcolor(fpsFont, "Main Geom Draw", statscolor1, vstats.mainGeomDraw_, 3.f);
					DisplayWithcolor(fpsFont, "Complex Stage", statscolor1, vstats.complexStage1_, 0.5f);
					DisplayWithcolor(fpsFont, "Details Draw", statscolor1, vstats.detailsDraw_, 1.f);
					DisplayWithcolor(fpsFont, "HUD Items UI", statscolor1, vstats.hudItemsWorldUI_, 0.5f);
					DisplayWithcolor(fpsFont, "Wallmarks Draw", statscolor1, vstats.wallmarksDraw_, 0.5f);
					DisplayWithcolor(fpsFont, "MSAA Edges", statscolor1, vstats.MSAAEdges_, 0.5f);
					DisplayWithcolor(fpsFont, "Wet Surfaces", statscolor1, vstats.wetSurfacesDraw_, 0.3f);
					DisplayWithcolor(fpsFont, "Sun SM Draw", statscolor1, vstats.sunSmDraw_, 1.5f);
					DisplayWithcolor(fpsFont, "Em Geom Draw", statscolor1, vstats.emissiveGeom_, 0.5f);
					DisplayWithcolor(fpsFont, "Lights Normal", statscolor1, vstats.lightsNormalTime_, 2.f);
					DisplayWithcolor(fpsFont, "Lights Pending", statscolor1, vstats.lightsPendingTime_, 0.5f);
					DisplayWithcolor(fpsFont, "Combiner | 2nd Geom", statscolor1, vstats.combinerAnd2ndGeom_, 1.0f);
					DisplayWithcolor(fpsFont, "Post Calc", statscolor1, vstats.postCalc_, 0.5f);
					DisplayWithcolor(fpsFont, "Mt wait", statscolor1, vstats.RenderMtWait, 0.2f);
					DisplayWithcolor(fpsFont, "GPU_Occ_Wait", statscolor1, vstats.GPU_Occ_Wait, 0.2f);
					DisplayWithcolor(fpsFont, "Render Total", statscolor1, vstats.RenderTotalTime, 12.f);

#if 0 // occ pool debug
					fpsFont.OutSkip();
					fpsFont.OutNext("Occlusion Debug:");
					fpsFont.OutNext("%u = Occ_pool_size", occ_pool_size);
					fpsFont.OutNext("%u = Occ_used_size", occ_used_size);
					fpsFont.OutNext("%u = Occ_freedids_size", occ_freed_ids_size);
#endif
					//Logging
					if (log_render_times)
					{
						Msg("Frame# = %u", CurrentFrame());
						Msg("%fms = Pre Render", vstats.preRender_);
						Msg("%fms = Pre Calc", vstats.preCalc_);
						Msg("%fms = GPU Sync Point", vstats.GPU_Sync_Point);
						Msg("%fms = Main Geom Vis", vstats.mainGeomVis_);
						Msg("%fms = Main Geom Draw", vstats.mainGeomDraw_);
						Msg("%fms = Complex Stage 1", vstats.complexStage1_);
						Msg("%fms = Details Draw", vstats.detailsDraw_);
						Msg("%fms = HUD Items UI", vstats.hudItemsWorldUI_);
						Msg("%fms = Wallmarks Draw", vstats.wallmarksDraw_);
						Msg("%fms = MSAA Edges", vstats.MSAAEdges_);
						Msg("%fms = Wet Surfaces", vstats.wetSurfacesDraw_);
						Msg("%fms = Sun SM Draw", vstats.sunSmDraw_);
						Msg("%fms = Em Geom Draw", vstats.emissiveGeom_);
						Msg("%fms = Lights Normal", vstats.lightsNormalTime_);
						Msg("%fms = Lights Pending", vstats.lightsPendingTime_);
						Msg("%fms = Combiner | 2nd Geom", vstats.combinerAnd2ndGeom_);
						Msg("%fms = Post Calc", vstats.postCalc_);
						Msg("%fms = Mt wait", vstats.RenderMtWait);
						Msg("%fms = GPU_Occ_Wait", vstats.GPU_Occ_Wait);
						Msg("%fms = Render Total", vstats.RenderTotalTime);

						Msg("Occlusion Debug:");
						Msg("%u = Occ_Bgn_calls", occ_Begin_calls);
						Msg("%u = Occ_End_calls", occ_End_calls);
						Msg("%u = Occ_Get_calls", occ_Get_calls);
						occ_Begin_calls = occ_End_calls = occ_Get_calls = 0;
					}

					vstats.RenderMtWait = 0.f;
				}
			}

			viewPortStats1.GPU_Occ_Wait = 0.f;
			viewPortStats2.GPU_Occ_Wait = 0.f;

			// Debug actor stelth
			if (showActorLuminocity_)
			{
				fpsFont.OutSkip();
				fpsFont.OutNext("Actor Lum %f", fActor_Lum);
			}

			FPSFont_->OnRender();
		}

		if (displayFrameGraph)
		{
			frameTimeGraph->PushBack(FrameTimeDelta() * 1000.f);
			frameTimeGraph->Draw();
		}
	};

	SecondBlockDrawRectAndSetFontPos();

	ShowUpdateCLTimes();
	ShowScUpdateTimes();
	ShowMultithredingTimes();
	ShowMTRenderDelays();
	ShowOnFrameTimes();

	RenderSecondBlock();

	if (psDeviceFlags.test(rsCameraPos))
	{
		_draw_cam_pos(GCS_StatsFont_);
		GCS_StatsFont_->OnRender();
	};

	{
		EngineTOTAL.FrameStart		();	
		Sheduler.FrameStart			();	
		UpdateClient.FrameStart		();	
		Physics.FrameStart			();	
		ph_collision.FrameStart		();
		ph_core.FrameStart			();
		Animation.FrameStart		();	
		AI_Think.FrameStart			();
		AI_Range.FrameStart			();
		AI_Path.FrameStart			();
		AI_Node.FrameStart			();
		AI_Vis.FrameStart			();
		AI_Vis_Query.FrameStart		();
		AI_Vis_RayTests.FrameStart	();
		
		RenderTOTAL.FrameStart		();
		RenderCALC.FrameStart		();
		RenderCALC_HOM.FrameStart	();
		RenderDUMP.FrameStart		();	
		RenderDUMP_RT.FrameStart	();
		RenderDUMP_SKIN.FrameStart	();	
		RenderDUMP_Wait.FrameStart	();	
		RenderDUMP_Wait_S.FrameStart();	
		RenderDUMP_HUD.FrameStart	();	
		RenderDUMP_Glows.FrameStart	();	
		RenderDUMP_Lights.FrameStart();	
		RenderDUMP_WM.FrameStart	();	
		RenderDUMP_DT_VIS.FrameStart();	
		RenderDUMP_DT_Render.FrameStart();	
		RenderDUMP_DT_Cache.FrameStart();	
		RenderDUMP_Pcalc.FrameStart	();	
		RenderDUMP_Scalc.FrameStart	();	
		RenderDUMP_Srender.FrameStart();	
		
		Sound.FrameStart			();
		Input.FrameStart			();
		clRAY.FrameStart			();	
		clBOX.FrameStart			();
		clFRUSTUM.FrameStart		();
		
		netClient1.FrameStart		();
		netClient2.FrameStart		();
		netServer.FrameStart		();
		netClientCompressor.FrameStart();
		netServerCompressor.FrameStart();

		TEST0.FrameStart			();
		TEST1.FrameStart			();
		TEST2.FrameStart			();
		TEST3.FrameStart			();

		g_SpatialSpace->stat_insert.FrameStart		();
		g_SpatialSpace->stat_remove.FrameStart		();

		g_SpatialSpacePhysic->stat_insert.FrameStart();
		g_SpatialSpacePhysic->stat_remove.FrameStart();
	}
	dwSND_Played = dwSND_Allocated = 0;
	Particles_starting = Particles_active = Particles_destroy = 0;
}

void	_LogCallback				(LPCSTR string)
{
	if (string && '!'==string[0] && ' '==string[1])
		Device.Statistic->errors.push_back	(shared_str(string));
}

void CStats::OnDeviceCreate			()
{
	g_bDisableRedText				= strstr(Core.Params,"-xclsx")?TRUE:FALSE;

	GCS_StatsFont_		= xr_new <CGameFont>("stat_font", CGameFont::fsDeviceIndependent);
	FPSFont_			= xr_new <CGameFont>("hud_font_di2", CGameFont::fsDeviceIndependent);
	secondBlockFont_	= xr_new <CGameFont>("hud_font_di2", CGameFont::fsDeviceIndependent);

	if(!pSettings->section_exist("evaluation")
		||!pSettings->line_exist("evaluation","line1")
		||!pSettings->line_exist("evaluation","line2")
		||!pSettings->line_exist("evaluation","line3") )
		FATAL	("CStats::OnDeviceCreate()");

	thread1Time_ = -1.f;
	thread2Time_ = -1.f;
	thread3Time_ = -1.f;
	thread4Time_ = -1.f;
	thread5Time_ = -1.f;
	indThread1_Time_ = -1.f;
	indThread2_Time_ = -1.f;
	indThread3_Time_ = -1.f;
	resThread1_Time_ = -1.f;

	mainThreadTime_ = -1.f;

	mainThreadWaitAux_1_ = 0;
	mainThreadWaitAux_2_ = 0;
	mainThreadWaitAux_3_ = 0;
	mainThreadWaitAux_4_ = 0;
	mainThreadWaitAux_5_ = 0;
	indThread1_Objects_ = 0;
	indThread2_Objects_ = 0;
	indThread3_Objects_ = 0;
	resThread1_Objects_ = 0;

	auxThread1Objects_ = 0;
	auxThread2Objects_ = 0;
	auxThread3Objects_ = 0;
	auxThread4Objects_ = 0;
	auxThread5Objects_ = 0;

	viewPortStats1.vpId = 1;
	viewPortStats2.vpId = 2;

	UpdateFPSCounterSkip = 0.f;
	UpdateFPSMinute = 0.f;
	fCounterTotalMinute = 0.f;
	iTotalMinute = -1;
	iAvrageMinute = -1;
	iFPS = -1;
	fDeviceMeasuredFPS	= -1.f;

	occ_pool_size = 0;
	occ_used_size = 0;
	occ_freed_ids_size = 0;
	occ_Begin_calls = 0;
	occ_End_calls = 0;
	occ_Get_calls		= 0;

#ifdef	MEASURE_MT
	mtPhysicsTime_ = 0.f;
	mtSoundTime_ = 0.f;
	mtNetworkTime_ = 0.f;
	mtParticlesTime_ = 0.f;

	mtHOMCopy_ = 0.f;
	mtDetailsCopy_ = 0.f;

	mtLevelPathTimeCopy_ = 0.f;
	mtDetailPathTimeCopy_ = 0.f;
	mtMapTimeCopy_ = 0.f;
	mtObjectHandlerTimeCopy_ = 0.f;
	mtSoundPlayerTimeCopy_ = 0.f;
	mtAiVisionTimeCopy_ = 0.f;
	mtAIMiscTimeCopy_ = 0.f;
	mtBulletsTimeCopy_ = 0.f;
	mtLUA_GCTimeCopy_ = 0.f;
	mtLevelSoundsTimeCopy_ = 0.f;
	mtALifeTimeCopy_ = 0.f;
	mtIKinematicsTimeCopy_ = 0.f;
	mtLTrackTimeCopy_ = 0.f;
	mtVisStructTimeCopy_ = 0.f;
	mtMeshBonesCopy_ = 0.f;

	mtRenderDelayPrior0Copy_ = 0.f;
	mtRenderDelayPrior1Copy_ = 0.f;
	mtRenderDelayLightsCopy_ = 0.f;
	mtRenderDelaySunCopy_ = 0.f;
	mtRenderDelayRainCopy_ = 0.f;
	mtRenderDelayBonesCopy_ = 0.f;
	mtRenderDelayDetailsCopy_ = 0.f;
	mtRenderDelayHomCopy_ = 0.f;

#endif

	Fvector2 top_left = { Device.dwWidth * 0.02f, Device.dwHeight * 0.60f };
	Fvector2 but_right = { Device.dwWidth * 0.35f, Device.dwHeight * 0.75f };

	frameTimeGraph = xr_new<StatDynGraph<float>>(600, top_left, but_right, Fvector2().set(1.f, 1.f), color_rgba(255, 5, 5, 255));

#ifdef LA_SHADERS_DEBUG
	g_pConstantsDebug = xr_new <CConstantsDebug>();
#endif
}

ViewPortRenderingStats::ViewPortRenderingStats()
{
	preRender_ = -1.f;
	preCalc_ = -1.f;
	GPU_Sync_Point = -1.f;
	GPU_Occ_Wait = -1.f;
	mainGeomVis_ = -1.f;
	mainGeomDraw_ = -1.f;
	complexStage1_ = -1.f;
	detailsDraw_ = -1.f;
	hudItemsWorldUI_ = -1.f;
	wallmarksDraw_ = -1.f;
	MSAAEdges_ = -1.f;
	wetSurfacesDraw_ = -1.f;
	sunSmDraw_ = -1.f;
	emissiveGeom_ = -1.f;
	lightsNormalTime_ = -1.f;
	lightsPendingTime_ = -1.f;
	combinerAnd2ndGeom_ = -1.f;
	postCalc_ = -1.f;
	RenderMtWait = -1.f;
	RenderTotalTime = -1.f;
}

void CStats::OnDeviceDestroy		()
{
	SetLogCB(NULL);

	xr_delete	(GCS_StatsFont_);
	xr_delete	(FPSFont_);
	xr_delete	(secondBlockFont_);

	xr_delete	(back_shader);
#ifdef LA_SHADERS_DEBUG
	xr_delete	(g_pConstantsDebug);
#endif
}

void CStats::OnRender				()
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;
}

//for total process ram usage
#include "windows.h"
#include "psapi.h"
#pragma comment( lib, "psapi.lib" )

u64 CStats::GetTotalRAMConsumption()
{
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

	u64 RAMUsed = pmc.WorkingSetSize;

	return RAMUsed;
}
