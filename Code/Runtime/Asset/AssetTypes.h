// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_RUNTIME_ASSET_TYPES_H
#define LF_RUNTIME_ASSET_TYPES_H

#include "Core/Common/Enum.h"
#include "Core/String/Token.h"
#include "Core/Utility/Bitfield.h"

// **********************************
// Notes:
// Because of the complexity of the system, certain tasks are seperated into their
// own class/module. We refer to these as controllers.
//
// AssetDataController:
//      Provides efficient lookup tables from ID/Name to Type/Handle/InstanceData
//      Owns the Type/Handle/InstanceData
//
//   table:
//      ID | Name => Index -> { Type & Handle & Instance }
//
//      { Name => Index } -- A table of names sorted (pointer compare) for quick binary search. 
//      {   ID => Index } -- A table of IDs sorted (integer compare) for quick binary search.
// 
// AssetCacheController:
//      Maintains a reference of cache blocks based on exported Bundles/Tags/Category Extensions
//      All 'Cache Titles' are maintained in the cache controller, they can be accessed via a
//      index key
// 
//      Cache Title = Bundle + [Optional] Tag + [Optional] Category Extension
//      Full Cache Title = Cache Title + BlobID + CacheObjectID

//   table:
//      CacheName => Index -> { CacheBlock }
//
//      { CacheName => Index } -- A table of full cache names sorted (pointer compare) for quick binary search
//          
// AssetPackageController:
//      Maintains a list of package export details, in order for content to be used outside the editor
//      it must be marked for export in a package. 
//      Packages allow a tagging system to create seperate cache blocks on demand.
// 
// AssetBundleController:
//      Maintains a list of bundles.
//      Bundles help seperate data for seperate content updates
//
// AssetExportController:
//      Uses package controller/bundle controller to build a list of actions for what assets to export and
//      how to proceed with updating the cache. Details in an export command is as follows
//
//   input:
//      Asset Name:     What the export is called
//      ID:             What the export is identified as in cache
//      Source File:    What data to export
//      CacheOp:        (Create|Update|Destroy)
//      CacheLocation:  The 'Cache Title' of the asset
//   output:
//      Asset Name:     The asset that was exported
//      ID:             The ID of the asset that was exported
//      CacheResult:    (Failed|Created|Updated|UpdateRelocated|Destroy)
//      CacheBlob:      The 'Full Cache Title' of the asset after the cache manager has resolved it's location
//      Uncompressed:   The size of the asset in it's uncompressed form
//      Compressed:     The size of the asset after compression
//      CacheCapacity:  The available memory of the CacheObject inside the CacheBlob
//      CacheBlobCapacity: The available memory of the CacheBlob
//      ProcessingTime: The amount of time spent on preparing the asset for export (compression/encryption etc)
//      ExportTime:     The amount of time spent writing the asset to the CacheBlob
//
// AssetStreamController:
//
//
//
// Multithreading:
//      Asset loading/queries can be completed async/concurrently
//      Asset instance instantiation must be completed on the Asset thread, which for the time being is the 
//      main thread. 
//      Incrementing an asset instance's reference count outside the main thread will result in undefined behavior
//      it is safe however to pass raw pointers if you can guarantee the life-time of the asset instance on the
//      main thread.
// **********************************
namespace lf {

// External:
class Stream;

// Internal:
struct AssetHash;
struct AssetType;
struct UnknownAssetHandle;
struct AssetHandle;
class AssetObject;

// **********************************
// Hints for AssetMgr when importing/caching assets
// 
// Texture: '_t' cache block
// Font:    '_f' cache block
// Audio:   '_a' cache block
// Mesh:    '_m' cache block
// Shader:  '_s' cache block
// Level:   '_l' cache block
// Script:  '_x' cache block
// Other:   default cache block
// **********************************
DECLARE_ENUM
(
    AssetCategory,
    AC_TEXTURE,
    AC_FONT,
    AC_AUDIO,
    AC_MESH,
    AC_SHADER,
    AC_LEVEL,
    AC_SCRIPT,
    AC_SERIALIZED_OBJECT
);

// **********************************
// Runtime and serialized flags.
// 
// Binary:          Asset data is serialized as pure binary
// Encoded:         Asset data was encrypted
// Concrete:        Asset is base asset class for a concrete type
// Deleted:         Asset was deleted in editor
// Created:         Asset was created in editor
// RootContainer:   AssetType records instances for extending assets (that are not RootContainers)
// **********************************
DECLARE_ENUM
(
    AssetFlags,
    AF_BINARY,
    AF_ENCODED,
    AF_CONCRETE,
    AF_DELETED,
    AF_CREATED,
    AF_ROOT_CONTAINER,
    AF_RESERVED
);
using AssetFlagsBitfield = Bitfield<AssetFlags::Value, UInt8>;


// **********************************
// 
// **********************************
struct LF_RUNTIME_API AssetHash
{
    AssetHash();
    void Serialize(Stream& s);

    bool Parse(const String& string);
    String ToString() const;

    void SetZero();
    bool IsZero() const;

    ByteT mData[16];
};
LF_FORCE_INLINE Stream& operator<<(Stream& s, AssetHash& self)
{
    self.Serialize(s);
    return s;
}

// **********************************
// The runtime version of asset type, for saving for persistence see AssetTypeData
//
// This is a really fat data structure and here are some numbers I've run
//
// The non-editor version uses 72 bytes
// To store this raw datastructure in a table at runtime beware of certain memory budgets
//     # Instances : Memory Used
//            1000 |   62.5(KB)
//            5000 |  312.5(KB)
//           15000 |  937.5(KB)
//           65535 |    4.0(MB)
//          100000 |    6.1(MB)
//          250000 |   15.3(MB)
//          400000 |   24.4(MB) --- Around this I might consider loading in types on demand.
//
// Those calculations do not take into account of table overhead and indexing.
// **********************************
struct AssetType
{
    // ** The fullname of the asset
    Token              mFullName;
    // ** Concrete type of the asset
    const Type*        mConcreteType;
    // ** Parent type of the asset (Concrete Assets have no parent)
    AssetType*         mParentType;
    // ** Unique ID of the asset (Generated by 'Content Server')
    UInt32             mUID;
    // ** Version number for the asset
    UInt16             mVersion;
    // ** Cache file location (ID should map to AssetCacheController )
    UInt16             mCacheFileID;
    // ** Cache Object index within a blob
    UInt16             mCacheObjectIndex;
    // ** todo: This may not be necessary
    UInt16             mAttributes;
    // ** Asset flags of the asset
    AssetFlagsBitfield mFlags;
    // ** 
    UInt8              mCategory;
    // ** Current loading state of the asset
    UInt8              mLoadState;
    // ** Content Server hash of the asset.
    AssetHash          mHash;

    // AssetEditorTypeInfo mEditorInfo;
};

// **********************************
// Asset type data in the format saved for persistence 
// **********************************
struct LF_RUNTIME_API AssetTypeData
{
    AssetTypeData();
    void Serialize(Stream& s);

    Token mFullName;
    Token mConcreteType;
    Token mCacheName;
    UInt32 mUID;
    UInt32 mParentUID;
    UInt16 mVersion;
    UInt16 mAttributes;
    UInt8  mFlags;
    UInt8  mCategory;
    AssetHash mHash;
};
LF_FORCE_INLINE Stream& operator<<(Stream& s, AssetTypeData& self)
{
    self.Serialize(s);
    return s;
}

struct UnknownAssetHandle { };
struct AssetHandle : public UnknownAssetHandle
{
    AssetObject*     mPrototype;
    const AssetType* mType;
    UInt32           mStrongRefs;
    UInt32           mWeakRefs;
};

} // namespace lf

#endif // LF_RUNTIME_ASSET_TYPES_H