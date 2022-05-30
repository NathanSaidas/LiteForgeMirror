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
#include "Core/String/StringUtil.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/FNVHash.h"
#include <unordered_map>

namespace lf {


class LF_CORE_API StringHashTable
{
public:
    using HashType = FNV::HashT;
    struct HashedString
    {
        HashedString() : mHash(0), mString(nullptr) {}
        HashedString(HashType hash, const char* string) : mHash(hash), mString(string) {}

        bool Valid() { return mString != nullptr; } 

        HashType    mHash;
        const char* mString;
    };
    struct Bucket
    {
        HashType       mName;
        TVector<char*>  mStrings;
        TVector<UInt32> mStringLengths;
    };

    StringHashTable();
    ~StringHashTable();

    HashedString Create(const char* string, SizeT len = INVALID);
    HashedString Find(const char* string, SizeT len = INVALID) const;
    void Clear();
    
    SizeT Size() const { return mSize; }
    SizeT Collisions() const;

private:
    std::unordered_map<HashType, Bucket> mMap;
    SizeT mSize;
};

} // namespace lf