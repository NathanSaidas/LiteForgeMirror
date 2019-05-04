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
#include "IOCPQueue.h"

#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Utility/ErrorCore.h"

#ifdef LF_OS_WINDOWS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace lf { namespace InternalHooks {

struct IOCPQueueImpl
{
    HANDLE mPort;

};

void IOCPInitialize(IOCPQueueImpl*& self, SizeT numConsumers)
{
    AssertError(!self, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    self = LFNew<IOCPQueueImpl>();
    AssertError(self, LF_ERROR_OUT_OF_MEMORY, ERROR_API_CORE);
    if (!self)
    {
        return;
    }
    AssertError(numConsumers > 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    self->mPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, static_cast<DWORD>(numConsumers));
    if (!self->mPort)
    {
        ReportBug("Failed to create IO CompletionPort for IOCPQueue", LF_ERROR_INTERNAL, ERROR_API_CORE);
    }
}
void IOCPRelease(IOCPQueueImpl*& self)
{
    AssertError(self, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    if (self->mPort)
    {
        if (CloseHandle(self->mPort) == FALSE)
        {
            ReportBug("Failed to release IO CompletionPort for IOCPQueue", LF_ERROR_INTERNAL, ERROR_API_CORE);
        }
    }
    LFDelete<IOCPQueueImpl>(self);
    self = nullptr;
}
bool IOCPEnqueue(IOCPQueueImpl* self, SizeT itemSize, void* item)
{
    AssertError(self, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(self->mPort, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    BOOL result = PostQueuedCompletionStatus(self->mPort, static_cast<DWORD>(itemSize), reinterpret_cast<ULONG_PTR>(item), NULL);
    return result != FALSE;
}
bool IOCPDequeue(IOCPQueueImpl* self, void*& item)
{
    AssertError(self, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(self->mPort, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);


    DWORD bytes = 0;
    ULONG_PTR rawItem = NULL;
    LPOVERLAPPED overlapped = 0;
    BOOL result = GetQueuedCompletionStatus(self->mPort, &bytes, &rawItem, &overlapped, 0);
    if (result == TRUE)
    {
        item = reinterpret_cast<void*>(rawItem);
        return true;
    }
    return false;
}

} } // namespace lf::InternalHooks