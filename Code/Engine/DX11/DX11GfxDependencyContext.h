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
#include "AbstractEngine/Gfx/GfxBase.h"
#include "Engine/DX11/DX11Common.h"

namespace lf {
#if defined(LF_DIRECTX11)

class LF_ENGINE_API DX11GfxDependencyContext : public GfxDependencyContext
{
    DECLARE_CLASS(DX11GfxDependencyContext, GfxDependencyContext);
public:
    DX11GfxDependencyContext() = delete;
    DX11GfxDependencyContext(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, const ServiceContainer* services);

    ComPtr<ID3D11Device>& GetDevice() { return mDevice; }
    ComPtr<ID3D11DeviceContext>& GetDeviceContext() { return mDeviceContext; }
private:
    ComPtr<ID3D11Device>  mDevice;
    ComPtr<ID3D11DeviceContext> mDeviceContext;
};


#endif

} // namespace lf