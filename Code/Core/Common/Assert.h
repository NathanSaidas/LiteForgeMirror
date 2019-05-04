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
#ifndef LF_CORE_ASSERT_H
#define LF_CORE_ASSERT_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

#include <intrin.h> // __debugbreak()

// Assert,Crash:
//   When error handling is enabled the program will come to a hard stop. (Request for debugger in non-publish builds)
// ReportBug:
//   Records a bug report that will prompt the user to send to the bug-server.
//
// @[uint32] error_code - An error code based on the context of the reporting function.
//                        

#define Assert(expression) if(!(expression)) { ::lf::gAssertCallback(#expression, ::lf::ERROR_CODE_UNKNOWN, ::lf::ERROR_API_UNKNOWN); __debugbreak(); } 
#define AssertError(expression, error_code, error_api) if(!(expression)) { ::lf::gAssertCallback(#expression, error_code, error_api); __debugbreak(); } 
#define Crash(message, error_code, error_api) { ::lf::gCrashCallback(message, error_code, error_api); __debugbreak(); } 
#define ReportBug(message, error_code, error_api) { ::lf::gReportBugCallback(message, error_code, error_api); } 

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
    ERROR_API_UNKNOWN
};

const ErrorCode ERROR_CODE_UNKNOWN = INVALID32;

using AssertCallback = void(*)(const char*, ErrorCode, ErrorApi);
using CrashCallback  = void(*)(const char*, ErrorCode, ErrorApi);
using BugCallback    = void(*)(const char*, ErrorCode, ErrorApi);

LF_CORE_API extern AssertCallback gAssertCallback;
LF_CORE_API extern CrashCallback  gCrashCallback;
LF_CORE_API extern BugCallback    gReportBugCallback;
} // namespace lf



#endif // LF_CORE_ASSERT_H