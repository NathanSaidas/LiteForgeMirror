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
#include "CacheBlob.h"

namespace lf {

namespace CacheBlobError {
const char* ERROR_MSG_INVALID_OPERATION_BLOB_INITIALIZED = "CacheBlob is already initialized!";
const char* ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED = "CacheBlob is not initialized!";
const char* ERROR_MSG_INVALID_ARGUMENT_CAPACITY = "Invalid argument 'capacity'";
const char* ERROR_MSG_INVALID_ARGUMENT_ASSET_ID = "Invalid argument 'assetID'";
const char* ERROR_MSG_INVALID_ARGUMENT_SIZE = "Invalid argument 'size'";
const char* ERROR_MSG_INVALID_ARGUMENT_OBJECT_ID = "Invalid argument 'objectID'";
const char* ERROR_MSG_INVALID_OPERATION_ASSOC_OBJECT_ID = "Invalid operation, 'objectID' is not associated with this CacheBlob";
const char* ERROR_MSG_INVALID_OPERATION_OBJECT_NULL = "Invalid operation, the cache object associated with 'objectID' is null.";
} // namespace CacheBlobError


using namespace CacheBlobError;

CacheBlob::CacheBlob() :
mObjects(),
mUsed(0),
mReserved(0),
mCapacity(0)
{
}
CacheBlob::~CacheBlob()
{
    Release();
}
void CacheBlob::Initialize(const TArray<CacheObject>& objects, UInt32 capacity)
{
    if (capacity == 0)
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_CAPACITY, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return;
    }
    if (mUsed != 0 || mReserved != 0 || mCapacity != 0)
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_BLOB_INITIALIZED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return;
    }

    mObjects = objects;
    mCapacity = capacity;
    CalculateMemoryUsage();
}
void CacheBlob::Release()
{
    mObjects.Clear();
    mUsed = 0;
    mReserved = 0;
    mCapacity = 0;
}
CacheObjectId CacheBlob::Reserve(UInt32 assetID, UInt32 size)
{
    if (Invalid(assetID))
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_ASSET_ID, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return INVALID16;
    }
    if (size == 0)
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_SIZE, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return INVALID16;
    }
    if (mCapacity == 0)
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return INVALID16;
    }

    // Check if the allocation is even possible:
    if (size > (mCapacity - mUsed))
    {
        return INVALID16;
    }

    // Check if we can re-use a null object
    for (SizeT i = 0, objectSize = mObjects.Size(); i < objectSize; ++i)
    {
        if (Invalid(mObjects[i].mUID) && mObjects[i].mCapacity >= size)
        {
            mObjects[i].mUID = assetID;
            mObjects[i].mSize = size;
            mUsed += size;
            return static_cast<CacheObjectId>(i);
        }
    }

    // Check if space exists on the end.
    UInt32 freeBytes = mCapacity - mReserved;
    if (freeBytes >= size)
    {
        UInt32 location = 0;
        SizeT id = mObjects.Size();
        if (id > 0)
        {
            location = mObjects.GetLast().mLocation + mObjects.GetLast().mCapacity;
        }

        mObjects.Add(CacheObject());
        auto& obj = mObjects.GetLast();
        obj.mUID = assetID;
        obj.mCapacity = obj.mSize = size;
        obj.mLocation = location;
        mUsed += size;
        mReserved += size;
        return static_cast<CacheObjectId>(id);
    }
    return INVALID16;
}
bool CacheBlob::Update(CacheObjectId objectID, UInt32 size)
{
    if (Invalid(objectID))
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_OBJECT_ID, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return false;
    }
    if (mCapacity == 0)
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return false;
    }
    if (objectID >= mObjects.Size())
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_ASSOC_OBJECT_ID, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return false;
    }

    if (Invalid(mObjects[objectID].mUID))
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_OBJECT_NULL, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return false;
    }

    if (mObjects[objectID].mCapacity < size)
    {
        return false; // not enough capacity:
    }
    CacheObject& object = mObjects[objectID];
    mUsed -= object.mSize;
    object.mSize = size;
    mUsed += size;
    return true;
}
bool CacheBlob::Destroy(CacheObjectId objectID)
{
    if (Invalid(objectID))
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_OBJECT_ID, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return false;
    }
    if (mCapacity == 0)
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return false;
    }
    if (objectID >= mObjects.Size())
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_ASSOC_OBJECT_ID, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return false;
    }
    if (Invalid(mObjects[objectID].mUID))
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_OBJECT_NULL, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return false;
    }
    CacheObject& object = mObjects[objectID];
    mUsed -= object.mSize;
    object.mUID = INVALID32;
    object.mSize = 0;
    return true;
}
bool CacheBlob::GetObject(CacheObjectId objectID, CacheObject& outObject) const
{
    if (Invalid(objectID))
    {
        ReportBug(ERROR_MSG_INVALID_ARGUMENT_OBJECT_ID, LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return false;
    }
    if (mCapacity == 0)
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return INVALID16;
    }
    if (objectID >= mObjects.Size())
    {
        ReportBug(ERROR_MSG_INVALID_OPERATION_ASSOC_OBJECT_ID, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return false;
    }
    outObject = mObjects[objectID];
    return true;
}
void CacheBlob::CalculateMemoryUsage()
{
    mUsed = 0;
    mReserved = 0;
    for (const CacheObject& obj : mObjects)
    {
        mUsed += obj.mSize;
        mReserved += obj.mCapacity;
    }
}

}