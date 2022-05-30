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
#include "Runtime/PCH.h"
#include "MemoryResourceLocator.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Utility/Utility.h"
namespace lf {

MemoryResourceLocator::MemoryResourceLocator()
: FileResourceLocator()
, mResourceLock()
, mResources()
{
}
MemoryResourceLocator::~MemoryResourceLocator()
{

}

bool MemoryResourceLocator::WriteResource(const String& name, const TVector<ByteT>& resource, const DateTime& lastModified)
{
    ResourcePtr res;
    {
        ScopeRWSpinLockRead lock(mResourceLock);
        auto iter = mResources.find(name);
        if (iter != mResources.end())
        {
            if (!iter->second)
            {
                iter->second = ResourcePtr(LFNew<Resource>());
            }
            res = iter->second;
        }
    }

    if (!res)
    {
        res = ResourcePtr(LFNew<Resource>());
        ScopeRWSpinLockWrite lock(mResourceLock);
        mResources.insert(std::make_pair(name, res));
    }

    res->mData = resource;
    res->mLastModifyTime = lastModified;
    
    Crypto::SHA256Hash hash(resource.data(), resource.size());
    memcpy(res->mHash, hash.Bytes(), hash.Size());
    return true;
}
bool MemoryResourceLocator::DeleteResource(const String& name) 
{
    ScopeRWSpinLockWrite lock(mResourceLock);
    return mResources.erase(name) == 1;
}

bool MemoryResourceLocator::QueryResourceInfo(const String& resourceName, FileResourceInfo& info) const
{
    ScopeRWSpinLockRead lock(mResourceLock);
    auto iter = mResources.find(resourceName);
    if (iter == mResources.end())
    {
        return false;
    }

    Resource* resource = iter->second;

    info.mName = resourceName;
    info.mLastModifyTime = resource->mLastModifyTime;
    info.mSize = resource->mData.size();
    memcpy(info.mHash, resource->mHash, sizeof(info.mHash));
    info.mFragmentCount = FileResourceUtil::FileSizeToFragmentCount(info.mSize);
    info.mChunkCount = FileResourceUtil::FragmentCountToChunkCount(info.mFragmentCount);
    return true;
}
bool MemoryResourceLocator::QueryChunk(const FileResourceInfo& resourceInfo, SizeT chunkID, FileResourceChunk& chunk) const
{
    ScopeRWSpinLockRead lock(mResourceLock);
    auto iter = mResources.find(resourceInfo.mName);
    if (iter == mResources.end())
    {
        return false;
    }

    Resource* resource = iter->second;
    if (resource->mLastModifyTime != resourceInfo.mLastModifyTime) 
    {
        return false;
    }

    const SizeT MAX_CHUNK_SIZE = FILE_SERVER_MAX_FRAGMENT_SIZE * FILE_SERVER_MAX_FRAGMENTS_IN_CHUNK;
    SizeT offset = chunkID * MAX_CHUNK_SIZE;
    if (offset >= resource->mData.size())
    {
        return false;
    }
    SizeT length = Min(MAX_CHUNK_SIZE, resource->mData.size() - offset);

    chunk.mData.resize(length);
    memcpy(chunk.mData.data(), &resource->mData[offset], length);
    return true;
}

}