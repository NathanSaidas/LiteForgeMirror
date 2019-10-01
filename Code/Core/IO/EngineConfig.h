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
#ifndef LF_CORE_ENGINE_CONFIG_H
#define LF_CORE_ENGINE_CONFIG_H

#include "Core/String/String.h"

namespace lf {

class Stream;

class LF_CORE_API EngineConfig
{
public:
    void Open(const String& filename);
    void Close();

    void Serialize(Stream& s);

    const String& GetTempDirectory() const { return mResolvedTempDirectory; }
    const String& GetProjectDirectory() const { return mProjectDirectory; }
    const String& GetUserDirectory() const { return mUserDirectory; }
    const String& GetCacheDirectory() const { return mCacheDirectory; }
    const String& GetLogName() const { return mLogFilename; }
    void SetLogName(const String& value) { mLogFilename = value; }
private:
    String mTempDirectory;
    String mProjectDirectory;
    String mUserDirectory;
    String mCacheDirectory;
    String mLogFilename;

    String mResolvedTempDirectory;
    String mResolvedProjectDirectory;
    String mResolvedUserDirectory;
    String mResolvedCacheDirectory;
};

} // namespace lf

#endif // LF_CORE_ENGINE_CONFIG_H