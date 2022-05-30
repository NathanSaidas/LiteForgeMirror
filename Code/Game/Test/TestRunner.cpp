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
#include "Core/Test/Test.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"
#include "Engine/App/Application.h"
#include "Engine/App/Program.h"
#include "Engine/App/GameApp.h"
#include "AbstractEngine/App/AppService.h"
#include "Engine/DX11/DX11GfxDevice.h"

namespace lf {

REGISTER_TEST(ExampleSetup, "Core.Tests", TestFlags::TF_SETUP)
{
    gTestLog.Info(LogMessage("TempDir=") << TestFramework::GetTempDirectory());


}

class TestRunnerService : public Service
{
    DECLARE_CLASS(TestRunnerService, Service);
public:
    static void Get(const char* arg, bool& optionValue)
    {
        if (CmdLine::HasArgOption("test", arg))
        {
            optionValue = true;
        }
    }

    static void GetExclusive(const char* arg, bool& optionValue)
    {
        String exclusive;
        if (CmdLine::GetArgOption("test", arg, exclusive) && StrToLower(exclusive) == "exclusive")
        {
            optionValue = true;
        }
    }

    APIResult<ServiceResult::Value> OnFrameUpdate() override
    {
        AppService* appService = GetServices()->GetService<AppService>();
        if (!appService)
        {
            return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_FAILED);
        }
        Run();
        appService->Stop();
        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }

    void Run()
    {
        if (CmdLine::HasArgOption("test", "list"))
        {
            TestFramework::ListGroups();
            return;
        }


        TestConfig config;
        config.mEngineConfig = GetConfig();
        config.mSetupEnabled = false;
        config.mStressEnabled = false;
        config.mBenchmarkEnabled = false;
        config.mParallelExecution = false;
        config.mTriggerBreakpoint = true;
        config.mStressExclusive = false;
        config.mBenchmarkExclusive = false;
        config.mSetupExclusive = false;
        config.mClean = false;

        String testConfigPath = FileSystem::PathResolve(GetConfig()->GetTestConfig());
        if (FileSystem::FileExists(testConfigPath))
        {
            mTestConfig.Open(testConfigPath);
            config.mEngineConfig = &mTestConfig;
        }

        Get("setup", config.mSetupEnabled);
        Get("stress", config.mStressEnabled);
        Get("benchmark", config.mBenchmarkEnabled);
        Get("parallel", config.mParallelExecution);
        Get("no_debug", config.mTriggerBreakpoint);
        Get("clean", config.mClean);

        GetExclusive("setup", config.mSetupExclusive);
        GetExclusive("benchmark", config.mBenchmarkExclusive);
        GetExclusive("stress", config.mStressExclusive);
        // todo: -test /config=<...>


        // ignored...
        String ignored;
        if (CmdLine::GetArgOption("test", "ignored_tests", ignored))
        {
            StrSplit(ignored, ',', config.mIgnoredTests);
        }
        if (CmdLine::GetArgOption("test", "ignored_groups", ignored))
        {
            StrSplit(ignored, ',', config.mIgnoredGroups);
        }


        // execution type
        bool execute = false;
        String executionStr;
        if (CmdLine::GetArgOption("test", "single", executionStr))
        {
            config.mSetupEnabled = true;
            config.mStressEnabled = true;
            config.mBenchmarkEnabled = true;
            config.mTestTargets.push_back(executionStr);
            execute = true;
        }
        else if (CmdLine::GetArgOption("test", "batch", executionStr))
        {
            config.mSetupEnabled = true;
            config.mStressEnabled = true;
            config.mBenchmarkEnabled = true;
            if (CmdLine::HasArgOption("test", "group"))
            {
                StrSplit(executionStr, ',', config.mGroupTargets);
            }
            else
            {
                StrSplit(executionStr, ',', config.mTestTargets);
            }
            execute = true;
        }
        else if (CmdLine::GetArgOption("test", "group", executionStr))
        {
            StrSplit(executionStr, ',', config.mGroupTargets);
            execute = true;
        }
        else if (CmdLine::HasArgOption("test", "all"))
        {
            execute = true;
        }

        if (execute)
        {
            TestFramework::ExecuteAll(config);
        }
        else
        {
            gTestLog.Warning(LogMessage("Skipping test execution, no execution method found. Consider using /single /batch /group /all"));
        }

        mTestConfig.Close();
    }

    void SetConfig(const EngineConfig* config) { mConfig = config; }
    const EngineConfig* GetConfig() const { return mConfig; }
private:
    EngineConfig mTestConfig;
    const EngineConfig* mConfig;
};
DEFINE_CLASS(lf::TestRunnerService) { NO_REFLECTION; }

class TestRunner : public GameApp
{
DECLARE_CLASS(TestRunner, Application);
public:
    ServiceResult::Value RegisterServices()
    {
        auto appService = MakeService<AppService>();
        if (!GetServices().Register(appService))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        // if (!GetServices().Register(MakeService<DX11GfxDevice>()))
        // {
        //     return ServiceResult::SERVICE_RESULT_FAILED;
        // }
        auto testService = MakeService<TestRunnerService>();
        if (!GetServices().Register(testService))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        testService->SetConfig(GetConfig());

        appService->SetRunning();
        return ServiceResult::SERVICE_RESULT_SUCCESS;
    }
};
DEFINE_CLASS(lf::TestRunner) { NO_REFLECTION; }


}


// int main(int argc, const char** argv)
// {
//     lf::Program::Execute(argc, argv);
// }

#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, char* cmdString, int)
{
    const char* args[] = { "", cmdString };
    lf::Program::Execute(2, args);
    return 0;
}
