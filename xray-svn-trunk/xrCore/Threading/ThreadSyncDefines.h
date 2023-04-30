#pragma once

#include "xr_CriticalSection.h"

typedef xrCriticalSection AccessLock; // Custom MT access protection mechanism. SDK does not work if we use "Mutex" instead of "xrCriticalSection"

#ifndef _EDITOR

// Stuff for SDK shared bins (SDK makes use of xrCore, xrSE_Factory, ETOOLs and other bins)
#include <mutex>
#include <atomic>

typedef std::mutex Mutex; // Make sure SDK does not use it in code or it can FREEZE
typedef std::recursive_mutex RMutex; // Make sure SDK does not use it in code or it can FREEZE

#endif