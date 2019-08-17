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
#include "Program.h"
#include "Core/Common/Enum.h"
#include "Core/Common/Assert.h"
#include "Core/String/TokenTable.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Platform/Thread.h"
#include "Core/Utility/Utility.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/StaticCallback.h"
#include "Core/Utility/StackTrace.h"
#include "Core/Utility/Time.h"

#include "Core/Memory/SmartPointer.h"
#include "Core/Reflection/Object.h"
#include "Runtime/Async/AsyncImpl.h"
#include "Runtime/Common/RuntimeGlobals.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "Runtime/Reflection/ReflectionMgr.h"

#include "Engine/App/Application.h"

#include <Windows.h>

namespace lf {

void WaitDebugger()
{
    Int64 begin = GetClockTime();
    while (!IsDebuggerPresent())
    {
        SleepCallingThread(10);

        Float64 time = (GetClockTime() - begin) / static_cast<Float64>(GetClockFrequency());
        if (time > 60.0)
        {
            break;
        }
    }
}

class Sample : public Object
{
    DECLARE_CLASS(Sample, Object);
public:


};
// 
class AbstractSample : public Object
{
    DECLARE_CLASS(AbstractSample, Object);
};
// 
DEFINE_CLASS(Sample)
{
    NO_REFLECTION;
}
// 
DEFINE_ABSTRACT_CLASS(AbstractSample)
{
    NO_REFLECTION;
}

// Tags to simplifiy the process of initializing the static callbacks
enum SafeStaticActions
{
    INIT_SCP_PRE_INIT_CORE = SCP_PRE_INIT_RUNTIME - 1,
    INIT_SCP_PRE_INIT_RUNTIME = SCP_PRE_INIT_SERVICE - 1,
    INIT_SCP_PRE_INIT_SERVICE = SCP_INIT_CORE - 1,
    
    INIT_SCP_INIT_CORE = SCP_INIT_RUNTIME - 1,
    INIT_SCP_INIT_RUNTIME = SCP_INIT_SERVICE - 1,
    INIT_SCP_INIT_SERVICE = SCP_INIT_ENGINE - 1,
    INIT_SCP_INIT_ENGINE = SCP_POST_INIT - 1,

    INIT_SCP_POST_INIT = 999999,


    DESTROY_SCP_PRE_INIT_CORE = 0,
    DESTROY_SCP_PRE_INIT_RUNTIME = SCP_PRE_INIT_RUNTIME - 1,
    DESTROY_SCP_PRE_INIT_SERVICE = SCP_PRE_INIT_SERVICE - 1,

    DESTROY_SCP_INIT_CORE = SCP_INIT_CORE - 1,
    DESTROY_SCP_INIT_RUNTIME = SCP_INIT_RUNTIME - 1,
    DESTROY_SCP_INIT_SERVICE = SCP_INIT_SERVICE - 1,
    DESTROY_SCP_INIT_ENGINE = SCP_INIT_ENGINE - 1,

    DESTROY_SCP_POST_INIT = SCP_POST_INIT - 1,
};

const SizeT DebugOffset = 250;
STATIC_INIT(DebugPreInitCore, SCP_PRE_INIT_CORE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_INIT(DebugPreInitRuntime, SCP_PRE_INIT_RUNTIME + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_INIT(DebugPreInitService, SCP_PRE_INIT_SERVICE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_INIT(DebugInitCore, SCP_INIT_CORE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_INIT(DebugInitRuntime, SCP_INIT_RUNTIME + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_INIT(DebugInitService, SCP_INIT_SERVICE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_INIT(DebugInitEngine, SCP_INIT_ENGINE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_INIT(DebugPostInit, SCP_POST_INIT + DebugOffset)
{
    LF_DEBUG_BREAK;
}

STATIC_DESTROY(DebugDPreInitCore, SCP_PRE_INIT_CORE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_DESTROY(DebugDPreInitRuntime, SCP_PRE_INIT_RUNTIME + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_DESTROY(DebugDPreInitService, SCP_PRE_INIT_SERVICE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_DESTROY(DebugDInitCore, SCP_INIT_CORE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_DESTROY(DebugDInitRuntime, SCP_INIT_RUNTIME + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_DESTROY(DebugDInitService, SCP_INIT_SERVICE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_DESTROY(DebugDInitEngine, SCP_INIT_ENGINE + DebugOffset)
{
    LF_DEBUG_BREAK;
}
STATIC_DESTROY(DebugDPostInit, SCP_POST_INIT + DebugOffset)
{
    LF_DEBUG_BREAK;
}

// Config:
const bool VERBOSE_START_UP = true;

// Globals:
TokenTable    gTokenTableInstance;
AsyncImpl*    gAsyncInstance = nullptr;
bool          gProgramRunning = true;

// LogControl:
Log* gLogGroup[] =
{ 
    &gMasterLog,
    &gSysLog,
    &gIOLog,
    &gTestLog,
    &gGfxLog
};
static void SyncLogs()
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(gLogGroup); ++i)
    {
        gLogGroup[i]->Sync();
    }
}
static void CloseLogs()
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(gLogGroup); ++i)
    {
        gLogGroup[i]->Close();
    }
}
static void UpdateLogs(void*)
{
    while (gProgramRunning)
    {
        SleepCallingThread(100);
        SyncLogs();
    }
}

void InitializeCore(const String& cmdLine);
void TerminateCore();

void InitializeRuntime();
void TerminateRuntime();

void Program::Execute(SizeT argc, const char** argv)
{
    lf::SizeT bytesAtStartup = lf::LFGetBytesAllocated();

    // Merge CommandLine: Ignoring the filepath passed into first arg. (I don't think this happens with regular .exe)
    String cmdString;
    for (SizeT i = 1; i < argc; ++i)
    {
        cmdString.Append(argv[i]).Append(' ');
    }
    
    InitializeCore(cmdString);
    cmdString.Clear();
    InitializeRuntime();

    EngineConfig config;
    String configPath = "Engine.config";
    CmdLine::GetArgOption("app", "config", configPath);
    config.Open(configPath);

    for (SizeT i = 0; i < LF_ARRAY_SIZE(gLogGroup); ++i)
    {
        gLogGroup[i]->SetConfig(&config);
    }

    // system, io, math, util
    Thread logUpdater;
    logUpdater.Fork(UpdateLogs, nullptr);
    ExecuteStaticInit(INIT_SCP_POST_INIT, nullptr);
    StaticInitFence();
    gSysLog.Debug(LogMessage("Program::Initialize Complete"));
    gSysLog.Debug(LogMessage("  MainThread=") << GetPlatformThreadId());
    gSysLog.Info(LogMessage("Command Line=") << CmdLine::GetCmdString());
    SyncLogs();

    // todo: What sort of application should we run? 
    // app.Run();
    String appTypeName;
    if (CmdLine::GetArgOption("app", "type", appTypeName))
    {
        const Type* appType = GetReflectionMgr().FindType(Token(StrStripWhitespace(appTypeName)));
        if (appType)
        {
            auto app = GetReflectionMgr().Create<Application>(appType);
            app->SetConfig(&config);
            if (app)
            {
                app->OnStart();
                app->OnExit();
            }
        }
        else
        {
            gSysLog.Error(LogMessage("Failed to find an app with the name ") << appTypeName);
        }
    }
    
    gProgramRunning = false;
    logUpdater.Join();
    ExecuteStaticDestroy(DESTROY_SCP_PRE_INIT_SERVICE, nullptr);
    TerminateRuntime();
    TerminateCore();

    StaticDestroyFence();
    gSysLog.Debug(LogMessage("Program::Terminate Complete"));
    SyncLogs();
    CloseLogs();
    config.Close();

    lf::SizeT bytesAtShutdown = lf::LFGetBytesAllocated();
    Assert(bytesAtStartup == bytesAtShutdown);
}

void GenerateReportCommon(const StackTrace& stackTrace)
{
    if ((gAssertFlags & ERROR_FLAG_LOG_THREAD) > 0)
    {
        gSysLog.Info(LogMessage("  Current Thread = [") << GetThreadName() << "] " << GetPlatformThreadId());
    }

    if ((gAssertFlags & ERROR_FLAG_LOG_CALLSTACK) > 0)
    {
        for (SizeT i = 0; i < stackTrace.frameCount; ++i)
        {
            if (stackTrace.frames[i].filename)
            {
                gSysLog.Info(LogMessage("  ") << stackTrace.frames[i].filename << ":" << stackTrace.frames[i].line << "  " << stackTrace.frames[i].function);
            }
            else
            {
                gSysLog.Info(LogMessage("  [Unknown]  ") << stackTrace.frames[i].function);
            }
        }
    }
}

void HandleAssert(const char* msg, const StackTrace& stackTrace, UInt32 code, UInt32 api)
{
    if ((gAssertFlags & ERROR_FLAG_LOG) > 0)
    {
        gSysLog.Error(LogMessage("Assertion failed (") << msg << ") Code=" << code << ", API=" << api);
        GenerateReportCommon(stackTrace);
        gSysLog.Sync();
    }
    WaitDebugger();
    
}
void HandleCrash(const char* msg, const StackTrace& stackTrace, UInt32 code, UInt32 api)
{
    if ((gAssertFlags & ERROR_FLAG_LOG) > 0)
    {
        gSysLog.Error(LogMessage("Critical error detected! Crash(") << msg << ") Code=" << code << ", API=" << api);
        GenerateReportCommon(stackTrace);
        gSysLog.Sync();
    }
    WaitDebugger();

}
void HandleBug(const char* msg, const StackTrace& stackTrace, UInt32 code, UInt32 api)
{
    if ((gAssertFlags & ERROR_FLAG_LOG) > 0)
    {
        gSysLog.Error(LogMessage("Reporting Bug (") << msg << ") Code=" << code << ", API=" << api);
        GenerateReportCommon(stackTrace);
        gSysLog.Sync();
    }
    WaitDebugger();

}

void InitializeCore(const String& cmdLine)
{
    // Set thread-local variable to signal this as the main thread.
    SetMainThread();
    // Setup error handlers.
    gAssertCallback     = HandleAssert;
    gCriticalAssertCallback = HandleCrash;
    gReportBugCallback  = HandleBug;
    // todo: StackTrace.Init
    gSysLog.Debug(LogMessage("InitializeCore -- Default assert handlers assigned."));
    
    // Initialize CommandLine:
    CmdLine::ParseCmdLine(cmdLine);
    gSysLog.Debug(LogMessage("InitializeCore -- CmdLine::Initialize \"") << cmdLine << "\"");
    if (CmdLine::HasArg("waitdbg"))
    {
        WaitDebugger();
    }

    // Setup token table ( a critical part of the Core library )
    gTokenTable = &gTokenTableInstance;
    gTokenTable->Initialize();
    gSysLog.Debug(LogMessage("InitializeCore -- TokenTable assigned."));

    if (InitStackTrace())
    {
        gSysLog.Debug(LogMessage("InitializeCore -- StackTrace Initialized."));
    }

    // Invoke any static initialization functions.
    ExecuteStaticInit(INIT_SCP_PRE_INIT_CORE, nullptr);
    gSysLog.Debug(LogMessage("InitializeCore -- Complete"));
}

void TerminateCore()
{
    CmdLine::Release();
    ExecuteStaticDestroy(DESTROY_SCP_PRE_INIT_CORE, nullptr);

    TerminateStackTrace();

    gTokenTableInstance.Shutdown();
    GetEnumRegistry().Clear();
    gSysLog.Debug(LogMessage("TerminateCore -- Complete"));
}

void InitializeRuntime()
{
    gAsyncInstance = LFNew<AsyncImpl>();
    gAsync = gAsyncInstance;
    gAsyncInstance->Initialize();
    gSysLog.Debug(LogMessage("Async::Initialized"));

    GetReflectionMgr().BuildTypes();
    gSysLog.Debug(LogMessage("ReflectionMgr::BuildTypes"));
    ExecuteStaticInit(INIT_SCP_PRE_INIT_RUNTIME, nullptr);

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION procInfo[64];
    DWORD length = sizeof(procInfo);
    if (GetLogicalProcessorInformation(procInfo, &length) == TRUE)
    {
        SizeT numInfo = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        SizeT numPhysicalCores = 0;
        SizeT numLogicalCores = 0;

        for (SizeT i = 0; i < numInfo; ++i)
        {
            auto info = procInfo[i];
            switch (info.Relationship)
            {
                case LOGICAL_PROCESSOR_RELATIONSHIP::RelationCache:
                {
                    gSysLog.Debug(LogMessage("ProcessorInfo[") << i << "]: Relationship=Cache");
                    
                    gSysLog.Debug(LogMessage("  Cache Level     =") << info.Cache.Level);
                    gSysLog.Debug(LogMessage("  Cache Line Size =") << info.Cache.LineSize);
                    gSysLog.Debug(LogMessage("  Cache Size      =") << static_cast<SizeT>(info.Cache.Size));
                    gSysLog.Debug(LogMessage("  Cache Type      =") << info.Cache.Type);
                    gSysLog.Debug(LogMessage("  Cache Level     =") << info.Cache.Level);
                    break;
                }
                case LOGICAL_PROCESSOR_RELATIONSHIP::RelationProcessorCore:
                {
                    gSysLog.Debug(LogMessage("ProcessorInfo[") << i << "]: Relationship=ProcessorCore");
                    ++numPhysicalCores;
                    if (info.ProcessorCore.Flags)
                    {
                        numLogicalCores += BitCount(static_cast<SizeT>(info.ProcessorMask));
                    }
                    break;
                }
                case LOGICAL_PROCESSOR_RELATIONSHIP::RelationNumaNode:
                {
                    gSysLog.Debug(LogMessage("ProcessorInfo[") << i << "]: Relationship=NumaNode");
                    break;
                }
                case LOGICAL_PROCESSOR_RELATIONSHIP::RelationProcessorPackage:
                {
                    gSysLog.Debug(LogMessage("ProcessorInfo[") << i << "]: Relationship=ProcessorPackage");
                    break;
                }
                default:
                {
                    gSysLog.Debug(LogMessage("Unknown Core Relationship ") << info.Relationship);
                    break;
                }
            }
        }

        gSysLog.Debug(LogMessage("Processor Info:"));
        gSysLog.Debug(LogMessage(" Physical Cores   =") << numPhysicalCores);
        gSysLog.Debug(LogMessage(" Logical Cores    =") << numLogicalCores);

    }

    gSysLog.Debug(LogMessage("InitializeRuntime -- Complete"));

    // todo: CmdLine.Init         @see ProgramEnvironment.CommandLine
    // todo: Config.Init          @see ProgramEnvironment.Config
    // todo: LogControl.Init      @see ProgramEnvironment.LoggerThread
    //
    // todo: Reflection.Init      @see Service.ReflectionMgr
    // todo: ASync.Init           @see Service.Async
    // todo: TaskMgr.Init         @see Service.TaskMgr
    // todo: Profiler.Init        @see Service.Profiler
}

void TerminateRuntime()
{
    ExecuteStaticDestroy(DESTROY_SCP_PRE_INIT_RUNTIME, nullptr);
    GetReflectionMgr().ReleaseTypes();

    gAsyncInstance->Shutdown();
    LFDelete(gAsyncInstance);
    gAsync = nullptr;
    gSysLog.Debug(LogMessage("TerminateRuntime -- Complete"));
}



}