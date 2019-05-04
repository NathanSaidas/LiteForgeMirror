#ifndef LF_RUNTIME_ASSET_CACHE_CONTROLLER_H
#define LF_RUNTIME_ASSET_CACHE_CONTROLLER_H

#include "Core/Utility/Array.h"
#include "Runtime/Asset/AssetIndex.h"
#include "Runtime/Asset/CacheBlock.h"

namespace lf {

using CacheBlobStatArray = TArray<CacheBlobStats>;

class AssetCacheController
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