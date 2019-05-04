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
        ~StackTrace() { ReleaseStackTrace(*this); }

        size_t frameCount;    //** Number of frames captured
        StackFrame* frames;   //** Information about each frame.
    };
}

#endif // SHIELD_HEART_CORE_STACK_TRACE_H
