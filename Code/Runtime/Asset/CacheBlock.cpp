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
#include "Runtime/PCH.h"
#include "CacheBlock.h"
#include "Core/IO/Stream.h"
#include <algorithm>

namespace lf {

namespace CacheBlockError {
const char* ERROR_MSG_INVALID_OPERATION_INITIALIZED = "Invalid operation, the CacheBlock is already initialized.";
const char* ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED = "Invalid operation, the CacheBlock has not been initialized yet.";
const char* ERROR_MSG_INVALID_OPERATION_OBJECT_EXISTS = "Invalid operation, a object with that id already exists!";
const char* ERROR_MSG_INVALID_ARGUMENT_SIZE = "Invalid argument 'size'";
const char* ERROR_MSG_INVALID_ARGUMENT_NAME = "Invalid argument 'name'";
const char* ERROR_MSG_INVALID_ARGUMENT_DEFAULT_CAPACITY = "Invalid argument 'defaultCapacity'";
const char* ERROR_MSG_INVALID_ARGUMENT_UID = "Invalid argument 'uid'";
const char* ERROR_MSG_INVALID_ARGUMENT_INDEX = "Invalid argument 'index'";
}
using namespace CacheBlockError;

CacheBlock::CacheBlock()
: mName()
, mDefaultCapacity(0)
, mIndices()
, mBlobs()
, mLock()
{}
CacheBlock::~CacheBlock()
{}

void CacheBlock::Initialize(const Token& name, UInt32 defaultCapacity)
{
    ScopeRWSpinLockWrite writeLock(mLock);
    if (name.Empty())
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_NAME, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return;
    }

    if (defaultCapacity < 1024) // Needs to be some sane value
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_DEFAULT_CAPACITY, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return;
    }

    if (!mName.Empty() || mDefaultCapacity > 0) 
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_OPERATION_INITIALIZED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return;
    }
    mName = name;
    mDefaultCapacity = defaultCapacity;
}
void CacheBlock::Release()
{
    ScopeRWSpinLockWrite writeLock(mLock);
    mName.Clear();
    mDefaultCapacity = 0;
    mIndices.clear();
    mBlobs.clear();
}

void CacheBlock::Serialize(Stream& s)
{
    ScopeRWSpinLockWrite writeLock(mLock);
    SERIALIZE_STRUCT_ARRAY(s, mIndices, "");
    SERIALIZE_STRUCT_ARRAY(s, mBlobs, "");
}

CacheIndex CacheBlock::Create(UInt32 uid, UInt32 size)
{
    if (Invalid(uid))
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_UID, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (size > mDefaultCapacity || size == 0)
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_SIZE, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (mName.Empty() || mDefaultCapacity == 0)
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (Find(uid))
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_OPERATION_OBJECT_EXISTS, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    ScopeRWSpinLockWrite writeLock(mLock);
    CacheIndex result;
    for (SizeT i = 0; i < mBlobs.size(); ++i)
    {
        CacheBlob& blob = mBlobs[i];
        CacheObjectId id = blob.Reserve(uid, size);
        if (Valid(id))
        {
            result.mUID = uid;
            result.mObjectID = static_cast<UInt32>(id);
            result.mBlobID = static_cast<UInt32>(i);
            mIndices.push_back(result);
            return result;
        }
    }
    mBlobs.push_back(CacheBlob());
    {
        CacheBlob& blob = mBlobs.back();
        blob.Initialize({}, mDefaultCapacity);
        CacheObjectId id = blob.Reserve(uid, size);
        Assert(Valid(id));
        result.mUID = uid;
        result.mObjectID = static_cast<UInt32>(id);
        result.mBlobID = static_cast<UInt32>(mBlobs.size()-1);
        mIndices.push_back(result);
    }

    return result;
}
CacheIndex CacheBlock::Update(CacheIndex index, UInt32 size)
{
    ScopeRWSpinLockWrite writeLock(mLock);
    if (!index)
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    // Size=0 means Use Destroy instead!
    if (size > mDefaultCapacity || size == 0) 
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_SIZE, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (mName.Empty() || mDefaultCapacity == 0)
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    // Search:
    if (index.mBlobID >= mBlobs.size())
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }
    
    CacheObject object;
    if (!mBlobs[index.mBlobID].GetObject(static_cast<CacheObjectId>(index.mObjectID), object) || object.mUID != index.mUID)
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    // TryUpdate:
    UInt32 uid = index.mUID;
    auto iter = std::find_if(mIndices.begin(), mIndices.end(), [uid](const CacheIndex& item) { return item.mUID == uid; });
    Assert(iter != mIndices.end()); // Indices/Blobs out of date!
    CacheIndex result;
    if (!mBlobs[index.mBlobID].Update(static_cast<CacheObjectId>(index.mObjectID), size))
    {
        Assert(mBlobs[index.mBlobID].Destroy(static_cast<CacheObjectId>(index.mObjectID)));
        CacheObjectId objectID = mBlobs[index.mBlobID].Reserve(index.mUID, size);
        if (Valid(objectID))
        {
            result.mUID = index.mUID;
            result.mBlobID = index.mBlobID;
            result.mObjectID = static_cast<UInt32>(objectID);
            iter->mObjectID = result.mObjectID;
            return result;
        }
    }
    else
    {
        result = index;
        return result;
    }

    // Try allocate existing:
    for (UInt32 blobID = 0; blobID < mBlobs.size(); ++blobID)
    {
        if (blobID == index.mBlobID)
        {
            continue;
        }
        CacheObjectId objectID = mBlobs[blobID].Reserve(index.mUID, size);
        if (Valid(objectID))
        {
            result.mUID = index.mUID;
            result.mBlobID = blobID;
            result.mObjectID = static_cast<UInt32>(objectID);
            iter->mBlobID = result.mBlobID;
            iter->mObjectID = result.mObjectID;
            return result;
        }
    }

    // Allocate another blob
    mBlobs.push_back(CacheBlob());
    mBlobs.back().Initialize({}, mDefaultCapacity);
    CacheObjectId id = mBlobs.back().Reserve(index.mUID, size);
    Assert(Valid(id));
    result.mUID = index.mUID;
    result.mBlobID = static_cast<UInt32>(mBlobs.size() - 1);
    result.mObjectID = static_cast<UInt32>(id);
    iter->mBlobID = result.mBlobID;
    iter->mObjectID = result.mObjectID;
    return result;
}
CacheIndex CacheBlock::Destroy(CacheIndex index)
{
    ScopeRWSpinLockWrite writeLock(mLock);
    if (!index)
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (mName.Empty() || mDefaultCapacity == 0)
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return CacheIndex();
    }
    if (index.mBlobID >= mBlobs.size())
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    CacheObject object;
    if (!mBlobs[index.mBlobID].GetObject(static_cast<CacheObjectId>(index.mObjectID), object) || object.mUID != index.mUID)
    {
        ReportBugMsgEx(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    UInt32 uid = index.mUID;
    auto iter = std::find_if(mIndices.begin(), mIndices.end(), [uid](const CacheIndex& item) { return item.mUID == uid; });
    Assert(iter != mIndices.end());
    Assert(mBlobs[index.mBlobID].Destroy(static_cast<CacheObjectId>(index.mObjectID)));
    mIndices.swap_erase(iter);

    return index;
}
CacheIndex CacheBlock::Find(UInt32 uid)
{
    ScopeRWSpinLockRead readLock(mLock);
    for (const CacheIndex& index : mIndices)
    {
        if (index.mUID == uid)
        {
            return index;
        }
    }
    return CacheIndex();
}

bool CacheBlock::DestroyObject(UInt32 uid)
{
    ScopeRWSpinLockWrite writeLock(mLock);
    for (SizeT blobID = 0; blobID < mBlobs.size(); ++blobID)
    {
        for (SizeT objectID = 0; objectID < mBlobs[blobID].Size(); ++objectID)
        {
            CacheObject object;
            if (mBlobs[blobID].GetObject(static_cast<CacheObjectId>(objectID), object) && object.mUID == uid)
            {
                mBlobs[blobID].Destroy(static_cast<CacheObjectId>(objectID));
                return true;
            }
        }
    }
    return false;
}

bool CacheBlock::DestroyIndex(const CacheIndex& cacheIndex)
{
    SizeT removed = 0;
    for (TVector<CacheIndex>::iterator it = mIndices.begin(); it != mIndices.end();)
    {
        if (it->mUID == cacheIndex.mUID)
        {
            it = mIndices.swap_erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }
    return removed > 0;
}

bool CacheBlock::FindObject(UInt32 uid, CacheObject& outObject, CacheIndex& outIndex) const
{
    ScopeRWSpinLockRead readLock(mLock);
    for (SizeT blobID = 0; blobID < mBlobs.size(); ++blobID)
    {
        for (SizeT objectID = 0; objectID < mBlobs[blobID].Size(); ++objectID)
        {
            if (mBlobs[blobID].GetObject(static_cast<CacheObjectId>(objectID), outObject) && outObject.mUID == uid)
            {
                outIndex.mUID = uid;
                outIndex.mBlobID = static_cast<UInt32>(blobID);
                outIndex.mObjectID = static_cast<UInt32>(objectID);

                return true;
            }
        }
    }
    outObject = CacheObject();
    return false;
}

bool CacheBlock::GetObject(CacheIndex index, CacheObject& outObject) const
{
    ScopeRWSpinLockRead readLock(mLock);
    if (index && index.mBlobID < mBlobs.size() && mBlobs[index.mBlobID].GetObject(static_cast<CacheObjectId>(index.mObjectID), outObject))
    {
        return true;
    }
    return false;
}

CacheBlobStats CacheBlock::GetBlobStat(SizeT index) const
{
    ScopeRWSpinLockRead readLock(mLock);
    if (index >= mBlobs.size())
    {
        return CacheBlobStats();
    }
    const CacheBlob& blob = mBlobs[index];
    CacheBlobStats stats;
    stats.mBytesUsed = blob.GetBytesUsed();
    stats.mBytesReserved = blob.GetBytesReserved();
    stats.mBytesFragmented = blob.GetFragmentedBytes();
    stats.mBlobCapacity = blob.GetCapacity();
    stats.mNumObjects = blob.Size();
    stats.mNumObjectsFragmented = blob.GetFragmentedObjects();
    stats.mCacheBlock = mName.CStr();
    stats.mBlobID = index;
    return stats;
}

TVector<CacheDefragStep> CacheBlock::GetDefragSteps() const
{
    ScopeRWSpinLockRead readLock(mLock);
    TVector<CacheDefragStep> steps;
    for(SizeT i = 0; i < mBlobs.size(); ++i)
    {
        const CacheBlob& blob = mBlobs[i];
        for (SizeT k = 0; k < blob.Size(); ++k)
        {
            CacheObject obj;
            if (blob.GetObject(static_cast<CacheObjectId>(k), obj) && Valid(obj.mUID))
            {
                CacheDefragStep step;
                step.mUID = obj.mUID;
                step.mSize = obj.mSize;
                step.mSourceBlobID = static_cast<UInt32>(i);
                step.mSourceObjectID = static_cast<UInt32>(k);
                steps.push_back(step);
            }
        }
    }

    std::sort(steps.begin(), steps.end(),
        [](const CacheDefragStep& a, const CacheDefragStep& b)
        {
            return a.mSize > b.mSize;
        }
    );

    CacheBlock defragger;
    defragger.Initialize(Token("defragger"), mDefaultCapacity);
    for (CacheDefragStep& step : steps)
    {
        CacheIndex index = defragger.Create(step.mUID, step.mSize);
        Assert(index == true);
        step.mDestBlobID = index.mBlobID;
        step.mDestObjectID = index.mObjectID;
    }

    std::sort(steps.begin(), steps.end(),
        [](const CacheDefragStep& a, const CacheDefragStep& b)
        {
            if(a.mDestBlobID >= b.mDestBlobID)
            {
                return false;
            }
            return a.mSize < b.mSize;
        }
    );

    return steps;
}

} // namespace lf