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
#include "Core/String/Token.h"

namespace lf {

// ********************************************************************
// Provides a conveinent set of functions to store a path to an asset
// in a way that efficient in both memory usage/comparisons.
//   
//                                              Extension
//                                                 vvv 
// Example: lite_forge_game//example/path/to/asset.png
//          ^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^ ^^^^^^^^^
//               domain          scope         name
//
//
// ********************************************************************
class LF_RUNTIME_API AssetPath
{
public:
    AssetPath();
    explicit AssetPath(const char* path);
    explicit AssetPath(const Token& path);
    explicit AssetPath(const String& path);
    ~AssetPath();

    void SetPath(const char* path);
    void SetPath(const Token& path);
    void SetPath(const String& path);

    String GetDomain() const;
    String GetScope() const;
    String GetScopedName() const;
    String GetName() const;
    String GetExtension() const;

    bool Empty() const { return mPath.Empty(); }
    SizeT Size() const { return mPath.Size(); }
    const char* CStr() const { return mPath.CStr(); }

    const Token& AsToken() const { return mPath; }

    bool operator==(const AssetPath& other) const { return mPath == other.mPath; }
    bool operator!=(const AssetPath& other) const { return mPath != other.mPath; }
    bool operator<(const AssetPath& other) const { return mPath < other.mPath; }

private:
    Token mPath;
};

extern LF_RUNTIME_API AssetPath EMPTY_PATH;

} // namespace lf