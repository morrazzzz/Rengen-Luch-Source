#include "stdafx.h"
#include "device.h"
#include "x_ray.h"
#include "igame_persistent.h"

#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)

ENGINE_API bool checkFreezing_ = true;

ENGINE_API Mutex GameSaveMutex;

extern ENGINE_API BOOL debugSecondViewPort;

AccessLock mustExitProtect_;
AccessLock freezeMustExitProtect_;

AccessLock	syncThreads_Freeze_and_Main_;

CTimer mainThreadTimer_, thread1Timer_, thread2Timer_, thread3Timer_, thread4Timer_, thread5Timer_, engineFreezeTimer_;

float auxThread1TimeDeffered_ = 0.f, auxThread2TimeDeffered_ = 0.f, auxThread3TimeDeffered_ = 0.f, auxThread4TimeDeffered_ = 0.f, auxThread5TimeDeffered_ = 0.f;
u32 auxThread1ObjectsDeffered_ = 0, auxThread2ObjectsDeffered_ = 0, auxThread3ObjectsDeffered_ = 0, auxThread4ObjectsDeffered_ = 0, auxThread5ObjectsDeffered_ = 0;

// to measure waiting count
bool thread1Ready_ = false; 
bool thread2Ready_ = false;
bool thread3Ready_ = false;
bool thread4Ready_ = false;
bool thread5Ready_ = false;

AccessLock	bThreadReadyProtect_;

float ps_frames_per_sec = 144.f;

extern BOOL mtUseCustomAffinity_;

void CRenderDevice::SetAuxThreadsMustExit(bool val)
{
	mustExitProtect_.Enter();
	auxTreadsMustExit_ = val;
	mustExitProtect_.Leave();
}

bool CRenderDevice::IsAuxThreadsMustExit()
{
	mustExitProtect_.Enter();
	bool ress = auxTreadsMustExit_;
	mustExitProtect_.Leave();

	return ress;
}


void CRenderDevice::SetFreezeThreadMustExit(bool val)
{
	freezeMustExitProtect_.Enter();
	freezeThreadExit_ = val;
	freezeMustExitProtect_.Leave();
}

bool CRenderDevice::IsFreezeThreadMustExit()
{
	freezeMustExitProtect_.Enter();
	bool ress = freezeThreadExit_;
	freezeMustExitProtect_.Leave();

	return ress;
}


//---------- Auxilary thread that executes delayed stuff while Main thread is buisy with Render (1)
void CRenderDevice::AuxThread_1(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		device.auxThread_1_Allowed_.Wait(); // Wait until main thread allows aux thread 1 to execute

		if (device.IsAuxThreadsMustExit())
		{
			device.aux1ExitSync_.Set();

			return;
		}

		// we are ok to execute
		thread1Timer_.Start();

		thread1Ready_ = false;

		device.AuxPool_1_Protection_.Enter(); //Protect pool, while copying it to temporary one

		auxThread1ObjectsDeffered_ = device.auxThreadPool_1_.size() + device.seqFrameMT.R.size();

		// make a copy of corrent pool, so that the access to pool is not raced by threads
		device.AuxThreadPool_1_TempCopy_ = device.auxThreadPool_1_;
		device.auxThreadPool_1_.clear_not_free();

		device.AuxPool_1_Protection_.Leave();

		GameSaveMutex.lock();
		for (u32 pit = 0; pit < device.AuxThreadPool_1_TempCopy_.size(); pit++)
		{
			device.AuxThreadPool_1_TempCopy_[pit]();
		}

		device.seqFrameMT.Process(rp_Frame);

		GameSaveMutex.unlock();

		device.AuxThreadPool_1_TempCopy_.clear_not_free();

		bThreadReadyProtect_.Enter();
		auxThread1TimeDeffered_ = thread1Timer_.GetElapsed_sec() * 1000.f;
		thread1Ready_ = true;
		bThreadReadyProtect_.Leave();

		device.auxThread_1_Ready_.Set(); // tell main thread that aux thread 1 has finished its job
	}
}


//---------- Auxilary thread that executes delayed stuff while Main thread is buisy with Render (2)
void CRenderDevice::AuxThread_2(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		device.auxThread_2_Allowed_.Wait(); // Wait until main thread allows aux thread 2 to execute

		if (device.IsAuxThreadsMustExit())
		{
			device.aux2ExitSync_.Set();

			return;
		}

		// we are ok to execute
		thread2Timer_.Start();

		thread2Ready_ = false;

		device.AuxPool_2_Protection_.Enter(); //Protect pool, while copying it to temporary one

		auxThread2ObjectsDeffered_ = device.auxThreadPool_2_.size();

		// make a copy of corrent pool, so that the access to pool is not raced by threads
		device.AuxThreadPool_2_TempCopy_ = device.auxThreadPool_2_;
		device.auxThreadPool_2_.clear_not_free();

		device.AuxPool_2_Protection_.Leave();

		for (u32 pit = 0; pit < device.AuxThreadPool_2_TempCopy_.size(); pit++)
		{
			device.AuxThreadPool_2_TempCopy_[pit]();
		}

		device.AuxThreadPool_2_TempCopy_.clear_not_free();

		bThreadReadyProtect_.Enter();
		auxThread2TimeDeffered_ = thread2Timer_.GetElapsed_sec() * 1000.f;
		thread2Ready_ = true;
		bThreadReadyProtect_.Leave();

		device.auxThread_2_Ready_.Set(); // tell main thread that aux thread 2 has finished its job
	}
}


//---------- Auxilary thread that executes delayed stuff while Main thread is buisy with Render (3)
void CRenderDevice::AuxThread_3(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		device.auxThread_3_Allowed_.Wait(); // Wait until main thread allows aux thread 3 to execute

		if (device.IsAuxThreadsMustExit())
		{
			device.aux3ExitSync_.Set();

			return;
		}

		// we are ok to execute
		thread3Timer_.Start();

		thread3Ready_ = false;

		device.AuxPool_3_Protection_.Enter(); //Protect pool, while copying it to temporary one

		auxThread3ObjectsDeffered_ = device.auxThreadPool_3_.size();

		// make a copy of corrent pool, so that the access to pool is not raced by threads
		device.AuxThreadPool_3_TempCopy_ = device.auxThreadPool_3_;
		device.auxThreadPool_3_.clear_not_free();

		device.AuxPool_3_Protection_.Leave();

		for (u32 pit = 0; pit < device.AuxThreadPool_3_TempCopy_.size(); pit++)
		{
			device.AuxThreadPool_3_TempCopy_[pit]();
		}

		device.AuxThreadPool_3_TempCopy_.clear_not_free();

		bThreadReadyProtect_.Enter();
		auxThread3TimeDeffered_ = thread3Timer_.GetElapsed_sec() * 1000.f;
		thread3Ready_ = true;
		bThreadReadyProtect_.Leave();

		device.auxThread_3_Ready_.Set(); // tell main thread that aux thread 3 has finished its job
	}
}

//---------- Auxilary thread that executes delayed stuff while Main thread is buisy with Render (4)
void CRenderDevice::AuxThread_4(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		device.auxThread_4_Allowed_.Wait(); // Wait until main thread allows aux thread 4 to execute

		if (device.IsAuxThreadsMustExit())
		{
			device.aux4ExitSync_.Set();

			return;
		}

		// we are ok to execute
		thread4Timer_.Start();

		thread4Ready_ = false;

		device.AuxPool_4_Protection_.Enter(); //Protect pool, while copying it to temporary one

		auxThread4ObjectsDeffered_ = device.auxThreadPool_4_.size();

		// make a copy of corrent pool, so that the access to pool is not raced by threads
		device.AuxThreadPool_4_TempCopy_ = device.auxThreadPool_4_;
		device.auxThreadPool_4_.clear_not_free();

		device.AuxPool_4_Protection_.Leave();

		for (u32 pit = 0; pit < device.AuxThreadPool_4_TempCopy_.size(); pit++)
		{
			device.AuxThreadPool_4_TempCopy_[pit]();
		}

		device.AuxThreadPool_4_TempCopy_.clear_not_free();

		bThreadReadyProtect_.Enter();
		auxThread4TimeDeffered_ = thread4Timer_.GetElapsed_sec() * 1000.f;
		thread4Ready_ = true;
		bThreadReadyProtect_.Leave();

		device.auxThread_4_Ready_.Set(); // tell main thread that aux thread 4 has finished its job
	}
}

//---------- Auxilary thread that executes delayed stuff while Main thread is buisy with Render (5)
void CRenderDevice::AuxThread_5(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		device.auxThread_5_Allowed_.Wait(); // Wait until main thread allows aux thread 5 to execute

		if (device.IsAuxThreadsMustExit())
		{
			device.aux5ExitSync_.Set();

			return;
		}

		// we are ok to execute
		thread5Timer_.Start();

		thread5Ready_ = false;

		device.AuxPool_5_Protection_.Enter(); //Protect pool, while copying it to temporary one

		auxThread5ObjectsDeffered_ = device.auxThreadPool_5_.size();

		// make a copy of corrent pool, so that the access to pool is not raced by threads
		device.AuxThreadPool_5_TempCopy_ = device.auxThreadPool_5_;
		device.auxThreadPool_5_.clear_not_free();

		device.AuxPool_5_Protection_.Leave();

		for (u32 pit = 0; pit < device.AuxThreadPool_5_TempCopy_.size(); pit++)
		{
			device.AuxThreadPool_5_TempCopy_[pit]();
		}

		device.AuxThreadPool_5_TempCopy_.clear_not_free();

		bThreadReadyProtect_.Enter();
		thread5Ready_ = true;
		bThreadReadyProtect_.Leave();

		auxThread5TimeDeffered_ = thread5Timer_.GetElapsed_sec() * 1000.f;
		device.auxThread_5_Ready_.Set(); // tell main thread that aux thread 5 has finished its job
	}
}

//---------- Auxilary thread that executes delayed stuff independantly from Main thread (1)
void CRenderDevice::AuxThread_Independable_1(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		device.IndAuxThreadWakeUp1.Wait(1);

		if (device.IsAuxThreadsMustExit()) // exit, if needed
		{
			device.AuxIndependable1Exit_Sync.Set();

			return;
		}

		device.IndAuxProcessingProtection_1.Enter();

		device.IndAuxPoolProtection_1.Enter();

		u32 workload_size = device.independableAuxThreadPool_1_.size();
		bool usefull_to_process = workload_size > 0;
	
		device.IndAuxPoolProtection_1.Leave();

		if (usefull_to_process)
		{
			device.IndAuxPoolProtection_1.Enter(); //Protect pool, while copying it to temporary one

			// make a copy of corrent pool, so that the access to pool is not raced by threads
			device.independableAuxThreadPool_1_TempCopy_ = device.independableAuxThreadPool_1_;

			device.independableAuxThreadPool_1_.clear_not_free();

			device.IndAuxPoolProtection_1.Leave();

			float processing_time = 0.f;

			CTimer time; time.Start();

			for (u32 i = 0; i < device.independableAuxThreadPool_1_TempCopy_.size(); ++i)
			{
				//Msg("# Processeing %u", i + 1);

				device.independableAuxThreadPool_1_TempCopy_[i]();
			}

			processing_time = time.GetElapsed_sec() * 1000.f;

			device.independableAuxThreadPool_1_TempCopy_.clear_not_free();

			device.Statistic->protectIndThread1_Stats.Enter();
			device.Statistic->indThread1_Objects_ += workload_size;
			device.Statistic->indThread1_Time_ += processing_time;
			device.Statistic->protectIndThread1_Stats.Leave();
		}

		device.IndAuxProcessingProtection_1.Leave();
	}
}


//---------- Auxilary thread that executes delayed stuff independantly from Main thread (2)
void CRenderDevice::AuxThread_Independable_2(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		device.IndAuxThreadWakeUp2.Wait(1);

		if (device.IsAuxThreadsMustExit()) // exit, if needed
		{
			device.AuxIndependable2Exit_Sync.Set();

			return;
		}

		device.IndAuxProcessingProtection_2.Enter();

		device.IndAuxPoolProtection_2.Enter();

		u32 workload_size = device.independableAuxThreadPool_2_.size();
		bool usefull_to_process = workload_size > 0;

		device.IndAuxPoolProtection_2.Leave();

		if (usefull_to_process)
		{
			device.IndAuxPoolProtection_2.Enter(); //Protect pool, while copying it to temporary one

			// make a copy of corrent pool, so that the access to pool is not raced by threads
			device.independableAuxThreadPool_2_TempCopy_ = device.independableAuxThreadPool_2_;

			device.independableAuxThreadPool_2_.clear_not_free();

			device.IndAuxPoolProtection_2.Leave();

			float processing_time = 0.f;

			CTimer time; time.Start();

			for (u32 i = 0; i < device.independableAuxThreadPool_2_TempCopy_.size(); ++i)
			{
				//Msg("# Processeing %u", i + 1);

				device.independableAuxThreadPool_2_TempCopy_[i]();
			}

			processing_time = time.GetElapsed_sec() * 1000.f;

			device.independableAuxThreadPool_2_TempCopy_.clear_not_free();

			device.Statistic->protectIndThread2_Stats.Enter();
			device.Statistic->indThread2_Objects_ += workload_size;
			device.Statistic->indThread2_Time_ += processing_time;
			device.Statistic->protectIndThread2_Stats.Leave();
		}

		device.IndAuxProcessingProtection_2.Leave();
	}
}


//---------- Auxilary thread that executes delayed stuff independantly from Main thread (3)
void CRenderDevice::AuxThread_Independable_3(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		device.IndAuxThreadWakeUp3.Wait(1);

		if (device.IsAuxThreadsMustExit()) // exit, if needed
		{
			device.AuxIndependable3Exit_Sync.Set();

			return;
		}

		device.IndAuxProcessingProtection_3.Enter();

		device.IndAuxPoolProtection_3.Enter();

		u32 workload_size = device.independableAuxThreadPool_3_.size();
		bool usefull_to_process = workload_size > 0;

		device.IndAuxPoolProtection_3.Leave();

		if (usefull_to_process)
		{
			device.IndAuxPoolProtection_3.Enter(); //Protect pool, while copying it to temporary one

			// make a copy of corrent pool, so that the access to pool is not raced by threads
			device.independableAuxThreadPool_3_TempCopy_ = device.independableAuxThreadPool_3_;

			device.independableAuxThreadPool_3_.clear_not_free();

			device.IndAuxPoolProtection_3.Leave();

			float processing_time = 0.f;

			CTimer time; time.Start();

			for (u32 i = 0; i < device.independableAuxThreadPool_3_TempCopy_.size(); ++i)
			{
				//Msg("# Processeing %u", i + 1);

				device.independableAuxThreadPool_3_TempCopy_[i]();
			}

			processing_time = time.GetElapsed_sec() * 1000.f;

			device.independableAuxThreadPool_3_TempCopy_.clear_not_free();

			device.Statistic->protectIndThread3_Stats.Enter();
			device.Statistic->indThread3_Objects_ += workload_size;
			device.Statistic->indThread3_Time_ += processing_time;
			device.Statistic->protectIndThread3_Stats.Leave();
		}

		device.IndAuxProcessingProtection_3.Leave();
	}
}


//---------- Resource uploading thread, Can be used for textures, and other mt safe resource uploading
void CRenderDevice::ResourceUploadingThread_1(void *context)
{
	CRenderDevice& device = *static_cast<CRenderDevice*>(context);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		Sleep(1);

		if (device.IsAuxThreadsMustExit()) // exit, if needed
		{
			device.ResUploadingThread1Exit_Sync.Set();

			return;
		}

		device.resUploadingProcessingProtection_1.Enter();

		device.resUploadingPoolProtection_1.Enter();

		u32 workload_size = device.resourceUploadingThreadPool_1_.size();
		bool usefull_to_process = workload_size > 0;

		device.resUploadingPoolProtection_1.Leave();

		if (usefull_to_process)
		{
			device.resUploadingPoolProtection_1.Enter(); //Protect pool, while copying it to temporary one

			// make a copy of corrent pool, so that the access to pool is not raced by threads
			device.resourceUploadingThreadPool_1_TempCopy_ = device.resourceUploadingThreadPool_1_;

			device.resourceUploadingThreadPool_1_.clear_not_free();

			device.resUploadingPoolProtection_1.Leave();

			float processing_time = 0.f;

			CTimer time; time.Start();

			for (u32 i = 0; i < device.resourceUploadingThreadPool_1_TempCopy_.size(); ++i)
			{
				//Msg("# Processeing %u", i + 1);

				device.resourceUploadingThreadPool_1_TempCopy_[i]();
			}

			processing_time = time.GetElapsed_sec() * 1000.f;

			device.resourceUploadingThreadPool_1_TempCopy_.clear_not_free();

			device.Statistic->protectResThread1_Stats.Enter();
			device.Statistic->resThread1_Objects_ += workload_size;
			device.Statistic->resThread1_Time_ += processing_time;
			device.Statistic->protectResThread1_Stats.Leave();
		}

		device.resUploadingProcessingProtection_1.Leave();
	}
}

//---------- Freeze Detecting Thread
void FreezeDetectingThread(void *ptr)
{
	u16 freezetime = 0;
	u16 repeatcheck = 500;
	engineFreezeTimer_.Start(); // initialization

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		if (checkFreezing_)
		{
			if (g_loading_events.size())
				freezetime = 7000;
			else
				freezetime = 3000;

			syncThreads_Freeze_and_Main_.Enter();
			u32 elapsedtime = engineFreezeTimer_.GetElapsed_ms();
			engineFreezeTimer_.Start();
			syncThreads_Freeze_and_Main_.Leave();

			if (elapsedtime > freezetime)
			{
				if (IsLogFilePresentMT())
				{
					Msg("---Engine is freezed, saving log [%u]", elapsedtime);
					FlushLog();
				}
			}
		}

		//Exit self, if main thread said so
		if (Device.IsFreezeThreadMustExit())
		{
			Device.freezeTreadExitSync_.Set();

			return;
		}

		Sleep(repeatcheck);
	}
}

#include "Render.h"
#include "igame_level.h"

//---------- Basicly, this is a main thread of our app
void CRenderDevice::LoopFrames()
{
	// Frame rate controll and other pre frame stuff
	if (!PreFrame())
		return;

	Statistic->EngineTOTAL.Begin();

	// Move frame counter and calc time delta
	FrameMove();

	// Perform some stuff in render, before new frame is processed
	m_pRender->OnNewFrame();

	// Process OnFrameBegin calls from various registered objects
	engineState.set(FRAME_BEGIN, true);
	Device.seqFrameBegin.Process(rp_FrameBegin);
	engineState.set(FRAME_BEGIN, false);

	// Process OnFrame calls from various registered objects
	engineState.set(FRAME_PROCESING, true);
	Device.seqFrame.Process(rp_Frame);

	Statistic->EngineTOTAL.End();

	Statistic->RenderTOTAL_Real.FrameStart();
	Statistic->RenderTOTAL_Real.Begin();

	engineState.set(FRAME_RENDERING, true);

	// View Ports
	SetUpViewPorts();

	u32 t_width = Device.dwWidth, t_height = Device.dwHeight;

	for (u32 i = 0; i < Render->viewPortsThisFrame.size(); ++i)
	{
		Render->currentViewPort = Render->viewPortsThisFrame[i];
		Render->needPresenting = (Render->currentViewPort == MAIN_VIEWPORT) ? true : false;

		ViewPort vp = Render->currentViewPort;

		if (vp == SECONDARY_WEAPON_SCOPE)
		{
			Device.dwWidth = m_SecondViewport.screenWidth;
			Device.dwHeight = m_SecondViewport.screenHeight;
		}

		if(g_pGameLevel)
			g_pGameLevel->ApplyCamera();

		// Set up and misc
		PrepareRender(vp);

		// Allow aux threads to do their workload
		if (i == 0)
		{
			auxThread_1_Allowed_.Set();
			auxThread_2_Allowed_.Set();
			auxThread_3_Allowed_.Set();
			auxThread_5_Allowed_.Set();
		}

		// Rendering
		if (b_is_Active && BeginRendering(vp))
		{
			seqRender.Process(rp_Render); // Process OnRender calls from various registered objects

			if (Statistic->NeedToShow() && Render->currentViewPort == MAIN_VIEWPORT)
				Statistic->Show();

			FinishRendering(vp);
		}
		else
		{
			auxThread_4_Allowed_.Set();

			break;
		}

		Device.dwWidth = t_width;
		Device.dwHeight = t_height;
	}

	Render->viewPortsThisFrame.clear();
	Render->currentViewPort = MAIN_VIEWPORT; // For next frame updating - some code needs to know that we are in main viewport. 
	
	if (g_pGameLevel) // reapply camera params as for the main vp, for next frame stuff(we dont want to use last vp camera in next frame possible usages)
		g_pGameLevel->ApplyCamera();

	// The below statistics will be actually shown in next frame, so keep it in mind

	Statistic->RenderTOTAL_Real.End();
	Statistic->RenderTOTAL_Real.FrameEnd();
	Statistic->RenderTOTAL.accum = Statistic->RenderTOTAL_Real.accum;

	engineState.set(FRAME_RENDERING, false);

	// Check if any aux thread is still processing job
	bThreadReadyProtect_.Enter();

	if (!thread1Ready_)
		Statistic->mainThreadWaitAux_1_ += 1;

	if (!thread2Ready_)
		Statistic->mainThreadWaitAux_2_ += 1;

	if (!thread3Ready_)
		Statistic->mainThreadWaitAux_3_ += 1;

	if (!thread4Ready_)
		Statistic->mainThreadWaitAux_4_ += 1;

	if (!thread5Ready_)
		Statistic->mainThreadWaitAux_5_ += 1;

	bThreadReadyProtect_.Leave();

	// Wait until aux threads
	auxThread_1_Ready_.Wait();
	auxThread_2_Ready_.Wait();
	auxThread_3_Ready_.Wait();
	auxThread_4_Ready_.Wait();
	auxThread_5_Ready_.Wait();

	engineState.set(FRAME_PROCESING, false);

	// Process deleting of various resources and objects after frame is done
	Statistic->EngineTOTAL.Begin();

	engineState.set(FRAME_END, true);
	Device.seqFrameEnd.Process(rp_FrameEnd);
	engineState.set(FRAME_END, false);

	Statistic->EngineTOTAL.End();

	Statistic->thread1Time_ = auxThread1TimeDeffered_;
	Statistic->thread2Time_ = auxThread2TimeDeffered_;
	Statistic->thread3Time_ = auxThread3TimeDeffered_;
	Statistic->thread4Time_ = auxThread4TimeDeffered_;
	Statistic->thread5Time_ = auxThread5TimeDeffered_;
	Statistic->auxThread1Objects_ = auxThread1ObjectsDeffered_;
	Statistic->auxThread2Objects_ = auxThread2ObjectsDeffered_;
	Statistic->auxThread3Objects_ = auxThread3ObjectsDeffered_;
	Statistic->auxThread4Objects_ = auxThread4ObjectsDeffered_;
	Statistic->auxThread5Objects_ = auxThread5ObjectsDeffered_;

	Statistic->GatherMeasurments();

	Statistic->mainThreadTime_ = mainThreadTimer_.GetElapsed_sec() * 1000.f;
}


bool CRenderDevice::PreFrame()
{
	mainThreadTimer_.Start();


	syncThreads_Freeze_and_Main_.Enter();
	engineFreezeTimer_.Start();
	syncThreads_Freeze_and_Main_.Leave();

	if (!b_is_Ready)
	{
		Sleep(100);
		return false;
	}

	if (Statistic->NeedToGatherSpecificInfo())
		g_bEnableStatGather = TRUE;
	else
		g_bEnableStatGather = FALSE;

	// Process next Loading Event and return
	if (g_loading_events.size())
	{
		engineState.set(LEVEL_LOADED, false);

		if (g_loading_events.front()())
			g_loading_events.pop_front();

		pApp->LoadDraw();

		return false;
	}

	engineState.set(LEVEL_LOADED, true);

	// Cap frame rate to user specified value or if scene rendering is skipped
	bool b_not_rendering_scene = (g_pGamePersistent && g_pGamePersistent->SceneRenderingBlocked());
	SetFrameTimeDelta(rateControlingTimer_.GetElapsed_sec() - previousFrameTime_);

	float fps_to_rate = 1000.f / ps_frames_per_sec;

	if (b_not_rendering_scene || fps_to_rate > 0.f)
	{
		float device_rate_ms = b_not_rendering_scene ? 6.94f : fps_to_rate;
		if (device_rate_ms >= FrameTimeDelta() * 1000.f)
		{
			return false;
		}
	}

	// Measure FPS
	Statistic->fDeviceMeasuredFPS = 1.f / FrameTimeDelta();
	previousFrameTime_ = rateControlingTimer_.GetElapsed_sec();

	return true;
}

void CRenderDevice::SetUpViewPorts()
{
	if (Device.m_SecondViewport.IsSVPActive())
	{
		Render->firstViewPort = MAIN_VIEWPORT;
		Render->lastViewPort = SECONDARY_WEAPON_SCOPE;
		Render->viewPortsThisFrame.push_back(MAIN_VIEWPORT);
		Render->viewPortsThisFrame.push_back(SECONDARY_WEAPON_SCOPE);
	}
	else
	{
		Render->firstViewPort = MAIN_VIEWPORT;
		Render->lastViewPort = MAIN_VIEWPORT;

		Render->viewPortsThisFrame.push_back(MAIN_VIEWPORT);
	}
}

bool CRenderDevice::PrepareRender(ViewPort viewport)
{
	if (dwPrecacheFrame)
	{
		float factor = float(dwPrecacheFrame) / float(dwPrecacheTotal);
		float angle = PI_MUL_2 * factor;
		vCameraDirection.set(_sin(angle), 0, _cos(angle));
		vCameraDirection.normalize();
		vCameraTop.set(0, 1, 0);
		vCameraRight.crossproduct(vCameraTop, vCameraDirection);
		mView.build_camera_dir(vCameraPosition, vCameraDirection, vCameraTop);
	}

	m_pRender->SwitchViewPortRTZB(viewport);

	// Matrices
	mFullTransform.mul(mProject, mView);
	m_pRender->SetCacheXform(mView, mProject);
	D3DXMatrixInverse((D3DXMATRIX*)&mInvFullTransform, 0, (D3DXMATRIX*)&mFullTransform);

	currentVpSavedView = &(viewport == MAIN_VIEWPORT ? vpSavedView1 : vpSavedView2);

	if(Render->currentViewPort == MAIN_VIEWPORT) // only for main vp can use gloabal Device.fFOV
		currentVpSavedView->fFov = Device.fFOV;

	currentVpSavedView->SetCameraPosition_saved(vCameraPosition);
	currentVpSavedView->SetFullTransform_saved(mFullTransform);
	currentVpSavedView->SetView_saved(mView);
	currentVpSavedView->SetProject_saved(mProject);

	return true;
}
