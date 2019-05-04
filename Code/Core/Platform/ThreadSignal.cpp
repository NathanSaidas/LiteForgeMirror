// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "ThreadSignal.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"

#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif


namespace lf {

void ThreadSignal::Wait()
{
    AssertError(WaitOnAddress(&mValue, &mValueDummy, sizeof(Int32), INFINITE) == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
}
void ThreadSignal::WakeOne()
{
    WakeByAddressSingle(&const_cast<Int32&>(mValue));
}
void ThreadSignal::WakeAll()
{
    WakeByAddressAll(&const_cast<Int32&>(mValue));
}

}