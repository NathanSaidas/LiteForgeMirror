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
#include "DX11GfxFactory.h"
#include "Core/Utility/Log.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "AbstractEngine/Gfx/GfxBase.h"

#include "Engine/DX11/DX11GfxWindow.h"
#include "Engine/DX11/DX11GfxShader.h"
#include "Engine/DX11/DX11GfxMaterial.h"

namespace lf {
#if defined(LF_DIRECTX11)

    void DX11GfxFactory::Initialize()
    {
        CreateMapping(
            typeof(GfxWindow),
            typeof(DX11GfxWindow),
            typeof(DX11GfxWindowAdapter));

        CreateMapping(
            typeof(GfxShader),
            typeof(DX11GfxShader),
            typeof(DX11GfxShaderAdapter));

        CreateMapping(
            typeof(GfxMaterial),
            typeof(DX11GfxMaterial),
            typeof(DX11GfxMaterialAdapter));

        CreateMapping(
            typeof(DX11GfxMaterial),
            typeof(DX11GfxMaterial),
            typeof(DX11GfxMaterialAdapter));

    }

#endif

} // namespace lf