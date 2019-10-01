// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

// todo: On windows 7 we must implement an alternative way as WakeByAddressAll/WakeByAddressSingle/WaitOnAddress are not supported.

void ThreadSignal::Wait()
{
    AssertEx(WaitOnAddress(&mValue, &mValueDummy, sizeof(Int32), INFINITE) == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
    // CriticalAssertMsgEx("Unsupported operation", LF_ERROR_INTERNAL, ERROR_API_CORE);
}
void ThreadSignal::WakeOne()
{
    WakeByAddressSingle(&const_cast<Int32&>(mValue));
    // CriticalAssertMsgEx("Unsupported operation", LF_ERROR_INTERNAL, ERROR_API_CORE);
}
void ThreadSignal::WakeAll()
{
    WakeByAddressAll(&const_cast<Int32&>(mValue));
    // CriticalAssertMsgEx("Unsupported operation", LF_ERROR_INTERNAL, ERROR_API_CORE);
}

}