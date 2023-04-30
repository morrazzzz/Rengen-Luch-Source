//Device base for Game and EDITOR
//Contains common class and variables for both devices(Game and SDK)
//Created to avoid bardak in game varsion of device.cpp
#ifndef xr_device
#define xr_device

#pragma once

#ifdef __BORLANDC__
#	include "..\editors\ECore\Editor\estats.h"
#else
#	include "Stats.h"
#endif

#include "pure.h"
#include "../xrcore/ftimer.h"
#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/RenderDeviceRender.h"

class IRenderDevice
{
public:
	virtual		CStatsPhysics*	_BCL		StatPhysics() = 0;
	virtual				void	_BCL		AddSeqFrame(pureFrame* f, bool mt) = 0;
	virtual				void	_BCL		RemoveSeqFrame(pureFrame* f) = 0;
};

class ENGINE_API CRenderDeviceData
{

public:
	u32										dwWidth;
	u32										dwHeight;

	u32										dwPrecacheFrame;
	BOOL									b_is_Ready;
	BOOL									b_is_Active;
public:

	// Current Frame
	AccessLock								protectCurrentFrame_;
	u32										currentFrame_;

	// time delta depending on Time Factor (use this for objects that are depending on time factor)
	float									timeDelta_;
	// time delta depending on Time Factor U32 (use this for objects that are depending on time factor)
	u32										timeDeltaU_;

	// time delta !NOT! depending on Time Factor (use this for stuff like camera control, or other stuff that is not meant to be scaled to Time Factor)
	float									frameTimeDelta_;

	// Engine time since start up
	float									timeGlobal_;
	// Engine time since start up in U32
	u32										timeGlobalU_;

	u32										timeContinual_;

	Fvector									vCameraPosition;
	Fvector									vCameraDirection;
	Fvector									vCameraTop;
	Fvector									vCameraRight;

	Fmatrix									mView;
	Fmatrix									mProject;
	Fmatrix									mFullTransform;

	float									fFOV;
	float									fASPECT;

	// Frame Rate Controll
	// A timer used for controlling and calculating frame rate. Stores real time from engine start
	CTimer									rateControlingTimer_;
	// Time from engine start till previous frame in Seconds
	float									previousFrameTime_;
protected:

	u32										Timer_MM_Delta;
	CTimer_paused							Timer;
	CTimer_paused							TimerGlobal;
public:

	// Registrators
	CRegistrator	<pureRender			>			seqRender;
	CRegistrator	<pureAppActivate	>			seqAppActivate;
	CRegistrator	<pureAppDeactivate	>			seqAppDeactivate;
	CRegistrator	<pureAppStart		>			seqAppStart;
	CRegistrator	<pureAppEnd			>			seqAppEnd;
	CRegistrator	<pureFrame			>			seqFrame;
	CRegistrator	<pureScreenResolutionChanged>	seqResolutionChanged;

	HWND									m_hWnd;

};

class	ENGINE_API CRenderDeviceBase :
	public IRenderDevice,
	public CRenderDeviceData
{
public:
};

extern		ENGINE_API		CTimer				loading_save_timer;
extern		ENGINE_API		bool				loading_save_timer_started;
extern		ENGINE_API		bool				prefetching_in_progress;

#ifndef	_EDITOR
#define	RDEVICE	Device
#else
#define RDEVICE	EDevice
#endif


#endif