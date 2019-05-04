// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_RUNTIME_CACHE_BLOCK_H
#define LF_RUNTIME_CACHE_BLOCK_H

#include "Core/Common/API.h"
#include "Core/String/Token.h"
#include "Core/Utility/Array.h"
#include "Runtime/Asset/CacheTypes.h"
#include "Runtime/Asset/CacheBlob.h"

namespace lf {
namespace CacheBlockError {
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_OPERATION_INITIALIZED;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_OPERATION_INITIALIZATION_REQUIRED;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_OPERATION_OBJECT_EXISTS;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_SIZE;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_NAME;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_DEFAULT_CAPACITY;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_UID;
LF_RUNTIME_API extern const char* ERROR_MSG_INVALID_ARGUMENT_INDEX;
} // namespace CacheBlockError

// **********************************
// Manages blobs for a specific block.
// **********************************
class LF_RUNTIME_API CacheBlock
{
public:
    CacheBlock();
    ~CacheBlock();

    void Initialize(const Token& name, UInt32 defaultCapacity = 8 * 1024 * 1024);
    void Release();

    // ** Creates a cache object ( if the uid does not exist within any blobs)
    CacheIndex Create(UInt32 uid, UInt32 size);
    // ** Updates the size of the cached object, final object location returned by cache index
    CacheIndex Update(CacheIndex index, UInt32 size);
    CacheIndex Destroy(CacheIndex index);
    CacheIndex Find(UInt32 uid);

    bool GetObject(CacheIndex index, CacheObject& outObject) const;
    CacheBlobStats GetBlobStat(SizeT index) const;
    SizeT GetNumBlobs() const { return mBlobs.Size(); }

    const Token& GetName() const { return mName; }
    void SetFilename(const Token& value) { mFilename = value; }
    const Token& GetFilename() const { return mFilename; }
    UInt32 GetDefaultCapacity() const { return mDefaultCapacity; }

    TArray<CacheDefragStep> GetDefragSteps() const;
private:
    // ** The name of the cache block file
    Token              mName;
    // ** The full filename of the cache block
    Token              mFilename;
    // ** Default capacity of cache blobs (in bytes)
    UInt32             mDefaultCapacity;
    // ** List of assets held in the cache block ( UID -> Blob -> Object )
    TArray<CacheIndex> mIndices;
    // ** List of cache blob data (Object -> Data Location) the ID in blobs is redundant?
    TArray<CacheBlob>  mBlobs;
};

}

#endif // LF_RUNTIME_CACHE_BLOCK_H