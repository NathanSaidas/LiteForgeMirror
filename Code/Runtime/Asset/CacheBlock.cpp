#include "CacheBlock.h"
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

CacheBlock::CacheBlock() : 
mName(),
mDefaultCapacity(0),
mIndices(),
mBlobs()
{}
CacheBlock::~CacheBlock()
{}

void CacheBlock::Initialize(const Token& name, UInt32 defaultCapacity)
{
    if (name.Empty())
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_NAME, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return;
    }

    if (defaultCapacity < 1024) // Needs to be some sane value
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_DEFAULT_CAPACITY, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return;
    }

    if (!mName.Empty() || mDefaultCapacity > 0) 
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_INITIALIZED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return;
    }
    mName = name;
    mDefaultCapacity = defaultCapacity;
}
void CacheBlock::Release()
{
    mName.Clear();
    mDefaultCapacity = 0;
    mIndices.Clear();
    mBlobs.Clear();
}

CacheIndex CacheBlock::Create(UInt32 uid, UInt32 size)
{
    if (Invalid(uid))
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_UID, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (size > mDefaultCapacity || size == 0)
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_SIZE, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (mName.Empty() || mDefaultCapacity == 0)
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (Find(uid))
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_OBJECT_EXISTS, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    CacheIndex result;
    for (SizeT i = 0; i < mBlobs.Size(); ++i)
    {
        CacheBlob& blob = mBlobs[i];
        CacheObjectId id = blob.Reserve(uid, size);
        if (Valid(id))
        {
            result.mUID = uid;
            result.mObjectID = static_cast<UInt32>(id);
            result.mBlobID = static_cast<UInt32>(i);
            mIndices.Add(result);
            return result;
        }
    }
    mBlobs.Add(CacheBlob());
    {
        CacheBlob& blob = mBlobs.GetLast();
        blob.Initialize({}, mDefaultCapacity);
        CacheObjectId id = blob.Reserve(uid, size);
        Assert(Valid(id));
        result.mUID = uid;
        result.mObjectID = static_cast<UInt32>(id);
        result.mBlobID = static_cast<UInt32>(mBlobs.Size()-1);
        mIndices.Add(result);
    }

    return result;
}
CacheIndex CacheBlock::Update(CacheIndex index, UInt32 size)
{
    if (!index)
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    // Size=0 means Use Destroy instead!
    if (size > mDefaultCapacity || size == 0) 
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_SIZE, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (mName.Empty() || mDefaultCapacity == 0)
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    // Search:
    if (index.mBlobID >= mBlobs.Size())
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }
    
    CacheObject object;
    if (!mBlobs[index.mBlobID].GetObject(static_cast<CacheObjectId>(index.mObjectID), object) || object.mUID != index.mUID)
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
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
    for (UInt32 blobID = 0; blobID < mBlobs.Size(); ++blobID)
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
    mBlobs.Add(CacheBlob());
    mBlobs.GetLast().Initialize({}, mDefaultCapacity);
    CacheObjectId id = mBlobs.GetLast().Reserve(index.mUID, size);
    Assert(Valid(id));
    result.mUID = index.mUID;
    result.mBlobID = static_cast<UInt32>(mBlobs.Size() - 1);
    result.mObjectID = static_cast<UInt32>(id);
    iter->mBlobID = result.mBlobID;
    iter->mObjectID = result.mObjectID;
    return result;
}
CacheIndex CacheBlock::Destroy(CacheIndex index)
{
    if (!index)
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    if (mName.Empty() || mDefaultCapacity == 0)
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return CacheIndex();
    }
    if (index.mBlobID >= mBlobs.Size())
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    CacheObject object;
    if (!mBlobs[index.mBlobID].GetObject(static_cast<CacheObjectId>(index.mObjectID), object) || object.mUID != index.mUID)
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_INDEX, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return CacheIndex();
    }

    UInt32 uid = index.mUID;
    auto iter = std::find_if(mIndices.begin(), mIndices.end(), [uid](const CacheIndex& item) { return item.mUID == uid; });
    Assert(iter != mIndices.end());
    Assert(mBlobs[index.mBlobID].Destroy(static_cast<CacheObjectId>(index.mObjectID)));
    mIndices.SwapRemove(iter);

    return index;
}
CacheIndex CacheBlock::Find(UInt32 uid)
{
    for (const CacheIndex& index : mIndices)
    {
        if (index.mUID == uid)
        {
            return index;
        }
    }
    return CacheIndex();
}

bool CacheBlock::GetObject(CacheIndex index, CacheObject& outObject) const
{
    if (index.mBlobID < mBlobs.Size() && mBlobs[index.mBlobID].GetObject(static_cast<CacheObjectId>(index.mObjectID), outObject))
    {
        return true;
    }
    return false;
}

CacheBlobStats CacheBlock::GetBlobStat(SizeT index) const
{
    if (index >= mBlobs.Size())
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

TArray<CacheDefragStep> CacheBlock::GetDefragSteps() const
{
    TArray<CacheDefragStep> steps;
    for(SizeT i = 0; i < mBlobs.Size(); ++i)
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
                steps.Add(step);
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