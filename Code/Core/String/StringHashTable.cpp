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
#include "Core/PCH.h"
#include "StringHashTable.h"

namespace lf {

StringHashTable::StringHashTable() : mMap(), mSize(0) {}
StringHashTable::~StringHashTable() { Clear(); }

StringHashTable::HashedString StringHashTable::Create(const char* string, SizeT len)
{
    if (Invalid(len))
    {
        len = StrLen(string);
    }
    Assert(len > 0);
    const char* stringEnd = string + len;

    // hash(s)
    HashType hash = FNV::Hash(string, len);
    Assert(Valid(hash));
    // bucket(h, s)
    auto& bucket = mMap[hash];

    auto strIter = bucket.mStrings.begin();
    auto strEnd = bucket.mStrings.end();
    auto lenIter = bucket.mStringLengths.begin();
    auto lenEnd = bucket.mStringLengths.end();

    for (; strIter != strEnd; ++strIter, ++lenIter)
    {
        if (StrEqual(*strIter, string, (*strIter) + *lenIter, stringEnd))
        {
            return HashedString(hash, *strIter);
        }
    }

    // bucket_alloc(h, s)
    char* hashedString = reinterpret_cast<char*>(LFAlloc(len + 1, LF_SIMD_ALIGN));
    memcpy(hashedString, string, len);
    hashedString[len] = '\0';

    bucket.mName = hash;
    bucket.mStrings.push_back(hashedString);
    bucket.mStringLengths.push_back(static_cast<UInt32>(len));

    ++mSize;

    return HashedString(hash, hashedString);
}

StringHashTable::HashedString StringHashTable::Find(const char* string, SizeT len) const
{
    if (Invalid(len))
    {
        len = StrLen(string);
    }
    Assert(len > 0);
    const char* stringEnd = string + len;

    // hash(s)
    HashType hash = FNV::Hash(string, len);
    Assert(Valid(hash));

    // bucket(h, s)
    auto bucket = mMap.find(hash);
    if (bucket != mMap.end())
    {
        auto strIter = bucket->second.mStrings.begin();
        auto strEnd = bucket->second.mStrings.end();
        auto lenIter = bucket->second.mStringLengths.begin();
        auto lenEnd = bucket->second.mStringLengths.end();

        for (; strIter != strEnd; ++strIter, ++lenIter)
        {
            if (StrEqual(*strIter, string, (*strIter) + *lenIter, stringEnd))
            {
                return HashedString(hash, *strIter);
            }
        }
    }
    return HashedString(INVALID, nullptr);
}

void StringHashTable::Clear()
{
    // free all allocated strings
    for (auto& bucket : mMap)
    {
        for (auto& str : bucket.second.mStrings)
        {
            LFFree(str);
        }
    }
    mMap.clear();
    mSize = 0;
}

SizeT StringHashTable::Collisions() const
{
    SizeT val = 0;
    for (auto& bucket : mMap)
    {
        if (bucket.second.mStrings.size() > 1)
        {
            ++val;
        }
    }
    return val;
}

}