#pragma once

// Note:
// ZNear - always 0.0f
// ZFar  - always 1.0f

#include "device_base.h"
#include "pure.h"
#include "../xrcore/ftimer.h"

#include "../xrCore/Threading/Event.hpp"
#include "../xrcore/Threading/EventIDs.h"

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/RenderDeviceRender.h"

#define BASE_FOV 67.f

enum ViewPort;

class ENGINE_API CSecondVPParams //--#SM+#-- +SecondVP+
{
	bool isActive; // Флаг активации рендера во второй вьюпорт
	u8 frameDelay;  // На каком кадре с момента прошлого рендера во второй вьюпорт мы начнём новый
					  //(не может быть меньше 2 - каждый второй кадр, чем больше тем более низкий FPS во втором вьюпорте)
public:
	bool isCamReady; // Флаг готовности камеры (FOV, позиция, и т.п) к рендеру второго вьюпорта

	u32 screenWidth;
	u32 screenHeight;

	IC bool IsSVPActive() { return isActive; }
	IC void SetSVPActive(bool bState);
	bool    IsSVPFrame();

	IC u8 GetSVPFrameDelay() { return frameDelay; }
	void  SetSVPFrameDelay(u8 iDelay)
	{
		frameDelay = iDelay;
		clamp<u8>(frameDelay, 1, u8(-1));
	}
};

struct SavedViewParams
{
	AccessLock								protectCamPosSaved_;
	AccessLock								protectViewSaved_;
	AccessLock								protectProjSaved_;
	AccessLock								protectFullTransSaved_;
	AccessLock								protectShrinkedFullTransSaved_;

	float									fFov;

	// Copies of corresponding members. Used for synchronization.
	Fvector									vCameraPosition_saved;
	Fmatrix									mView_saved;
	Fmatrix									mProject_saved;
	Fmatrix									mFullTransform_saved;

	Fmatrix									fullTransformShrinked;

	// Camera pos for mt stuff. Don't call when main thread is in PreRender
	IC Fvector GetCameraPosition_saved		() const			{ return vCameraPosition_saved; };
	// Use this mt safe getter function, if your code can happen to execute in mt while main thread is in PreRender (Slow)
	IC Fvector GetCameraPosition_saved_MT	()					{ protectCamPosSaved_.Enter(); Fvector v = vCameraPosition_saved; protectCamPosSaved_.Leave(); return v; };
	void SetCameraPosition_saved			(const Fvector& v)	{ protectCamPosSaved_.Enter(); vCameraPosition_saved = v; protectCamPosSaved_.Leave(); };

	// View matrix for mt stuff. Don't call when main thread is in PreRender
	IC Fmatrix GetView_saved				() const			{ return mView_saved; };
	// Use this mt safe getter function, if your code can happen to execute in mt while main thread is in PreRender
	IC Fmatrix GetView_saved_MT				()					{ protectViewSaved_.Enter(); Fmatrix m = mView_saved; protectViewSaved_.Leave(); return m; };
	void SetView_saved						(const Fmatrix& m)	{ protectViewSaved_.Enter(); mView_saved = m; protectViewSaved_.Leave(); };

	// Projection matrix for mt stuff. Don't call when main thread is in PreRender
	IC Fmatrix GetProject_saved				() const			{ return mProject_saved; };
	// Use this mt safe getter function, if your code can happen to execute in mt while main thread is in PreRender
	IC Fmatrix GetProject_saved_MT			()					{ protectProjSaved_.Enter(); Fmatrix m = mProject_saved; protectProjSaved_.Leave(); return m; };
	void SetProject_saved					(const Fmatrix& m)	{ protectProjSaved_.Enter(); mProject_saved = m; protectProjSaved_.Leave(); };

	// Transformation matrix for mt stuff. Don't call when main thread is in PreRender
	IC Fmatrix GetFullTransform_saved		() const			{ return mFullTransform_saved; };
	// Use this mt safe getter function, if your code can happen to execute in mt while main thread is in PreRender
	IC Fmatrix GetFullTransform_saved_MT	()					{ protectFullTransSaved_.Enter(); Fmatrix m = mFullTransform_saved; protectFullTransSaved_.Leave(); return m; };
	void SetFullTransform_saved				(const Fmatrix& m)	{ protectFullTransSaved_.Enter(); mFullTransform_saved = m; protectFullTransSaved_.Leave(); };

	// Shrinked transformation matrix for mt stuff. Don't call when main thread is in PreRender
	IC Fmatrix GetShrinkedFullTransform_saved		() const			{ return fullTransformShrinked; };
	// Use this mt safe getter function, if your code can happen to execute in mt while main thread is in PreRender
	IC Fmatrix GetShrinkedFullTransform_saved_MT	()					{ protectShrinkedFullTransSaved_.Enter(); Fmatrix m = fullTransformShrinked; protectShrinkedFullTransSaved_.Leave(); return m; };
	void SetShrinkedFullTransform_saved				(const Fmatrix& m)	{ protectShrinkedFullTransSaved_.Enter(); fullTransformShrinked = m; protectShrinkedFullTransSaved_.Leave(); };


	// returns the aproximate, adjusted by camera fov, distance / For multhithreaded environment. Mt safe, when not called during PreRender
	IC float GetDistFromCameraMT(Fvector from_position) const
	{
		float distance = GetCameraPosition_saved().distance_to(from_position);
		float fov_K = BASE_FOV / fFov;
		float adjusted_distane = distance / fov_K;

		return adjusted_distane;
	}

	// returns the aproximate, adjusted by camera fov, distance / For multhithreaded environment. Very safe. For situations, when expected to be called during PreRender (Slow)
	IC float GetDistFromCameraMT_2(Fvector from_position) // const
	{
		float distance = GetCameraPosition_saved_MT().distance_to(from_position);
		float fov_K = BASE_FOV / fFov;
		float adjusted_distane = distance / fov_K;

		return adjusted_distane;
	}
};

// refs
class ENGINE_API CRenderDevice: public CRenderDeviceBase
{
public:

private:
    // Main objects used for creating and rendering the 3D scene
    u32										m_dwWindowStyle;
    RECT									m_rcWindowBounds;
    RECT									m_rcWindowClient;

	CTimer									TimerMM;

	void									_Create		(LPCSTR shName);
	void									_Destroy	(BOOL	bKeepTextures);
	void									_SetupStates();
public:
	u32										dwPrecacheTotal;

	float									fWidth_2, fHeight_2;

	void									OnWM_Activate(WPARAM wParam, LPARAM lParam);
public:

	IRenderDeviceRender						*m_pRender;

	BOOL									m_bNearer;
	void									SetNearer	(BOOL enabled)
	{
		if (enabled&&!m_bNearer)
		{
			m_bNearer						= TRUE;
			mProject._43					-= EPS_L;
		}
		else if (!enabled&&m_bNearer)
		{
			m_bNearer						= FALSE;
			mProject._43					+= EPS_L;
		}

		m_pRender->SetCacheXform(mView, mProject);

		//TODO: re-implement set projection
		//RCache.set_xform_project			(mProject);
	}

	void									DumpResourcesMemoryUsage() { m_pRender->ResourcesDumpMemoryUsage();}
public:
	// Registrators
	CRegistrator	<pureFrame>				seqFrameMT;
	CRegistrator	<pureDeviceReset>		seqDeviceReset;
	CRegistrator	<pureFrameBegin>		seqFrameBegin;
	CRegistrator	<pureFrameEnd>			seqFrameEnd;

	CStats*									Statistic;

	Fmatrix									mInvFullTransform;

	// returns the aproximate, adjusted by camera fov, distance 
	IC float GetDistFromCamera(const Fvector& from_position) const
	{
		float distance = vCameraPosition.distance_to(from_position);
		float fov_K = BASE_FOV / fFOV;
		float adjusted_distane = distance / fov_K;

		return adjusted_distane;
	}

	SavedViewParams vpSavedView1; // main
	SavedViewParams vpSavedView2;
	SavedViewParams* currentVpSavedView;

	bool IsR4Active();
	
	CRenderDevice();
	~CRenderDevice();

	void	Pause							(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
	BOOL	Paused							();

	void PreCache							(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input);

	bool PreFrame							();
	void FrameMove							();

	void SetUpViewPorts						();
	bool PrepareRender						(ViewPort viewport);
	void Clear								();
	BOOL BeginRendering						(ViewPort viewport);
	void FinishRendering					(ViewPort viewport);
	
	void overdrawBegin						();
	void overdrawEnd						();

	// Mode control
#ifdef _WIN64
	IC __unaligned CTimer_paused* GetTimerGlobal()	{ return &TimerGlobal; };
#else
	IC CTimer_paused* GetTimerGlobal()				{ return &TimerGlobal; };
#endif

	u32	 TimerAsync									()	{ return TimerGlobal.GetElapsed_ms(); }
	u32	 TimerAsync_MMT								()	{ return TimerMM.GetElapsed_ms() + Timer_MM_Delta; }

	// Creation & Destroying
	void ConnectToRender();
	void Create								();
	void Run								();
	void Destroy							();
	void Reset								(bool precache = true);

	void Initialize							();

public:
	void time_factor						(const float &time_factor)
	{
		Timer.time_factor		(time_factor);
		TimerGlobal.time_factor	(time_factor);
	}
	
	IC	const float time_factor			()
	{
		VERIFY(Timer.time_factor() == TimerGlobal.time_factor());

		float t_factor = Timer.time_factor();

		return(t_factor);
	}

	// Dependable from MainThread Aux thread 1 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	auxThreadPool_1_;
	// Dependable from MainThread Aux thread 2 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	auxThreadPool_2_;
	// Dependable from MainThread Aux thread 3 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	auxThreadPool_3_;
	// Dependable from MainThread Aux thread 4 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	auxThreadPool_4_;
	// Dependable from MainThread Aux thread 5 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	auxThreadPool_5_;

	// Independable from MainThread Aux thread 1 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	independableAuxThreadPool_1_;
	// Independable from MainThread Aux thread 2 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	independableAuxThreadPool_2_;
	// Independable from MainThread Aux thread 3 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	independableAuxThreadPool_3_;

	// Resource uploading thread 1 workload pool
	xr_vector		<fastdelegate::FastDelegate0<>>	resourceUploadingThreadPool_1_;

	// Processing copies of threads workload pools
	
	xr_vector		<fastdelegate::FastDelegate0<>>	AuxThreadPool_1_TempCopy_;
	xr_vector		<fastdelegate::FastDelegate0<>>	AuxThreadPool_2_TempCopy_;
	xr_vector		<fastdelegate::FastDelegate0<>>	AuxThreadPool_3_TempCopy_;
	xr_vector		<fastdelegate::FastDelegate0<>>	AuxThreadPool_4_TempCopy_;
	xr_vector		<fastdelegate::FastDelegate0<>>	AuxThreadPool_5_TempCopy_;
	xr_vector		<fastdelegate::FastDelegate0<>>	independableAuxThreadPool_1_TempCopy_;
	xr_vector		<fastdelegate::FastDelegate0<>>	independableAuxThreadPool_2_TempCopy_;
	xr_vector		<fastdelegate::FastDelegate0<>>	independableAuxThreadPool_3_TempCopy_;
	xr_vector		<fastdelegate::FastDelegate0<>>	resourceUploadingThreadPool_1_TempCopy_;

	CSecondVPParams m_SecondViewport;	//--#SM+#-- +SecondVP+

	// Locks

	AccessLock		AuxPool_1_Protection_;
	AccessLock		AuxPool_2_Protection_;
	AccessLock		AuxPool_3_Protection_;
	AccessLock		AuxPool_4_Protection_;
	AccessLock		AuxPool_5_Protection_;
	AccessLock		IndAuxPoolProtection_1;
	AccessLock		IndAuxPoolProtection_2;
	AccessLock		IndAuxPoolProtection_3;
	AccessLock		resUploadingPoolProtection_1;

	AccessLock		IndAuxProcessingProtection_1; // for various stages where we need to wait for aux thread to finish current pass
	AccessLock		IndAuxProcessingProtection_2; // for various stages where we need to wait for aux thread to finish current pass
	AccessLock		IndAuxProcessingProtection_3; // for various stages where we need to wait for aux thread to finish current pass
	AccessLock		resUploadingProcessingProtection_1; // for various stages where we need to wait for aux thread to finish current pass

private:
	// --------Multi-threading

	//depended from main thread auxilary thread 1
	static void AuxThread_1(void *context);
	//depended from main thread auxilary thread 2
	static void AuxThread_2(void *context);
	//depended from main thread auxilary thread 3
	static void AuxThread_3(void *context);
	//depended from main thread auxilary thread 4
	static void AuxThread_4(void *context);
	//depended from main thread auxilary thread 5
	static void AuxThread_5(void *context);

#define SEQUANTIAL_AUX_THREADS_COUNT 5

	// "independent from main thread" auxilary thread 1 // Used for render, cant wait other workload much, or will make engine stutter 
	static void AuxThread_Independable_1(void *context);
	// "independent from main thread" auxilary thread 2 // Used for render, cant wait other workload much, or will make engine stutter 
	static void AuxThread_Independable_2(void *context);
	// "independent from main thread" auxilary thread 3 // Used for render, cant wait other workload much, or will make engine stutter 
	static void AuxThread_Independable_3(void *context);

#define INDEPENDABLE_AUX_THREADS_COUNT 3

	// "independent from main thread" auxilary thread 1 // Dont use for time critical stuff
	static void ResourceUploadingThread_1(void *context);

public:

	Event auxThread_1_Allowed_;
	Event auxThread_1_Ready_;
	Event auxThread_2_Allowed_;
	Event auxThread_2_Ready_;
	Event auxThread_3_Allowed_;
	Event auxThread_3_Ready_;
	Event auxThread_4_Allowed_;
	Event auxThread_4_Ready_;
	Event auxThread_5_Allowed_;
	Event auxThread_5_Ready_;

	Event aux1ExitSync_; // Aux thread 1 exit event sync
	Event aux2ExitSync_; // Aux thread 2 exit event sync
	Event aux3ExitSync_; // Aux thread 3 exit event sync
	Event aux4ExitSync_; // Aux thread 4 exit event sync
	Event aux5ExitSync_; // Aux thread 5 exit event sync

	Event AuxIndependable1Exit_Sync; // Independable on MainThread Aux thread 1 exit event sync
	Event AuxIndependable2Exit_Sync; // Independable on MainThread Aux thread 2 exit event sync
	Event AuxIndependable3Exit_Sync; // Independable on MainThread Aux thread 3 exit event sync

	Event ResUploadingThread1Exit_Sync; // Res uploading thread 1 exit event sync

	Event freezeTreadExitSync_; // Aux thread 2 exit event sync

	Event SoundsThreadExit_Sync; // sound loading thread exit event sync

	Event IndAuxThreadWakeUp1;
	Event IndAuxThreadWakeUp2;
	Event IndAuxThreadWakeUp3;

	void				SetAuxThreadsMustExit(bool val);
	bool				IsAuxThreadsMustExit();

	void				SetFreezeThreadMustExit(bool val);
	bool				IsFreezeThreadMustExit();

	volatile bool		auxTreadsMustExit_;
	volatile bool		freezeThreadExit_;

	// Add and process me as soon as inddependable Aux thread 1 is idleing again // Used for render, cant wait other workload much, or will make engine stutter 
	ICF	void			AddToIndAuxThread_1_Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		IndAuxThreadWakeUp1.Set();

		IndAuxPoolProtection_1.Enter();
		independableAuxThreadPool_1_.push_back(delegate);
		IndAuxPoolProtection_1.Leave();
	}

	// Add and process me as soon as inddependable Aux thread 2 is idleing again // Used for render, cant wait other workload much, or will make engine stutter 
	ICF	void			AddToIndAuxThread_2_Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		IndAuxThreadWakeUp2.Set();

		IndAuxPoolProtection_2.Enter();
		independableAuxThreadPool_2_.push_back(delegate);
		IndAuxPoolProtection_2.Leave();
	}

	// Add and process me as soon as inddependable Aux thread 3 is idleing again // Used for render, cant wait other workload much, or will make engine stutter 
	ICF	void			AddToIndAuxThread_3_Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		IndAuxThreadWakeUp3.Set();

		IndAuxPoolProtection_3.Enter();
		independableAuxThreadPool_3_.push_back(delegate);
		IndAuxPoolProtection_3.Leave();
	}

	// Add and process me as soon as inddependable resource uploading thread 1 is idleing again // Dont use for time critical stuff
	ICF	void			AddToResUploadingThread_1_Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		resUploadingPoolProtection_1.Enter();
		resourceUploadingThreadPool_1_.push_back(delegate);
		resUploadingPoolProtection_1.Leave();
	}

	// Add and process me when next time main thread is buisy with Render (Aux thread#, Fast delegate)
	ICF	void			AddToAuxThread_Pool(u8 aux_thread_no, const fastdelegate::FastDelegate0<> &delegate)
	{
		R_ASSERT(aux_thread_no > 0 && aux_thread_no <= SEQUANTIAL_AUX_THREADS_COUNT);

		if (aux_thread_no == 1)
		{
			AuxPool_1_Protection_.Enter();
			auxThreadPool_1_.push_back(delegate);
			AuxPool_1_Protection_.Leave();
		}
		else if (aux_thread_no == 2)
		{
			AuxPool_2_Protection_.Enter();
			auxThreadPool_2_.push_back(delegate);
			AuxPool_2_Protection_.Leave();
		}
		else if (aux_thread_no == 3)
		{
			AuxPool_3_Protection_.Enter();
			auxThreadPool_3_.push_back(delegate);
			AuxPool_3_Protection_.Leave();
		}
		else if (aux_thread_no == 4)
		{
			AuxPool_4_Protection_.Enter();
			auxThreadPool_4_.push_back(delegate);
			AuxPool_4_Protection_.Leave();
		}
		else if (aux_thread_no == 5)
		{
			AuxPool_5_Protection_.Enter();
			auxThreadPool_5_.push_back(delegate);
			AuxPool_5_Protection_.Leave();
		}
		else
			R_ASSERT(false);
	}

	//// Insert at front

	// Add and process me FIRST as soon as independable Aux thread 1 is idleing again
	ICF	void			AddAtFrontToIndAuxThread_1_Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		IndAuxThreadWakeUp1.Set();

		IndAuxPoolProtection_1.Enter();
		independableAuxThreadPool_1_.insert(independableAuxThreadPool_1_.begin(), delegate);
		IndAuxPoolProtection_1.Leave();
	}

	// Add and process me FIRST as soon as independable Aux thread 2 is idleing again
	ICF	void			AddAtFrontToIndAuxThread_2_Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		IndAuxThreadWakeUp2.Set();

		IndAuxPoolProtection_2.Enter();
		independableAuxThreadPool_2_.insert(independableAuxThreadPool_2_.begin(), delegate);
		IndAuxPoolProtection_2.Leave();
	}

	// Add and process me FIRST as soon as independable Aux thread 3 is idleing again
	ICF	void			AddAtFrontToIndAuxThread_3_Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		IndAuxThreadWakeUp3.Set();

		IndAuxPoolProtection_3.Enter();
		independableAuxThreadPool_3_.insert(independableAuxThreadPool_3_.begin(), delegate);
		IndAuxPoolProtection_3.Leave();
	}

	// Add and process me FIRST as soon as resource uploading thread 1 is idleing again
	ICF	void			AddAtFrontToResUploadingThread_1_Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		resUploadingPoolProtection_1.Enter();
		resourceUploadingThreadPool_1_.insert(resourceUploadingThreadPool_1_.begin(), delegate);
		resUploadingPoolProtection_1.Leave();
	}

	// Add and process me FIRST when next time main thread is buisy with Render (Aux thread#, Fast delegate)
	ICF	void			AddAtFrontToAuxThread_Pool(u8 aux_thread_no, const fastdelegate::FastDelegate0<> &delegate)
	{
		R_ASSERT(aux_thread_no > 0 && aux_thread_no <= SEQUANTIAL_AUX_THREADS_COUNT);

		if (aux_thread_no == 1)
		{
			AuxPool_1_Protection_.Enter();
			auxThreadPool_1_.insert(auxThreadPool_1_.begin(), delegate);
			AuxPool_1_Protection_.Leave();
		}
		else if (aux_thread_no == 2)
		{
			AuxPool_2_Protection_.Enter();
			auxThreadPool_2_.insert(auxThreadPool_2_.begin(), delegate);
			AuxPool_2_Protection_.Leave();
		}
		else if (aux_thread_no == 3)
		{
			AuxPool_3_Protection_.Enter();
			auxThreadPool_3_.insert(auxThreadPool_3_.begin(), delegate);
			AuxPool_3_Protection_.Leave();
		}
		else if (aux_thread_no == 4)
		{
			AuxPool_4_Protection_.Enter();
			auxThreadPool_4_.insert(auxThreadPool_4_.begin(), delegate);
			AuxPool_4_Protection_.Leave();
		}
		else if (aux_thread_no == 5)
		{
			AuxPool_5_Protection_.Enter();
			auxThreadPool_5_.insert(auxThreadPool_5_.begin(), delegate);
			AuxPool_5_Protection_.Leave();
		}
		else
			R_ASSERT(false);
	}

	//// Individual thread object removal

	ICF	void			RemoveFromAuxthread1Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		AuxPool_1_Protection_.Enter();

		xr_vector<fastdelegate::FastDelegate0<> >::iterator I = std::find(
			auxThreadPool_1_.begin(),
			auxThreadPool_1_.end(),
			delegate
		);

		if (I != auxThreadPool_1_.end())
			auxThreadPool_1_.erase(I);

		AuxPool_1_Protection_.Leave();
	}

	ICF	void			RemoveFromAuxthread2Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		AuxPool_2_Protection_.Enter();

		xr_vector<fastdelegate::FastDelegate0<> >::iterator I = std::find(
			auxThreadPool_2_.begin(),
			auxThreadPool_2_.end(),
			delegate
			);

		if (I != auxThreadPool_2_.end())
			auxThreadPool_2_.erase(I);

		AuxPool_2_Protection_.Leave();
	}

	ICF	void			RemoveFromAuxthread3Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		AuxPool_3_Protection_.Enter();

		xr_vector<fastdelegate::FastDelegate0<> >::iterator I = std::find(
			auxThreadPool_3_.begin(),
			auxThreadPool_3_.end(),
			delegate
			);

		if (I != auxThreadPool_3_.end())
			auxThreadPool_3_.erase(I);

		AuxPool_3_Protection_.Leave();
	}

	ICF	void			RemoveFromAuxthread4Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		AuxPool_4_Protection_.Enter();

		xr_vector<fastdelegate::FastDelegate0<> >::iterator I = std::find(
			auxThreadPool_4_.begin(),
			auxThreadPool_4_.end(),
			delegate
			);

		if (I != auxThreadPool_4_.end())
			auxThreadPool_4_.erase(I);

		AuxPool_4_Protection_.Leave();
	}

	ICF	void			RemoveFromAuxthread5Pool(const fastdelegate::FastDelegate0<> &delegate)
	{
		AuxPool_5_Protection_.Enter();

		xr_vector<fastdelegate::FastDelegate0<> >::iterator I = std::find(
			auxThreadPool_5_.begin(),
			auxThreadPool_5_.end(),
			delegate
			);

		if (I != auxThreadPool_5_.end())
			auxThreadPool_5_.erase(I);

		AuxPool_5_Protection_.Leave();
	}


public:
			void __stdcall		LoopFrames();
			bool __stdcall		on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &result);

private:
			void					message_loop		();
virtual		void			_BCL	AddSeqFrame			( pureFrame* f, bool mt );
virtual		void			_BCL	RemoveSeqFrame		( pureFrame* f );
virtual		CStatsPhysics*	_BCL	StatPhysics			()	{ return  Statistic ;}
};

class ENGINE_API CLoadScreenRenderer :public pureRender
{
public:
	CLoadScreenRenderer();
	void			start(bool b_user_input);
	void			stop();
	virtual void	OnRender();

	bool			b_registered;
	bool			b_need_user_input;
};

extern ENGINE_API CRenderDevice		Device;
extern ENGINE_API float				VIEWPORT_NEAR;
extern ENGINE_API Mutex				GameSaveMutex;

// For freeze detecting thread
extern ENGINE_API bool				checkFreezing_;


typedef fastdelegate::FastDelegate0<bool>		LOADING_EVENT;
extern ENGINE_API xr_list<LOADING_EVENT>		g_loading_events;

extern ENGINE_API CLoadScreenRenderer load_screen_renderer;

// These variables have corrensponding console commands, and can be used to quickly tune or test smth, without quiting from game
extern ENGINE_API float devfloat1;
extern ENGINE_API float devfloat2;
extern ENGINE_API float devfloat3;
extern ENGINE_API float devfloat4;

#include "times.h" // important to be at the end