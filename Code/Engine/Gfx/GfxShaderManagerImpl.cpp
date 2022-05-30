// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "Engine/PCH.h"
#include "Engine/Gfx/GfxShaderManagerImpl.h"
#include "AbstractEngine/Gfx/GfxShader.h"
#include "AbstractEngine/Gfx/GfxShaderBinary.h"
#include "AbstractEngine/Gfx/GfxShaderUtil.h"

namespace lf {

    GfxShaderManagerImpl::GfxShaderManagerImpl()
    {}
    GfxShaderManagerImpl::~GfxShaderManagerImpl()
    {}

    void GfxShaderManagerImpl::ShaderData::Serialize(Stream& s)
    {
        SERIALIZE(s, mInfo, "");
        SERIALIZE(s, mData, "");
    }

    GfxShaderBinaryBundle GfxShaderManagerImpl::CreateShaderAssets(
        const AssetTypeInfoCPtr& creatorType
        , const GfxShaderAsset& shader
        , const TVector<Token>& defines
        , Gfx::ShaderType shaderType)
    {
        GfxShaderBinaryBundle bundle;
        if (!shader || InvalidEnum(shaderType))
        {
            return bundle;
        }
        // const AssetPath& path = shader.GetPath();
        Gfx::ShaderHash shaderHash = Gfx::ComputeHash(shaderType, shader.GetPath(), defines);
        LF_STATIC_ASSERT(EnumValue(Gfx::GraphicsApi::ANY) == 0);
        if (shader->SupportsApi(Gfx::GraphicsApi::ANY))
        {
            for (SizeT i = 1; i < ENUM_SIZE(Gfx::GraphicsApi); ++i)
            {
                CreateBinaryAssets(
                    bundle
                    , creatorType
                    , shader
                    , shader->GetText(Gfx::GraphicsApi::ANY)
                    , defines
                    , shaderType
                    , static_cast<Gfx::GraphicsApi>(i)
                    , shaderHash);
            }

            // Compile the 'generic' as dx11/dx12
        }
        else 
        {
            for (SizeT i = 1; i < ENUM_SIZE(Gfx::GraphicsApi); ++i)
            {
                CreateBinaryAssets(
                    bundle
                    , creatorType
                    , shader
                    , shader->GetText(static_cast<Gfx::GraphicsApi>(i))
                    , defines
                    , shaderType
                    , static_cast<Gfx::GraphicsApi>(i)
                    , shaderHash);
            }

            // Compile specifically dx11/dx12 as dx11 and dx12
        }

        return bundle;
    }

    void GfxShaderManagerImpl::DestroyShaderAssets(const AssetTypeInfoCPtr& creatorType, const GfxShaderBinaryBundle& bundle)
    {
        // NOTE: We don't actually delete assets at this time, we just remove the dependency.
        LF_STATIC_ASSERT(EnumValue(Gfx::GraphicsApi::ANY) == 0);
        for (SizeT i = 1; i < ENUM_SIZE(Gfx::GraphicsApi); ++i)
        {
            const GfxShaderBinaryInfoAsset& info = bundle.GetInfo(static_cast<Gfx::GraphicsApi>(i));
            const GfxShaderBinaryDataAsset& data = bundle.GetData(static_cast<Gfx::GraphicsApi>(i));
            if (info)
            {
                GetAssetMgr().RemoveDependency(info.GetType(), creatorType, false);
            }
            if (data)
            {
                GetAssetMgr().RemoveDependency(data.GetType(), creatorType, false);
            }

            QueueDelete(info.GetType());
            QueueDelete(data.GetType());
        }
    }

    void GfxShaderManagerImpl::Update()
    {
        // Note: We don't delete assets unless they are no longer referenced... 
        {
            ScopeLock queueLock(mDeleteQueueLock);
            ScopedDeleteLock deleteLock(mCreateDeleteLock);

            for (AssetTypeInfoCPtr& type : mDeleteQueue)
            {
                if (type->GetStrongReferences() == 0)
                {
                    GetAssetMgr().Wait(GetAssetMgr().Delete(type));
                }
            }

            mDeleteQueue.clear();
        }
    }

    void GfxShaderManagerImpl::CreateBinaryAssets(
        GfxShaderBinaryBundle& bundle
        , const AssetTypeInfoCPtr& creatorType
        , const GfxShaderAsset& shader
        , const GfxShaderTextAsset& shaderText
        , const TVector<Token>& defines
        , Gfx::ShaderType shaderType
        , Gfx::GraphicsApi api
        , Gfx::ShaderHash hash)
    {
        CriticalAssert(shader != NULL_PTR);
        const String basePath = Gfx::ComputePath(shaderType, api, shader.GetPath(), hash);
        const AssetPath infoPath(basePath + ".shaderinfo");
        const AssetPath dataPath(basePath + ".shaderdata");

        ScopedCreateLock lock(mCreateDeleteLock);

        // Try and find the types and skip the create process.
        AssetTypeInfoCPtr infoType = GetAssetMgr().FindType(infoPath);
        AssetTypeInfoCPtr dataType = GetAssetMgr().FindType(dataPath);

        bool success = true;

        // If the types don't exist, try and create.
        auto info = infoType ? NULL_PTR : GetAssetMgr().CreateEditable<GfxShaderBinaryInfo>();
        auto data = dataType ? NULL_PTR : GetAssetMgr().CreateEditable<GfxShaderBinaryData>();

        if (info)
        {
            info->SetShaderType(shaderType);
            info->SetApi(api);
            info->SetHash(hash);
            info->SetShader(shader);
            info->SetShaderText(shaderText);
            info->SetDefines(defines);

            GetAssetMgr().Wait(GetAssetMgr().Create(infoPath, info, nullptr));
            // Because 2 threads can create at the same time, just ignore the error and try to fetch the type.
            infoType = GetAssetMgr().FindType(infoPath);

            // If still failed to fetch the type then we know we failed to create the asset.
            success = success && infoType != nullptr;
        }

        // Track dependency
        if (infoType)
        {
            GetAssetMgr().AddDependency(infoType, creatorType, false);
        }
        
        if (data)
        {
            GetAssetMgr().Wait(GetAssetMgr().Create(dataPath, info, nullptr));
            // Because 2 threads can create at the same time, just ignore the error and try to fetch the type.
            dataType = GetAssetMgr().FindType(dataPath);
            
            // If still failed to fetch the type then we know we failed to create the asset.
            success = success && dataType != nullptr;
        }

        // Track dependency
        if (dataType)
        {
            GetAssetMgr().AddDependency(dataType, creatorType, false);
        }


        if (!success)
        {
            if (infoType)
            {
                GetAssetMgr().RemoveDependency(infoType, creatorType, false);
            }
            if (dataType)
            {
                GetAssetMgr().RemoveDependency(dataType, creatorType, false);
            }
            QueueDelete(infoType);
            QueueDelete(dataType);
            return;
        }

        const auto flags = AssetLoadFlags::LF_HIGH_PRIORITY | AssetLoadFlags::LF_RECURSIVE_PROPERTIES;

        GfxShaderBinaryDataAsset dataAsset(dataType, flags);
        GfxShaderBinaryInfoAsset infoAsset(infoType, flags);
        ReportBug(dataAsset.IsLoaded());
        ReportBug(infoAsset.IsLoaded());

        bundle.Set(api, infoAsset, dataAsset);
    }

    void GfxShaderManagerImpl::QueueDelete(const AssetTypeInfoCPtr& type)
    {
        if (!type)
        {
            return;
        }

        ScopeLock lock(mDeleteQueueLock);
        mDeleteQueue.push_back(type);
    }


}