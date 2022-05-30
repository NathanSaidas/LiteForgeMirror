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
#include "AbstractEngine/Gfx/GfxTypes.h"
#include <windows.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <d3d11.h>
// #include <d3dx12.h> // TODO: We can expose this internally, but shouldn't have it externally..
#include <wrl.h>
#if defined(LF_DIRECTX11)

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace lf {
namespace Gfx {

LF_INLINE D3D11_BLEND DX11Value(Gfx::BlendType value)
{
    switch (value)
    {
    case Gfx::BlendType::ZERO: return D3D11_BLEND_ZERO;
    case Gfx::BlendType::ONE: return D3D11_BLEND_ONE;
    case Gfx::BlendType::SRC_COLOR: return D3D11_BLEND_SRC_COLOR;
    case Gfx::BlendType::ONE_MINUS_SRC_COLOR: return D3D11_BLEND_INV_SRC_COLOR;
    case Gfx::BlendType::SRC_ALPHA: return D3D11_BLEND_SRC_ALPHA;
    case Gfx::BlendType::ONE_MINUS_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
    case Gfx::BlendType::DEST_COLOR: return D3D11_BLEND_DEST_COLOR;
    case Gfx::BlendType::ONE_MINUS_DEST_COLOR: return D3D11_BLEND_INV_DEST_COLOR;
    case Gfx::BlendType::DEST_ALPHA: return D3D11_BLEND_DEST_ALPHA;
    case Gfx::BlendType::ONE_MINUS_DEST_ALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::BlendType )");
        break;
    }
    return D3D11_BLEND_ONE;
}

LF_INLINE D3D11_BLEND_OP DX11Value(Gfx::BlendOp value)
{
    switch (value)
    {
    case Gfx::BlendOp::ADD: return D3D11_BLEND_OP_ADD;
    case Gfx::BlendOp::MINUS: return D3D11_BLEND_OP_SUBTRACT;
    case Gfx::BlendOp::INVERSE_MINUS: return D3D11_BLEND_OP_REV_SUBTRACT;
    case Gfx::BlendOp::MIN: return D3D11_BLEND_OP_MIN;
    case Gfx::BlendOp::MAX: return D3D11_BLEND_OP_MAX;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::BlendOp )");
        break;
    }
    return D3D11_BLEND_OP_ADD;
}

LF_INLINE D3D11_CULL_MODE DX11Value(Gfx::CullFace value)
{
    switch (value)
    {
    case Gfx::CullFace::NONE: return D3D11_CULL_NONE;
    case Gfx::CullFace::BACK: return D3D11_CULL_BACK;
    case Gfx::CullFace::FRONT: return D3D11_CULL_FRONT;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::CullFace )");
        break;
    }
    return D3D11_CULL_NONE;
}

LF_INLINE BOOL DX11Value(Gfx::CullMode value)
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

LF_INLINE D3D11_COMPARISON_FUNC DX11Value(Gfx::DepthFunc value)
{
    switch (value)
    {
    case Gfx::DepthFunc::NEVER: return D3D11_COMPARISON_NEVER;
    case Gfx::DepthFunc::LESS: return D3D11_COMPARISON_LESS;
    case Gfx::DepthFunc::EQUAL: return D3D11_COMPARISON_EQUAL;
    case Gfx::DepthFunc::LESS_EQUAL: return D3D11_COMPARISON_LESS_EQUAL;
    case Gfx::DepthFunc::GREATER: return D3D11_COMPARISON_GREATER;
    case Gfx::DepthFunc::NOT_EQUAL: return D3D11_COMPARISON_NOT_EQUAL;
    case Gfx::DepthFunc::GREATER_EQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
    case Gfx::DepthFunc::ALWAYS: return D3D11_COMPARISON_ALWAYS;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::DepthFunc )");
        break;
    }
    return D3D11_COMPARISON_LESS;
}

LF_INLINE D3D11_PRIMITIVE_TOPOLOGY DX11Value(RenderMode mode)
{
    switch (mode)
    {
    case RenderMode::LINES: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    case RenderMode::LINE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case RenderMode::POINTS: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    case RenderMode::TRIANGLES: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case RenderMode::TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:
        CriticalAssertMsg("Unknown enum value. ( Gfx::RenderMode )");
    }
    return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

LF_INLINE UINT DX11CPUUsage(Gfx::BufferUsage usage)
{
    switch (usage)
    {
    case Gfx::BufferUsage::READ_WRITE: return D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    case Gfx::BufferUsage::DYNAMIC: return D3D11_CPU_ACCESS_WRITE;
    case Gfx::BufferUsage::STATIC: return 0;
    default:
        CriticalAssert("Unknown enum value. ( Gfx::BufferUsage )");
    }
    return 0;
}


static const DXGI_FORMAT SHADER_ATTRIB_FORMAT_TO_DXGI[] =
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

} // namespace Gfx

} // namespace lf
#endif