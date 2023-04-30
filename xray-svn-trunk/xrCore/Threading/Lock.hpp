//Should be included only for GAME bins. Unless we manage to upgrade SDK projects from borland to VS
#pragma once

#ifdef PROFILE_CRITICAL_SECTIONS
typedef void (*add_profile_portion_callback)(LPCSTR id, const u64& time);
void XRCORE_API set_add_profile_portion(add_profile_portion_callback callback);

#define STRINGIZER_HELPER(a) #a
#define STRINGIZER(a) STRINGIZER_HELPER(a)
#define CONCATENIZE_HELPER(a, b) a##b
#define CONCATENIZE(a, b) CONCATENIZE_HELPER(a, b)
#define MUTEX_PROFILE_PREFIX_ID #mutexes /
#define MUTEX_PROFILE_ID(a) STRINGIZER(CONCATENIZE(MUTEX_PROFILE_PREFIX_ID, a))
#endif

class XRCORE_API Lock
{
public:
#ifdef PROFILE_CRITICAL_SECTIONS
	Lock(const char* id) : id(id) {std::atomic_init(&lockCounter, 0);}
#else
	Lock(){ std::atomic_init(&lockCounter, 0); }
#endif

    Lock(const Lock&) = delete;
    Lock operator=(const Lock&) = delete;

#ifdef PROFILE_CRITICAL_SECTIONS
    void Enter();
#else
	// Code that should be accessed by one thread begins. Only one thread can enter it
    void Enter()
    {
        mutex.lock();
        lockCounter++;
    }
#endif

    bool TryEnter()
    {
        bool locked = mutex.try_lock();
        if (locked)
            lockCounter++;
        return locked;
    }

	// Code that should be accessed by one thread FINISHED. Thread that entered critical part of code should "Leave" it in same order
    void Leave()
    {
        mutex.unlock();
        lockCounter--;
    }

    bool IsLocked() const { return !!lockCounter; }
private:
	RMutex mutex;
    std::atomic_int lockCounter;
#ifdef PROFILE_CRITICAL_SECTIONS
    const char* id;
#endif
};
