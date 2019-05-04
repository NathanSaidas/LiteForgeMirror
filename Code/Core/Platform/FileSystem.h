// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#ifndef LF_CORE_FILE_SYSTEM_H
#define LF_CORE_FILE_SYSTEM_H

#include "Core/Platform/PlatformTypes.h"

namespace lf {

class String;

namespace FileSystem {

bool FileCreate(const String& filename);
bool FileDelete(const String& filename);
bool FileExists(const String& filename);
bool FileReserve(const String& filename, FileSize size);

// **********************************
// Create all directories for a leading-up to a path.
// 
// @param {String} path
// **********************************
bool PathCreate(const String& path);
// **********************************
// Check if a path exists.
// 
// @param {String} path
// **********************************
bool PathExists(const String& path);

// **********************************
// Joins two strings together to represent a path.
//
// @param {String} path
// @param {String} other
// **********************************
String PathJoin(const String& path, const String& other);
// **********************************
// Returns a path pointing to the parent directory.
//
// @param {String} path
// **********************************
String PathGetParent(const String& path);
// **********************************
// Returns the extension of a file path 
// 
// Example:
//      "pig.png" => "png"
//      "pig" => ""
//
// @param {String} path
// **********************************
String PathGetExtension(const String& path);
// **********************************
// Returns the full path a given path. (Resolves any '../' )
//
// @param {String} path
// **********************************
String PathResolve(const String& path);
// **********************************
// Returns path but with corrected DIR_CHAR eg path.replace('/','\\')
//
// @param {String} path
// **********************************
String PathCorrectPath(const String& path);
// **********************************
// Returns the working directory. (Where the executable is launched.)
//
// @param {String} path
// **********************************
String GetWorkingPath();


} // namespace FileSystem

} // namespace lf

#endif // LF_CORE_FILE_SYSTEM_H