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
#pragma once
#include "Core/Memory/MemoryBuffer.h"
#include "Core/String/String.h"
#include "AbstractEngine/Gfx/GfxTypes.h"


namespace lf {

// This class relies on the new shader compiler... Requires you to add 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64' to your system path. (Or wherever the DLL is located).
// TODO: Should we just include this DLL within our project?
// TODO: Or we can write a script to properly install this?
class LF_ENGINE_API DX12GfxShaderCompiler
{
public:
    bool Compile(Gfx::ShaderType shaderType, const String& text, const TVector<Token>& defines, MemoryBuffer& outBuffer);
};

} // namespace lf