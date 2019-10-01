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
#ifndef LF_CORE_STACK_TRACE_H
#define LF_CORE_STACK_TRACE_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf
{
    struct StackTrace;
    struct StackFrame;

    // **********************************
    // Initialize the stack trace library. (Loads DLL)
    // @return Returns true if the library was initialized and safe to use.
    // **********************************
    LF_CORE_API bool InitStackTrace();
    // **********************************
    // Releases the stack trace library memory. (Frees DLL)
    // **********************************
    LF_CORE_API void TerminateStackTrace();
    // **********************************
    // Gets information about the current callstack in the calling thread/process.
    // **********************************
    LF_CORE_API void CaptureStackTrace(StackTrace& trace, size_t maxLines);
    // **********************************
    // Releases allocated resources from CaptureStackTrace
    // **********************************
    LF_CORE_API void ReleaseStackTrace(StackTrace& trace);

    struct StackFrame
    {
        size_t line;          //** Line of the source file
        const char* function; //** Function Name
        const char* filename; //** Filename
    };

    struct StackTrace
    {
        StackTrace() : frameCount(0), frames(nullptr) {}
        StackTrace(const StackTrace& other) : frameCount(other.frameCount), frames(other.frames) {}

        size_t frameCount;    //** Number of frames captured
        StackFrame* frames;   //** Information about each frame.
    };

    struct ScopedStackTrace : public StackTrace
    {
        ~ScopedStackTrace() { ReleaseStackTrace(*this); }

    };
}

#endif // SHIELD_HEART_CORE_STACK_TRACE_H
