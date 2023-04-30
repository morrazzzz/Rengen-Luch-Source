
#ifndef _EDITOR
#pragma once

namespace ETimes
{
	IC const float ETimeFactor()
	{
		return Device.time_factor();
	}

	extern ENGINE_API void SetETimeFactor(float new_t_factor);

	IC float EngineTime()
	{
		float engine_time = Device.timeGlobal_;

		return engine_time;
	}

	extern ENGINE_API void SetEngineTime(float time);

	IC u32 EngineTimeU()
	{
		u32 engine_time = Device.timeGlobalU_;

		return engine_time;
	}

	extern ENGINE_API void SetEngineTimeU(u32 engine_time);

	IC u32 EngineTimeContinual()
	{
		u32 frame = Device.timeContinual_;

		return frame;
	}

	extern ENGINE_API void SetEngineTimeContinual(u32 tcontinual);

	IC u32 CurrentFrame()
	{
		u32 frame = Device.currentFrame_;

		return frame;
	}

	// Use this mt safe getter function, if your code can happen to execute in mt while main thread is in FrameMove
	IC u32 CurrentFrameMT()
	{
		Device.protectCurrentFrame_.Enter();
		u32 frame = Device.currentFrame_;
		Device.protectCurrentFrame_.Leave();

		return frame;
	}

	extern ENGINE_API void SetCurrentFrame(u32 frame);

	IC float TimeDelta()
	{
		float delta = Device.timeDelta_;

		return delta;
	}

	extern ENGINE_API void SetTimeDelta(float tdelta);

	IC float FrameTimeDelta()
	{
		float delta = Device.frameTimeDelta_;

		return delta;
	}

	extern ENGINE_API void SetFrameTimeDelta(float ftdelta);

	IC u32 TimeDeltaU()
	{
		u32 delta = Device.timeDeltaU_;

		return delta;
	}

	extern ENGINE_API void SetTimeDeltaU(u32 tdelta);
}

using namespace ETimes;

#else

#ifndef Borland_Times_included
#define Borland_Times_included

namespace ETimes
{
	IC const float ETimeFactor()
	{
		return EDevice.time_factor();
	}

	extern ENGINE_API void SetETimeFactor(float new_t_factor);

	IC float EngineTime()
	{
		float engine_time = EDevice.timeGlobal_;

		return engine_time;
	}

	extern ENGINE_API void SetEngineTime(float time);

	IC u32 EngineTimeU()
	{
		u32 engine_time = EDevice.timeGlobalU_;

		return engine_time;
	}

	extern ENGINE_API void SetEngineTimeU(u32 engine_time);

	IC u32 EngineTimeContinual()
	{
		u32 frame = EDevice.timeContinual_;

		return frame;
	}

	extern ENGINE_API void SetEngineTimeContinual(u32 tcontinual);

	IC u32 CurrentFrame()
	{
		u32 frame = EDevice.currentFrame_;

		return frame;
	}

	extern ENGINE_API void SetCurrentFrame(u32 frame);

	IC float TimeDelta()
	{
		float delta = EDevice.timeDelta_;

		return delta;
	}

	extern ENGINE_API void SetTimeDelta(float tdelta);

	IC float FrameTimeDelta()
	{
		float delta = EDevice.frameTimeDelta_;

		return delta;
	}

	extern ENGINE_API void SetFrameTimeDelta(float ftdelta);

	IC u32 TimeDeltaU()
	{
		u32 delta = EDevice.timeDeltaU_;

		return delta;
	}

	extern ENGINE_API void SetTimeDeltaU(u32 tdelta);
}

using namespace ETimes;

#endif

#endif

