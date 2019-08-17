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
#ifndef LF_RUNTIME_ASSET_CACHE_CONTROLLER_H
#define LF_RUNTIME_ASSET_CACHE_CONTROLLER_H

#include "Core/Utility/Array.h"
#include "Runtime/Asset/AssetIndex.h"
#include "Runtime/Asset/CacheBlock.h"

namespace lf {

using CacheBlobStatArray = TArray<CacheBlobStats>;

class LF_RUNTIME_API AssetCacheController
{
public:
    using CacheNameIndex = AssetUtil::DefaultNameIndex;

    AssetCacheController();
    ~AssetCacheController();

    // Create a cache block and update the index afterwards
    bool CreateBlock(const Token& name, UInt32 defaultCapacity = 8 * 1024 * 1024);
    // Create a cache block but don't update the index right away!
    bool LazyCreateBlock(const Token& name, UInt32 defaultCapacity = 8 * 1024 * 1024);

    CacheBlockIndex FindCacheBlockIndex(const Token& name) const;

    CacheIndex Create(CacheBlockIndex index, UInt32 uid, UInt32 size);
    CacheIndex Update(CacheBlockIndex index, CacheIndex object, UInt32 size);
    CacheIndex Destroy(CacheBlockIndex index, CacheIndex object);
    CacheIndex Find(CacheBlockIndex index, UInt32 uid);

    void UpdateIndex() const;

    CacheBlobStatArray GetBlobStats() const;
private:
    CacheBlock* FindBlock(const Token& name);
    CacheBlock* LazyFindBlock(const Token& name);

    mutable CacheNameIndex mCacheNameIndex;
    TArray<CacheBlock>     mCacheBlocks;
    mutable bool mDirtyIndex;

};

} // namespace lf 

#endif // LF_RUNTIME_ASSET_CACHE_CONTROLLER_H