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

class LF_CORE_API MemoryBuffer
{
public:
    // **********************************
    // Ownership flag gives the user the ability
    // to provide the MemoryBuffer a static buffer
    // of memory that can be read/modified.
    //
    // STATIC - Memory is owned by the user (buffer cannot be resized)
    // DYNAMIC - Memory is owned by the memory buffer (buffer can be resized)
    // **********************************
    enum Ownership
    {
        STATIC,
        DYNAMIC
    };

    MemoryBuffer(const MemoryBuffer&) = delete;
    MemoryBuffer& operator=(const MemoryBuffer&) = delete;

    MemoryBuffer();
    MemoryBuffer(void* data, SizeT capacity, Ownership ownership = STATIC);
    // TODO: Implement a 'STATIC READ ONLY'
    MemoryBuffer(const void* data, SizeT capacity, Ownership ownership = STATIC);
    MemoryBuffer(MemoryBuffer&& other);
    ~MemoryBuffer();

    MemoryBuffer& operator=(MemoryBuffer&& other);

    // **********************************
    // Swaps the contents of 'this' memory buffer
    // with the 'other' memory buffer
    // **********************************
    void Swap(MemoryBuffer& other);

    void Copy(const MemoryBuffer& other);

    // **********************************
    // Attempts to allocate the specified number of bytes
    // on the specified alignment.
    // **********************************
    bool Allocate(SizeT bytes, SizeT alignment);
    // **********************************
    // Attempts to reallocate the buffer to the specified number of bytes
    // on the specified alignment.
    // This operation maintains the values contained within the current memory
    // **********************************
    bool Reallocate(SizeT bytes, SizeT alignment);
    // **********************************
    // Frees the memory in the buffer.
    //
    // note: For static ownership this method is a nop
    // **********************************
    void Free();

    void SetSize(SizeT size);
    SizeT GetSize() const { return mSize; }
    SizeT GetCapacity() const { return mCapacity; }
    SizeT GetAlignedCapacity() const;
    void* GetData() { return mPaddedData; }
    const void* GetData() const { return mPaddedData; }
    Ownership GetOwnership() const { return mOwnership; }
private:
    // Pointer to the actual memory allocated
    void* mData;
    // Pointer to where the users should read/write from (in case we must pad for alignment)
    void* mPaddedData;
    SizeT mSize;
    SizeT mCapacity;
    Ownership mOwnership;
};
} // namespace lf