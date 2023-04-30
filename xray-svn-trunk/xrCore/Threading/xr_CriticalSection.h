#ifndef _EDITOR
#pragma once

class XRCORE_API xrCriticalSection
{
private:
	// actual critical section object
	CRITICAL_SECTION	critSection_;
public:

	xrCriticalSection()
	{
		InitializeCriticalSection(&critSection_);
	};

	~xrCriticalSection()
	{
		DeleteCriticalSection(&critSection_);
	};

	// Code that should be accessed by one thread begins. Only one thread can enter it
	__forceinline void Enter()
	{
		EnterCriticalSection(&critSection_);
	};

	// Code that should be accessed by one thread FINISHED. Thread that entered critical part of code should "Leave" it in same order
	__forceinline void Leave()
	{
		LeaveCriticalSection(&critSection_);
	};

	__forceinline BOOL TryEnter()
	{
		return TryEnterCriticalSection(&critSection_);
	};
};

#else

#ifndef Borland_xr_CriticalSection_Included
#define Borland_xr_CriticalSection_Included

class XRCORE_API xrCriticalSection
{
private:
	// actual critical section object
	CRITICAL_SECTION	critSection_;
public:

	xrCriticalSection()
	{
		InitializeCriticalSection(&critSection_);
	};

	~xrCriticalSection()
	{
		DeleteCriticalSection(&critSection_);
	};

	// Code that should be accessed by one thread begins. Only one thread can enter it
	inline void Enter()
	{
		EnterCriticalSection(&critSection_);
	};

	// Code that should be accessed by one thread FINISHED. Thread that entered critical part of code should "Leave" it in same order
	inline void Leave()
	{
		LeaveCriticalSection(&critSection_);
	};

	inline BOOL TryEnter()
	{
		return TryEnterCriticalSection(&critSection_);
	};
};

#endif

#endif