#include "stdafx.h"
#include "Times.h"

#ifndef _EDITOR

namespace ETimes
{
	void SetETimeFactor(float new_t_factor)
	{
		Device.time_factor(new_t_factor);
	}

	void SetEngineTime(float time)
	{
		Device.timeGlobal_ = time;
	}

	void SetEngineTimeU(u32 engine_time)
	{
		Device.timeGlobalU_ = engine_time;
	}

	void SetEngineTimeContinual(u32 tcontinual)
	{
		Device.timeContinual_ = tcontinual;
	}

	void SetCurrentFrame(u32 frame)
	{
		Device.protectCurrentFrame_.Enter();
		Device.currentFrame_ = frame;
		Device.protectCurrentFrame_.Leave();
	}

	void SetTimeDelta(float tdelta)
	{
		Device.timeDelta_ = tdelta;
	}

	void SetFrameTimeDelta(float ftdelta)
	{
		Device.frameTimeDelta_ = ftdelta;
	}

	void SetTimeDeltaU(u32 tdelta)
	{
		Device.timeDeltaU_ = tdelta;
	}
}

#else

namespace ETimes
{
	void SetETimeFactor(float new_t_factor)
	{
		EDevice.time_factor(new_t_factor);
	}

	void SetEngineTime(float time)
	{
		EDevice.timeGlobal_ = time;
	}

	void SetEngineTimeU(u32 engine_time)
	{
		EDevice.timeGlobalU_ = engine_time;
	}

	void SetEngineTimeContinual(u32 tcontinual)
	{
		EDevice.timeContinual_ = tcontinual;
	}

	void SetCurrentFrame(u32 frame)
	{
		EDevice.currentFrame_ = frame;
	}

	void SetTimeDelta(float tdelta)
	{
		EDevice.timeDelta_ = tdelta;
	}

	void SetFrameTimeDelta(float ftdelta)
	{
		EDevice.frameTimeDelta_ = ftdelta;
	}

	void SetTimeDeltaU(u32 tdelta)
	{
		EDevice.timeDeltaU_ = tdelta;
	}
}

#endif