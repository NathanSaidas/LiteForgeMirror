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
#ifndef LF_CORE_TEST_H
#define LF_CORE_TEST_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include <intrin.h>

namespace lf {

using TestCallback = void(*)();
struct TestRegristration;
class EngineConfig;

struct LF_CORE_API TestConfig
{
    TestConfig();

    bool                mStress;
    bool                mTriggerBreakpoint;
    const EngineConfig* mEngineConfig;
};

struct TestContext
{
    bool                mTriggerBreakpoint;
    const EngineConfig* mEngineConfig;
    TestContext*        mPrev;
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
    static bool TestAll();

    static TestConfig GetConfig();
private:
    static UInt32 sFailed;
    static UInt32 sExecuted;
    static Float64 sExecutionTime;
};

struct LF_CORE_API TestRegristration
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
struct LF_CORE_API TestSuite
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

#define TEST_CRITICAL_EXCEPTION(expression_)                                                              \
{                                                                                                         \
    do {                                                                                                  \
        bool exceptionThrown = false;                                                                     \
        try { expression_; } catch( ::lf::Exception& ) { exceptionThrown = true; }              \
                                                                                                          \
        if (!::lf::TestFramework::ProcessTest(exceptionThrown, #expression_, __LINE__)) { TestBreak(); return; }  \
    } while (false);                                                                                      \
}


} // namespace lf

#endif // LF_CORE_TEST_H