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
#include "AssetCacheController.h"

namespace lf {


AssetCacheController::AssetCacheController() :
mCacheNameIndex(),
mCacheBlocks(),
mDirtyIndex(false)
{

}
AssetCacheController::~AssetCacheController()
{

}
    
bool AssetCacheController::CreateBlock(const Token& name, UInt32 defaultCapacity)
{
    if (FindBlock(name))
    {
        return false;
    }
    mCacheBlocks.Add(CacheBlock());
    CacheBlock& block = mCacheBlocks.GetLast();
    block.Initialize(name, defaultCapacity);
    UpdateIndex();
    return true;
}
bool AssetCacheController::LazyCreateBlock(const Token& name, UInt32 defaultCapacity )
{
    if (LazyFindBlock(name))
    {
        return false;
    }
    mCacheBlocks.Add(CacheBlock());
    CacheBlock& block = mCacheBlocks.GetLast();
    block.Initialize(name, defaultCapacity);
    mDirtyIndex = true;
    return true;
}

UInt32 AssetCacheController::FindCacheBlockIndex(const Token& name) const
{
    if (mDirtyIndex)
    {
        UpdateIndex();
    }
    return mCacheNameIndex.Find(name.CStr());
}

CacheIndex AssetCacheController::Create(CacheBlockIndex index, UInt32 uid, UInt32 size)
{
    if (Invalid(index) || index >= mCacheBlocks.Size())
    {
        return CacheIndex();
    }
    return mCacheBlocks[index].Create(uid, size);
}
CacheIndex AssetCacheController::Update(CacheBlockIndex index, CacheIndex object, UInt32 size)
{
    if (Invalid(index) || index >= mCacheBlocks.Size())
    {
        return CacheIndex();
    }
    return mCacheBlocks[index].Update(object, size);
}
CacheIndex AssetCacheController::Destroy(CacheBlockIndex index, CacheIndex object)
{
    if (Invalid(index) || index >= mCacheBlocks.Size())
    {
        return CacheIndex();
    }
    return mCacheBlocks[index].Destroy(object);
}
CacheIndex AssetCacheController::Find(CacheBlockIndex index, UInt32 uid)
{
    if (Invalid(index) || index >= mCacheBlocks.Size())
    {
        return CacheIndex();
    }
    return mCacheBlocks[index].Find(uid);
}

void AssetCacheController::UpdateIndex() const
{
    mCacheNameIndex.Clear();
    CacheNameIndex::TraitsType::BuilderType builder;
    for(SizeT i = 0; i < mCacheBlocks.Size(); ++i)
    {
        const CacheBlock& block = mCacheBlocks[i];
        builder.Add(std::make_pair(block.GetName().CStr(), static_cast<UInt32>(i)));
    }
    std::sort(builder.begin(), builder.end());
    mCacheNameIndex.Build(builder);
    mDirtyIndex = false;
}

CacheBlobStatArray AssetCacheController::GetBlobStats() const
{
    CacheBlobStatArray stats;
    for (const CacheBlock& block : mCacheBlocks)
    {
        for (SizeT i = 0; i < block.GetNumBlobs(); ++i)
        {
            stats.Add(block.GetBlobStat(i));
        }
    }
    return stats;
}

CacheBlock* AssetCacheController::FindBlock(const Token& name)
{
    if (mDirtyIndex)
    {
        UpdateIndex();
    }
    UInt32 index = mCacheNameIndex.Find(name.CStr());
    return Invalid(index) ? nullptr : &mCacheBlocks[index];
}
CacheBlock* AssetCacheController::LazyFindBlock(const Token& name)
{
    if (mDirtyIndex)
    {
        for (CacheBlock& block : mCacheBlocks)
        {
            if (block.GetName() == name)
            {
                return &block;
            }
        }
        return nullptr;
    }
    return FindBlock(name);
}

}