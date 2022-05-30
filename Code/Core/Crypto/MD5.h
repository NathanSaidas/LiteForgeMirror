// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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

class LF_CORE_API LF_ALIGN(16) MD5Hash
{
public:
    // **********************************
    // Constructs a 'empty' hash by default. 
    // Empty hashes are an array of bytes filled
    // with zeroes.
    // **********************************
    LF_INLINE MD5Hash();
    LF_INLINE ~MD5Hash();
    LF_INLINE MD5Hash(const MD5Hash & other);
    // **********************************
    // Computes the SHA256 with the given set of bytes.
    // **********************************
    LF_INLINE MD5Hash(const ByteT * data, SizeT dataLength);

    LF_INLINE MD5Hash& operator=(const MD5Hash & other);
    LF_INLINE bool operator==(const MD5Hash & other) const;
    LF_INLINE bool operator!=(const MD5Hash & other) const;
    LF_INLINE bool operator<(const MD5Hash & other) const;

    LF_INLINE bool Empty() const;
    LF_INLINE SizeT Size() const;

    LF_INLINE ByteT* Bytes();
    LF_INLINE const ByteT* Bytes() const;

    // **********************************
    // Computes the SHA256 with the given set of bytes.
    // **********************************
    void Compute(const ByteT * data, SizeT dataLength);
private:
    internal_ivector mBytes[1];
};
LF_STATIC_ASSERT(sizeof(MD5Hash) == 16);

MD5Hash::MD5Hash()
: mBytes()
{
    mBytes[0] = ivector_zero;
}
MD5Hash::~MD5Hash()
{
}
MD5Hash::MD5Hash(const MD5Hash& other)
: mBytes()
{
    mBytes[0] = other.mBytes[0];
}

MD5Hash::MD5Hash(const ByteT* data, SizeT dataLength)
: mBytes()
{
    Compute(data, dataLength);
}

MD5Hash& MD5Hash::operator=(const MD5Hash& other)
{
    mBytes[0] = other.mBytes[0];
    return *this;
}
bool MD5Hash::operator==(const MD5Hash& other) const
{
    return ivector_cmp(mBytes[0], other.mBytes[0]);
}
bool MD5Hash::operator!=(const MD5Hash& other) const
{
    return ivector_ncmp(mBytes[0], other.mBytes[0]);
}
bool MD5Hash::operator<(const MD5Hash& other) const
{
    return memcmp(Bytes(), other.Bytes(), Size()) < 0;
}

bool MD5Hash::Empty() const
{
    return ivector_cmp(mBytes[0], ivector_zero);
}
SizeT MD5Hash::Size() const
{
    return sizeof(mBytes);
}

ByteT* MD5Hash::Bytes()
{
    return reinterpret_cast<ByteT*>(&mBytes[0]);
}
const ByteT* MD5Hash::Bytes() const
{
    return reinterpret_cast<const ByteT*>(&mBytes[0]);
}

} // namespace Crypto
} // namespace lf