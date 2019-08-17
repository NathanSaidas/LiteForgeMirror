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
#ifndef LF_CORE_LOG_H
#define LF_CORE_LOG_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/String/SStream.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Platform/File.h"

namespace lf {

class EngineConfig;

enum LogLevel
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

struct LF_CORE_API LoggerMessage
{
    LoggerMessage() : 
        mFilename(""),
        mLine(1),
        mContent()
    {
        mContent.Reserve(1024);
    }

    template<typename ValueT>
    LoggerMessage(const char* filename, SizeT line, const ValueT& message) :
        mFilename(filename),
        mLine(line),
        mContent()
    {
        mContent.Reserve(1024);
        mContent << message;
    }

    LoggerMessage& operator<<(bool value) { mContent << value; return *this; }
    LoggerMessage& operator<<(Int8 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(Int16 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(Int32 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(Int64 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(UInt8 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(UInt16 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(UInt32 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(UInt64 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(Float32 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(Float64 value) { mContent << value; return *this; }
    LoggerMessage& operator<<(const char* value) { mContent << value; return *this; }
    LoggerMessage& operator<<(const String& value) { mContent << value; return *this; }
    LoggerMessage& operator<<(const Token& value) { mContent << value; return *this; }

    LoggerMessage& operator<<(const StreamFillRight& fill) { mContent << fill; return *this; }
    LoggerMessage& operator<<(const StreamFillLeft& fill) { mContent << fill; return *this; }
    LoggerMessage& operator<<(const StreamFillChar& fill) { mContent << fill; return *this; }
    LoggerMessage& operator<<(const StreamPrecision& precision) { mContent << precision; return *this; }
    LoggerMessage& operator<<(const StreamBoolAlpha& option) { mContent << option; return *this; }
    LoggerMessage& operator<<(const StreamCharAlpha& option) { mContent << option; return *this; }

    const char* mFilename;
    SizeT       mLine;
    SStream     mContent;
};

// **********************************
// Creates a message for logging. 
// Example Usage:
// gSysLog.Info(LogMessage("Begin Message") << details << go << " here"); // Automatically submits with new line.
//
// @param {any:primitive} message
// **********************************
#define LogMessage(message) lf::LoggerMessage(__FILE__, __LINE__, message)

// Info( message )
//   header = GenerateHeader("Info");
//   if(masterLog)
//      masterLog.Write(header, message);
//      return;
//   Write(header, message);
//
// Write( header, message )
//   Lock(bufferStream)
//   bufferStream.Append(header);
//   bufferStream.Append(message);
//   bufferStream.Append("\n");
//   Unlock(bufferStream)
//
// Sync()
//   Lock(bufferStream)
//   file.Write(bufferStream.Str());
//   console.Write(bufferStream.Str());
//   remoteLog.Write(bufferStream.Str());
//   Unlock(bufferStream);
class LF_CORE_API Log 
{
public:
    Log(const String& name, Log* master = nullptr);
    ~Log();

    void Info(const LoggerMessage& message);
    void Warning(const LoggerMessage& message);
    void Error(const LoggerMessage& message);
    void Debug(const LoggerMessage& message);

    void Write(const String& header, const String& message);
    void Sync();
    void Close();

    void SetLogLevel(LogLevel value) { mLogLevel = value; }
    void SetConfig(const EngineConfig* config) { mConfig = config; }
private:
    Log(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(const Log&) = delete;
    Log& operator=(Log&&) = delete;

    String FormatHeader(const LoggerMessage& message, const char* logLevel);
    void Output(const SStream& buffer);

    SpinLock mBufferStreamLock;
    SStream  mBufferStream;
    SpinLock mOutputLock;
    File     mFile;
    String   mName;
    Log*     mMasterLog;
    LogLevel mLogLevel;
    const EngineConfig* mConfig;
};

extern LF_CORE_API Log gMasterLog;
extern LF_CORE_API Log gSysLog;
extern LF_CORE_API Log gIOLog;
extern LF_CORE_API Log gTestLog;
extern LF_CORE_API Log gGfxLog;
}

#endif // LF_CORE_LOG_H