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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include "Core/Common/Assert.h"
#include "Core/Memory/MemoryBuffer.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include <windows.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <d3d12.h>
// #include <d3dx12.h> // TODO: We can expose this internally, but shouldn't have it externally..
#include <wrl.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#ifdef CreateWindow
#undef CreateWindow
#endif


#ifdef Yield
#undef Yield
#endif

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

namespace lf {

namespace Gfx {

using DescriptorViewID = UIntPtrT;
struct DescriptorView
{
    DescriptorView()
        : mCPUHandle{ 0 }
        , mGPUHandle{ 0 }
        , mViewID(INVALID)
#if defined(LF_DEBUG)
        , mDebugHeap(nullptr)
#endif
    {}

    D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
    SizeT                       mViewID;
#if defined(LF_DEBUG)
    void*                       mDebugHeap;
#endif
};

LF_INLINE D3D12_BLEND DX12Value(Gfx::BlendType value)
{
    switch (value)
    {
    case Gfx::BlendType::ZERO: return D3D12_BLEND_ZERO;
    case Gfx::BlendType::ONE: return D3D12_BLEND_ONE;
    case Gfx::BlendType::SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
    case Gfx::BlendType::ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
    case Gfx::BlendType::SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
    case Gfx::BlendType::ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
    case Gfx::BlendType::DEST_COLOR: return D3D12_BLEND_DEST_COLOR;
    case Gfx::BlendType::ONE_MINUS_DEST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
    case Gfx::BlendType::DEST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
    case Gfx::BlendType::ONE_MINUS_DEST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::BlendType )");
        break;
    }
    return D3D12_BLEND_ONE;
}

LF_INLINE D3D12_BLEND_OP DX12Value(Gfx::BlendOp value)
{
    switch (value)
    {
    case Gfx::BlendOp::ADD: return D3D12_BLEND_OP_ADD;
    case Gfx::BlendOp::MINUS: return D3D12_BLEND_OP_SUBTRACT;
    case Gfx::BlendOp::INVERSE_MINUS: return D3D12_BLEND_OP_REV_SUBTRACT;
    case Gfx::BlendOp::MIN: return D3D12_BLEND_OP_MIN;
    case Gfx::BlendOp::MAX: return D3D12_BLEND_OP_MAX;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::BlendOp )");
        break;
    }
    return D3D12_BLEND_OP_ADD;
}

LF_INLINE D3D12_CULL_MODE DX12Value(Gfx::CullFace value)
{
    switch (value)
    {
    case Gfx::CullFace::NONE: return D3D12_CULL_MODE_NONE;
    case Gfx::CullFace::BACK: return D3D12_CULL_MODE_BACK;
    case Gfx::CullFace::FRONT: return D3D12_CULL_MODE_FRONT;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::CullFace )");
        break;
    }
    return D3D12_CULL_MODE_NONE;
}

LF_INLINE BOOL DX12Value(Gfx::CullMode value)
{
    switch (value)
    {
    case Gfx::CullMode::CLOCK_WISE: return FALSE; // D3D12_RASTERIZER_DESC::FrontCounterClockwise 
    case Gfx::CullMode::COUNTER_CLOCK_WISE: return TRUE; // D3D12_RASTERIZER_DESC::FrontCounterClockwise
    default:
        CriticalAssert("Unknown enum value. ( Gfx::CullMode )");
        break;
    }
    return FALSE;
}

LF_INLINE D3D12_COMPARISON_FUNC DX12Value(Gfx::DepthFunc value)
{
    switch (value)
    {
    case Gfx::DepthFunc::NEVER: return D3D12_COMPARISON_FUNC_NEVER;
    case Gfx::DepthFunc::LESS: return D3D12_COMPARISON_FUNC_LESS;
    case Gfx::DepthFunc::EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
    case Gfx::DepthFunc::LESS_EQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case Gfx::DepthFunc::GREATER: return D3D12_COMPARISON_FUNC_GREATER;
    case Gfx::DepthFunc::NOT_EQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case Gfx::DepthFunc::GREATER_EQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case Gfx::DepthFunc::ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::DepthFunc )");
        break;
    }
    return D3D12_COMPARISON_FUNC_LESS;
}

LF_INLINE D3D12_LOGIC_OP DX12Value(Gfx::BlendLogicOp value)
{
    switch (value)
    {
    case Gfx::BlendLogicOp::AND: return D3D12_LOGIC_OP_AND;
    case Gfx::BlendLogicOp::AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
    case Gfx::BlendLogicOp::AND_REVERSE: return D3D12_LOGIC_OP_AND_REVERSE;
    case Gfx::BlendLogicOp::CLEAR: return D3D12_LOGIC_OP_CLEAR;
    case Gfx::BlendLogicOp::COPY: return D3D12_LOGIC_OP_COPY;
    case Gfx::BlendLogicOp::COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
    case Gfx::BlendLogicOp::EQUIV: return D3D12_LOGIC_OP_EQUIV;
    case Gfx::BlendLogicOp::INVERT: return D3D12_LOGIC_OP_INVERT;
    case Gfx::BlendLogicOp::NAND: return D3D12_LOGIC_OP_NAND;
    case Gfx::BlendLogicOp::NOOP: return D3D12_LOGIC_OP_NOOP;
    case Gfx::BlendLogicOp::NOR: return D3D12_LOGIC_OP_NOR;
    case Gfx::BlendLogicOp::OR: return D3D12_LOGIC_OP_OR;
    case Gfx::BlendLogicOp::OR_INVERTED: return D3D12_LOGIC_OP_OR_INVERTED;
    case Gfx::BlendLogicOp::OR_REVERSE: return D3D12_LOGIC_OP_OR_REVERSE;
    case Gfx::BlendLogicOp::SET: return D3D12_LOGIC_OP_SET;
    case Gfx::BlendLogicOp::XOR: return D3D12_LOGIC_OP_XOR;
    default:
        CriticalAssertMsg("Unknown enum value. ( Gfx::BlendLogicOp )");
        break;
    }
    return D3D12_LOGIC_OP_NOOP;
}

LF_INLINE D3D12_STENCIL_OP DX12Value(Gfx::StencilOp value)
{
    switch (value)
    {
    case Gfx::StencilOp::DECR: return D3D12_STENCIL_OP_DECR;
    case Gfx::StencilOp::DECR_SAT: return D3D12_STENCIL_OP_DECR_SAT;
    case Gfx::StencilOp::INCR: return D3D12_STENCIL_OP_INCR;
    case Gfx::StencilOp::INCR_SAT: return D3D12_STENCIL_OP_INCR_SAT;
    case Gfx::StencilOp::INVERT: return D3D12_STENCIL_OP_INVERT;
    case Gfx::StencilOp::KEEP: return D3D12_STENCIL_OP_KEEP;
    case Gfx::StencilOp::REPLACE: return D3D12_STENCIL_OP_REPLACE;
    case Gfx::StencilOp::ZERO: return D3D12_STENCIL_OP_ZERO;
    default:
        CriticalAssertMsg("Unknown enum value. ( Gfx::StencilOp )");
        break;
    }
    return D3D12_STENCIL_OP_KEEP;
}

LF_INLINE DXGI_FORMAT DX12Value(Gfx::ResourceFormat value)
{
    switch (value)
    {
    case Gfx::ResourceFormat::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case Gfx::ResourceFormat::R32G32B32A32_INT: return DXGI_FORMAT_R32G32B32A32_SINT;
    case Gfx::ResourceFormat::R32G32B32A32_UINT: return DXGI_FORMAT_R32G32B32A32_UINT;
    case Gfx::ResourceFormat::R32G32B32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
    case Gfx::ResourceFormat::R32G32B32_INT: return DXGI_FORMAT_R32G32B32_SINT;
    case Gfx::ResourceFormat::R32G32B32_UINT: return DXGI_FORMAT_R32G32B32_UINT;
    case Gfx::ResourceFormat::R32G32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
    case Gfx::ResourceFormat::R32G32_INT: return DXGI_FORMAT_R32G32_SINT;
    case Gfx::ResourceFormat::R32G32_UINT: return DXGI_FORMAT_R32G32_UINT;
    case Gfx::ResourceFormat::R8G8B8A8_INT: return DXGI_FORMAT_R8G8B8A8_SINT;
    case Gfx::ResourceFormat::R8G8B8A8_NORM: return DXGI_FORMAT_R8G8B8A8_SNORM;
    case Gfx::ResourceFormat::R8G8B8A8_UINT: return DXGI_FORMAT_R8G8B8A8_UINT;
    case Gfx::ResourceFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case Gfx::ResourceFormat::R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    default:
        CriticalAssertMsg("Unknown enum value. ( Gfx::RenderMode )");
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
}

LF_INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE ToTopologyType(Gfx::RenderMode value)
{
    switch (value)
    {
    case Gfx::RenderMode::LINES: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case Gfx::RenderMode::POINTS: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case Gfx::RenderMode::TRIANGLES: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case Gfx::RenderMode::LINE_STRIP:
    case Gfx::RenderMode::TRIANGLE_STRIP:
        ReportBugMsg("Unsupported enum value. ( Gfx::RenderMode )");
        break;
    default:
        CriticalAssertMsg("Unknown enum value. ( Gfx::RenderMode )");
        break;
    }
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

LF_INLINE D3D12_PRIMITIVE_TOPOLOGY ToTopology(Gfx::RenderMode value)
{
    switch (value)
    {
    case Gfx::RenderMode::LINES: return D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    case Gfx::RenderMode::POINTS: return D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    case Gfx::RenderMode::TRIANGLES: return D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case Gfx::RenderMode::LINE_STRIP: return D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case Gfx::RenderMode::TRIANGLE_STRIP: return D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:
        CriticalAssertMsg("Unknown enum value. ( Gfx::RenderMode )");
        break;
    }
    return D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

const static D3D12_STREAM_OUTPUT_DESC DEFAULT_STREAM_OUTPUT = { 0, 0, 0, 0, 0 };


LF_INLINE D3D12_BLEND_DESC ToDX12(const BlendStateDesc& i)
{
    D3D12_BLEND_DESC o = { 0 };
    o.AlphaToCoverageEnable = FALSE;
    o.IndependentBlendEnable = FALSE; // Only use the first render target.
    D3D12_COLOR_WRITE_ENABLE_ALL;
    o.RenderTarget[0].BlendEnable = i.mBlendEnabled;
    o.RenderTarget[0].BlendOp = DX12Value(i.mBlendOp);
    o.RenderTarget[0].BlendOpAlpha = DX12Value(i.mBlendOpAlpha);
    o.RenderTarget[0].DestBlend = DX12Value(i.mDestBlend);
    o.RenderTarget[0].DestBlendAlpha = DX12Value(i.mDestBlendAlpha);
    o.RenderTarget[0].SrcBlend = DX12Value(i.mSrcBlend);
    o.RenderTarget[0].SrcBlendAlpha = DX12Value(i.mSrcBlendAlpha);
    o.RenderTarget[0].LogicOpEnable = i.mLogicOpEnabled;
    o.RenderTarget[0].LogicOp = DX12Value(i.mLogicOp);
    o.RenderTarget[0].RenderTargetWriteMask = 0;

    if (i.mWriteMask.Has(ColorChannel::RED))
    {
        o.RenderTarget[0].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
    }

    if (i.mWriteMask.Has(ColorChannel::GREEN))
    {
        o.RenderTarget[0].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
    }

    if (i.mWriteMask.Has(ColorChannel::BLUE))
    {
        o.RenderTarget[0].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
    }

    if (i.mWriteMask.Has(ColorChannel::ALPHA))
    {
        o.RenderTarget[0].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
    }

    return o;
}

LF_INLINE D3D12_RASTERIZER_DESC ToDX12(const RasterStateDesc& i)
{
    D3D12_RASTERIZER_DESC o = { };
    o.DepthBias = 0;
    o.DepthBiasClamp = 0.0f;
    o.SlopeScaledDepthBias = 0.0f;
    o.ForcedSampleCount = 0;
    o.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    o.FrontCounterClockwise = i.mCullMode == CullMode::COUNTER_CLOCK_WISE;
    o.CullMode = DX12Value(i.mCullFace);
    o.FillMode = i.mWireFrame ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    o.DepthClipEnable = i.mDepthClipEnabled;
    o.AntialiasedLineEnable = i.mAntialiasedLineEnabled;
    o.MultisampleEnable = i.mMultisampleEnabled;

    return o;
}

LF_INLINE D3D12_DEPTH_STENCILOP_DESC ToDX12(const StencilOpDesc& i)
{
    D3D12_DEPTH_STENCILOP_DESC o = { };
    o.StencilDepthFailOp = DX12Value(i.mStencilDepthFailOp);
    o.StencilFailOp = DX12Value(i.mStencilFailOp);
    o.StencilPassOp = DX12Value(i.mStencilPassOp);
    o.StencilFunc = DX12Value(i.mStencilFunc);
    return o;
}

LF_INLINE D3D12_DEPTH_STENCIL_DESC ToDX12(const DepthStencilStateDesc& i)
{
    D3D12_DEPTH_STENCIL_DESC o = { };

    o.DepthFunc = DX12Value(i.mDepthCompareFunc);
    o.DepthEnable = i.mDepthEnabled;
    o.DepthWriteMask = i.mDepthWriteMaskAll ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    o.StencilEnable = i.mStencilEnabled;
    o.StencilReadMask = i.mStencilReadMask;
    o.StencilWriteMask = i.mStencilWriteMask;
    o.BackFace = ToDX12(i.mBackFace);
    o.FrontFace = ToDX12(i.mFrontFace);

    return o;
}

LF_INLINE D3D12_SHADER_BYTECODE ToDX12(const MemoryBuffer& buffer)
{
    D3D12_SHADER_BYTECODE o = { 0 };
    if (buffer.GetSize() > 0)
    {
        o.BytecodeLength = buffer.GetSize();
        o.pShaderBytecode = buffer.GetData();
    }
    return o;
}

// D3D12_INPUT_ELEMENT_DESC ToDX12()

LF_INLINE void ToDX12(const TStackVector<VertexInputElement, 8>& inputLayout, TStackVector<D3D12_INPUT_ELEMENT_DESC, 8>& outInputLayout)
{
    outInputLayout.resize(inputLayout.size());
    for (SizeT i = 0; i < inputLayout.size(); ++i)
    {
        const auto& in = inputLayout[i];
        auto& out = outInputLayout[i];

        // Required by implementation
        ReportBug(!in.mPerVertexData || in.mInstanceDataStepRate == 0);

        out.AlignedByteOffset = in.mAlignedByteOffset;
        out.Format = DX12Value(in.mFormat);
        out.InputSlot = in.mInputSlot;
        out.InputSlotClass = in.mPerVertexData ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        out.InstanceDataStepRate = in.mPerVertexData ? 0 : in.mInstanceDataStepRate;
        out.SemanticIndex = in.mSemanticIndex;
        out.SemanticName = in.mSemanticName.CStr();
    }
}

LF_INLINE D3D12_SHADER_VISIBILITY ToDX12(ShaderParamVisibility::Value value)
{
    switch (value)
    {
    case ShaderParamVisibility::SPV_ALL: return D3D12_SHADER_VISIBILITY_ALL;
    case ShaderParamVisibility::SPV_PIXEL: return D3D12_SHADER_VISIBILITY_PIXEL;
    case ShaderParamVisibility::SPV_VERTEX: return D3D12_SHADER_VISIBILITY_VERTEX;
    default:
        CriticalAssertMsg("InvalidEnum");
            break;
    }
    return D3D12_SHADER_VISIBILITY_ALL;
}

// from: Introduction to 3D Game Programming with DirectX 12
constexpr const SizeT CalcConstantBufferByteSize(const SizeT byteSize)
{
    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes).  So round up to nearest
    // multiple of 256.  We do this by adding 255 and then masking off
    // the lower 2 bytes which store all bits < 256.
    // Example: Suppose byteSize = 300.
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x022B & 0xff00
    // 0x0200
    // 512
    return (byteSize + 255) & ~255;
}


const DXGI_FORMAT SHADER_ATTRIB_FORMAT_TO_DXGI[] =
{
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_UNKNOWN, // matrix not supported vertex format,
    DXGI_FORMAT_UNKNOWN, // matrix not supported vertex format.
    DXGI_FORMAT_UNKNOWN, // texture not supported vertex format
    DXGI_FORMAT_UNKNOWN, // sampler not supported vertex format
};

}

} // namespace lf