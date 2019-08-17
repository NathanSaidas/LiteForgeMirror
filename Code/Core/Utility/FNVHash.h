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
#ifndef LF_CORE_FNV_HASH_H
#define LF_CORE_FNV_HASH_H

#include "Core/Common/Types.h"

namespace lf {

// **********************************
// Fowler-Noll-Vo hash function: https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
// 
// note: 
//   - This hash function is not a cryptographic hash function
//   - This hash function is sensitive to the number 0.    
// **********************************
namespace FNV {
using HashT = UInt64;
const UInt64 FNV_OFFSET_BASIS = 0xCBF29CE484222325;
const UInt64 FNV_PRIME = 0x100000001B3;

LF_INLINE HashT Hash(const ByteT* data, SizeT numBytes)
{
    HashT hash = FNV_OFFSET_BASIS;
    const ByteT* last = data + numBytes;
    while (data != last)
    {
        hash = hash * FNV_PRIME;
        hash = hash ^ *data;
        ++data;
    }
    return hash;
}

LF_INLINE HashT Hash(const SByteT* data, SizeT numBytes)
{
    HashT hash = FNV_OFFSET_BASIS;
    const SByteT* last = data + numBytes;
    while (data != last)
    {
        hash = hash * FNV_PRIME;
        hash = hash ^ static_cast<ByteT>(*data);
        ++data;
    }
    return hash;
}

LF_INLINE HashT Hash1A(const ByteT* data, SizeT numBytes)
{
    HashT hash = FNV_OFFSET_BASIS;
    const ByteT* last = data + numBytes;
    while (data != last)
    {
        hash = hash ^ *data;
        hash = hash * FNV_PRIME;
        ++data;
    }
    return hash;
}

LF_INLINE HashT Hash1A(const SByteT* data, SizeT numBytes)
{
    HashT hash = FNV_OFFSET_BASIS;
    const SByteT* last = data + numBytes;
    while (data != last)
    {
        hash = hash ^ static_cast<ByteT>(*data);
        hash = hash * FNV_PRIME;
        ++data;
    }
    return hash;
}

}


} // namespace lf

#endif // LF_CORE_FNV_HASH_H