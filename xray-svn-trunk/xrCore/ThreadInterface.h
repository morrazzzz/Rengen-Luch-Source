#pragma once

struct XRCORE_API ThreadRuntimeParams
{
private:
	bool				mtTextureLoadingAllowed; // Because now the render has very complex ways of actual calling of texture loading
	u32					owningThreadID; // for debug
	std::string			threadName;

public:
	ThreadRuntimeParams();

	void	SetMtTextureLoadingAllowed			(bool val)		{ mtTextureLoadingAllowed = val; };
	bool	GetMtTextureLoadingAllowed			()				{ return mtTextureLoadingAllowed; };

	void	SetOwningThreadID					(u32 val)		{ owningThreadID = val; };
	u32		GetOwningThreadID					()				{ return owningThreadID; };

	void	SetThreadName						(LPCSTR val)	{ threadName = val; };
	LPCSTR	GetThreadName						()				{ return threadName.c_str(); };
};

extern XRCORE_API void _initialize_cpu();
extern XRCORE_API void _initialize_cpu_thread();

// threading
typedef void thread_t(void*);

extern XRCORE_API ThreadRuntimeParams*		GetCurrentThreadParams();
extern XRCORE_API ThreadRuntimeParams*		CreateThreadRuntimeParams(u32 thread_id);
extern XRCORE_API void						DeleteThreadRuntimeParams(u32 thread_id);
extern XRCORE_API LPCSTR					GetCurrentThreadName(); // For convinience

extern XRCORE_API void thread_name(LPCSTR name);
extern XRCORE_API void thread_spawn(thread_t* entry, LPCSTR name, unsigned stack, void* arglist);
