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
#include "Log.h"
#include "Core/Common/Assert.h"
#include "Core/IO/EngineConfig.h"
#include "Core/String/StringCommon.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Utility/Time.h"
#include "Core/Platform/FileSystem.h"

#if defined(LF_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace lf {

Timer gLogTimer;
Log gMasterLog("Engine");
Log gSysLog("Sys", &gMasterLog);
Log gIOLog("IO", &gMasterLog);
Log gTestLog("Test", &gMasterLog);
Log gGfxLog("Gfx", &gMasterLog);
Log gNetLog("Net", &gMasterLog);

String LoggerMessage::GetPointerString(const LogPtr& ptr)
{
    String hex = ToHexString(ptr.mValue);
    String result = "0x";
    if (hex.Size() < 16)
    {
        const SizeT numZero = 16 - hex.Size();
        for (SizeT i = 0; i < numZero; ++i)
        {
            result.Append('0');
        }
    }
    return result + hex;
}

Log::Log(const String& name, Log* master) :
mBufferStreamLock(),
mBufferStream(),
mFile(),
mName(name),
mMasterLog(master),
mLogLevel(LOG_INFO),
mConfig(nullptr),
mWorkingDirectoryCached()
{
    if (&gMasterLog == this) { gLogTimer.Start(); }
}
Log::~Log()
{
    mFile.Close();
}

void Log::Info(const LoggerMessage& message)
{
    if (mLogLevel <= LOG_INFO)
    {
        String header = FormatHeader(message, "Info");
        Write(header, message.mContent.Str());
    }
}
void Log::Warning(const LoggerMessage& message)
{
    if (mLogLevel <= LOG_WARNING)
    {
        String header = FormatHeader(message, "Warning");
        Write(header, message.mContent.Str());
    }
}
void Log::Error(const LoggerMessage& message)
{
    if (mLogLevel <= LOG_ERROR)
    {
        String header = FormatHeader(message, "Error");
        Write(header, message.mContent.Str());
    }
}
void Log::Debug(const LoggerMessage& message)
{
    if (mLogLevel <= LOG_DEBUG)
    {
        String header = FormatHeader(message, "Debug");
        Write(header, message.mContent.Str());
    }
}
void Log::Write(const String& header, const String& message)
{
    ScopeLock lock(mBufferStreamLock);
    mBufferStream << header << message << "\n";
}
void Log::Sync()
{
    ScopeLock lock(mBufferStreamLock);
    if (!mBufferStream.Empty())
    {
        Output(mBufferStream);
        mBufferStream.Clear();
    }
}

void Log::Close()
{
    Sync();
    if (!mMasterLog)
    {
        ScopeLock lock(mOutputLock);
        mFile.Close();
    }
    mWorkingDirectoryCached.Clear();
}

const char* StripWorkingDirectory(const char* filename, String& workingDir)
{
    if (workingDir.Empty())
    {
        workingDir = FileSystem::GetWorkingPath();
    }
    String filenameStr(filename, COPY_ON_WRITE);
    SizeT find = filenameStr.Find(workingDir);
    if (Valid(find))
    {
        return (filename + find) + workingDir.Size();
    }
    return filename;
}

String Log::FormatHeader(const LoggerMessage& message, const char* logLevel)
{
    SStream header;
    header << "[" << ToString(gLogTimer.PeekDelta(), 3) << "][" << mName << "][" << logLevel << "][" << StripWorkingDirectory(message.mFilename, mWorkingDirectoryCached) << ":" << message.mLine << "]:";
    return header.Str();
}

void Log::Output(const SStream& buffer)
{
    if (mMasterLog)
    {
        mMasterLog->Output(buffer);
    }
    else
    {
        ScopeLock lock(mOutputLock);
        OutputDebugString(buffer.CStr());
        printf("%s", buffer.CStr());

        if (!mFile.IsOpen() && mConfig)
        {
            // Get config
            // Try Load Directory
            String tempDirectory;
            if (mConfig)
            {
                tempDirectory = FileSystem::PathJoin(mConfig->GetTempDirectory(), "Logs");
            }
            else
            {
                tempDirectory = FileSystem::PathResolve("../Temp/Logs");
            }

            // todo: Date the log file name.
            FileSystem::PathCreate(tempDirectory);
            const String path = FileSystem::PathJoin(FileSystem::PathResolve(tempDirectory), mConfig->GetLogName() + ".log");
            if (!mFile.Open(path, FF_READ | FF_WRITE | FF_SHARE_READ, FILE_OPEN_ALWAYS))
            {
                CriticalAssertMsgEx("Failed to open log file", LF_ERROR_INTERNAL, ERROR_API_CORE);
            }
        }

        if (mFile.IsOpen())
        {
            mFile.Write(buffer.CStr(), buffer.Size());
            if (mFile.GetSize() > 1024 * 1024 * 64)
            {
                mFile.SetCursor(0, FILE_CURSOR_BEGIN);
            }
        }
    }
}



} // namespace lf