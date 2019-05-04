// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "Log.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Platform/FileSystem.h"

#if defined(LF_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace lf {

Log gMasterLog("Engine");
Log gSysLog("Sys", &gMasterLog);
Log gIOLog("IO", &gMasterLog);
Log gTestLog("Test", &gMasterLog);
Log gGfxLog("Gfx", &gMasterLog);

Log::Log(const String& name, Log* master) :
mBufferStreamLock(),
mBufferStream(),
mFile(),
mName(name),
mMasterLog(master),
mLogLevel(LOG_INFO)
{}
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
}

String Log::FormatHeader(const LoggerMessage& message, const char* logLevel)
{
    SStream header;
    header << "[" << mName << "][" << logLevel << "][" << message.mFilename << ":" << message.mLine << "]:";
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

        if (!mFile.IsOpen())
        {
            // todo: Date the log file name.
            FileSystem::PathCreate(FileSystem::PathResolve("../Temp/Logs"));
            const String path = FileSystem::PathResolve("../Temp/Logs/") + mName + ".log";
            if (!mFile.Open(path, FF_READ | FF_WRITE | FF_SHARE_READ, FILE_OPEN_ALWAYS))
            {
                Crash("Failed to open log file", LF_ERROR_INTERNAL, ERROR_API_CORE);
            }
        }

        if (mFile.IsOpen())
        {
            mFile.Write(buffer.CStr(), buffer.Size());
        }
    }
}



} // namespace lf