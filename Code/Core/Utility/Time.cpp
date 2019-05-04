// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "Time.h"

#ifdef LF_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace lf {

Int64 GetClockFrequency()
{
#ifdef LF_OS_WINDOWS
    LARGE_INTEGER i;
    QueryPerformanceFrequency(&i);
    return i.QuadPart;
#else
    LF_STATIC_CRASH("Missing platform implementation.");
    return 0;
#endif
}
Int64 GetClockTime()
{
#ifdef LF_OS_WINDOWS
    LARGE_INTEGER i;
    QueryPerformanceCounter(&i);
    return i.QuadPart;
#else
    LF_STATIC_CRASH("Missing platform implementation.");
    return 0;
#endif
}

}