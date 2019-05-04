#include "Test.h"
#include "Core/Common/Assert.h"
#include "Core/String/StringUtil.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"

namespace lf {

UInt32 TestFramework::sFailed = 0;
UInt32 TestFramework::sExecuted = 0;
Float64 TestFramework::sExecutionTime = 0;
TestRegristration* sTestHead = nullptr;
TestContext* sTestContextStack = nullptr;

Float64 FormatTime(Float64 time)
{
    
    if (time < 0.001)
    {
        return time * 1000000.0;
    }
    else if (time < 1.0)
    {
        return time * 1000.0;
    }
    return time;
}
const char* FormatTimeStr(Float64 time)
{
    if (time < 0.001)
    {
        return "us";
    }
    else if (time < 1.0)
    {
        return "ms";
    }
    return "s";
}

TestConfig::TestConfig() :
mTriggerBreakpoint(true)
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
void TestFramework::ExecuteTest(const Char8* name, const TestConfig& config)
{
    TestRegristration* test = FindTest(sTestHead, name);
    if (test)
    {
        // Push:
        TestContext context;
        context.mTriggerBreakpoint = config.mTriggerBreakpoint;
        context.mPrev = sTestContextStack;
        sTestContextStack = &context;

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

        if (sFailed > 0)
        {
            gTestLog.Error(LogMessage("Test ") << name << " failed! Failures=" << sFailed);
        }
        else
        {
            gTestLog.Info(LogMessage("Test ") << name << " passed! Execution Time=" << FormatTime(sExecutionTime) << FormatTimeStr(sExecutionTime));
        }

        // Pop:
        sTestContextStack = sTestContextStack->mPrev;

    }
    else
    {
        gTestLog.Error(LogMessage("Test ") << name << " does not exist!");
    }
}
void TestFramework::ExecuteAllTests(const TestConfig& config)
{
    TestRegristration* test = sTestHead;
    while (test)
    {
        TestContext context;
        context.mTriggerBreakpoint = config.mTriggerBreakpoint;
        context.mPrev = sTestContextStack;
        sTestContextStack = &context;

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
        test = test->mNext;

        if (sFailed > 0)
        {
            gTestLog.Error(LogMessage("Test ") << test->mName << " failed! Failures=" << sFailed);
        }
        else
        {
            gTestLog.Info(LogMessage("Test ") << test->mName << " passed! Execution Time=" << FormatTime(sExecutionTime) << FormatTimeStr(sExecutionTime));
        }

        sTestContextStack = sTestContextStack->mPrev;
    }
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
    return sTestContextStack ? sTestContextStack->mTriggerBreakpoint : true;
}

TestConfig TestFramework::GetConfig()
{
    TestConfig config;
    config.mTriggerBreakpoint = TriggerBreakPoint();
    return config;
}

} // namespace lf