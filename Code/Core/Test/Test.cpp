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
#include "Test.h"
#include "Core/Common/Assert.h"
#include "Core/String/StringUtil.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Debug.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Platform/FileSystem.h"
#include <algorithm>

namespace lf {


static UInt32 sFailed = 0;
static UInt32 sExecuted = 0;
static Float64 sExecutionTime = 0.0;
TestRegristration* sTestHead = nullptr;
TestContext* sTestContextStack = nullptr;



static String GetTempDirectory(const EngineConfig* config)
{
    if (!config)
    {
        return String();
    }

    String tempDir = FileSystem::PathResolve(FileSystem::PathJoin(config->GetTempDirectory(), "TestOutput"));
    if (!FileSystem::PathExists(tempDir) && !FileSystem::PathCreate(tempDir))
    {
        return String();
    }
    return tempDir;
}

static bool FilterTestByGroup(TestRegristration* test, const TestConfig& config)
{
    if (!config.mStressEnabled && (test->mFlags & TestFlags::TF_STRESS))
    {
        return false;
    }

    if (!config.mBenchmarkEnabled && (test->mFlags & TestFlags::TF_BENCHMARK))
    {
        return false;
    }

    if (!config.mSetupEnabled && (test->mFlags & TestFlags::TF_SETUP))
    {
        return false;
    }

    const String testGroup(test->mGroup, COPY_ON_WRITE);
    const String testName(test->mName, COPY_ON_WRITE);

    if (!config.mGroupTargets.empty())
    {
        if (std::find(config.mGroupTargets.begin(), config.mGroupTargets.end(), testGroup) == config.mGroupTargets.end())
        {
            return false;
        }
    }

    if (!config.mTestTargets.empty())
    {
        if (std::find(config.mTestTargets.begin(), config.mTestTargets.end(), testName) == config.mTestTargets.end())
        {
            return false;
        }
    }

    if (std::find(config.mIgnoredGroups.begin(), config.mIgnoredGroups.end(), testGroup) != config.mIgnoredGroups.end())
    {
        return false;
    }

    if (std::find(config.mIgnoredTests.begin(), config.mIgnoredTests.end(), testName) != config.mIgnoredTests.end())
    {
        return false;
    }

    return true;
}

static TVector<TestRegristration*> QueryTestsByGroup(const TestConfig& config)
{
    TVector<TestRegristration*> tests;
    TestRegristration* it = sTestHead;
    while (it)
    {
        if (FilterTestByGroup(it, config))
        {
            tests.push_back(it);
        }
        it = it->mNext;
    }
    return tests;
}

struct TestExecutionResult
{
    TestExecutionResult(bool logOnExit = false);
    ~TestExecutionResult();

    TestExecutionResult& operator+=(const TestExecutionResult& other)
    {
        mTestCasesFailed += other.mTestCasesFailed;
        mTestCasesExecuted += other.mTestCasesExecuted;
        mTestsExecuted += other.mTestsExecuted;
        mTestsFailed += other.mTestsFailed;
        return *this;
    }

    SizeT mTestCasesFailed;
    SizeT mTestCasesExecuted;
    SizeT mTestsFailed;
    SizeT mTestsExecuted;
    bool  mLogOnExit;
};

class TestSelector
{
public:
    TestSelector()
    : mTests()
    {}

    TestSelector(TVector<TestRegristration*>&& tests)
    : mTests(std::move(tests))
    {}

    TestSelector(const TestSelector& other) = default;
    TestSelector& operator=(const TestSelector& other) = default;

    TestSelector(TestSelector&& other)
    : mTests(std::move(other.mTests))
    {}

    TestSelector& operator=(TestSelector&& other)
    {
        if (this != &other)
        {
            mTests = std::move(other.mTests);
        }
        return *this;
    }

    TestSelector  Select();
    TestSelector  Select(UInt32 testFlag);
    TestSelector& Mask(UInt32 testFlags);
    TestSelector& SortPriority();
    TestExecutionResult Execute(const TestConfig& config);

    SizeT Size() const { return mTests.size(); }

private:
    using TestArray = TVector<TestRegristration*>;
    using TestIterator = TestArray::iterator;

    TVector<TestRegristration*> mTests;
};

TestExecutionResult::TestExecutionResult(bool logOnExit)
: mTestCasesFailed(0)
, mTestCasesExecuted(0)
, mTestsFailed(0)
, mTestsExecuted(0)
, mLogOnExit(logOnExit)
{

}

TestExecutionResult::~TestExecutionResult()
{
    if (mLogOnExit)
    {
        SizeT passed = (mTestsExecuted - mTestsFailed);
        SizeT rate = static_cast<SizeT>(passed / static_cast<Float64>(mTestsExecuted) * 100.0);

        gTestLog.Info(LogMessage("Test Results"));
        gTestLog.Info(LogMessage("  Tests Executed =") << mTestsExecuted);
        gTestLog.Info(LogMessage("  Tests Passed   =") << passed << "/" << mTestsExecuted << " (" << rate << "%)");
        gTestLog.Info(LogMessage("  Test Cases Executed =") << mTestCasesExecuted);
        gTestLog.Info(LogMessage("  Test Cases Failed = ") << mTestCasesFailed);
        gTestLog.Info(LogMessage("----------------------------------------"));
        gTestLog.Sync();
    }
}

TestSelector TestSelector::Select()
{
    return *this;
}

TestSelector TestSelector::Select(UInt32 testFlag)
{
    TestSelector selector;
    selector.mTests.reserve(mTests.size());

    for (TestIterator it = mTests.begin(); it != mTests.end(); ++it)
    {
        if (((*it)->mFlags & testFlag) > 0)
        {
            selector.mTests.push_back(*it);
        }
    }
    return selector;
}

TestSelector& TestSelector::Mask(UInt32 testFlags)
{
    for (TestIterator it = mTests.begin(); it != mTests.end(); )
    {
        if (((*it)->mFlags & testFlags) > 0)
        {
            it = mTests.swap_erase(it);
        }
        else
        {
            ++it;
        }
    }

    return *this;
}

TestSelector& TestSelector::SortPriority()
{
    std::stable_sort(mTests.begin()._Unwrapped(), mTests.end()._Unwrapped(), [](const TestRegristration* a, const TestRegristration* b) { return a->mPriority < b->mPriority; });
    return *this;
}
TestExecutionResult TestSelector::Execute(const TestConfig& config)
{
    TestExecutionResult result;

    for (TestIterator it = mTests.begin(); it != mTests.end(); ++it)
    {
        // todo: We probably won't need the test context?
        //       
        TestContext context;
        context.mTriggerBreakpoint = true; // todo:
        context.mPrev = sTestContextStack;
        context.mEngineConfig = config.mEngineConfig;
        sTestContextStack = &context;

        TestRegristration* test = *it;

        gTestLog.Info(LogMessage("Running test ") << test->mGroup << ":" << test->mName << "...");
        gTestLog.Sync();

        sFailed = 0;
        sExecuted = 0;
        sExecutionTime = 0.0;
        BugCallback bugReporter = gReportBugCallback;
        Int64 clockFreq = GetClockFrequency();
        Int64 clockBegin = GetClockTime();
        test->mCallback();
        Int64 clockEnd = GetClockTime();
        sExecutionTime = (clockEnd - clockBegin) / static_cast<Float64>(clockFreq);
        gReportBugCallback = bugReporter;
        ++result.mTestsExecuted;
        result.mTestCasesExecuted += sExecuted;
        result.mTestCasesFailed += sFailed;

        gSysLog.Sync();
        gIOLog.Sync();
        gGfxLog.Sync();
        gNetLog.Sync();

        if (sFailed > 0)
        {
            gTestLog.Error(LogMessage("Test ") << test->mGroup << ":" << test->mName << " failed! Failures=" << sFailed);
            ++result.mTestsFailed;
        }
        else
        {
            gTestLog.Info(LogMessage("Test ") << test->mGroup << ":" << test->mName << " passed! Execution Time=" << FormatTime(sExecutionTime) << FormatTimeStr(sExecutionTime));
        }
        gTestLog.Sync();

        sTestContextStack = sTestContextStack->mPrev;
    }
    return result;
}

TestConfig::TestConfig() 
: mTriggerBreakpoint(true)
, mEngineConfig(nullptr)
{}

TestRegristration* FindTest(TestRegristration* root, const Char8* string)
{
    const Char8* endString = string + StrLen(string);
    while (root->mNext)
    {
        if (StrEqual(root->mName, string, root->mName + StrLen(root->mName), endString))
        {
            return root;
        }
        root = root->mNext;
    }

    if (root && StrEqual(root->mName, string, root->mName + StrLen(root->mName), endString))
    {
        return root;
    }

    return nullptr;
}

TestRegristration* GetTestEnd(TestRegristration* root)
{
    while (root->mNext)
    {
        root = root->mNext;
    }
    return root;
}
void TestFramework::RegisterTest(TestRegristration* test)
{
    if (sTestHead == nullptr)
    {
        sTestHead = test;
    }
    else
    {
        TestRegristration* end = GetTestEnd(sTestHead);
        end->mNext = test;
    }
}

void TestFramework::ExecuteAll(const TestConfig& config)
{
    if (config.mEngineConfig == nullptr)
    {
        gTestLog.Error(LogMessage("Invalid test config. EngineConfig is required!"));
        return;
    }

    TestSelector tests(QueryTestsByGroup(config));
    TestExecutionResult results(true);
    gTestLog.Info(LogMessage("Running tests with config..."));
    gTestLog.Info(LogMessage("  SetupEnabled=") << config.mSetupEnabled);
    gTestLog.Info(LogMessage("  StressEnabled=") << config.mStressEnabled);
    gTestLog.Info(LogMessage("  BenchmarkEnabled=") << config.mBenchmarkEnabled);
    gTestLog.Info(LogMessage("  ParallelExecution=") << config.mParallelExecution);
    gTestLog.Info(LogMessage("  TestOutput=") << ::lf::GetTempDirectory(config.mEngineConfig));
    gTestLog.Info(LogMessage("  -------------------------------------"));
    gTestLog.Info(LogMessage("Selected ") << tests.Size() << " tests.");

    if (config.mClean)
    {
        String dir = ::lf::GetTempDirectory(config.mEngineConfig);
        if (!FileSystem::PathDeleteRecursive(dir))
        {
            gTestLog.Error(LogMessage("Failed to clean test output directory"));
            return;
        }
    }

    if (::lf::GetTempDirectory(config.mEngineConfig).Empty())
    {
        gTestLog.Error(LogMessage("Unable to create TestOutput directory!"));
        return;
    }

    // Execute the setup:
    if (config.mSetupEnabled)
    {
        gTestLog.Info(LogMessage("Executing setup tests..."));
        gTestLog.Sync();
        auto setup = tests.Select(TestFlags::TF_SETUP)
                          .SortPriority()
                          .Execute(config);
        results += setup;
        if (setup.mTestsFailed > 0)
        {
            gTestLog.Error(LogMessage("A setup test failed! Ignoring further testing."));
            return;
        }

        if (config.mSetupExclusive)
        {
            return;
        }
    }
    
    // Execute basic tests
    if (!config.mSetupExclusive && !config.mBenchmarkExclusive && !config.mStressExclusive)
    {

        UInt32 flags = TestFlags::TF_SETUP | TestFlags::TF_DISABLED;
        if (config.mStressEnabled) { flags |= TestFlags::TF_STRESS; }
        if (config.mBenchmarkEnabled) { flags |= TestFlags::TF_BENCHMARK; }

        gTestLog.Info(LogMessage("Executing basic tests..."));
        gTestLog.Sync();
        auto basicTests = tests.Select()
                               .Mask(flags)
                               .SortPriority()
                               .Execute(config);
        results += basicTests;
        if (basicTests.mTestsFailed > 0)
        {
            gTestLog.Error(LogMessage("A basic test has failed! Ignoring further testing."));
            return;
        }
    }

    if (config.mStressEnabled)
    {
        gTestLog.Info(LogMessage("Executing stress tests..."));
        gTestLog.Sync();
        auto stress = tests.Select(TestFlags::TF_STRESS)
            .SortPriority()
            .Execute(config);

        results += stress;
        if (stress.mTestsFailed > 0)
        {
            gTestLog.Error(LogMessage("A stress test has failed! Ignoring further testing."));
            return;
        }

        if (config.mStressExclusive)
        {
            return;
        }
    }

    if (config.mBenchmarkEnabled)
    {
        gTestLog.Info(LogMessage("Executing benchmark tests..."));
        gTestLog.Sync();
        auto benchmark = tests.Select(TestFlags::TF_BENCHMARK)
                              .SortPriority()
                              .Execute(config);
        results += benchmark;
        if (benchmark.mTestsFailed)
        {
            gTestLog.Error(LogMessage("A benchmark test has failed! Ignoring further testing."));
            return;
        }

        if (config.mBenchmarkExclusive) 
        {
            return;
        }
    }
}

void TestFramework::ListGroups()
{
    gTestLog.Info(LogMessage("Listing test groups..."));

    TVector<const char*> listed;
    TestRegristration* it = sTestHead;
    while (it)
    {
        if (std::find(listed.begin(), listed.end(), it->mGroup) == listed.end())
        {
            listed.push_back(it->mGroup);
        }
        it = it->mNext;
    }

    std::sort(listed.begin(), listed.end(), [](const char* a, const char* b) { return StrAlphaLess(a, b); });
    for (const char* group : listed)
    {
        gTestLog.Info(LogMessage("  ") << group);
    }


    gTestLog.Info(LogMessage("------------------"));
}

bool TestFramework::ProcessTest(bool result, const char* expression, int line)
{
    (expression);
    (line);

    ++sExecuted;
    if (result)
    {
        return result;
    }
    gTestLog.Error(LogMessage("Failed test: \"") << expression << "\" at line " << line);
    gTestLog.Sync();
    ++sFailed;
    return result;
}

bool TestFramework::HasFailed()
{
    return sFailed > 0;
}

void TestFramework::TestReset()
{
    sFailed = 0;
    sExecuted = 0;
    sExecutionTime = 0.0;
}

bool TestFramework::TriggerBreakPoint()
{
    if (!HasDebugger())
    {
        return false;
    }
    return sTestContextStack ? sTestContextStack->mTriggerBreakpoint : true;
}
bool TestFramework::TestAll()
{
    return CmdLine::HasArgOption("test", "all");
}

TestConfig TestFramework::GetConfig()
{
    TestConfig config;
    config.mTriggerBreakpoint = TriggerBreakPoint();
    config.mEngineConfig = sTestContextStack ? sTestContextStack->mEngineConfig : nullptr;
    return config;
}

String TestFramework::GetTempDirectory()
{
    if (!sTestContextStack || !sTestContextStack->mEngineConfig)
    {
        return String();
    }
    return ::lf::GetTempDirectory(sTestContextStack->mEngineConfig);
}

} // namespace lf