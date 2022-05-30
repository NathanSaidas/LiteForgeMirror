#pragma once
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
#include "Core/Common/API.h"
#include "Core/String/Token.h"
#include "Core/Utility/Array.h"

namespace lf {

class Stream;

// ********************************************************************
// A type mapping provides information to the AssetMgr during initialization
// on what types are available in the cache and their concrete type.
// 
// ********************************************************************
class LF_RUNTIME_API AssetTypeMapping
{
public:
    AssetTypeMapping();

    Token   mPath;
    Token   mParent;
    Token   mConcreteType;
    UInt32  mCacheUID;
    UInt32  mCacheBlobID;
    UInt32  mCacheObjectID;
    UInt32  mWeakReferences;
    UInt32  mStrongReferences;
};
LF_RUNTIME_API Stream& operator<<(Stream& s, AssetTypeMapping& obj);

class LF_RUNTIME_API AssetTypeMap
{
public:
    enum DataType
    {
        BINARY,
        JSON
    };

    bool Read(DataType dataType, const String& path);
    bool Write(DataType dataType, const String& path);

    TVector<AssetTypeMapping>& GetTypes() { return mTypes; }
    const TVector<AssetTypeMapping>& GetTypes() const { return mTypes; }
private:
    bool SerializeCommon(Stream& s);
    TVector<AssetTypeMapping> mTypes;
};

}