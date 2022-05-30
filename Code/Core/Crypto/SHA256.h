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

namespace Crypto
{
class LF_CORE_API LF_ALIGN(16) SHA256Hash
{
public:
    // **********************************
    // Constructs a 'empty' hash by default. 
    // Empty hashes are an array of bytes filled
    // with zeroes.
    // **********************************
    LF_INLINE SHA256Hash();
    LF_INLINE ~SHA256Hash();
    LF_INLINE SHA256Hash(const SHA256Hash& other);
    // **********************************
    // Computes the SHA256 with the given set of bytes.
    // **********************************
    LF_INLINE SHA256Hash(const ByteT* data, SizeT dataLength);

    LF_INLINE SHA256Hash& operator=(const SHA256Hash& other);
    LF_INLINE bool operator==(const SHA256Hash& other) const;
    LF_INLINE bool operator!=(const SHA256Hash& other) const;
    LF_INLINE bool operator<(const SHA256Hash& other) const;

    LF_INLINE bool Empty() const;
    LF_INLINE SizeT Size() const;

    LF_INLINE ByteT* Bytes();
    LF_INLINE const ByteT* Bytes() const;

    // **********************************
    // Computes the SHA256 with the given set of bytes.
    // **********************************
    void Compute(const ByteT* data, SizeT dataLength);
private:
    internal_ivector mBytes[2];
};
LF_STATIC_ASSERT(sizeof(SHA256Hash) == 32);


SHA256Hash::SHA256Hash()
: mBytes()
{
    mBytes[0] = mBytes[1] = ivector_zero;
}
SHA256Hash::~SHA256Hash()
{
}
SHA256Hash::SHA256Hash(const SHA256Hash& other)
: mBytes()
{
    mBytes[0] = other.mBytes[0];
    mBytes[1] = other.mBytes[1];
}

SHA256Hash::SHA256Hash(const ByteT* data, SizeT dataLength)
: mBytes()
{
    Compute(data, dataLength);
}

SHA256Hash& SHA256Hash::operator=(const SHA256Hash& other)
{
    mBytes[0] = other.mBytes[0];
    mBytes[1] = other.mBytes[1];
    return *this;
}
bool SHA256Hash::operator==(const SHA256Hash& other) const
{
    return ivector_cmp(mBytes[0], other.mBytes[0]) && ivector_cmp(mBytes[1], other.mBytes[1]);
}
bool SHA256Hash::operator!=(const SHA256Hash& other) const
{
    return ivector_ncmp(mBytes[0], other.mBytes[0]) || ivector_cmp(mBytes[1], other.mBytes[1]);
}
bool SHA256Hash::operator<(const SHA256Hash& other) const
{
    return memcmp(Bytes(), other.Bytes(), Size()) < 0;
}

bool SHA256Hash::Empty() const
{
    return ivector_cmp(mBytes[0], ivector_zero) && ivector_cmp(mBytes[1], ivector_zero);
}
SizeT SHA256Hash::Size() const
{
    return sizeof(mBytes);
}

ByteT* SHA256Hash::Bytes()
{
    return reinterpret_cast<ByteT*>(&mBytes[0]);
}
const ByteT* SHA256Hash::Bytes() const
{
    return reinterpret_cast<const ByteT*>(&mBytes[0]);
}

} // namespace Crypto

} // namespace lf
