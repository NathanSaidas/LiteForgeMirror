// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "AsyncIOBuffer.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"

#include <intrin.h>

namespace lf {

void AsyncIOBuffer::SetState(AsyncIOState state) volatile
{
    _InterlockedExchange(&mState, state);
}
bool AsyncIOBuffer::IsDone() const
{
    bool result = mState != ASYNC_IO_WAITING;
    _ReadWriteBarrier();
    return result;
}

void AsyncIOBuffer::SetBuffer(void* buffer)
{
    AssertError(mState != ASYNC_IO_WAITING, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    mBuffer = buffer;
    _ReadWriteBarrier();
}
void* AsyncIOBuffer::GetBuffer() const
{
    return mBuffer;
}

void AsyncIOBuffer::SetBytesTransferred(SizeT bytes) volatile
{
    mBytesTransferred = bytes;
    _ReadWriteBarrier();
}

SizeT AsyncIOBuffer::GetBytesTransferred() const
{
    SizeT result = mBytesTransferred;
    _ReadWriteBarrier();
    return result;
}

} // namespace lf