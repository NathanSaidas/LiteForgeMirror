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