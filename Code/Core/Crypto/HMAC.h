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
#include "Core/Math/SSECommon.h"
#include <cstring>

namespace lf {
namespace Crypto {
class HMACBuffer;

class LF_CORE_API LF_ALIGN(16) HMACKey
{
public:
    LF_INLINE HMACKey();
    LF_INLINE HMACKey(const HMACKey& other);
    LF_INLINE ~HMACKey();
    LF_INLINE HMACKey& operator=(const HMACKey& other);

    LF_INLINE bool Empty() const;
    LF_INLINE SizeT Size() const;

    LF_INLINE ByteT* Bytes();
    LF_INLINE const ByteT* Bytes() const;

    LF_INLINE bool Load(const ByteT* bytes, SizeT bytesLength);
    bool Generate();
    bool Compute(const ByteT* data, SizeT dataLength, HMACBuffer& outBuffer) const;
private:
    internal_ivector mBytes[2];
};
LF_STATIC_ASSERT(sizeof(HMACKey) == 32);

class LF_ALIGN(16) HMACBuffer
{
public:
    LF_INLINE HMACBuffer();
    LF_INLINE HMACBuffer(const HMACBuffer& other);
    LF_INLINE ~HMACBuffer();
    LF_INLINE HMACBuffer& operator=(const HMACBuffer& other);

    LF_INLINE bool operator==(const HMACBuffer& other) const;
    LF_INLINE bool operator!=(const HMACBuffer& other) const;

    LF_INLINE bool Empty() const;
    LF_INLINE SizeT Size() const;
    
    LF_INLINE ByteT* Bytes();
    LF_INLINE const ByteT* Bytes() const;
private:
    internal_ivector mBytes[2];
};
LF_STATIC_ASSERT(sizeof(HMACBuffer) == 32);


// HMAC Key

HMACKey::HMACKey()
: mBytes()
{
    mBytes[0] = mBytes[1] = ivector_zero;
}
HMACKey::HMACKey(const HMACKey& other) 
{
    mBytes[0] = other.mBytes[0];
    mBytes[1] = other.mBytes[1];
}
HMACKey::~HMACKey()
{
    mBytes[0] = mBytes[1] = ivector_zero;
}
HMACKey& HMACKey::operator=(const HMACKey& other)
{
    mBytes[0] = other.mBytes[0];
    mBytes[1] = other.mBytes[1];
    return *this;
}

bool HMACKey::Empty() const { return ivector_cmp(mBytes[0], ivector_zero) && ivector_cmp(mBytes[1], ivector_zero); }
SizeT HMACKey::Size() const { return sizeof(mBytes); }
ByteT* HMACKey::Bytes() { return reinterpret_cast<ByteT*>(&mBytes[0]); }
const ByteT* HMACKey::Bytes() const { return reinterpret_cast<const ByteT*>(&mBytes[0]); }
bool HMACKey::Load(const ByteT* bytes, SizeT bytesLength) 
{ 
    if (bytes && bytesLength == Size())
    {
        memcpy(Bytes(), bytes, bytesLength); 
        return true;
    }
    return false;
}

// HMAC Buffer

HMACBuffer::HMACBuffer()
{
    mBytes[0] = mBytes[1] = ivector_zero;
}
HMACBuffer::HMACBuffer(const HMACBuffer& other)
{
    mBytes[0] = other.mBytes[0];
    mBytes[1] = other.mBytes[1];
}
HMACBuffer::~HMACBuffer()
{
    mBytes[0] = mBytes[1] = ivector_zero;
}

HMACBuffer& HMACBuffer::operator=(const HMACBuffer& other)
{
    mBytes[0] = other.mBytes[0];
    mBytes[1] = other.mBytes[1];
    return *this;
}

bool HMACBuffer::operator==(const HMACBuffer& other) const
{
    return ivector_cmp(mBytes[0], other.mBytes[0]) && ivector_cmp(mBytes[1], other.mBytes[1]);
}

bool HMACBuffer::operator!=(const HMACBuffer& other) const
{
    return ivector_ncmp(mBytes[0], other.mBytes[0]) || ivector_ncmp(mBytes[1], other.mBytes[1]);
}

bool HMACBuffer::Empty() const { return ivector_cmp(mBytes[0], ivector_zero) && ivector_cmp(mBytes[1], ivector_zero); }
SizeT HMACBuffer::Size() const { return sizeof(mBytes); }

ByteT* HMACBuffer::Bytes() { return reinterpret_cast<ByteT*>(&mBytes[0]); }
const ByteT* HMACBuffer::Bytes() const { return reinterpret_cast<const ByteT*>(&mBytes[0]); }

} // namespace Crypto
} // namespace lf