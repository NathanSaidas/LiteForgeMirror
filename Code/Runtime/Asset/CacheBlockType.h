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
#include "Core/Common/API.h"
#include "Core/Common/Enum.h"

namespace lf {

class AssetPath;

DECLARE_ENUM(CacheBlockType,
    CBT_OBJECT,
    CBT_LEVEL,
    CBT_TEXTURE_DATA,
    CBT_SHADER_DATA,
    CBT_SCRIPT_DATA,
    CBT_FONT_DATA,
    CBT_AUDIO_DATA,
    CBT_MESH_DATA,
    CBT_JSON_DATA,
    CBT_TEXT_DATA,
    CBT_BINARY_DATA,
    CBT_RAW_DATA
);

namespace CacheBlockType
{
    LF_RUNTIME_API Value ToEnum(const AssetPath& path);
    LF_RUNTIME_API Value ToEnum(const char* extension);
    LF_RUNTIME_API const char* GetName(Value type);

}

} // namespace lf