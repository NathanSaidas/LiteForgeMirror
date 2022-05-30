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

#include "AbstractEngine/PCH.h"
#include "GfxShaderBinary.h"

namespace lf {
    
DEFINE_CLASS(lf::GfxShaderBinaryInfo) { NO_REFLECTION; }
DEFINE_CLASS(lf::GfxShaderBinaryData) { NO_REFLECTION; }

void GfxShaderBinaryInfo::Serialize(Stream& s)
{
    Super::Serialize(s);
    SERIALIZE(s, mShaderType, "");
    SERIALIZE(s, mApi, "");
    SERIALIZE(s, mShaderText, "");
    SERIALIZE(s, mShader, "");
    SERIALIZE_ARRAY(s, mDefines, "");
    SERIALIZE(s, mHash, "");
}
void GfxShaderBinaryInfo::OnClone(const Object& o)
{
    Super::OnClone(o);
    const GfxShaderBinaryInfo& other = static_cast<const GfxShaderBinaryInfo&>(o);
    mShaderType = other.mShaderType;
    mApi = other.mApi;
    mShaderText = other.mShaderText;
    mShader = other.mShader;
    mDefines = other.mDefines;
    mHash = other.mHash;
}

void GfxShaderBinaryData::Serialize(Stream& s) 
{
    Super::Serialize(s);
    SERIALIZE(s, mBuffer, "");
}
void GfxShaderBinaryData::SetBuffer(const void* value, SizeT byteCount)
{
    mBuffer.Free();
    mBuffer.Allocate(byteCount, 1);
    mBuffer.SetSize(byteCount);
    memcpy_s(mBuffer.GetData(), mBuffer.GetSize(), value, byteCount);
}
void GfxShaderBinaryData::OnClone(const Object& o)
{
    Super::OnClone(o);
    mBuffer.Copy(static_cast<const GfxShaderBinaryData&>(o).mBuffer);
}

void GfxShaderBinaryBundle::Serialize(Stream& s)
{
    const char* API_INFO_STRINGS[] = {
        "GenericInfo",
        "DX11Info",
        "DX12Info"
    };
    LF_STATIC_ASSERT(LF_ARRAY_SIZE(API_INFO_STRINGS) == ENUM_SIZE(Gfx::GraphicsApi));
    const char* API_DATA_STRINGS[] = {
        "GenericData",
        "DX11Data",
        "DX12Data"
    };
    LF_STATIC_ASSERT(LF_ARRAY_SIZE(API_DATA_STRINGS) == ENUM_SIZE(Gfx::GraphicsApi));

    for (SizeT i = 0; i < ENUM_SIZE(Gfx::GraphicsApi); ++i)
    {
        SERIALIZE_NAMED(s, API_INFO_STRINGS[i], mInfo[i], "");
        SERIALIZE_NAMED(s, API_DATA_STRINGS[i], mData[i], "");
    }
}

} // namespace lf 