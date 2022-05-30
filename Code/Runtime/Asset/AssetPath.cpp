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
#include "AssetPath.h"
#include "Core/String/String.h"
#include "Core/String/StringCommon.h"

namespace lf {

AssetPath EMPTY_PATH;

AssetPath::AssetPath()
: mPath()
{
}
AssetPath::AssetPath(const char* path)
: mPath()
{
    SetPath(path);
}
AssetPath::AssetPath(const Token& path)
    : mPath()
{
    SetPath(path);
}
AssetPath::AssetPath(const String& path)
: mPath()
{
    SetPath(path);
}
AssetPath::~AssetPath()
{

}

void AssetPath::SetPath(const char* path)
{
    SetPath(String(path, COPY_ON_WRITE));
}
void AssetPath::SetPath(const Token& path)
{
    SetPath(String(path.CStr(), COPY_ON_WRITE));
}
void AssetPath::SetPath(const String& path)
{
    String correctPath(path);
    correctPath.Replace('\\', '/');
    if (!correctPath.Empty() && correctPath.First() == '/')
    {
        correctPath = correctPath.SubString(1);
    }
    mPath = Token(correctPath);
}

String AssetPath::GetDomain() const
{
    String path(mPath.CStr(), COPY_ON_WRITE);
    SizeT index = path.Find("//");
    if (Invalid(index))
    {
        return String();
    }
    return path.SubString(0, index);
}
String AssetPath::GetScope() const
{
    String path(mPath.CStr(), COPY_ON_WRITE);
    SizeT index = path.Find("//");
    if (Invalid(index))
    {
        SizeT lastScope = path.FindLast('/');
        if (Invalid(lastScope))
        {
            return String();
        }
        return path.SubString(0, lastScope);
    }
    else
    {
        SizeT lastScope = path.FindLast('/');
        if (Invalid(lastScope) || (lastScope - index) == 1)
        {
            return String();
        }
        return path.SubString(index + 2, lastScope - (index + 2));
    }
}

String AssetPath::GetScopedName() const
{
    String path(mPath.CStr(), COPY_ON_WRITE);
    SizeT index = path.Find("//");
    if (Invalid(index))
    {
        return path;
    }
    return path.SubString(index + 2);
}

String AssetPath::GetName() const
{
    String path(mPath.CStr(), COPY_ON_WRITE);
    SizeT lastScope = path.FindLast('/');
    if (Invalid(lastScope))
    {
        return path;
    }
    return path.SubString(lastScope + 1);
}
String AssetPath::GetExtension() const
{
    String path(mPath.CStr(), COPY_ON_WRITE);
    SizeT extensionChar = path.FindLast('.');
    if (Invalid(extensionChar))
    {
        return String();
    }
    return path.SubString(extensionChar + 1);
}

} // namespace lf