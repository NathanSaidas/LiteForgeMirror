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
#ifndef LF_RUNTIME_ASSET_TYPES_H
#define LF_RUNTIME_ASSET_TYPES_H

#include "Core/Common/Enum.h"
#include "Core/Crypto/MD5.h"
#include "Core/String/Token.h"
#include "Core/Utility/Bitfield.h"
#include "Core/Utility/DateTime.h"

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
struct UnknownAssetHandle;
struct AssetHandle;
class AssetObject;
class AssetTypeInfo;
class AssetDataController;
class AssetCacheController;
class AssetSourceController;
class AssetOpController;

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

namespace AssetLoadFlags
{
    using Value = UInt32;
    const Value LF_ACQUIRE                  = 1 << 0;
    const Value LF_ASYNC                    = 1 << 1;
    const Value LF_IMMEDIATE_PROPERTIES     = 1 << 2;
    const Value LF_RECURSIVE_PROPERTIES     = 1 << 3;
    const Value LF_LOW_PRIORITY             = 1 << 4;
    const Value LF_HIGH_PRIORITY            = 1 << 5;
    const Value LF_SOURCE                   = 1 << 6;
}

DECLARE_ENUM
(
    AssetDataType,
    ADT_TEXT,
    ADT_JSON,
    ADT_BINARY
);

DECLARE_ENUM
(
    AssetLoadState,
    // The asset is not loaded at all
    ALS_UNLOADED,
    // The asset prototype has been created
    ALS_CREATED,
    // The asset properties have been written to the prototype
    ALS_SERIALIZED_PROPERTIES,
    // The asset has loaded all of it's dependencies
    ALS_SERIALIZED_DEPENDENCIES,
    // The asset is completely loaded
    ALS_LOADED,
    // The asset has been marked for delete
    ALS_DELETED,
    // The asset is corrupted (we may consider options for attempting to fix)
    ALS_CORRUPTED
);

namespace AssetLoadState {

LF_RUNTIME_API bool IsCreated(AssetLoadState::Value value);
LF_RUNTIME_API bool IsPropertyLoaded(AssetLoadState::Value value);
LF_RUNTIME_API bool IsDependencyLoaded(AssetLoadState::Value value);

}

DECLARE_ENUM
(
    AssetOpState,
    // The asset is available to transition to [ Loading | Downloading | Caching | Creating ]
    AOS_IDLE,
    // The asset is available to transition to [ Idle ]
    // + weak-read lock on the 'load target'
    // + weak-write lock on the prototype
    // + weak-write lock on the handle
    AOS_LOADING,
    // The asset is available to transition to [ Idle ]
    // + weak-write lock on the prototype
    // + weak-write lock on the handle
    AOS_UNLOADING,
    // The asset is available to transition to [ Idle ]
    // 
    AOS_DOWNLOADING,
    // The asset is available to transition to [ Idle ]
    // + weak-write lock on the cache
    // + weak-read lock on the source
    AOS_CACHING,
    // The asset is available to transition to [ Idle ]
    // + weak-write lock on both source/cache
    // TODO [Nathan] Maybe we'll support AOS_IMPORT as a separate op?
    AOS_CREATING,
    // 
    AOS_DELETED,
    // 
    AOS_UNDO_DELETE
);



// **********************************
// 
// **********************************
struct LF_RUNTIME_API AssetHash
{
    AssetHash();
    explicit AssetHash(const Crypto::MD5Hash& hash);
    void Serialize(Stream& s);

    bool Parse(const String& string);
    String ToString() const;

    void SetZero();
    bool IsZero() const;

    bool operator==(const AssetHash& other) const { return mValue == other.mValue; }
    bool operator!=(const AssetHash& other) const { return mValue != other.mValue; }

    Crypto::MD5Hash mValue;
};
LF_FORCE_INLINE Stream& operator<<(Stream& s, AssetHash& self)
{
    self.Serialize(s);
    return s;
}

struct AssetInfoQuery
{
    bool mModifyDate;
    bool mHash;
};

struct AssetInfoQueryResult
{
    DateTime  mModifyDate;
    AssetHash mHash;
};

struct UnknownAssetHandle { };
struct AssetHandle : public UnknownAssetHandle
{
    AssetObject*         mPrototype;
    volatile Atomic32    mStrongRefs;
    volatile Atomic32    mWeakRefs;
    const AssetTypeInfo* mType;
};

// ********************************************************************
// A data structure that holds pointers to various resources of the 
// asset mgr. The lifetime of these pointers is guaranteed to outlive
// any assets. 
//  
// AssetMgr::Initialize => These resources are created.
// AssetMgr::Shutdown => These resources are destroyed. 
// ********************************************************************
struct AssetDependencyContext
{
    AssetDependencyContext()
    : mDataController(nullptr)
    , mCacheController(nullptr)
    , mSourceController(nullptr)
    , mOpController(nullptr)
    {
    }

    AssetDataController* mDataController;
    AssetCacheController* mCacheController;
    AssetSourceController* mSourceController;
    AssetOpController* mOpController;
};

} // namespace lf

#endif // LF_RUNTIME_ASSET_TYPES_H