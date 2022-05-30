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

#include "Core/Net/NetTypes.h"

namespace lf {

class LF_RUNTIME_API NetResponseArgs
{
public:

    NetResponseArgs();

    bool Set(UInt16 status, UInt32 sourcePacketUID, const void* body, SizeT bodyLength);

    void ReserveBody(void* body, SizeT bodyLength);

    void Clear();

    SizeT Write(ByteT* buffer, const SizeT bufferLength) const;

    bool Read(const ByteT* buffer, const SizeT bufferLength);

    SizeT GetWriteSize() const;

    UInt16 GetStatus() const { return mStatus; }
    UInt32 GetSourcePacketUID() const { return mSourcePacketUID; }
    const void* GetBody() const { return mBody; }
    SizeT GetBodyLength() const { return mBodyLength; }

private:
    UInt16 mStatus;
    UInt32 mSourcePacketUID;
    const void*  mBody;
    SizeT  mBodyLength;

};

} // namespace lf
