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
#include "AbstractEngine/PCH.h"
#include "GfxMaterialProcessor.h"
#include "Core/IO/BinaryStream.h"
#include "Core/IO/DependencyStream.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "AbstractEngine/Gfx/GfxMaterial.h"
#include "AbstractEngine/Gfx/GfxShaderBinary.h"
#include "AbstractEngine/Gfx/GfxShaderUtil.h"

namespace lf {

#if defined(LF_GFX_ENGINE_REWORK)
DECLARE_PTR(GfxMaterial);

static bool ReadCacheData(GfxMaterial& material, AssetCacheController& cache, const AssetTypeInfo& assetType)
{
    SizeT bufferSize;
    if (cache.QuerySize(&assetType, bufferSize))
    {
        MemoryBuffer buffer;
        buffer.Allocate(bufferSize, 1);
        buffer.SetSize(bufferSize);
        CacheIndex cacheIndex;

        if (cache.Read(buffer, &assetType, cacheIndex))
        {
            String name = assetType.GetPath().GetName();
            String super = assetType.GetParent()->GetPath().CStr();

            BinaryStream bs(Stream::MEMORY, &buffer, Stream::SM_READ);
            bs.SetAssetLoadFlags(AssetLoadFlags::LF_IMMEDIATE_PROPERTIES);
            if (bs.BeginObject(name, super))
            {
                material.Serialize(bs);
                bs.EndObject();
                return true;
            }
        }
    }
    return false;
}

GfxMaterialProcessor::GfxMaterialProcessor(GfxDevice* device)
: Super()
, mDevice(device)
{
    Assert(mDevice);
}
GfxMaterialProcessor::~GfxMaterialProcessor()
{
}

const Type* GfxMaterialProcessor::GetTargetType() const
{
    return typeof(GfxMaterial);
}

const Type* GfxMaterialProcessor::GetPrototypeType(const Type* inputType) const
{
    const Type* implType = nullptr;
    if (!mDevice->QueryMappedTypes(inputType, &implType, nullptr))
    {
        implType = nullptr;
    }
    return implType;
}
const Type* GfxMaterialProcessor::GetConcreteType(const Type* inputType) const
{
    if (inputType->IsA(typeof(GfxMaterial)))
    {
        return typeof(GfxMaterial);
    }
    return nullptr;
}

AssetDataType::Value GfxMaterialProcessor::Export(AssetObject* object, MemoryBuffer& buffer, bool cache, AssetDataType::Value dataTypeHint) const
{
    // Remove Reference to Dynamic Asset
    // Add Reference to new Dynamic Asset
    if (cache)
    {
        const AssetTypeInfoCPtr& assetType = object->GetAssetType();
        Assert(assetType != nullptr);

        GfxMaterial* material = static_cast<GfxMaterial*>(object);
        
        // Create the old material to remove references.
        GfxMaterialPtr oldMaterial = GetReflectionMgr().Create<GfxMaterial>(material->GetType());
        Assert(oldMaterial != NULL_PTR);
        
        if (ReadCacheData(*oldMaterial, GetCacheController(), *assetType))
        {
            for (SizeT i = 0; i < ENUM_SIZE(Gfx::ShaderType); ++i)
            {
                if (oldMaterial->GetShader(Gfx::ShaderType(i)))
                {
                    const GfxShaderBinaryBundle& bundle = oldMaterial->GetBinary(Gfx::ShaderType(i));
                    for (SizeT k = 0; k < ENUM_SIZE(Gfx::GraphicsApi); ++k)
                    {
                        const GfxShaderBinaryDataAsset& data = bundle.GetData(Gfx::GraphicsApi(k));
                        const GfxShaderBinaryInfoAsset& info = bundle.GetInfo(Gfx::GraphicsApi(k));
                        
                        if (data.GetType())
                        {
                            GetDataController().RemoveDependency(data, assetType);
                        }
                        if (info.GetType())
                        {
                            GetDataController().RemoveDependency(info, assetType);
                        }
                    }
                }
            }
        }

        // Add new references
        {
            // Clear out existing binary...
            for (SizeT i = 0; i < ENUM_SIZE(Gfx::ShaderType); ++i)
            {
                material->SetBinary(Gfx::ShaderType(i), GfxShaderBinaryBundle());
            }

            const AssetPath& path = assetType->GetPath();
            const TVector<Token>& defines = oldMaterial->GetDefines();

            for (SizeT i = 0; i < ENUM_SIZE(Gfx::ShaderType); ++i)
            {
                if (material->GetShader(Gfx::ShaderType(i)))
                {
                    Gfx::ShaderHash hash = Gfx::ComputeHash(Gfx::ShaderType(i), path, defines);
                    for (SizeT k = 0; k < ENUM_SIZE(Gfx::GraphicsApi); ++k)
                    {
                        String dynamicAssetPath = Gfx::ComputePath(Gfx::ShaderType(i), Gfx::GraphicsApi(k), path, hash);

                        (dynamicAssetPath);
                        // TODO: mShaderManager.CreateAsset(api, hash, dynamicAssetPath);
                        // TODO: mShaderManager.CompileAsset(api, hash, dynamicAssetPath);

                    }
                }
            }
        }
        

        // mDevice->AddShaderReference(material);
        // mDevice->RemoveShaderReference(material);
    }
    return Super::Export(object, buffer, cache, dataTypeHint);
}

bool GfxMaterialProcessor::PrepareAsset(AssetObject* object, const MemoryBuffer& buffer, AssetLoadFlags::Value loadFlags) const
{
    // TODO: When we finish loading the dependencies, we should initialize the Material Runtime
    //      DirectX 11: Create Shaders/Create Vertex Layout/Create Depth State/Create Stencil State/Create
    //      DirectX 12: Create PSO (Pipeline State Object)
    return Super::PrepareAsset(object, buffer, loadFlags);
}

void GfxMaterialProcessor::OnLoadAsset(AssetObject* object) const
{
    // TODO: 
    // if (Services().AppInfo().IsDevBuild) {
    //     ShaderHandle = Services().Graphics().CompileAndLoadShader(Shader, Defines);
    // }
    // else if (Shader.IsModAsset()) {
    //     ShaderHandle = Services().Graphics().CompileAndLoadShader(Shader, Defines);
    // }
    // else {
    //     ShaderHandle = Services().Graphics().LoadShader(Shader, Defines);
    // }
    Assert(object != nullptr);
    GfxMaterial* material = static_cast<GfxMaterial*>(object);
    if (mDevice->CreateAdapter(material))
    {
        material->Commit();
    }
}

#endif

} // namespace lf