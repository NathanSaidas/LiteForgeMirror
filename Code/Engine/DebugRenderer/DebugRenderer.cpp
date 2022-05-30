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
#include "Engine/PCH.h"
#include "DebugRenderer.h"
#include "Core/Utility/Array.h"
#include "Core/Math/VectorCombined.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "AbstractEngine/Gfx/GfxMaterial.h"
#include "AbstractEngine/Geometry/GeometryTypes.h"

namespace lf {

static const SizeT MAX_DRAW_COMMAND = 5000; // Maximum number of draw commands before crash. (If we do crash were probably not flushing the commands)

DECLARE_ATOMIC_PTR(GfxMaterial);
using GfxMaterialPtr = GfxMaterialAtomicPtr;
DECLARE_ASSET(GfxMaterial);

template<typename T>
void CreateVertexBuffer(GfxDevice* g, Gfx::ResourcePtr& resource, SizeT vertexCount = T::DefaultVertexCount, SizeT vertexStride = T::DefaultVertexStride)
{
    (g);
    (resource);
    (vertexCount);
    (vertexStride);
    // resource = g->CreateVertexBuffer(vertexCount, vertexStride, Gfx::BufferUsage::DYNAMIC);
    // Assert(resource && resource->GetType() == Gfx::Resource::RT_VERTEX_BUFFER);
}

template<typename T>
void CreateIndexBuffer(GfxDevice* g, Gfx::ResourcePtr& resource, SizeT indexCount = T::DefaultIndexCount)
{
    (g);
    (resource);
    (indexCount);
    // resource = g->CreateIndexBuffer(indexCount, Gfx::IndexStride::SHORT, Gfx::BufferUsage::DYNAMIC);
    // Assert(resource && resource->GetType() == Gfx::Resource::RT_INDEX_BUFFER);
}

class DebugRendererImpl : public DebugRenderer
{
public:
    DebugRendererImpl(const GfxDevicePtr& device);
     
    void DrawBounds(const Vector& center, const Vector& size, const Color& color, Float32 persistence) override;

    void Set3DProjection(const Matrix& projection) override { m3DProjection = projection; }
    void Set3DView(const Matrix& view) override { m3DView = view; }
private:
    struct RenderObject
    {
        RenderObject() :
            mTransform(),
            mVertexBuffer(nullptr),
            mIndexBuffer(nullptr),
            mMaterial(nullptr),
            mVertexCount(INVALID),
            mIndexCount(INVALID),
            mMode(),
            mAlpha(true),
            mWireframe(false)
        {}

        Matrix mTransform;
        Gfx::Resource* mVertexBuffer;
        Gfx::Resource* mIndexBuffer;
        GfxMaterial* mMaterial;
        SizeT mVertexCount;
        SizeT mIndexCount;
        Gfx::RenderMode mMode;
        bool mAlpha;
        bool mWireframe;
    };

    enum DrawCommand
    {
        DC_2D_LINE,
        DC_2D_QUAD,
        DC_2D_IMAGE,
        DC_2D_TEXT,
        DC_3D_BOUNDS,
        DC_3D_PLANE,
        DC_3D_SPHERE,
        DC_3D_LINE,
        DC_3D_GRID,
        DC_3D_WIRE_BOUNDS, // remaps to DC_3D_BOUNDS
        DC_3D_WIRE_PLANE,  // remaps to DC_3D_PLANE
        DC_3D_WIRE_SPHERE, // remaps to DC_3D_SPHERE
        DC_SET_CLIPPING_RECT
    };
    using DrawCommandArray = TArray<DrawCommand>;

    // State:Vertex:Command per Draw Type
    // 2D is simple reusable geometry
    // 3D must prepare geometry foreach draw command
    // 

    struct Bounds3DCommand
    {
        Vector  mCenter;
        Vector  mSize;
        Color   mColor;
        Float32 mPersistence;
    };
    using Bounds3DCommands = TArray<Bounds3DCommand>;
    struct Bounds3DState
    {
        using CommandType = Bounds3DCommand;
        enum { DefaultVertexCount = Geometry::CUBE_VERTEX_COUNT, DefaultVertexStride = sizeof(Geometry::PositionT) };
        Bounds3DState();
        // Resources:

        Gfx::ResourcePtr mVertexBuffer;
        Bounds3DCommands mCommands;
        SizeT            mCommandIndex;
    };
    struct Bounds3DVertex
    {
        // unknown:
    };

    struct Plane3DCommand
    {
        Vector mCenter;
        Vector mSize;
        Color  mColor;
        Quaternion mRotation;
        Float32 mPersistence;
    };
    using Plane3DCommands = TArray<Plane3DCommand>;
    struct Plane3DState
    {
        using CommandType = Plane3DCommand;
        enum { DefaultVertexCount = Geometry::PLANE_VERTEX_COUNT, DefaultVertexStride = sizeof(Geometry::PositionT) };
        Plane3DState();

        Gfx::ResourcePtr mVertexBuffer;
        Plane3DCommands  mCommands;
        SizeT            mCommandIndex;
    };

    struct Sphere3DCommand
    {
        Vector  mCenter;
        Float32 mRadius;
        Color   mColor;
        Float32 mPersistence;
    };
    using Sphere3DCommands = TArray<Sphere3DCommand>;
    struct Sphere3DState
    {
        using CommandType = Sphere3DCommand;
        enum { DefaultRings = 12, DefaultSectors = 12 };
        enum { DefaultVertexCount = Geometry::SphereVertexCount(DefaultRings, DefaultSectors), DefaultVertexStride = sizeof(Geometry::PositionT) };
        enum { DefaultIndexCount = Geometry::SphereIndexCount(DefaultRings, DefaultSectors) };
        Sphere3DState();

        Gfx::ResourcePtr mVertexBuffer;
        Gfx::ResourcePtr mIndexBuffer;
        Sphere3DCommands mCommands;
        SizeT            mCommandIndex;
    };
    struct Sphere3DVertex
    {
        // unknown:
    };

    struct Line3DCommand
    {
        Vector mStart;
        Vector mEnd;
        Color  mColor;
        Float32 mPersistence;
    };
    using Line3DCommands = TArray<Line3DCommand>;
    struct Line3DState
    {
        using CommandType = Line3DCommand;
        enum { DefaultVertexCount = Geometry::LINE_VERTEX_COUNT, DefaultVertexStride = sizeof(Geometry::PositionT) };
        Line3DState();
        Gfx::ResourcePtr mVertexBuffer;
        Line3DCommands   mCommands;
        SizeT            mCommandIndex;
    };

    struct Grid3DCommand
    {
        Vector mCenter;
        Color   mColor;
        SizeT   mNumSegments;
        Float32 mSize;
        Float32 mPersistence;
    };
    using Grid3DCommands = TArray<Grid3DCommand>;
    struct Grid3DState
    {
        using CommandType = Grid3DCommand;
        enum { DefaultSegments = 256 };
        enum { DefaultVertexCount = Geometry::GridVertexCount(DefaultSegments), DefaultVertexStride = sizeof(Geometry::PositionT) };
        Grid3DState();
        Gfx::ResourcePtr mVertexBuffer;
        Grid3DCommands   mCommands;
        SizeT            mCommandIndex;
    };

    struct Shared3DState
    {
        Shared3DState();
        GfxMaterialPtr   mMaterial;
        GfxMaterialAsset mMaterialAsset;
        Gfx::MaterialPropertyId uTransform;
        Gfx::MaterialPropertyId uColor;
    };

    void ExecuteDraw3DBounds(const Bounds3DCommand* cmd, bool wireFrame);

    void ExecuteDrawCall(RenderObject& obj);

    void OnBeginDraw();
    void OnEndDraw();

    void BeginDraw();
    void EndDraw();
    void Render3D();
    void Render2D();
    void ClearCommand();

    GfxDevicePtr mGraphics;
    bool         mRenderReady;

    // 3D: States
    Bounds3DState   mBounds3D;
    Plane3DState    mPlane3D;
    Sphere3DState   mSphere3D;
    Line3DState     mLine3D;
    Grid3DState     mGrid3D;
    Shared3DState   mShared3D;

    DrawCommandArray m3DDrawCommands;

    Float32 mViewWidth;
    Float32 mViewHeight;
    Matrix m3DProjection;
    Matrix m3DView;
};

DebugRenderer* DebugRenderer::Create(const GfxDevicePtr& device)
{
    LF_SCOPED_MEMORY(MMT_GRAPHICS);
    return LFNew<DebugRendererImpl>(device);
}

DebugRendererImpl::Bounds3DState::Bounds3DState()
: mVertexBuffer()
, mCommands()
, mCommandIndex(0)
{}
DebugRendererImpl::Plane3DState::Plane3DState()
: mVertexBuffer()
, mCommands()
, mCommandIndex(0)
{}
DebugRendererImpl::Sphere3DState::Sphere3DState()
: mVertexBuffer()
, mIndexBuffer()
, mCommands()
, mCommandIndex(0)
{}
DebugRendererImpl::Line3DState::Line3DState()
: mVertexBuffer()
, mCommands()
, mCommandIndex(0)
{}
DebugRendererImpl::Grid3DState::Grid3DState()
: mVertexBuffer()
, mCommands()
, mCommandIndex(0)
{}
DebugRendererImpl::Shared3DState::Shared3DState()
: mMaterial()
, mMaterialAsset()
, uTransform(Gfx::INVALID_MATERIAL_PROPERTY_ID)
, uColor(Gfx::INVALID_MATERIAL_PROPERTY_ID)
{}

DebugRendererImpl::DebugRendererImpl(const GfxDevicePtr& device)
: mGraphics(device)
, mRenderReady(false)
, mBounds3D()
, mPlane3D()
, mSphere3D()
, mLine3D()
, mGrid3D()
, mShared3D()
, m3DDrawCommands()
, m3DProjection()
, m3DView()
{

}

void DebugRendererImpl::DrawBounds(const Vector& center, const Vector& size, const Color& color, Float32 persistence)
{
    Assert(m3DDrawCommands.Size() < MAX_DRAW_COMMAND);

    Bounds3DCommand cmd;
    cmd.mCenter = center;
    cmd.mSize = size;
    cmd.mColor = color;
    cmd.mPersistence = persistence;
    mBounds3D.mCommands.Add(cmd);
    m3DDrawCommands.Add(DC_3D_BOUNDS);
}



void DebugRendererImpl::ExecuteDraw3DBounds(const Bounds3DCommand* cmd, bool wireFrame)
{
    if (!cmd || !mRenderReady)
    {
        return;
    }

    // Create a cube:
    Geometry::FullVertexData vertexData;
    TVector<UInt16> indicies;
    Geometry::CreateCube(ToVector3(cmd->mSize), cmd->mColor, vertexData, indicies, Geometry::VT_POSITION, false);

    // if (!mGraphics->CopyVertexBuffer(mBounds3D.mVertexBuffer, vertexData.mPositions.size(), Bounds3DState::DefaultVertexStride, vertexData.mPositions.data()))
    // {
    //     return;
    // }

    (wireFrame); // TODO

    RenderObject obj;
    obj.mVertexBuffer = mBounds3D.mVertexBuffer;
    obj.mVertexCount = 36;
    obj.mMaterial = mShared3D.mMaterial;
    Matrix model = Matrix::TRS(cmd->mCenter, Quaternion::Identity, Vector::One);
    obj.mTransform = m3DProjection * m3DView * model;
    // obj.mMaterial->SetProperty(mShared3D.uTransform, obj.mTransform);
    // obj.mMaterial->SetProperty(mShared3D.uColor, cmd->mColor);
    obj.mWireframe = wireFrame;
    obj.mAlpha = true;
    obj.mMode = Gfx::RenderMode::TRIANGLES;

    ExecuteDrawCall(obj);
}

void DebugRendererImpl::ExecuteDrawCall(RenderObject& obj)
{
    if (!obj.mMaterial || !obj.mVertexBuffer)
    {
        return;
    }
    // TODO: Alpha selection has to be done based on material selection!
    // TODO: Wireframe selection has to be done based on material selection!
    // TODO: Depth selection has to be done based on material selection!
    // TODO: Topology is selected on material

    // mGraphics->BindPipelineState(static_cast<GfxMaterialAdapter*>(obj.mMaterial->GetAdapter()));
    // mGraphics->BindVertexBuffer(obj.mVertexBuffer);
    // if (obj.mIndexBuffer)
    // {
    //     mGraphics->BindIndexBuffer(obj.mIndexBuffer);
    //     mGraphics->DrawIndexed(obj.mIndexCount);
    //     mGraphics->UnbindIndexBuffer();
    // }
    // else
    // {
    //     mGraphics->Draw(obj.mVertexCount);
    // }
    // mGraphics->UnbindVertexBuffer();
    // mGraphics->UnbindPipelineState();
}

void DebugRendererImpl::OnBeginDraw()
{
    ReportBug(!mRenderReady); // Missing EndDraw!
    if (mRenderReady)
    {
        return;
    }

    if (!mGraphics)
    {
        ReportError(false, OperationFailureError, "Failed to begin draw missing graphics device.", "DebugRendererImpl");
        return;
    }

    BeginDraw();
    Render3D();
    Render2D();
    EndDraw();
    ClearCommand();
}
void DebugRendererImpl::OnEndDraw()
{
    
}

void DebugRendererImpl::BeginDraw()
{
    if (!mBounds3D.mVertexBuffer)
    {
        CreateVertexBuffer<Bounds3DState>(mGraphics, mBounds3D.mVertexBuffer);
    }
    if (!mPlane3D.mVertexBuffer)
    {
        CreateVertexBuffer<Plane3DState>(mGraphics, mPlane3D.mVertexBuffer);
    }
    if (!mSphere3D.mVertexBuffer)
    {
        CreateVertexBuffer<Sphere3DState>(mGraphics, mSphere3D.mVertexBuffer);
    }
    if (!mSphere3D.mIndexBuffer)
    {
        CreateIndexBuffer<Sphere3DState>(mGraphics, mSphere3D.mIndexBuffer);
    }
    if (!mLine3D.mVertexBuffer)
    {
        CreateVertexBuffer<Line3DState>(mGraphics, mLine3D.mVertexBuffer);
    }
    if (!mGrid3D.mVertexBuffer)
    {
        CreateVertexBuffer<Grid3DState>(mGraphics, mGrid3D.mVertexBuffer);
    }

    AssetLoadFlags::Value LOAD_FULL_SYNC = AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES;

    if (!mShared3D.mMaterialAsset.IsLoaded())
    {
        mShared3D.mMaterialAsset.Acquire(AssetPath("engine//DebugRenderer/Material.lob"), LOAD_FULL_SYNC);
    }
    if (!mShared3D.mMaterial && mShared3D.mMaterialAsset.IsLoaded())
    {
        // mShared3D.mMaterial = StaticCast<GfxMaterialPtr>(GetAssetMgr().CreateAssetInstance(mShared3D.mMaterialAsset.GetType()));
        // Assert(mGraphics->CreateAdapter(mShared3D.mMaterial));
        // mShared3D.mMaterial->Commit(); // TODO: Rewrite this whole renderer kekw
    }

    if (Invalid(mShared3D.uTransform) && mShared3D.mMaterial)
    {
        //mShared3D.uTransform = mShared3D.mMaterial->FindProperty(Token("uTransform"));
    }

    if (Invalid(mShared3D.uColor) && mShared3D.mMaterial)
    {
        //mShared3D.uColor = mShared3D.mMaterial->FindProperty(Token("uColor"));
    }

    mBounds3D.mCommandIndex = 0;
    mRenderReady = true;
}
void DebugRendererImpl::EndDraw()
{
    ReportBug(mRenderReady); // Missing BeginDraw!
    if (!mRenderReady)
    {
        return;
    }
    mRenderReady = false;
}

template<typename T>
LF_INLINE const typename T::CommandType* PopCommand(T& state)
{
    ReportBug(state.mCommandIndex < state.mCommands.Size());
    return state.mCommandIndex < state.mCommands.Size() ? &state.mCommands[state.mCommandIndex++] : nullptr;
}

void DebugRendererImpl::Render3D()
{
    // mGraphics->SetViewport(0.0f, 0.0f, mViewWidth, mViewHeight, 0.01f, 1000.0f);
    // mGraphics->SetScissorRect(0.0f, 0.0f, mViewWidth, mViewHeight);
    for (DrawCommand& dc : m3DDrawCommands)
    {
        switch (dc)
        {
        case DC_3D_BOUNDS:
        {
            ExecuteDraw3DBounds(PopCommand(mBounds3D), false);
        } break;
        case DC_3D_PLANE:
        {
            // ExecuteDraw3DPlane(PopCommand(mPlane3D), false);
        } break;
        case DC_3D_SPHERE:
        {
            // ExecuteDraw3DSphere(PopCommand(mSphere3D), false);
        } break;
        case DC_3D_LINE:
        {
            // ExecuteDraw3DLine(PopCommand(mLine3D));
        } break;
        case DC_3D_GRID:
        {
            // ExecuteDraw3DGrid(PopCommand(mGrid3D));
        } break;
        case DC_3D_WIRE_BOUNDS:
        {
            ExecuteDraw3DBounds(PopCommand(mBounds3D), true);
        } break;
        case DC_3D_WIRE_PLANE:
        {
            // ExecuteDraw3DPlane(PopCommand(mPlane3D), true);
        } break;
        case DC_3D_WIRE_SPHERE:
        {
            // ExecuteDraw3DSphere(PopCommand(mSphere3D), true);
        } break;
        case DC_SET_CLIPPING_RECT:
        {
            // Rect clippingRect = PopCommand(mClippingRect)->rect;
            // mGraphics->SetClippingRect(clippingRect.x, clippingRect.y, clippingRect.width, clippingRect.height);
        } break;
        default:
            CriticalAssertMsg("Invalid draw command in 3D buffer.");
            break;
        }
    }
}
void DebugRendererImpl::Render2D()
{

}
void DebugRendererImpl::ClearCommand()
{
    mBounds3D.mCommands.Clear();
    mPlane3D.mCommands.Clear();
    mSphere3D.mCommands.Clear();
    mLine3D.mCommands.Clear();
    mGrid3D.mCommands.Clear();
    
    m3DDrawCommands.Clear();
}

} // namespace lf