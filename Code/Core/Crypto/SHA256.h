// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#ifndef LF_CORE_SHA_256_H
#define LF_CORE_SHA_256_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

namespace Crypto
{

const SizeT SHA256_BLOCK_SIZE = 32;

struct SHA256Context
{
    ByteT  mData[64];
    UInt32 mDataLength;
    UInt64 mBitLen;
    UInt32 mState[8];
};

struct SHA256HashType
{
    ByteT mData[SHA256_BLOCK_SIZE];
};

// **********************************
// Initializes the SHA256 context
// 
// note: Call this before Update/Final call
// **********************************
LF_CORE_API void SHA256Init(SHA256Context* context);
// **********************************
// Updates the SHA256 context
// 
// note: Call this after Init as many times as you need with your data you need to hash
// 
// @param data -- Pointer to an array of data to be hashed.
// @param dataLength -- The size of the data being hashed.
// **********************************
LF_CORE_API void SHA256Update(SHA256Context* context, const ByteT* data, SizeT dataLength);
// **********************************
// Completes the SHA256 hash computation and copies the final hash to 'hash' variable
// 
// @param hash -- A pointer to a block of memory at least SHA256_BLOCK_SIZE large that holds the computed hash
// **********************************
LF_CORE_API void SHA256Final(SHA256Context* context, ByteT* hash);
// **********************************
// An all in one function that computes the SHA256 Hash for a single set of data.
// **********************************
LF_CORE_API SHA256HashType SHA256Hash(const ByteT* data, SizeT dataLength);
LF_CORE_API SHA256HashType SHA256Hash(const ByteT* data, SizeT dataLength, const ByteT* salt, SizeT saltLength);


} // namespace Crypto

} // namespace lf

#endif // LF_CORE_SHA_256_H