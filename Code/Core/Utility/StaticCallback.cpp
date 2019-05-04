// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "StaticCallback.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Utility/ErrorCore.h"
#include <algorithm>

namespace lf {


SafeStaticCallback* gStaticInitCallbacks = nullptr;
SizeT               gStaticInitCallbacksCount = 0;
SafeStaticCallback* gStaticDestroyCallbacks = nullptr;
SizeT               gStaticDestroyCallbacksCount = 0;

static bool                 sInitialized = false;
static bool                 sDestroyed = false;
static bool                 sSorted = false;
static SizeT                sCurrentCallback = 0;
static SafeStaticCallback*  sCallbacks = nullptr;

void RegisterStaticInit(SafeStaticCallback* callback)
{
    if (gStaticInitCallbacks)
    {
        SafeStaticCallback* current = gStaticInitCallbacks;
        while (current->mNext)
        {
            current = current->mNext;
        }
        current->mNext = callback;
    }
    else
    {
        gStaticInitCallbacks = callback;
    }

    ++gStaticInitCallbacksCount;
}

void RegisterStaticDestroy(SafeStaticCallback* callback)
{
    if (gStaticDestroyCallbacks)
    {
        SafeStaticCallback* current = gStaticDestroyCallbacks;
        while (current->mNext)
        {
            current = current->mNext;
        }
        current->mNext = callback;
    }
    else
    {
        gStaticDestroyCallbacks = callback;
    }

    ++gStaticDestroyCallbacksCount;
}


void ExecuteStaticInit(SizeT maxPriority, ProgramContext* programContext)
{
    if (!sSorted)
    {
        sCallbacks = static_cast<SafeStaticCallback*>(LFAlloc(sizeof(SafeStaticCallback) * gStaticInitCallbacksCount, alignof(SafeStaticCallback)));
        SafeStaticCallback* current = gStaticInitCallbacks;
        for (SizeT i = 0; i < gStaticInitCallbacksCount && current; ++i)
        {
            sCallbacks[i] = *current;
            current = current->mNext;
        }
        std::sort(sCallbacks, sCallbacks + gStaticInitCallbacksCount);
        sSorted = true;
    }

    
    for (SizeT i = sCurrentCallback; i < gStaticInitCallbacksCount; ++i)
    {
        if (sCallbacks[i].mPriority > maxPriority)
        {
            sCurrentCallback = i;
            return;
        }
        sCallbacks[i].mCallback(programContext);
        ++sCurrentCallback;
    }

    if (sCurrentCallback >= gStaticInitCallbacksCount)
    {
        LFFree(sCallbacks);
        sCallbacks = nullptr;
        sCurrentCallback = 0;
        sSorted = false;
        sInitialized = true;
    }
}

void ExecuteStaticDestroy(SizeT minPriorty, ProgramContext* programContext)
{
    if (!sSorted)
    {
        // Program should have fully initialized.
        AssertError(sCurrentCallback == 0 && sInitialized, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

        sCallbacks = static_cast<SafeStaticCallback*>(LFAlloc(sizeof(SafeStaticCallback) * gStaticDestroyCallbacksCount, alignof(SafeStaticCallback)));
        SafeStaticCallback* current = gStaticDestroyCallbacks;
        for (SizeT i = 0; i < gStaticDestroyCallbacksCount && current; ++i)
        {
            sCallbacks[i] = *current;
            current = current->mNext;
        }
        std::sort(sCallbacks, sCallbacks + gStaticDestroyCallbacksCount, 
            [](const SafeStaticCallback& a, const SafeStaticCallback& b) -> bool
            {   
                return a > b;
            });
        sSorted = true;
    }
    
    for (SizeT i = sCurrentCallback; i < gStaticDestroyCallbacksCount; ++i)
    {
        if (sCallbacks[i].mPriority < minPriorty)
        {
            sCurrentCallback = i;
            return;
        }
        sCallbacks[i].mCallback(programContext);
        ++sCurrentCallback;
    }

    if (sCurrentCallback >= gStaticDestroyCallbacksCount)
    {
        LFFree(sCallbacks);
        sCallbacks = nullptr;
        sCurrentCallback = 0;
        sSorted = false;
        sDestroyed = true;
    }
}

void StaticInitFence()
{
    // If this triggers it means not all static init callbacks were invoked
    AssertError(sInitialized, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
}
void StaticDestroyFence()
{
    // If this triggers it means not all static destroy callbacks were invoked
    AssertError(sDestroyed, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
}

}