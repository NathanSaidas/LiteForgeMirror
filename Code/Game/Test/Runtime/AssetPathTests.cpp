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
#include "Runtime/Asset/AssetPath.h"
namespace lf {

REGISTER_TEST(AssetPath_Test, "Runtime.Asset")
{
    {
        AssetPath path;
        TEST(path.Empty());
        TEST(path.Size() == 0);
        TEST(path.CStr() != nullptr);
        TEST(path.GetDomain() == "");
        TEST(path.GetScope() == "");
        TEST(path.GetName() == "");
        TEST(path.GetExtension() == "");
    }

    {
        String str("engine//builtin/textures/buildbtn.png");
        AssetPath path(str);
        TEST(path.Empty() == false);
        TEST(path.Size() == str.Size());
        TEST(path.CStr() != nullptr);
        TEST(path.GetDomain() == "engine");
        TEST(path.GetScope() == "builtin/textures");
        TEST(path.GetName() == "buildbtn.png");
        TEST(path.GetExtension() == "png");
    }

    {
        String str("builtin/textures/buildbtn.png");
        AssetPath path(str);
        TEST(path.Empty() == false);
        TEST(path.Size() == str.Size());
        TEST(path.CStr() != nullptr);
        TEST(path.GetDomain() == "");
        TEST(path.GetScope() == "builtin/textures");
        TEST(path.GetName() == "buildbtn.png");
        TEST(path.GetExtension() == "png");
    }

    {
        String str("buildbtn.png");
        AssetPath path(str);
        TEST(path.Empty() == false);
        TEST(path.Size() == str.Size());
        TEST(path.CStr() != nullptr);
        TEST(path.GetDomain() == "");
        TEST(path.GetScope() == "");
        TEST(path.GetName() == "buildbtn.png");
        TEST(path.GetExtension() == "png");
    }

    {
        String str("engine//buildbtn.png");
        AssetPath path(str);
        TEST(path.Empty() == false);
        TEST(path.Size() == str.Size());
        TEST(path.CStr() != nullptr);
        TEST(path.GetDomain() == "engine");
        TEST(path.GetScope() == "");
        TEST(path.GetName() == "buildbtn.png");
        TEST(path.GetExtension() == "png");
    }

}

} // namespace lf