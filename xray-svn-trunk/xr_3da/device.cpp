#include "stdafx.h"
#include "../xrCDB/frustum.h"

#pragma warning(disable:4995)

#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY

#include <mmsystem.h>
#include <d3dx9.h>

#pragma warning(default:4995)

#include "x_ray.h"
#include "render.h"

// must be defined before include of FS_impl.h
#define INCLUDE_FROM_ENGINE
#include "../xrCore/FS_impl.h"
 
#include "igame_persistent.h"
#include "../Include/xrRender/UIRender.h"

#pragma comment(lib, "d3dx9.lib")

ENGINE_API CRenderDevice Device;
ENGINE_API CLoadScreenRenderer load_screen_renderer;
ENGINE_API CTimer loading_save_timer;

ENGINE_API bool loading_save_timer_started				= false;
ENGINE_API bool prefetching_in_progress					= false;

ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

ENGINE_API float VIEWPORT_NEAR							= 0.2f;

ref_light	precache_light								= 0;

extern BOOL mtUseCustomAffinity_;

extern void FreezeDetectingThread(void *ptr);


CRenderDevice::CRenderDevice()
	:
	m_pRender(0),

	auxThread_1_Allowed_(LA_EVENT_NAME_1),
	auxThread_1_Ready_(LA_EVENT_NAME_2),

	auxThread_2_Allowed_(LA_EVENT_NAME_6),
	auxThread_2_Ready_(LA_EVENT_NAME_7),

	auxThread_3_Allowed_(LA_EVENT_NAME_11),
	auxThread_3_Ready_(LA_EVENT_NAME_12),

	auxThread_4_Allowed_(LA_EVENT_NAME_14),
	auxThread_4_Ready_(LA_EVENT_NAME_15),

	auxThread_5_Allowed_(LA_EVENT_NAME_17),
	auxThread_5_Ready_(LA_EVENT_NAME_18),

	aux1ExitSync_(LA_EVENT_NAME_3),
	aux2ExitSync_(LA_EVENT_NAME_4),
	aux3ExitSync_(LA_EVENT_NAME_10),
	aux4ExitSync_(LA_EVENT_NAME_13),
	aux5ExitSync_(LA_EVENT_NAME_16),

	AuxIndependable1Exit_Sync(LA_EVENT_NAME_8),
	AuxIndependable2Exit_Sync(LA_EVENT_NAME_19),
	AuxIndependable3Exit_Sync(LA_EVENT_NAME_20),

	ResUploadingThread1Exit_Sync(LA_EVENT_NAME_21),

	SoundsThreadExit_Sync(LA_EVENT_NAME_5),

	freezeTreadExitSync_(LA_EVENT_NAME_9),

	IndAuxThreadWakeUp1(LA_EVENT_NAME_22),
	IndAuxThreadWakeUp2(LA_EVENT_NAME_23),
	IndAuxThreadWakeUp3(LA_EVENT_NAME_24)
{
	m_hWnd = NULL;
	b_is_Active = FALSE;
	b_is_Ready = FALSE;
	Timer.Start();
	m_bNearer = FALSE;
	//--#SM+#-- +SecondVP+
	m_SecondViewport.SetSVPActive(false);
	m_SecondViewport.SetSVPFrameDelay(1);
	m_SecondViewport.isCamReady = false;
};


CRenderDevice::~CRenderDevice()
{
	SetFreezeThreadMustExit(true);

	Sleep(5);

	freezeTreadExitSync_.Wait(3000);
}

bool CRenderDevice::IsR4Active()
{
	return (psDeviceFlags.test(rsR4)) ? true : false;
}

void CRenderDevice::Clear	()
{
	m_pRender->Clear();
}

BOOL CRenderDevice::BeginRendering(ViewPort viewport)
{
	switch (m_pRender->GetDeviceState())
	{
	case IRenderDeviceRender::dsOK:
		break;

	case IRenderDeviceRender::dsLost:
		// If the device was lost, do not render until we get it back
		Sleep(100);
		return FALSE;
		break;

	case IRenderDeviceRender::dsNeedReset:
		// Check if the device is ready to be reset
		Reset();
		break;

	default:
		R_ASSERT(0);
	}

	m_pRender->RenderBegin(viewport);

	return		TRUE;
}


void CRenderDevice::FinishRendering(ViewPort viewport)
{
	if (dwPrecacheFrame)
	{
		::Sound->set_master_volume(0.f);
		dwPrecacheFrame--;

		if (dwPrecacheFrame == 0)
		{
			//Gamma.Update		();
			//m_pRender->updateGamma();

			if (precache_light)
			{
				precache_light->set_active(false);

				precache_light.destroy();
			}

			::Sound->set_master_volume(1.f);

			m_pRender->ResourcesDestroyNecessaryTextures();
			Memory.mem_compact();
			Msg("\n* End of synchronization A[%d] R[%d]", b_is_Active, b_is_Ready);
			Msg("* MEMORY USAGE: %d K", Statistic->GetTotalRAMConsumption() / 1024);

			if (loading_save_timer_started)
			{
				Msg("* Game Loading Time: %d ms\n", loading_save_timer.GetElapsed_ms());
				loading_save_timer_started = false;
			}

#ifdef FIND_CHUNK_BENCHMARK_ENABLE
			g_find_chunk_counter.flush();
#endif
			
			WINDOWINFO	wi;
			GetWindowInfo(m_hWnd,&wi);

			if(wi.dwWindowStatus!=WS_ACTIVECAPTION)
				Pause(TRUE,TRUE,TRUE,"application start");
		}
	}

	m_pRender->RenderFinish(viewport);
}


#include "igame_level.h"
void CRenderDevice::PreCache	(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input)
{
	if (m_pRender->GetForceGPU_REF()) amount=0;

	// Msg			("* PCACHE: start for %d...",amount);
	dwPrecacheFrame	= dwPrecacheTotal = amount;
	if (amount && !precache_light && g_pGameLevel && g_loading_events.empty()) {
		precache_light					= ::Render->light_create();
		precache_light->set_shadow		(false);
		precache_light->set_position	(vCameraPosition);
		precache_light->set_color		(255,255,255);
		precache_light->set_range		(5.0f);
		precache_light->set_active		(true);
	}

	if(amount && b_draw_loadscreen && load_screen_renderer.b_registered==false)
	{
		load_screen_renderer.start	(b_wait_user_input);
	}
}


void CRenderDevice::message_loop()
{
	if(CApplication::isDeveloperMode)
	b_is_Active = true;
	MSG msg;
	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}
		LoopFrames();
	}
}

void CRenderDevice::Run			()
{
	CTimer* engine_startin_timer = xr_new<CTimer>(); engine_startin_timer->Start();

	Msg("# Starting engine...");

	engineState.set(LEVEL_LOADED, false);

	// Startup timers and calculate timer delta
	timeGlobalU_				= 0;
	Timer_MM_Delta				= 0;

	previousFrameTime_			= 0.f;
	frameTimeDelta_				= 0.f;
	timeDelta_					= 0.f;

	rateControlingTimer_.Start();

	{
		u32 time_mm			= timeGetTime	();
		while (timeGetTime()==time_mm);			// wait for next tick
		u32 time_system		= timeGetTime	();
		u32 time_local		= TimerAsync	();
		Timer_MM_Delta		= time_system-time_local;
	}

	Msg("* Time %f ms \n", engine_startin_timer->GetElapsed_sec() * 1000.f);
	engine_startin_timer->Start();

	Msg("# Spawning AUX Threads...");

	// Start all threads
	SetAuxThreadsMustExit(false);
	SetFreezeThreadMustExit(false);

	thread_spawn				(AuxThread_1, "X-RAY AUX thread 1", 0, this);
	thread_spawn				(AuxThread_2, "X-RAY AUX thread 2", 0, this);
	thread_spawn				(AuxThread_3, "X-RAY AUX thread 3", 0, this);
	thread_spawn				(AuxThread_4, "X-RAY AUX thread 4", 0, this);
	thread_spawn				(AuxThread_5, "X-RAY AUX thread 5", 0, this);
	thread_spawn				(AuxThread_Independable_1, "X-RAY 'Render' AUX thread 1", 0, this);
	thread_spawn				(AuxThread_Independable_2, "X-RAY 'Render' AUX thread 2", 0, this);
	thread_spawn				(AuxThread_Independable_3, "X-RAY 'Render' AUX thread 3", 0, this);
	thread_spawn				(ResourceUploadingThread_1, "X-RAY 'Res Uploading' thread 1", 0, this);	// For uplading delyaed resources

	thread_spawn				(FreezeDetectingThread, "Freeze detecting thread", 0, 0);

	Msg("* Time %f ms \n", engine_startin_timer->GetElapsed_sec() * 1000.f);
	engine_startin_timer->Start();

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	if (mtUseCustomAffinity_)
	{
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetMainThreadBestAffinity());

		Msg("* Main thread cpu affinity is %u", CPU::GetMainThreadBestAffinity());
		Msg("* Secondary threads cpu affinity is %u", CPU::GetSecondaryThreadBestAffinity());
	}

	FlushLog(false);

	// Message cycle

	seqAppStart.Process			(rp_AppStart);

	Msg("* Time %f ms \n", engine_startin_timer->GetElapsed_sec() * 1000.f);
	engine_startin_timer->Start();

	UIRender->CreateUIGeom();
	m_pRender->ClearTarget();

	Msg("* Time %f ms \n", engine_startin_timer->GetElapsed_sec() * 1000.f);
	xr_delete(engine_startin_timer);

	message_loop				();

	UIRender->DestroyUIGeom();
	
	seqAppEnd.Process(rp_AppEnd);

	// Stop Aux-Thread
	SetAuxThreadsMustExit(true);

	auxThread_1_Allowed_.Set();
	auxThread_2_Allowed_.Set();
	auxThread_3_Allowed_.Set();
	auxThread_4_Allowed_.Set();
	auxThread_5_Allowed_.Set();

	aux1ExitSync_.Wait(3000);
	aux2ExitSync_.Wait(3000);
	aux3ExitSync_.Wait(3000);
	aux4ExitSync_.Wait(3000);
	aux5ExitSync_.Wait(3000);

	AuxIndependable1Exit_Sync.Wait(3000);
	AuxIndependable2Exit_Sync.Wait(3000);
	AuxIndependable3Exit_Sync.Wait(3000);

	ResUploadingThread1Exit_Sync.Wait(3000);

	SoundsThreadExit_Sync.Wait(3000);
}

u32 app_inactive_time		= 0;
u32 app_inactive_time_start = 0;

void CRenderDevice::FrameMove()
{
	protectCurrentFrame_.Enter();
	currentFrame_++;
	protectCurrentFrame_.Leave();

	SetEngineTimeContinual(TimerMM.GetElapsed_ms() - app_inactive_time);

	if (psDeviceFlags.test(rsConstantFPS))
	{
		SetTimeDelta(0.033f);

		float time = EngineTime();
		SetEngineTime (time + 0.033f);

		SetTimeDeltaU(33);

		u32 time_u = EngineTimeU();
		SetEngineTimeU(time_u + 33);
	}
	else
	{
		// Timer
		float fPreviousFrameTime = Timer.GetElapsed_sec(); Timer.Start();	// previous frame

		float delta = TimeDelta();

		SetTimeDelta(0.1f * delta + 0.9f * fPreviousFrameTime);				// smooth random system activity - worst case ~7% error

		if (TimeDelta() > 0.1f)    
			SetTimeDelta(0.1f);							// limit to 15fps minimum

		if (TimeDelta() <= 0.f)
			SetTimeDelta(EPS_S + EPS_S);				// limit to 15fps minimum

		if(Paused())	
			SetTimeDelta(0.0f);

		SetEngineTime(TimerGlobal.GetElapsed_sec());

		u32	_old_global = EngineTimeU();

		SetEngineTimeU(TimerGlobal.GetElapsed_ms());

		SetTimeDeltaU(EngineTimeU() - _old_global);
	}
}

ENGINE_API BOOL bShowPauseString = TRUE;
#include "IGame_Persistent.h"

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
	static int snd_emitters_ = -1;

	if(bOn)
	{
		if(!Paused())						
			bShowPauseString				= 
#ifdef DEBUG
				!xr_strcmp(reason, "li_pause_key_no_clip")?	FALSE:
#endif // DEBUG
				TRUE;

		if( bTimer && (!g_pGamePersistent || g_pGamePersistent->CanBePaused()) )
		{
			g_pauseMngr.Pause				(TRUE);
#ifdef DEBUG
			if(!xr_strcmp(reason, "li_pause_key_no_clip"))
				TimerGlobal.Pause				(FALSE);
#endif // DEBUG
		}
	
		if (bSound && ::Sound) {
			snd_emitters_ =					::Sound->pause_emitters(true);
#ifdef DEBUG
//			Log("snd_emitters_[true]",snd_emitters_);
#endif // DEBUG
		}
	}else
	{
		if( bTimer && g_pauseMngr.Paused() )
		{
			SetTimeDelta(EPS_S + EPS_S);

			g_pauseMngr.Pause(FALSE);
		}
		
		if(bSound)
		{
			if(snd_emitters_>0) //avoid crash
			{
				snd_emitters_ =				::Sound->pause_emitters(false);
#ifdef DEBUG
//				Log("snd_emitters_[false]",snd_emitters_);
#endif // DEBUG
			}else {
#ifdef DEBUG
				Log("Sound->pause_emitters underflow");
#endif // DEBUG
			}
		}
	}


}


BOOL CRenderDevice::Paused()
{
	return g_pauseMngr.Paused();
};


void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM lParam)
{
	if(CApplication::isDeveloperMode)return;
	u16 fActive						= LOWORD(wParam);
	BOOL fMinimized					= (BOOL) HIWORD(wParam);
	BOOL bActive					= ((fActive!=WA_INACTIVE) && (!fMinimized))?TRUE:FALSE;
	
	if (bActive!=Device.b_is_Active)
	{
		Device.b_is_Active			= bActive;

		if (Device.b_is_Active)	
		{
			Device.seqAppActivate.Process(rp_AppActivate);
			app_inactive_time		+= TimerMM.GetElapsed_ms() - app_inactive_time_start;

			ShowCursor			(FALSE);
		}else	
		{
			app_inactive_time_start	= TimerMM.GetElapsed_ms();
			Device.seqAppDeactivate.Process(rp_AppDeactivate);
			ShowCursor				(TRUE);
		}
	}
}


void	CRenderDevice::AddSeqFrame			( pureFrame* f, bool mt )
{
	if ( mt )	
		seqFrameMT.Add		(f,REG_PRIORITY_HIGH);
	else								
		seqFrame.Add		(f,REG_PRIORITY_LOW);

}


void CRenderDevice::RemoveSeqFrame(pureFrame* f)
{
	seqFrameMT.Remove	( f );
	seqFrame.Remove		( f );
}


CLoadScreenRenderer::CLoadScreenRenderer()
:b_registered(false)
{}


void CLoadScreenRenderer::start(bool b_user_input) 
{
	Device.seqRender.Add			(this, 0);
	b_registered					= true;
	b_need_user_input				= b_user_input;
}


void CLoadScreenRenderer::stop()
{
	if(!b_registered)				return;
	Device.seqRender.Remove			(this);
	pApp->destroy_loading_shaders	();
	b_registered					= false;
	b_need_user_input				= false;
}


void CLoadScreenRenderer::OnRender() 
{
	pApp->load_draw_internal();
}

void CSecondVPParams::SetSVPActive(bool bState) //--#SM+#-- +SecondVP+
{
	isActive = bState;
}

bool CSecondVPParams::IsSVPFrame() //--#SM+#-- +SecondVP+
{
	return (CurrentFrame() % GetSVPFrameDelay() == 0);
}