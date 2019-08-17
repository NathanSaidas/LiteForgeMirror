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
#include "StackTrace.h"
#include "Core/Memory/Memory.h"

#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#include <Windows.h>
#endif

// DbgHelp Constants:
#define MAX_SYM_NAME 2000
//
// options that are set/returned by SymSetOptions() & SymGetOptions()
// these are used as a mask
//
#define SYMOPT_CASE_INSENSITIVE          0x00000001
#define SYMOPT_UNDNAME                   0x00000002
#define SYMOPT_DEFERRED_LOADS            0x00000004
#define SYMOPT_NO_CPP                    0x00000008
#define SYMOPT_LOAD_LINES                0x00000010
#define SYMOPT_OMAP_FIND_NEAREST         0x00000020
#define SYMOPT_LOAD_ANYTHING             0x00000040
#define SYMOPT_IGNORE_CVREC              0x00000080
#define SYMOPT_NO_UNQUALIFIED_LOADS      0x00000100
#define SYMOPT_FAIL_CRITICAL_ERRORS      0x00000200
#define SYMOPT_EXACT_SYMBOLS             0x00000400
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS    0x00000800
#define SYMOPT_IGNORE_NT_SYMPATH         0x00001000
#define SYMOPT_INCLUDE_32BIT_MODULES     0x00002000
#define SYMOPT_PUBLICS_ONLY              0x00004000
#define SYMOPT_NO_PUBLICS                0x00008000
#define SYMOPT_AUTO_PUBLICS              0x00010000
#define SYMOPT_NO_IMAGE_SEARCH           0x00020000
#define SYMOPT_SECURE                    0x00040000
#define SYMOPT_NO_PROMPTS                0x00080000
#define SYMOPT_OVERWRITE                 0x00100000
#define SYMOPT_IGNORE_IMAGEDIR           0x00200000
#define SYMOPT_FLAT_DIRECTORY            0x00400000
#define SYMOPT_FAVOR_COMPRESSED          0x00800000
#define SYMOPT_ALLOW_ZERO_ADDRESS        0x01000000
#define SYMOPT_DISABLE_SYMSRV_AUTODETECT 0x02000000
#define SYMOPT_READONLY_CACHE            0x04000000
#define SYMOPT_SYMPATH_LAST              0x08000000
#define SYMOPT_DISABLE_FAST_SYMBOLS      0x10000000
#define SYMOPT_DISABLE_SYMSRV_TIMEOUT    0x20000000
#define SYMOPT_DISABLE_SRVSTAR_ON_STARTUP 0x40000000
#define SYMOPT_DEBUG                     0x80000000

typedef enum {
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
} ADDRESS_MODE;

// DbgHelp Structures:
typedef struct _tagADDRESS64 {
    DWORD64       Offset;
    WORD          Segment;
    ADDRESS_MODE  Mode;
} ADDRESS64, *LPADDRESS64;
//
// New KDHELP structure for 64 bit system support.
// This structure is preferred in new code.
//
typedef struct _KDHELP64 {

    //
    // address of kernel thread object, as provided in the
    // WAIT_STATE_CHANGE packet.
    //
    DWORD64   Thread;

    //
    // offset in thread object to pointer to the current callback frame
    // in kernel stack.
    //
    DWORD   ThCallbackStack;

    //
    // offset in thread object to pointer to the current callback backing
    // store frame in kernel stack.
    //
    DWORD   ThCallbackBStore;

    //
    // offsets to values in frame:
    //
    // address of next callback frame
    DWORD   NextCallback;

    // address of saved frame pointer (if applicable)
    DWORD   FramePointer;


    //
    // Address of the kernel function that calls out to user mode
    //
    DWORD64   KiCallUserMode;

    //
    // Address of the user mode dispatcher function
    //
    DWORD64   KeUserCallbackDispatcher;

    //
    // Lowest kernel mode address
    //
    DWORD64   SystemRangeStart;

    //
    // Address of the user mode exception dispatcher function.
    // Added in API version 10.
    //
    DWORD64   KiUserExceptionDispatcher;

    //
    // Stack bounds, added in API version 11.
    //
    DWORD64   StackBase;
    DWORD64   StackLimit;

    //
    // Target OS build number. Added in API version 12.
    //
    DWORD     BuildVersion;
    DWORD     Reserved0;
    DWORD64   Reserved1[4];

} KDHELP64, *PKDHELP64;
typedef struct _tagSTACKFRAME64 {
    ADDRESS64   AddrPC;               // program counter
    ADDRESS64   AddrReturn;           // return address
    ADDRESS64   AddrFrame;            // frame pointer
    ADDRESS64   AddrStack;            // stack pointer
    ADDRESS64   AddrBStore;           // backing store pointer
    PVOID       FuncTableEntry;       // pointer to pdata/fpo or NULL
    DWORD64     Params[4];            // possible arguments to the function
    BOOL        Far;                  // WOW far call
    BOOL        Virtual;              // is this a virtual frame?
    DWORD64     Reserved[3];
    KDHELP64    KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;
typedef struct _IMAGEHLP_SYMBOL64 {
    DWORD   SizeOfStruct;           // set to sizeof(IMAGEHLP_SYMBOL64)
    DWORD64 Address;                // virtual address including dll base address
    DWORD   Size;                   // estimated size of symbol, can be zero
    DWORD   Flags;                  // info about the symbols, see the SYMF defines
    DWORD   MaxNameLength;          // maximum size of symbol name in 'Name'
    CHAR    Name[1];                // symbol name (null terminated string)
} IMAGEHLP_SYMBOL64, *PIMAGEHLP_SYMBOL64;
typedef struct _SYMBOL_INFO {
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;        // Type Index of symbol
    ULONG64     Reserved[2];
    ULONG       Index;
    ULONG       Size;
    ULONG64     ModBase;          // Base Address of module comtaining this symbol
    ULONG       Flags;
    ULONG64     Value;            // Value of symbol, ValuePresent should be 1
    ULONG64     Address;          // Address of symbol including base address of module
    ULONG       Register;         // register holding value or pointer to value
    ULONG       Scope;            // scope of the symbol
    ULONG       Tag;              // pdb classification
    ULONG       NameLen;          // Actual length of name
    ULONG       MaxNameLen;
    CHAR        Name[1];          // Name of symbol
} SYMBOL_INFO, *PSYMBOL_INFO;
typedef struct _IMAGEHLP_LINE64 {
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE64)
    PVOID    Key;                    // internal
    DWORD    LineNumber;             // line number in file
    PCHAR    FileName;               // full filename
    DWORD64  Address;                // first instruction of line
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

// DbgHelp Function Ptrs:
typedef
BOOL
(__stdcall *PREAD_PROCESS_MEMORY_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 qwBaseAddress,
    _Out_writes_bytes_(nSize) PVOID lpBuffer,
    _In_ DWORD nSize,
    _Out_ LPDWORD lpNumberOfBytesRead
    );
typedef
PVOID
(__stdcall *PFUNCTION_TABLE_ACCESS_ROUTINE64)(
    _In_ HANDLE ahProcess,
    _In_ DWORD64 AddrBase
    );
typedef
DWORD64
(__stdcall *PGET_MODULE_BASE_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 Address
    );
typedef
DWORD64
(__stdcall *PTRANSLATE_ADDRESS_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ HANDLE hThread,
    _In_ LPADDRESS64 lpaddr
    );
typedef
BOOL
(__stdcall *PSTACK_WALK_ROUTINE64)(
    _In_ DWORD MachineType,
    _In_ HANDLE hProcess,
    _In_ HANDLE hThread,
    _Inout_ LPSTACKFRAME64 StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    );
typedef
BOOL
(__stdcall *PSYM_INITIALIZE_ROUTINE)(
    _In_ HANDLE hProcess,
    _In_opt_ PCSTR UserSearchPath,
    _In_ BOOL fInvadeProcess
    );
typedef
BOOL
(__stdcall *PSYM_CLEANUP_ROUTINE)(
    _In_ HANDLE hProcess
    );
typedef
BOOL
(__stdcall *PSYM_GET_SYM_FROM_NAME_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ PCSTR Name,
    _Inout_ PIMAGEHLP_SYMBOL64 Symbol
    );
typedef
BOOL
(__stdcall *PSYM_FROM_ADDR_ROUTINE)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 Address,
    _Out_opt_ PDWORD64 Displacement,
    _Inout_ PSYMBOL_INFO Symbol
    );
typedef
BOOL
(__stdcall *PSYM_GET_SYM_FROM_ADDR_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 qwAddr,
    _Out_opt_ PDWORD64 pdwDisplacement,
    _Inout_ PIMAGEHLP_SYMBOL64  Symbol
    );
typedef
DWORD
(__stdcall *PSYM_SET_OPTIONS_ROUTINE)(
    _In_ DWORD   SymOptions
    );
typedef BOOL
(__stdcall *PSYM_GET_LINE_FROM_ADDR_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 qwAddr,
    _Out_ PDWORD pdwDisplacement,
    _Out_ PIMAGEHLP_LINE64 Line64
    );


namespace lf
{
    PFUNCTION_TABLE_ACCESS_ROUTINE64 gSymFunctionTableAccess64 = nullptr;
    PGET_MODULE_BASE_ROUTINE64 gSymGetModuleBase = nullptr;
    PSTACK_WALK_ROUTINE64 gStackWalk64 = nullptr;
    PSYM_INITIALIZE_ROUTINE gSymInitialize = nullptr;
    PSYM_CLEANUP_ROUTINE gSymCleanup = nullptr;
    PSYM_GET_SYM_FROM_NAME_ROUTINE64 gSymGetSymFromName64 = nullptr;
    PSYM_GET_SYM_FROM_ADDR_ROUTINE64 gSymGetSymFromAddr64 = nullptr;
    PSYM_GET_LINE_FROM_ADDR_ROUTINE64 gSymGetLineFromAddr64 = nullptr;
    PSYM_FROM_ADDR_ROUTINE gSymFromAddr = nullptr;
    PSYM_SET_OPTIONS_ROUTINE gSymSetOptions = nullptr;

    HMODULE gDbgHelp = nullptr;
    CRITICAL_SECTION gCriticalSection;

    bool InitStackTrace()
    {
        if (gDbgHelp)
        {
            return true;
        }

        gDbgHelp = LoadLibrary(TEXT("dbghelp.dll"));
        if (gDbgHelp == NULL)
        {
            return false;
        }

        gSymInitialize = (PSYM_INITIALIZE_ROUTINE)GetProcAddress(gDbgHelp, "SymInitialize");
        gSymCleanup = (PSYM_CLEANUP_ROUTINE)GetProcAddress(gDbgHelp, "SymCleanup");
        gStackWalk64 = (PSTACK_WALK_ROUTINE64)GetProcAddress(gDbgHelp, "StackWalk64");
        gSymFunctionTableAccess64 = (PFUNCTION_TABLE_ACCESS_ROUTINE64)GetProcAddress(gDbgHelp, "SymFunctionTableAccess64");
        gSymGetModuleBase = (PGET_MODULE_BASE_ROUTINE64)GetProcAddress(gDbgHelp, "SymGetModuleBase");
        gSymGetSymFromName64 = (PSYM_GET_SYM_FROM_NAME_ROUTINE64)GetProcAddress(gDbgHelp, "SymGetSymFromName64");
        gSymGetSymFromAddr64 = (PSYM_GET_SYM_FROM_ADDR_ROUTINE64)GetProcAddress(gDbgHelp, "SymGetSymFromAddr64");
        gSymGetLineFromAddr64 = (PSYM_GET_LINE_FROM_ADDR_ROUTINE64)GetProcAddress(gDbgHelp, "SymGetLineFromAddr64");
        gSymFromAddr = (PSYM_FROM_ADDR_ROUTINE)GetProcAddress(gDbgHelp, "SymFromAddr");
        gSymSetOptions = (PSYM_SET_OPTIONS_ROUTINE)GetProcAddress(gDbgHelp, "SymSetOptions");

        if (gSymInitialize == NULL,
            gSymCleanup == NULL,
            gStackWalk64 == NULL,
            gSymFunctionTableAccess64 == NULL,
            gSymGetModuleBase == NULL,
            gSymGetSymFromName64 == NULL,
            gSymFromAddr == NULL,
            gSymGetSymFromAddr64 == NULL,
            gSymGetLineFromAddr64 == NULL,
            gSymSetOptions == NULL
            )
        {
            FreeLibrary(gDbgHelp);
            gSymInitialize = NULL;
            gSymCleanup = NULL;
            gStackWalk64 = NULL;
            gSymFunctionTableAccess64 = NULL;
            gSymGetModuleBase = NULL;
            gSymGetSymFromName64 = NULL;
            gSymFromAddr = NULL;
            gSymGetSymFromAddr64 = NULL;
            gSymGetLineFromAddr64 = NULL;
            gSymSetOptions = NULL;
            gDbgHelp = NULL;
            return false;
        }

        gSymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        if (gSymInitialize(GetCurrentProcess(), NULL, TRUE) == FALSE)
        {
            return false;
        }
        InitializeCriticalSection(&gCriticalSection);
        return true;
    }

    void TerminateStackTrace()
    {
        if (gDbgHelp == NULL)
        {
            return;
        }

        DeleteCriticalSection(&gCriticalSection);
        gSymCleanup(GetCurrentProcess());
        FreeLibrary(gDbgHelp);
        gSymInitialize = NULL;
        gSymCleanup = NULL;
        gStackWalk64 = NULL;
        gSymFunctionTableAccess64 = NULL;
        gSymGetModuleBase = NULL;
        gSymGetSymFromName64 = NULL;
        gSymFromAddr = NULL;
        gSymGetSymFromAddr64 = NULL;
        gSymGetLineFromAddr64 = NULL;
        gSymSetOptions = NULL;
        gDbgHelp = NULL;
    }

    void CaptureStackTrace(StackTrace& trace, size_t maxFrames)
    {
        if (gDbgHelp == NULL)
        {
            return;
        }
        DWORD machineType;
        CONTEXT c;
        PVOID cr;
        STACKFRAME64 sf;

        // GetContext:
#ifdef _M_IX86 
        // For 32-bit use asm hack to get registers for context.
        ZeroMemory(&c, sizeof(CONTEXT));
        c.ContextFlags = CONTEXT_CONTROL;
        __asm
        {
        Label:
            mov[c.Ebp], ebp;
            mov[c.Esp], esp;
            mov eax, [Label];
            mov[c.Eip], eax;
        }
#else 
        // For 64-bit we can use the handy RtlCaptureContext
        RtlCaptureContext(&c);
#endif
        // GetStackFrame:
#ifdef _M_IX86 
        machineType = IMAGE_FILE_MACHINE_I386;
        sf.AddrPC.Offset = c.Eip;
        sf.AddrPC.Mode = AddrModeFlat;
        sf.AddrFrame.Offset = c.Ebp;
        sf.AddrFrame.Mode = AddrModeFlat;
        sf.AddrStack.Offset = c.Esp;
        sf.AddrStack.Mode = AddrModeFlat;
        cr = NULL;
#elif _M_X64
        machineType = IMAGE_FILE_MACHINE_AMD64;
        sf.AddrPC.Offset = c.Rip;
        sf.AddrPC.Mode = AddrModeFlat;
        sf.AddrFrame.Offset = c.Rsp;;
        sf.AddrFrame.Mode = AddrModeFlat;
        sf.AddrStack.Offset = c.Rsp;
        sf.AddrStack.Mode = AddrModeFlat;
        cr = &c;
#else 
        COMPILE_CRASH("Unsupported platform.");
#endif

        // WalkTheStack:
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();
        trace.frameCount = 0;
        trace.frames = static_cast<StackFrame*>(LFAlloc(sizeof(StackFrame) * maxFrames, alignof(StackFrame)));
        EnterCriticalSection(&gCriticalSection);
        while (trace.frameCount < maxFrames)
        {
            if (!gStackWalk64(machineType, process, thread, &sf, cr, NULL, NULL, NULL, NULL))
            {
                break;
            }
            if (sf.AddrPC.Offset != 0)
            {
                StackFrame& outFrame = trace.frames[trace.frameCount++];
                outFrame.filename = nullptr;
                outFrame.function = nullptr;
                outFrame.line = INVALID;

                DWORD64 symOffset;
                char symBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME + 1];
                SYMBOL_INFO* symInfo = (SYMBOL_INFO*)symBuffer;
                symInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
                symInfo->MaxNameLen = MAX_SYM_NAME;
                BOOL symResult = gSymFromAddr(process, sf.AddrPC.Offset, &symOffset, symInfo);

                DWORD lineOffset;
                char lineBuffer[sizeof(IMAGEHLP_LINE64) + MAX_SYM_NAME + 1];
                IMAGEHLP_LINE64* lineInfo = (IMAGEHLP_LINE64*)lineBuffer;
                lineInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
                BOOL lineResult = gSymGetLineFromAddr64(process, sf.AddrPC.Offset, &lineOffset, lineInfo);

                if (symResult == TRUE && symInfo->NameLen > 0)
                {
                    outFrame.function = static_cast<const char*>(LFAlloc(1 + sizeof(char) * symInfo->NameLen, 1));
                    memcpy(const_cast<char*>(outFrame.function), symInfo->Name, symInfo->NameLen);
                    const_cast<char*>(outFrame.function)[symInfo->NameLen] = '\0';
                }

                if (lineResult == TRUE)
                {
                    outFrame.line = lineInfo->LineNumber;

                    size_t filenameLength = strlen(lineInfo->FileName);
                    outFrame.filename = static_cast<const char*>(LFAlloc(1 + sizeof(char) * filenameLength, 1));
                    memcpy(const_cast<char*>(outFrame.filename), lineInfo->FileName, filenameLength);
                    const_cast<char*>(outFrame.filename)[filenameLength] = '\0';
                }
            }
            else
            {
                break;
            }
        }
        LeaveCriticalSection(&gCriticalSection);
    }

    void ReleaseStackTrace(StackTrace& trace)
    {
        for (size_t i = 0; i < trace.frameCount; ++i)
        {
            if (trace.frames[i].filename)
            {
                LFFree(const_cast<char*>(trace.frames[i].filename));
            }
            if (trace.frames[i].function)
            {
                LFFree(const_cast<char*>(trace.frames[i].function));
            }
        }
        if (trace.frames)
        {
            LFFree(trace.frames);
        }

        trace.frameCount = 0;
        trace.frames = nullptr;
    }
}