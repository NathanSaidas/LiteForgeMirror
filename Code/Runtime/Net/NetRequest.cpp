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
#include "Runtime/PCH.h"
#include "NetRequest.h"
#include <utility>

namespace lf {

DEFINE_CLASS(lf::NetRequest) { NO_REFLECTION; }

NetRequest::NetRequest()
: Super()
, mRoute()
, mBuffer(nullptr)
, mBufferEnd(nullptr)
, mBufferLast(nullptr)
, mLock()
{

}
NetRequest::~NetRequest()
{
    ClearBuffer();
}

bool NetRequest::Write()
{
    // implemented in derived classes.
    return true;
}
bool NetRequest::Read(const ByteT* , SizeT )
{
    // implemented in derived classes
    return true;
}

bool NetRequest::CopyTo(ByteT* destBytes, SizeT destLength)
{
    ScopeMultiLock lock(mLock);
    if (GetBufferLength() > destLength)
    {
        return false;
    }
    memcpy(destBytes, mBuffer, GetBufferLength());
    return true;
}

void NetRequest::ClearBuffer()
{
    if (AllocatedMemory() && mBuffer)
    {
        Free(mBuffer);
        mBuffer = nullptr;
        mBufferEnd = nullptr;
        mBufferLast = nullptr;
    }
}

bool NetRequest::AllocatedMemory()
{
    return true;
}
ByteT* NetRequest::Allocate(SizeT bytes)
{
    return static_cast<ByteT*>(LFAlloc(bytes, 16));
}
void NetRequest::Free(void* pointer)
{
    LFFree(pointer);
}

} // namespace lf