// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_ASYNC_IO_BUFFER_H
#define LF_CORE_ASYNC_IO_BUFFER_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

enum AsyncIOState
{
    ASYNC_IO_IDLE,      // Waiting to be assigned a task
    ASYNC_IO_WAITING,   // Waiting for IO to complete
    ASYNC_IO_DONE       // Results have been copied to buffer and it's ready to be read from.
};

class LF_CORE_API AsyncIOBuffer
{
public:
    AsyncIOBuffer(const AsyncIOBuffer&) = delete;
    AsyncIOBuffer(AsyncIOBuffer&&) = delete;
    AsyncIOBuffer& operator=(const AsyncIOBuffer&) = delete;
    AsyncIOBuffer& operator=(AsyncIOBuffer&&) = delete;

    AsyncIOBuffer() :
    mBuffer(nullptr),
    mState(ASYNC_IO_IDLE),
    mBytesTransferred(0)
    {}

    AsyncIOBuffer(void* buffer) :
    mBuffer(buffer),
    mState(ASYNC_IO_IDLE),
    mBytesTransferred(0)
    {}

    // Changes the state to the specified state.
    void SetState(AsyncIOState state) volatile;
    // Checks if buffer is ready to be accessed.
    bool IsDone() const;

    // Sets the buffer.
    // Note: Must not be waiting for IO
    void SetBuffer(void* buffer);
    // Retrives a pointer to the buffer.
    void* GetBuffer() const;

    // Set the number of bytes read/wrote by the async operation
    void SetBytesTransferred(SizeT bytes) volatile;
    // Returns the number of bytes read/wrote by the async operation
    SizeT GetBytesTransferred() const;
private:
    void* mBuffer;
    volatile long mState;
    volatile SizeT mBytesTransferred;
};
} // namespace lf

#endif 