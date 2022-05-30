// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#include "Core/PCH.h"
#include "EngineConfig.h"
#include "Core/Platform/FileSystem.h"
#include "Core/IO/TextStream.h"

namespace lf {

EngineConfig::EngineConfig()
: mTempDirectory()
, mProjectDirectory()
, mUserDirectory()
, mCacheDirectory()
, mLogFilename()
, mDebugGPU(false)
, mResolvedTempDirectory()
, mResolvedProjectDirectory()
, mResolvedUserDirectory()
, mResolvedCacheDirectory()
{

}


void EngineConfig::Open(const String& filename)
{
    String fullpath = FileSystem::PathResolve(filename);

    mTempDirectory = "../Temp";
    mProjectDirectory = "../Project";
    mUserDirectory = "../User";
    mCacheDirectory = "../Cache";
    mLogFilename = "Engine";
    mTestConfig = "TestRunner.config";
    mAppConfig = "AppConfig.config";

    if (!FileSystem::FileExists(fullpath))
    {
        TextStream ts(Stream::FILE, fullpath, Stream::SM_WRITE);
        if (ts.GetMode() == Stream::SM_WRITE)
        {
            ts.BeginObject("Config", "BaseConfig");
            Serialize(ts);
            ts.EndObject();
            ts.Close();
        }
    }
    else
    {
        TextStream ts(Stream::FILE, fullpath, Stream::SM_READ);
        if (ts.GetMode() == Stream::SM_READ)
        {
            ts.BeginObject("Config", "BaseConfig");
            Serialize(ts);
            ts.EndObject();
            ts.Close();
        }

        ts.Open(Stream::FILE, fullpath, Stream::SM_WRITE);
        if (ts.GetMode() == Stream::SM_WRITE)
        {
            ts.BeginObject("Config", "BaseConfig");
            Serialize(ts);
            ts.EndObject();
            ts.Close();
        }
    }

    mResolvedTempDirectory = FileSystem::PathResolve(mTempDirectory);
    mResolvedProjectDirectory = FileSystem::PathResolve(mProjectDirectory);
    mResolvedUserDirectory = FileSystem::PathResolve(mUserDirectory);
    mResolvedCacheDirectory = FileSystem::PathResolve(mCacheDirectory);
}

void EngineConfig::Close()
{
    mTempDirectory.Clear();
    mProjectDirectory.Clear();
    mUserDirectory.Clear();
    mCacheDirectory.Clear();


    mResolvedTempDirectory.Clear();
    mResolvedProjectDirectory.Clear();
    mResolvedUserDirectory.Clear();
    mResolvedCacheDirectory.Clear();

    mLogFilename.Clear();
    mTestConfig.Clear();
}

void EngineConfig::Serialize(Stream& s)
{
    SERIALIZE(s, mTempDirectory, "");
    SERIALIZE(s, mProjectDirectory, "");
    SERIALIZE(s, mUserDirectory, "");
    SERIALIZE(s, mCacheDirectory, "");
    SERIALIZE(s, mLogFilename, "");
    SERIALIZE(s, mDebugGPU, "");
    SERIALIZE(s, mTestConfig, "");
    SERIALIZE(s, mAppConfig, "");
}

} // namespace lf 