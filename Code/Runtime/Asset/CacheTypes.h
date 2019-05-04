// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_CACHE_TYPES_H
#define LF_CORE_CACHE_TYPES_H

#include "Core/Common/Types.h"

namespace lf {

/*
    Cache Location

    [ Source File ] -> { Convert Name to Block w/ Exporter } -> [ Block Title ]
    
    [ Block Title ] -> { Append Extension For Type } -> [ Cache Block ]

    [ Cache Block ] -> { Fetch Blob for BlobID } -> { FetchObject for BlobIndex } -> [ Compiled Asset ]
*/

struct CacheFile;
struct CacheObject;
using  CacheObjectId = UInt16;
using  CacheBlockIndex = UInt32;
class CacheBlob;
class CacheBlock;
class CacheWriter;
class CacheReader;

struct CacheObject
{
    LF_FORCE_INLINE CacheObject() : mUID(INVALID32), mLocation(0), mSize(0), mCapacity(0) {}

    // ** Unique ID of the cache object across all blobs/blocks
    UInt32 mUID;
    // ** Offset from the base file pointer ( This can be deduced from the array of CacheObjects, but caching may be faster)
    UInt32 mLocation;
    // ** Size in bytes for the object
    UInt32 mSize;
    // ** Size in bytes the object has allocated for.
    UInt32 mCapacity;
};

struct CacheIndex
{
    LF_FORCE_INLINE CacheIndex() : mUID(INVALID32), mBlobID(INVALID32), mObjectID(INVALID32) {}
    LF_FORCE_INLINE CacheIndex(UInt32 uid, UInt32 blobID, UInt32 objectID) : mUID(uid), mBlobID(blobID), mObjectID(objectID) {}
    LF_FORCE_INLINE operator bool() const { return Valid(mUID) && Valid(mBlobID) && Valid(mObjectID); }

    // ** Unique ID of the cache object this index is associated with
    UInt32 mUID;
    // ** Index of the blob, local to a block
    UInt32 mBlobID;
    // ** Index of the cache object, local to a blob
    UInt32 mObjectID;
};

struct CacheBlobStats
{
    LF_FORCE_INLINE CacheBlobStats() : 
    mBytesUsed(0),
    mBytesReserved(0),
    mBytesFragmented(0),
    mBlobCapacity(0),
    mNumObjects(0),
    mNumObjectsFragmented(0),
    mCacheBlock(""),
    mBlobID(0)
    {}

    // ** Bytes all the objects are currently using
    SizeT mBytesUsed;
    // ** Bytes all the objects currently have reserved
    SizeT mBytesReserved;
    // ** Bytes reserved for null objects
    SizeT mBytesFragmented;
    // ** Bytes allocated by the blob
    SizeT mBlobCapacity;
    // ** Number of objects stored in the blob
    SizeT mNumObjects;
    // ** Number of null objects stored in the blob
    SizeT mNumObjectsFragmented;

    // ** [Appended by CacheBlock] -- Name of the block
    const char* mCacheBlock;
    // ** [Appended by CacheBlock] -- ID of the blob within the block
    SizeT      mBlobID;
};

struct CacheDefragStep
{
    UInt32 mUID;
    UInt32 mSize;
    UInt32 mSourceBlobID;
    UInt32 mSourceObjectID;
    UInt32 mDestBlobID;
    UInt32 mDestObjectID;
};

}

#endif // LF_CORE_CACHE_TYPES_H