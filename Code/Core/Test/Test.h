#ifndef LF_CORE_TEST_H
#define LF_CORE_TEST_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include <intrin.h>

namespace lf {

using TestCallback = void(*)();
struct TestRegristration;

struct TestConfig
{
    TestConfig();

    bool         mTriggerBreakpoint;
};

struct TestContext
{
    bool         mTriggerBreakpoint;
    TestContext* mPrev;
};

class LF_CORE_API TestFramework
{
public:
    static void RegisterTest(TestRegristration* test);
    static void ExecuteTest(const Char8* name, const TestConfig& config);
    static void ExecuteAllTests(const TestConfig& config);

    static bool ProcessTest(bool result, const char* expression, int line);
    static bool HasFailed();
    static void TestReset();
    static bool TriggerBreakPoint();

    static TestConfig GetConfig();
private:
    static UInt32 sFailed;
    static UInt32 sExecuted;
    static Float64 sExecutionTime;
};

struct TestRegristration
{
    TestRegristration(const char* name, TestCallback callback) : 
    mName(name),
    mCallback(callback),
    mNext(nullptr)
    {
        TestFramework::RegisterTest(this);
    }

    const char*        mName;
    TestCallback       mCallback;
    TestRegristration* mNext;
};

// Use this to force test initialization
struct TestSuite
{
public:
    TestSuite(...)
    {

    }
};

#define TestBreak() if(lf::TestFramework::TriggerBreakPoint()) { __debugbreak(); }
#define TestExecute(ExpressionM) (::lf::TestFramework::ProcessTest((ExpressionM), #ExpressionM, __LINE__))

#define REGISTER_TEST(NameM)                          \
static void NameM##_TestFunction();                   \
::lf::TestRegristration (NameM)(#NameM, NameM##_TestFunction); \
static void NameM##_TestFunction()                    \

#define TEST(ExpressionM) if(!TestExecute((ExpressionM))) { TestBreak(); }
#define TEST_CRITICAL(ExpressionM) if(!TestExecute((ExpressionM))) { TestBreak(); return; }



} // namespace lf

#endif // LF_CORE_TEST_H