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
#include "Core/Test/Test.h"
#include "Runtime/Asset/CacheBlockType.h"

namespace lf {

REGISTER_TEST(CacheBlockType_Test, "Runtime.Asset")
{
    TEST(CacheBlockType::ToEnum("lob") == CacheBlockType::CBT_OBJECT);
    TEST(CacheBlockType::ToEnum("level") == CacheBlockType::CBT_LEVEL);
    TEST(CacheBlockType::ToEnum("png") == CacheBlockType::CBT_TEXTURE_DATA);
    TEST(CacheBlockType::ToEnum("jpeg") == CacheBlockType::CBT_TEXTURE_DATA);
    TEST(CacheBlockType::ToEnum("shader") == CacheBlockType::CBT_TEXT_DATA);
    TEST(CacheBlockType::ToEnum("hlsl") == CacheBlockType::CBT_TEXT_DATA);
    TEST(CacheBlockType::ToEnum("lua") == CacheBlockType::CBT_SCRIPT_DATA);
    TEST(CacheBlockType::ToEnum("js") == CacheBlockType::CBT_SCRIPT_DATA);
    TEST(CacheBlockType::ToEnum("ttf") == CacheBlockType::CBT_FONT_DATA);
    TEST(CacheBlockType::ToEnum("wav") == CacheBlockType::CBT_AUDIO_DATA);
    TEST(CacheBlockType::ToEnum("ogg") == CacheBlockType::CBT_AUDIO_DATA);
    TEST(CacheBlockType::ToEnum("fbx") == CacheBlockType::CBT_MESH_DATA);
    TEST(CacheBlockType::ToEnum("obj") == CacheBlockType::CBT_MESH_DATA);
    TEST(CacheBlockType::ToEnum("json") == CacheBlockType::CBT_JSON_DATA);
    TEST(CacheBlockType::ToEnum("lftext") == CacheBlockType::CBT_TEXT_DATA);
    TEST(CacheBlockType::ToEnum("lfbin") == CacheBlockType::CBT_BINARY_DATA);
    TEST(CacheBlockType::ToEnum("txt") == CacheBlockType::CBT_RAW_DATA);
}

} // namespace lf