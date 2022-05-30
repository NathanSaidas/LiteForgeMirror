// ********************************************************************
// Copyright (c) 2022 Nathan Hanlan
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

#include "Core/Math/Vector4.h"
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include "Engine/Gfx/ModelRenderers/ProceduralModelRenderer.h"


namespace lf
{
    DECLARE_ATOMIC_PTR(ProceduralModelRenderer);

    struct ProceduralMeshComponentData : public ComponentData
    {
        using VertexUV = ProceduralModelRenderer::Vertex;

        TVector<VertexUV> mVertices;
        TVector<UInt16> mIndices;
        GfxTextureBinaryAsset mTexture;
        ProceduralModelRendererAtomicPtr mModelRenderer;
        bool            mDirty = false; // This is a perfect example of waste!
        bool            mDirtyTexture = false;
    };

    class LF_ENGINE_API ProceduralMeshComponent : public Component
    {
        DECLARE_CLASS(ProceduralMeshComponent, Component);
    public:
        using ComponentDataType = ProceduralMeshComponentData;

        ProceduralMeshComponent();
        virtual ~ProceduralMeshComponent();

        void Serialize(Stream& s) override;

        void BeginSerialize(ComponentData* dataPtr) override;
        void EndSerialize() override;
        ComponentFactory* GetFactory() const override;
    private:
        ProceduralMeshComponentData* mData;
        mutable TComponentFactory<ProceduralMeshComponent> mFactory;
    };

    class IgnoreComponentData : public ComponentData
    {};

    class LF_ENGINE_API IgnoreComponent : public Component
    {
        DECLARE_CLASS(IgnoreComponent, Component);
    public:
        using ComponentDataType = IgnoreComponentData;

        void BeginSerialize(ComponentData*) override {}
        void EndSerialize() override {}
        ComponentFactory* GetFactory() const override;
    private:
        mutable TComponentFactory<ProceduralMeshComponent> mFactory;
    };




    // struct MeshSimpleComponentData : public ComponentData
    // {
    // 
    // };
    // 
    // // These are exclusive components (eg; They will not work with each other)
    // class LF_ENGINE_API MeshSimpleComponent : public Component
    // {
    //     DECLARE_COMPONENT(MeshSimpleComponent);
    //     // Features  - Position
    // };
    /*
    class LF_ENGINE_API MeshTextureComponent : public Component
    {
        // Features - Position, TexCoord, TintColor
    };
    class LF_ENGINE_API MeshStandardComponent : public Component
    {
        // Features - Position, TexCoord, TintColor, Normal
    };

    // Also exclusive against other mesh components however uses 'asset data' 
    class LF_ENGINE_API StaticMeshComponent : public Component
    {
        // Features - TBD
    };
    class LF_ENGINE_API SkeletalMeshComponent : public Component
    {
        // Features - TBD
    };


    // Non-exclusive
    class LF_ENGINE_API MeshRendererComponent : public Component
    {
        // Holds the actual 'Model' Renderer
    };

    class LF_ENGINE_API MeshFlagsComponent : public Component
    {
        // 
    };

    */

} // namespace lf