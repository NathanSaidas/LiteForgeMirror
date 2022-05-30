// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#pragma once

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