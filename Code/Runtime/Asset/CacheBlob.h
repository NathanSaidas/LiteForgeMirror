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
#ifndef LF_RUNTIME_CACHE_BLOB_H
#define LF_RUNTIME_CACHE_BLOB_H

#include "Core/Common/API.h"
#include "Core/Utility/Array.h"
#include "Runtime/Asset/CacheTypes.h"

namespace lf {

// Error messages reported when bugs are detected.
namespace CacheBlobError {
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_OPERATION_BLOB_INITIALIZED;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_OPERATION_BLOB_NOT_INITIALIZED;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_CAPACITY;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_ASSET_ID;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_SIZE;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_OBJECT_ID;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_OPERATION_ASSOC_OBJECT_ID;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_OPERATION_OBJECT_NULL;
} // namespace CacheBlobError

// **********************************
// This is a container that manages individual cache blobs and cache objects 
// contained within the blob.
//
// The CacheBlob does not read or write to any memory but instead keeps track
// a) What is an object (ID)
// b) Where is the object (Location Relative to Blob)
// c) How big is the object
//
// It also keeps track of capacity so an object can technically reserve a size
// larger than it requires and then update it later to match the size or shrink
//
// Because cache objects can change their size this can result in a fragmented
// blob and memory ends up being wasted.
// 
// In order to fix this you must defragment the blob by creating a second one 
// temporarily and copying over objects and keeping 0 waste.
//
// todo: Initialize needs more safety around ensuring the objects passed in actually fit in the blob
// todo: Initialize could possible have move semantics
// todo: Come up with a policy for Copy Construction and Assignment
// **********************************
class LF_RUNTIME_API CacheBlob
{
public:
    CacheBlob();
    ~CacheBlob();

    // **********************************
    // Initialize a blob with a list of objects as well as the internal storage size
    // of the entire blob.
    // This function must be called before you use any others, they will all fail if you
    // do.
    //
    // @param objects -- An array of cache objects that could've been previously stored
    //                   in another blob.
    // @param capacity -- How large the entire blob is in bytes.
    // **********************************
    void Initialize(const TArray<CacheObject>& objects, UInt32 capacity);
    // **********************************
    // Releases all the CacheObjects and resets the internal state.
    // **********************************
    void Release();
    // **********************************
    // Attempts to allocate space in the blob for the 'asset' matching assetID
    // 
    // Failure:
    //    (Bug) If the 'assetID' is invalid (aka null)
    //    (Bug) If the 'size' of the asset is 0 bytes.
    //    (Bug) If the blob has not been initialized yet.
    //    If there isn't enough memory in any of the null CacheObjects or the reserved space of the cache blob
    // 
    // @param assetID -- An ID corresponding to an asset
    // @param size -- The size of the asset in bytes
    // @returns INVALID16 if a CacheObject was not allocated, otherwise an ID corresponding to the CacheObject allocated.
    // **********************************
    CacheObjectId Reserve(UInt32 assetID, UInt32 size);
    // **********************************
    // Updates the CacheObject, associated with the objectID, size property
    // 
    // Failure:
    //    (Bug) If 'objectID' is invalid or not associated with this blob (index out of bounds)
    //    (Bug) If object associated with 'objectID' is null
    //    (Bug) If the blob has not been initialized yet.
    //    If size >= object.Capacity
    // @param objectID -- The ID(index) of the CacheObject allocated with Reserve
    // @param size -- The new size of the CacheObject
    // **********************************
    bool Update(CacheObjectId objectID, UInt32 size);
    // **********************************
    // Updates the CacheObject associated with the objectID to be represented as null
    //
    // Failure:
    //    (Bug) If 'objectID' is invalid or not associated with this blob (index out of bounds)
    //    (Bug) If object associated with 'objectID' is null
    //    (Bug) If the blob has not been initialized yet.
    // @param objectID -- The ID(index) of the CacheObject allocated with Reserve
    // **********************************
    bool Destroy(CacheObjectId objectID);
    // **********************************
    // Attempts to get data associated with objectID
    //
    // Failure:
    //    (Bug) If 'objectID' is invalid or not associated with this blob (index out of bounds)
    //    (Bug) If the blob has not been initialized yet.
    // @param objectID -- The ID(index) of the CacheObject allocated with Reserve
    // @param outObject- -- Output target of the cache object data
    // **********************************
    bool GetObject(CacheObjectId objectID, CacheObject& outObject) const;


    // Returns the number of objects contained in the blob
    SizeT Size() const { return mObjects.Size(); }

    // How many bytes are actually used by the CacheObjects
    SizeT GetBytesUsed() const { return static_cast<SizeT>(mUsed); }
    // How many bytes are reserved for the CacheObjects
    SizeT GetBytesReserved() const { return static_cast<SizeT>(mReserved); }
    // How many bytes is reserved for the blob in total
    SizeT GetCapacity() const { return static_cast<SizeT>(mCapacity); }
    // How many bytes stored in null objects.
    SizeT GetFragmentedBytes() const
    {
        SizeT total = 0;
        for (const CacheObject& object : mObjects)
        {
            if (Invalid(object.mUID))
            {
                total += object.mCapacity;
            }
        }
        return total;
    }

    SizeT GetFragmentedObjects() const
    {
        SizeT total = 0;
        for (const CacheObject& object : mObjects)
        {
            if (Invalid(object.mUID))
            {
                ++total;
            }
        }
        return total;
    }
private:
    void CalculateMemoryUsage();

    // List of Objects the blob owns
    TArray<CacheObject> mObjects;
    // Bytes currently used by all the objects
    UInt32              mUsed;
    // Bytes currently reserved by all objects.
    UInt32              mReserved;
    // Bytes reserved for this blob
    UInt32              mCapacity;
};

} // namespace lf

#endif // LF_RUNTIME_CACHE_BLOB_H