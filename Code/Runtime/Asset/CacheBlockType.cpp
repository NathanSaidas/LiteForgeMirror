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
#include "Runtime/PCH.h"
#include "CacheBlockType.h"
#include "Core/Utility/Array.h"
#include "Runtime/Asset/AssetPath.h"

namespace lf {

struct CacheBlockMapping
{
    CacheBlockType::Value   mValue;
    String                  mName;
    String                  mAcceptedExtensions[3];
};

struct CacheBlockMap
{
    CacheBlockMap()
    {
        LF_STATIC_ASSERT(CacheBlockType::MAX_VALUE == 12, "Update CacheBlock mapping.");

        using namespace CacheBlockType;
        mMappings[CBT_OBJECT]       = { CBT_OBJECT, "Objects", { "lob", "" } };
        mMappings[CBT_LEVEL]        = { CBT_LEVEL, "Levels", { "level", "" } };
        mMappings[CBT_TEXTURE_DATA] = { CBT_TEXTURE_DATA, "Textures", { "png", "jpeg" } };
        mMappings[CBT_SHADER_DATA]  = { CBT_SHADER_DATA, "Shaders", { "" }};
        mMappings[CBT_SCRIPT_DATA]  = { CBT_SCRIPT_DATA, "Scripts", { "lua", "js" } };
        mMappings[CBT_FONT_DATA]    = { CBT_FONT_DATA, "Fonts", { "ttf", "" } };
        mMappings[CBT_AUDIO_DATA]   = { CBT_AUDIO_DATA, "Audio", { "wav", "ogg" } };
        mMappings[CBT_MESH_DATA]    = { CBT_MESH_DATA, "Meshes", { "fbx", "obj" } };
        mMappings[CBT_JSON_DATA]    = { CBT_JSON_DATA, "Json", { "json", "" } };
        mMappings[CBT_TEXT_DATA]    = { CBT_TEXT_DATA, "Text", { "lftext", "shader", "hlsl" }};
        mMappings[CBT_BINARY_DATA]  = { CBT_BINARY_DATA, "BinaryData", { "lfbin", "" } };
        mMappings[CBT_RAW_DATA]     = { CBT_RAW_DATA, "RawData", { "", "" } };
    }

    CacheBlockType::Value ToEnum(const char* extension)
    {
        for (SizeT i = 0; i < LF_ARRAY_SIZE(mMappings); ++i)
        {
            for (SizeT k = 0; k < LF_ARRAY_SIZE(mMappings[i].mAcceptedExtensions); ++k)
            {
                if (!mMappings[i].mAcceptedExtensions->Empty() && mMappings[i].mAcceptedExtensions[k] == extension)
                {
                    return mMappings[i].mValue;
                }
            }
        }
        return CacheBlockType::CBT_RAW_DATA;
    }

    const char* GetName(CacheBlockType::Value type)
    {
        return mMappings[type].mName.CStr();
    }

    CacheBlockMapping mMappings[CacheBlockType::MAX_VALUE];
};

CacheBlockMap& GetCacheBlockMap()
{
    static CacheBlockMap sMap;
    return sMap;
}

CacheBlockType::Value CacheBlockType::ToEnum(const AssetPath& path)
{
    return GetCacheBlockMap().ToEnum(path.GetExtension().CStr());
}

CacheBlockType::Value CacheBlockType::ToEnum(const char* extension)
{
    return GetCacheBlockMap().ToEnum(extension);
}

const char* CacheBlockType::GetName(Value type)
{
    return GetCacheBlockMap().GetName(type);
}

} // namespace lf