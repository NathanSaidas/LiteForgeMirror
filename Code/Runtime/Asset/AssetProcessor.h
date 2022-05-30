#pragma once
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
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetPath.h"
#include "Runtime/Asset/CacheBlockType.h"

namespace lf {

struct AssetDependencyContext;
class Type;
class AssetObject;
class AssetPath;
DECLARE_ATOMIC_PTR(AssetObject);
DECLARE_MANAGED_CPTR(AssetTypeInfo);

struct LF_RUNTIME_API AssetImportResult
{
    LF_INLINE AssetImportResult()
        : mObject()
        , mConcreteType(nullptr)
        , mParentType()
        , mDependencies()
    {}

    // ********************************************************************
    // The object created from 'import' operation which can later be used
    // for exporting.
    // @static
    // ********************************************************************
    AssetObjectAtomicPtr mObject;
    // ********************************************************************
    // The concrete type of the imported object. 
    // NOTE: (Use GetPrototypeType to get the underlying runtime type)
    // @nullable, static
    // ********************************************************************
    const Type* mConcreteType;
    // ********************************************************************
    // The parent type of the import operation. (If the object had a parent
    // asset type)
    // @nullable, static
    // ********************************************************************
    AssetTypeInfoCPtr mParentType;
    // ********************************************************************
    // Dependencies discovered during import process, these dependencies
    // must be imported before this can be imported.
    // @static
    // ********************************************************************
    TVector<AssetPath>    mDependencies;
};

// ********************************************************************
// This serves as the abstract base class for future asset processors to
//  a) Intercept various asset events for any extra event handling,
//  b) Provide additional data for prototype creation
//  c) Provide an interface to import/export assets.
//
// The AssetDataController will have a function to select the best AssetProcessor
// based on 'Type' (from an AssetTypeInfo). The closest parent type will
// be chosen to process the asset.
//
// class A          : AssetProcessorA
// class B : A      : AssetProcessorB
// class C : B      : <none>
// class D : C      : AssetProcessorD
// class E : A      : <none>
//
// GetProcessor(typeof(A)) = AssetProcessorA
// GetProcessor(typeof(B)) = AssetProcessorB
// GetProcessor(typeof(C)) = AssetProcessorB
// GetProcessor(typeof(D)) = AssetProcessorD
// GetProcessor(typeof(E)) = AssetProcessorA
// ********************************************************************
class LF_RUNTIME_API AssetProcessor
{
public:
    // ********************************************************************
    // Constructor
    // ********************************************************************
    AssetProcessor();
    // ********************************************************************
    // Destructor
    // ********************************************************************
    virtual ~AssetProcessor();

    // ********************************************************************
    // Called to initialize the dependencies.
    // 
    // note: Non virtual because this is not specific to an Asset but just
    // the processor in general.
    // ********************************************************************
    void Initialize(AssetDependencyContext& context);

    // ********************************************************************
    // The type of AssetObject the AssetProcessor can accept. 
    // ********************************************************************
    virtual const Type* GetTargetType() const = 0;
    // ********************************************************************
    // Returns a score on a scale of 0-10 for how best to the processor can
    // handle the cache block item. 
    // 0 = Best Processor for the Job
    // 10 = Worst Processor for the Job
    // INVALID = Cannot process
    //
    // The idea behind this value is that multiple cache blocks might be 
    // handled by one processor.
    // 
    // Examples:
    // OBJECT, LEVEL (maybe) => DefaultAssetProcessor
    // TEXTURE_DATA => TextureAssetProcessor (maybe)
    // JSON, TEXT => TextAssetProcessor 
    // ********************************************************************
    virtual SizeT GetCacheBlockScore(CacheBlockType::Value cacheBlock) const = 0;

    virtual bool AcceptImportPath(const AssetPath& path) const = 0;
    // ********************************************************************
    // The type that is created for the prototype for mapped types.
    // @param inputType -- The type to be transformed into the 'prototype type'
    //
    // eg; GfxShader => DX12GfxShader or OpenGLGfxShader depending on 
    //     active graphics settings.
    // ********************************************************************
    virtual const Type* GetPrototypeType(const Type* inputType) const = 0;
    // ********************************************************************
    // ********************************************************************
    virtual const Type* GetConcreteType(const Type* inputType) const = 0;
    // ********************************************************************
    // Imports an asset from the source 'assetPath' creating an AssetObject 
    // with all the data necessary to function.
    // 
    // If the import fails (ie mObject is null) then check the dependencies as
    // the dependencies must be imported for the asset to be imported.
    // ********************************************************************
    virtual AssetImportResult Import(const AssetPath& assetPath) const = 0;
    // ********************************************************************
    // Exports an object to memory. The object must be a complete asset
    // object.
    // 
    // You can provide a data type hint that the importer might use to
    // for exporting the data.
    // 
    // @param cache -- Whether or not the export target is the cache or source
    // @param dataTypeHint  -- A optional hint as to how the data should
    //        be exported.
    // ********************************************************************
    virtual AssetDataType::Value Export(AssetObject* object, MemoryBuffer& buffer, bool cache, AssetDataType::Value dataTypeHint = AssetDataType::INVALID_ENUM) const = 0;
    // ********************************************************************
    // Gets called when the Prototype is created.
    // ********************************************************************
    virtual void OnCreatePrototype(AssetObject* object) const = 0;
    // ********************************************************************
    // Gets called when the Prototype is destroyed.
    // ********************************************************************
    virtual void OnDestroyPrototype(AssetObject* object) const = 0;
    // ********************************************************************
    // Gets called to load the data into the prototype.
    // 
    // @threading: AssetWorker
    // ********************************************************************
    virtual bool PrepareAsset(AssetObject* object, const MemoryBuffer& buffer, AssetLoadFlags::Value loadFlags) const = 0;
    // ********************************************************************
    // Gets called when the asset is loaded (ALS_LOADED)
    //
    // @threading: AssetWorker
    // ********************************************************************
    virtual void OnLoadAsset(AssetObject* object) const = 0;
    // ********************************************************************
    // Gets called when the Prototype has unloaded it's data.
    // ********************************************************************
    virtual void OnUnloadAsset(AssetObject* object) const = 0;
protected:
    AssetDataController& GetDataController() const;
    AssetCacheController& GetCacheController() const;
    AssetSourceController& GetSourceController() const;
    AssetOpController& GetOpController() const;
private:
    AssetDependencyContext mContext;
};

}