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
#pragma once

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Utility/StackTrace.h"

#include <intrin.h> // __debugbreak()
#if defined(LF_USE_EXCEPTIONS)
#include <exception>
#endif

// Assert,Crash:
//   When error handling is enabled the program will come to a hard stop. (Request for debugger in non-publish builds)
// ReportBug:
//   Records a bug report that will prompt the user to send to the bug-server.
//
// @[uint32] error_code - An error code based on the context of the reporting function.
//                        

#define LF_ERROR_HANDLING_ENABLED

#if !defined(LF_ERROR_HANDLING_ENABLED)

// Below are the following error handling routines you can use to report bugs/assert logic.
// 
// The main names are as follows ReportBug/Assert/CriticalAssert
//
//   ReportBug -- This is a non-fatal assertion that just verifies the code is correct.
//      - [DEBUG] -- Results in DebugBreak
//      - [TEST]  -- Results in DebugBreak/Exception
//      - [RELEASE/FINAL] -- Report to log
//
//   Assert -- This is a non-fatal (except final builds) assertion that verifies correctness. It can be stepped over with the debugger.
//      - [DEBUG] -- Results in DebugBreak/Exception
//      - [TEST]  -- Results in Exception
//      - [RELEASE] -- DebugBreak
//      - [FINAL] -- Report to log and Fatal Crash
//   
//   CriticalAssert -- This is a fatal assertion that verifies correctness.
//      - [ALL] -- Report to log and Fatal Crash

#define ReportBug(expression_)
#define Assert(expression_)
#define CriticalAssert(expression_)

#define ReportBugMsg(message_)
#define AssertMsg(message_)
#define CriticalAssertMsg(message_)

#define ReportBugEx(expression_, errorCode_, errorApi_)
#define AssertEx(expression_, errorCode_, errorApi_)
#define CriticalAssertEx(expression_, errorCode_, errorApi_)

#define ReportBugMsgEx(message_, errorCode_, errorApi_)
#define AssertMsgEx(message_, errorCode_, errorApi_)
#define CriticalAssertMsgEx(message_, errorCode_, errorApi_)
#endif

#define LF_ERROR_FATAL_BREAK volatile int ERROR_BREAK_ = *(int*)0; (ERROR_BREAK_);

#define LF_ERROR_DEBUG_BREAK __debugbreak()

#if defined(LF_DEBUG) || defined(LF_RELEASE)
#define LF_ERROR_BREAK LF_ERROR_DEBUG_BREAK
#define LF_ERROR_BUG_BREAK LF_ERROR_DEBUG_BREAK
#elif defined(LF_FINAL)
#define LF_ERROR_BREAK LF_ERROR_FATAL_BREAK
#define LF_ERROR_BUG_BREAK
#else
#define LF_ERROR_BREAK
#define LF_ERROR_BUG_BREAK
#endif

#if defined(LF_DEBUG) || defined(LF_TEST)
#define LF_ERROR_EXCEPTIONS
#define LF_ERROR_THROW(expression_, trace_) throw ::lf::Exception(expression_, trace_)
#define LF_ERROR_THROW_EX(expression_, trace_, errorCode_, errorApi_) throw ::lf::Exception(expression_, trace_, errorCode_, errorApi_)
#define LF_ERROR_STACK_TRACE_VAR ERROR_STACK_TRACE_
#define LF_ERROR_STACK_TRACE ::lf::StackTrace LF_ERROR_STACK_TRACE_VAR; ::lf::CaptureStackTrace(LF_ERROR_STACK_TRACE_VAR, 64)
#define LF_ERROR_RELEASE_STACK_TRACE ::lf::ReleaseStackTrace(LF_ERROR_STACK_TRACE_VAR)
#else
#define LF_ERROR_THROW(expression_, trace_) ::lf::ReleaseStackTrace(LF_ERROR_STACK_TRACE_VAR)
#define LF_ERROR_THROW_EX(expression_, trace_, errorCode_, errorApi_) ::lf::ReleaseStackTrace(LF_ERROR_STACK_TRACE_VAR)
#define LF_ERROR_STACK_TRACE_VAR trace
#define LF_ERROR_STACK_TRACE ::lf::ScopedStackTrace LF_ERROR_STACK_TRACE_VAR; ::lf::CaptureStackTrace(LF_ERROR_STACK_TRACE_VAR, 64)
#define LF_ERROR_RELEASE_STACK_TRACE ::lf::ReleaseStackTrace(LF_ERROR_STACK_TRACE_VAR)
#endif

#if defined(LF_TEST)
#define LF_ERROR_TEST_THROW(expression_, trace_) LF_ERROR_THROW(expression_, trace_)
#define LF_ERROR_TEST_THROW_EX(expression_, trace_, errorCode_, errorApi_) LF_ERROR_THROW_EX(expression_, trace_, errorCode_, errorApi_)
#else
#define LF_ERROR_TEST_THROW(expression_, trace_) ::lf::ReleaseStackTrace(LF_ERROR_STACK_TRACE_VAR)
#define LF_ERROR_TEST_THROW_EX(expression_, trace_, errorCode_, errorApi_)
#endif

#if defined(LF_OS_WINDOWS)
#define LF_SET_PLATFORM_ERROR_CODE ::lf::SetPlatformErrorCode();
#else
#define LF_SET_PLATFORM_ERROR_CODE
#endif

// Assert => Assert
// AssertError => AssertEx
// ReportBug => ReportBugMsg
// Crash => CriticalAssertMsgEx

#define Assert(expression_)                                                                         \
{                                                                                                   \
    do {                                                                                            \
        if (!(expression_))                                                                         \
        {                                                                                           \
            LF_SET_PLATFORM_ERROR_CODE;                                                             \
            LF_ERROR_STACK_TRACE;                                                                   \
            ::lf::gAssertCallback(#expression_, LF_ERROR_STACK_TRACE_VAR, ::lf::INVALID32, ::lf::INVALID32);     \
            LF_ERROR_BREAK;                                                                         \
            LF_ERROR_THROW(#expression_, LF_ERROR_STACK_TRACE_VAR);                                 \
        }                                                                                           \
    } while (false);                                                                                \
}

#define AssertEx(expression_, errorCode_, errorApi_)                                                \
{                                                                                                   \
    do {                                                                                            \
        if(!(expression_))                                                                          \
        {                                                                                           \
            LF_SET_PLATFORM_ERROR_CODE;                                                             \
            LF_ERROR_STACK_TRACE;                                                                   \
            ::lf::gAssertCallback(#expression_, LF_ERROR_STACK_TRACE_VAR, errorCode_, errorApi_);   \
            LF_ERROR_BREAK;                                                                         \
            LF_ERROR_THROW_EX(#expression_, LF_ERROR_STACK_TRACE_VAR, errorCode_, errorApi_);       \
        }                                                                                           \
    } while (false);                                                                                \
}

#define AssertMsg(message_)                                                                         \
{                                                                                                   \
    do {                                                                                            \
        if(true)                                                                                    \
        {                                                                                           \
            LF_SET_PLATFORM_ERROR_CODE;                                                             \
            LF_ERROR_STACK_TRACE;                                                                   \
            ::lf::gAssertCallback(message_, LF_ERROR_STACK_TRACE_VAR, INVALID32, INVALID32);        \
            LF_ERROR_BREAK;                                                                         \
            LF_ERROR_THROW_EX(message_, LF_ERROR_STACK_TRACE_VAR, INVALID32, INVALID32);            \
        }                                                                                           \
    } while (false);                                                                                \
}

#define AssertMsgEx(message_, errorCode_, errorApi_)                                                \
{                                                                                                   \
    do {                                                                                            \
        if(true)                                                                                \
        {                                                                                           \
            LF_SET_PLATFORM_ERROR_CODE;                                                             \
            LF_ERROR_STACK_TRACE;                                                                   \
            ::lf::gAssertCallback(message_, LF_ERROR_STACK_TRACE_VAR, errorCode_, errorApi_);       \
            LF_ERROR_BREAK;                                                                         \
            LF_ERROR_THROW_EX(message_, LF_ERROR_STACK_TRACE_VAR, errorCode_, errorApi_);           \
        }                                                                                           \
    } while (false);                                                                                \
}

#define ReportBug(expression_)                                                                          \
{                                                                                                       \
    do {                                                                                                \
        if (!(expression_))                                                                             \
        {                                                                                               \
            LF_SET_PLATFORM_ERROR_CODE;                                                                 \
            LF_ERROR_STACK_TRACE;                                                                       \
            ::lf::gReportBugCallback(#expression_, LF_ERROR_STACK_TRACE_VAR, INVALID32, INVALID32);     \
            LF_ERROR_BUG_BREAK;                                                                         \
            LF_ERROR_RELEASE_STACK_TRACE;                                                               \
        }                                                                                               \
    } while (false);                                                                                    \
}

#define ReportBugEx(expression_, errorCode_, errorApi_)                                                 \
{                                                                                                       \
    do {                                                                                                \
        if (!(expression_))                                                                             \
        {                                                                                               \
            LF_SET_PLATFORM_ERROR_CODE;                                                                 \
            LF_ERROR_STACK_TRACE;                                                                       \
            ::lf::gReportBugCallback(#expression_, LF_ERROR_STACK_TRACE_VAR, errorCode_, errorApi_);    \
            LF_ERROR_BUG_BREAK;                                                                         \
            LF_ERROR_RELEASE_STACK_TRACE;                                                               \
        }                                                                                               \
    } while (false);                                                                                    \
}

#define ReportBugMsg(message_)                                                                          \
{                                                                                                       \
    do                                                                                                  \
    {                                                                                                   \
        if(true)                                                                                        \
        {                                                                                               \
            LF_SET_PLATFORM_ERROR_CODE;                                                                 \
            LF_ERROR_STACK_TRACE;                                                                       \
            ::lf::gReportBugCallback(message_, LF_ERROR_STACK_TRACE_VAR, INVALID32, INVALID32);         \
            LF_ERROR_BUG_BREAK;                                                                         \
            LF_ERROR_RELEASE_STACK_TRACE;                                                               \
        }                                                                                               \
    } while (false);                                                                                    \
}

#define ReportBugMsgEx(message_, errorCode_, errorApi_)                                                 \
{                                                                                                       \
    do                                                                                                  \
    {                                                                                                   \
        if(true)                                                                                        \
        {                                                                                               \
            LF_SET_PLATFORM_ERROR_CODE;                                                                 \
            LF_ERROR_STACK_TRACE;                                                                       \
            ::lf::gReportBugCallback(message_, LF_ERROR_STACK_TRACE_VAR, errorCode_, errorApi_);        \
            LF_ERROR_BUG_BREAK;                                                                         \
            LF_ERROR_RELEASE_STACK_TRACE;                                                               \
        }                                                                                               \
    } while (false);                                                                                    \
}

#define CriticalAssert(expression_)                                                                         \
{                                                                                                           \
    do                                                                                                      \
    {                                                                                                       \
        if (!(expression_))                                                                                 \
        {                                                                                                   \
            LF_SET_PLATFORM_ERROR_CODE;                                                                 \
            LF_ERROR_STACK_TRACE;                                                                           \
            ::lf::gCriticalAssertCallback(#expression_, LF_ERROR_STACK_TRACE_VAR, INVALID32, INVALID32);    \
            LF_ERROR_RELEASE_STACK_TRACE;                                                                   \
            LF_ERROR_FATAL_BREAK;                                                                           \
        }                                                                                                   \
    } while (false);                                                                                        \
}

#define CriticalAssertEx(expression_, errorCode_, errorApi_)                                                \
{                                                                                                           \
    do                                                                                                      \
    {                                                                                                       \
        if (!(expression_))                                                                                 \
        {                                                                                                   \
            LF_SET_PLATFORM_ERROR_CODE;                                                                 \
            LF_ERROR_STACK_TRACE;                                                                           \
            ::lf::gCriticalAssertCallback(#expression_, LF_ERROR_STACK_TRACE_VAR, errorCode_, errorApi_);   \
            LF_ERROR_RELEASE_STACK_TRACE;                                                                   \
            LF_ERROR_FATAL_BREAK;                                                                           \
        }                                                                                                   \
    } while (false);                                                                                        \
}

#define CriticalAssertMsg(message_)                                                                     \
{                                                                                                       \
    do {                                                                                                \
        LF_SET_PLATFORM_ERROR_CODE;                                                                     \
        LF_ERROR_STACK_TRACE;                                                                           \
        ::lf::gCriticalAssertCallback(message_, LF_ERROR_STACK_TRACE_VAR, INVALID32, INVALID32);        \
        LF_ERROR_RELEASE_STACK_TRACE;                                                                   \
        LF_ERROR_FATAL_BREAK;                                                                           \
    } while (false);                                                                                    \
}

#define CriticalAssertMsgEx(message_, errorCode_, errorApi_)                                            \
{                                                                                                       \
    do {                                                                                                \
        LF_SET_PLATFORM_ERROR_CODE;                                                                     \
        LF_ERROR_STACK_TRACE;                                                                           \
        ::lf::gCriticalAssertCallback(message_, LF_ERROR_STACK_TRACE_VAR, errorCode_, errorApi_);       \
        LF_ERROR_RELEASE_STACK_TRACE;                                                                   \
        LF_ERROR_FATAL_BREAK;                                                                           \
    } while (false);                                                                                    \
}



#if defined(LF_USE_EXCEPTIONS)
namespace lf {
class Exception : public std::exception
{
public:
    using Super = std::exception;

    Exception() : Super(), mStackTrace(), mMessage(""), mErrorCode(INVALID32), mErrorAPI(INVALID32) { CaptureStackTrace(mStackTrace, 64); }
    Exception(const char* message, StackTrace& trace) : Super(), mStackTrace(trace), mMessage(message), mErrorCode(INVALID32), mErrorAPI(INVALID32) {}
    Exception(const char* message, StackTrace& trace, UInt32 errorCode, UInt32 errorApi) : Super(), mStackTrace(trace), mMessage(message), mErrorCode(errorCode), mErrorAPI(errorApi) {}
    ~Exception() { ReleaseStackTrace(mStackTrace); }

    SizeT GetFrameCount() const { return mStackTrace.frameCount; }
    const StackFrame& GetFrame(SizeT index) const { return mStackTrace.frames[index]; }
    const char* GetMessage() const { return mMessage; }
    UInt32 GetErrorCode() const { return mErrorCode; }
    UInt32 GetErrorAPI() const { return mErrorAPI; }
private:
    StackTrace mStackTrace;
    const char* mMessage;
    UInt32 mErrorCode;
    UInt32 mErrorAPI;
};
} // namespace lf
#endif

// #define Assert(expression) if(!(expression)) { ::lf::gAssertCallback(#expression, ::lf::ERROR_CODE_UNKNOWN, ::lf::ERROR_API_UNKNOWN); __debugbreak(); } 
// #define AssertError(expression, error_code, error_api) if(!(expression)) { ::lf::gAssertCallback(#expression, error_code, error_api); __debugbreak(); } 
// #define Crash(message, error_code, error_api) { ::lf::gCrashCallback(message, error_code, error_api); __debugbreak(); } 
// #define ReportBug(message, error_code, error_api) { ::lf::gReportBugCallback(message, error_code, error_api); } 

namespace lf {

using ErrorCode = UInt32;
enum ErrorApi : UInt32
{
    ERROR_API_CORE,
    ERROR_API_RUNTIME,
    ERROR_API_ASSET_SERVICE,
    ERROR_API_AUDIO_SERVICE,
    ERROR_API_GRAPHICS_SERVICE,
    ERROR_API_PHYSICS_SERVICE,
    ERROR_API_NET_SERVICE,
    ERROR_API_WEB_SERVICE,
    ERROR_API_WORLD_SERVICE,
    ERROR_API_EDITOR_SERVICE,
    ERROR_API_INPUT_SERVICE,
    ERROR_API_DEBUG_SERVICE,
    ERROR_API_SCRIPT_SERVICE,
    ERROR_API_PLUGIN_SERVICE,
    ERROR_API_ENGINE,
    ERROR_API_GAME,
    ERROR_API_UNKNOWN = INVALID32
};

const ErrorCode ERROR_CODE_UNKNOWN = INVALID32;

using AssertCallback = void(*)(const char*, const StackTrace&, UInt32, UInt32);
using CrashCallback  = void(*)(const char*, const StackTrace&, UInt32, UInt32);
using BugCallback    = void(*)(const char*, const StackTrace&, UInt32, UInt32);

LF_CORE_API extern Int32 gLastPlatformErrorCode;
LF_CORE_API extern Int32 gAssertFlags;
LF_CORE_API extern AssertCallback gAssertCallback;
LF_CORE_API extern CrashCallback  gCriticalAssertCallback;
LF_CORE_API extern BugCallback    gReportBugCallback;

LF_CORE_API void SetPlatformErrorCode();
} // namespace lf
