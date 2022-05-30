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
#pragma once
#include "AbstractEngine/Gfx/GfxShaderManager.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "Core/Concurrent/TaskHandle.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/Utility/StdMap.h"

namespace lf {
DECLARE_ASSET(GfxShaderText);
DECLARE_ASSET(GfxShaderBinaryInfo);
DECLARE_ASSET_TYPE(GfxShaderBinaryData);

// TODO:
// - Implement a function to compile necessary binary for the supported shader APIs
// - Implement a function load/acquire the shader assets
// - Implement a function delete unreferenced shader assets ( where Info|Data.StrongRefs == 0 )
// - Implement a function to detect leaked unreferenced shader assets ( slow crawl through AssetMgr of all materials )
// - 
// - Implement GfxShaderManager as a service?
// - Add GfxDevice as a dependency since we'll need it to load/acquire the shader assets.
class LF_ENGINE_API GfxShaderManagerImpl : public GfxShaderManager
{
public:
    GfxShaderManagerImpl();
    virtual ~GfxShaderManagerImpl();

    GfxShaderBinaryBundle CreateShaderAssets(
        const AssetTypeInfoCPtr& creatorType
        , const GfxShaderAsset& shader
        , const TVector<Token>& defines
        , Gfx::ShaderType shaderType) override;
    void DestroyShaderAssets(const AssetTypeInfoCPtr& creatorType, const GfxShaderBinaryBundle& bundle) override;

    void Update();

    // void LoadShader(const GfxShaderAsset& shader, const TVector<Token>& defines, Gfx::ShaderType shaderType, Gfx::GraphicsApi api) override;
private:
    void CreateBinaryAssets(
        GfxShaderBinaryBundle& bundle
        , const AssetTypeInfoCPtr& creatorType
        , const GfxShaderAsset& shader
        , const GfxShaderTextAsset& shaderText
        , const TVector<Token>& defines
        , Gfx::ShaderType shaderType
        , Gfx::GraphicsApi api
        , Gfx::ShaderHash hash);

    void QueueDelete(const AssetTypeInfoCPtr& type);

    struct ShaderData
    {
        void Serialize(Stream& s);

        GfxShaderBinaryInfoAsset mInfo;
        GfxShaderBinaryDataAssetType mData;
        Gfx::ResourcePtr mResource;
        TaskHandle mCompileTask;

        friend static Stream& operator<<(Stream& s, ShaderData& o)
        {
            o.Serialize(s);
            return s;
        }
    };

    // TODO: We can store this in a temp file to make debugging easier in the event of data corruption.
    struct ShaderReferenceData
    {
        AssetPath mShaderPath;
        TVector<Token> mDefines;
        Gfx::ShaderType mType;
        Gfx::ShaderHash mHash[ENUM_SIZE(Gfx::GraphicsApi)];
    };
    
    // 
    // Hash = Hash(Api,ShaderType,Defines,Path)
    // HashedPath = ComputePath(Api,ShaderType,Hash,Path)


    TMap<Token, TAtomicStrongPointer<ShaderData>> mShaders;
    
    // Create operations can happen asynchronously, but we sync up to delete assets.
    using ScopedCreateLock = ScopeRWSpinLockRead;
    using ScopedDeleteLock = ScopeRWSpinLockWrite;
    RWSpinLock mCreateDeleteLock;

    SpinLock mDeleteQueueLock;

    // 
    TVector<AssetTypeInfoCPtr> mDeleteQueue;

};

} // namespace lf
