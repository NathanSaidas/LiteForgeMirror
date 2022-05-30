#pragma once
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

#include "Core/Reflection/Object.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Platform/SpinLock.h"
#include "Core/String/String.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {

class LF_RUNTIME_API NetRequest : public Object, public TAtomicWeakPointerConvertible<NetRequest>
{
    DECLARE_CLASS(NetRequest, Object);
public:
    using PointerConvertible = PointerConvertibleType;

    NetRequest();
    virtual ~NetRequest();

    // Serialize 'user state' to 'buffer'
    virtual bool Write();
    // Serialize 'buffer' to user state
    virtual bool Read(const ByteT* sourceBytes, SizeT sourceLength);

    // Write request to buffer
    bool CopyTo(ByteT* destBytes, SizeT destLength);
    // Clear the request internal buffers
    void ClearBuffer();

    // Accessors to internal buffer;
    const ByteT* GetBuffer() const { return mBuffer; }
    SizeT GetBufferLength() const { return mBufferEnd - mBuffer; }
    SizeT GetBufferCapacity() const { return mBufferLast - mBuffer; }

    MultiSpinLock& GetLock() { return mLock; }

    const String& GetRoute() const { return mRoute; }
protected:
    virtual bool AllocatedMemory();
    ByteT* Allocate(SizeT bytes);
    void  Free(void* pointer);

    String mRoute; // todo: Could become Token
    ByteT* mBuffer;
    ByteT* mBufferEnd;  // Buffer + Size
    ByteT* mBufferLast; // Buffer + Capacity

private:
    MultiSpinLock mLock;
};

} // namespace lf