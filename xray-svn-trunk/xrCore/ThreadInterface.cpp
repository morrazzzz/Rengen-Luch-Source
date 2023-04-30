#include "stdafx.h"

#include <process.h>

// Initialized on startup
XRCORE_API Fmatrix Fidentity;
XRCORE_API Dmatrix Didentity;
XRCORE_API CRandom Random;

void _initialize_cpu(void)
{
	::Random.seed(u32(CPU::QPC() % (1i64 << 32i64)));

    Fidentity.identity(); // Identity matrix
    Didentity.identity(); // Identity matrix

    pvInitializeStatics(); // Lookup table for compressed normals

    _initialize_cpu_thread();
}

// per-thread initialization

extern void __cdecl _terminate();
void debug_on_thread_spawn();

void _initialize_cpu_thread()
{
    debug_on_thread_spawn();
}

// threading API
#pragma pack(push,8)
struct THREAD_NAME
{
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
};

void thread_name(LPCSTR name)
{
    THREAD_NAME tn;
    tn.dwType = 0x1000;
    tn.szName = name;
    tn.dwThreadID = DWORD(-1);
    tn.dwFlags = 0;

    __try
    {
        RaiseException(0x406D1388, 0, sizeof(tn) / sizeof(ULONG_PTR), (ULONG_PTR*)&tn);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }

	R_ASSERT(GetCurrentThreadParams());
	GetCurrentThreadParams()->SetThreadName(name);
}
#pragma pack(pop)

struct THREAD_STARTUP
{
    thread_t* entry;
    shared_str name;
    void* args;
};

AccessLock protectTCounter_;
int currentThreadsCount_ = 0;

void ThreadsInc()
{
	protectTCounter_.Enter();
	
	currentThreadsCount_++;
	
	protectTCounter_.Leave();
}

void ThreadsDec()
{
	protectTCounter_.Enter();
	
	currentThreadsCount_--;
	
	protectTCounter_.Leave();
}

int GetCurrentThreadsNum() // only 'thread_spawn' threads
{
	protectTCounter_.Enter();
	
	int res = currentThreadsCount_;
	
	protectTCounter_.Leave();
	
	return res;
}

xr_map<u32, ThreadRuntimeParams*> threadsParamsStorage;
AccessLock protectThreadsParamsStorage;

ThreadRuntimeParams* CreateThreadRuntimeParams(u32 thread_id)
{
	protectThreadsParamsStorage.Enter();

	ThreadRuntimeParams* p = xr_new<ThreadRuntimeParams>();
	threadsParamsStorage.insert(mk_pair(thread_id, p));

	p->SetOwningThreadID(thread_id);

	protectThreadsParamsStorage.Leave();

	return p;
}

void DeleteThreadRuntimeParams(u32 thread_id)
{
	protectThreadsParamsStorage.Enter();

	auto it = threadsParamsStorage.find(thread_id);

	if (it == threadsParamsStorage.end())
		NODEFAULT;

	xr_delete(it->second);

	threadsParamsStorage.erase(it);

	protectThreadsParamsStorage.Leave();
}

ThreadRuntimeParams* GetCurrentThreadParams()
{
	protectThreadsParamsStorage.Enter();

	auto it = threadsParamsStorage.find(GetCurrentThreadId());

	if (it == threadsParamsStorage.end())
		NODEFAULT;

	ThreadRuntimeParams* p = it->second;

	protectThreadsParamsStorage.Leave();

	return p;
}

LPCSTR GetCurrentThreadName()
{
	return GetCurrentThreadParams()->GetThreadName();;
}

void __cdecl thread_entry(void* _params)
{
    // Initialize
	ThreadRuntimeParams* runtime = CreateThreadRuntimeParams(GetCurrentThreadId());

	THREAD_STARTUP* startup = (THREAD_STARTUP*)_params;
	thread_name(*startup->name);

	thread_t* entry = startup->entry;
	void* arglist = startup->args;

	ThreadsInc();
	
#ifndef _EDITOR // Retarded borland cant enter critical section from other thread in log msg(or something like that)
	try
	{
		if (IsLogFilePresentMT())
			Msg("^ New Thread: [%s] ID: [%u]. Threads count [%i]", runtime->GetThreadName(), GetCurrentThreadId(), GetCurrentThreadsNum());
	}
	catch (...)
	{
		MessageBox(NULL, "xrCore _math.cpp", "error 1", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
	}
#endif

    xr_delete(startup);

    _initialize_cpu_thread();

	//if(arglist)
    // Call
    entry(arglist);
	
	
	// Finish 
	ThreadsDec();
	
#ifndef _EDITOR
	try
	{
		if(IsLogFilePresentMT())
			Msg("^ Thread Exited: [%s] ID: [%u]. Threads count [%i]", runtime->GetThreadName(), GetCurrentThreadId(), GetCurrentThreadsNum());
	}
	catch (...)
	{
		MessageBox(NULL, "xrCore _math.cpp", "error 2", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
	}
#endif

	DeleteThreadRuntimeParams(GetCurrentThreadId());
}

void thread_spawn(thread_t* entry, LPCSTR name, unsigned stack, void* arglist)
{
    Debug._initialize(false);

    THREAD_STARTUP* startup = xr_new <THREAD_STARTUP>();
    startup->entry = entry;
    startup->name = name;
    startup->args = arglist;

    _beginthread(thread_entry, stack, startup);
}

ThreadRuntimeParams::ThreadRuntimeParams()
{
	mtTextureLoadingAllowed = true;
}
