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
#pragma once

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/String/String.h" // Used by TestConfig
#include "Core/Utility/StdVector.h" // Used by TestConfig
#include <intrin.h>

namespace lf {

using TestCallback = void(*)();
struct TestRegristration;
class EngineConfig;

struct LF_CORE_API TestConfig
{
    TestConfig();

    // Enable stress tests
    bool mStressEnabled;
    // Enable setup tests
    bool mSetupEnabled;
    // Enable benchmark tests
    bool mBenchmarkEnabled;
    // Enable parallel execution
    bool mParallelExecution;
    // Test failures trigger a breakpoint.
    bool mTriggerBreakpoint;
    // Run setup tests only.
    bool mSetupExclusive;
    // Run benchmark tests only.
    bool mBenchmarkExclusive;
    // Run stress tests only.
    bool mStressExclusive;
    // Delete the temp folder and all its contents on startup
    bool mClean;

    TVector<String> mTestTargets;
    TVector<String> mGroupTargets;
    TVector<String> mIgnoredTests;
    TVector<String> mIgnoredGroups;

    // bool                mTriggerBreakpoint;
    const EngineConfig* mEngineConfig;
};

struct TestContext
{
    bool                mTriggerBreakpoint;
    const EngineConfig* mEngineConfig;
    TestContext*        mPrev;
};

namespace TestFlags
{
    enum Value
    {
        // No flags
        TF_NONE      = 0,
        // Test is marked disabled and will not run unless explicitly listed in a /single or /batch commandline argument.
        TF_DISABLED  = 1 << 1,
        // Test is marked as a stress test and will not run unless explicitly listed in a /single or /batch command line argument or has /stress command line argument.
        TF_STRESS    = 1 << 2,
        // Test is marked as a setup test and will not run unless explicitly listed in a /single or /batch command line argument or has /setup command line argument.
        TF_SETUP     = 1 << 3,
        // Test is marked as a benchmark test and will not run unless explicitly listed in a /single or /batch command line argument or has /benchmark command line argument.
        TF_BENCHMARK = 1 << 4,
        // Test is marked as parallel meaning it will run in parallel with other tests in a single process. (Intended for localized testing)
        TF_PARALLEL  = 1 << 5,

        TF_DEFAULT   = TF_NONE,
        TF_ALL       = TF_STRESS | TF_SETUP | TF_BENCHMARK
    };
};

class LF_CORE_API TestFramework
{
public:
    static void RegisterTest(TestRegristration* test);

    static void ExecuteAll(const TestConfig& config);
    static void ListGroups();

    static bool ProcessTest(bool result, const char* expression, int line);
    static bool HasFailed();
    static void TestReset();
    static bool TriggerBreakPoint();
    static bool TestAll();

    static TestConfig GetConfig();
    static String GetTempDirectory();
private:
    
};

struct LF_CORE_API TestRegristration
{
    TestRegristration(const char* name, TestCallback callback, const char* group = "DefaultGroup", UInt32 flags = TestFlags::TF_DEFAULT, SizeT priority = 1000) 
    : mName(name)
    , mGroup(group)
    , mFlags(flags)
    , mPriority(priority)
    , mCallback(callback)
    , mNext(nullptr)
    {
        TestFramework::RegisterTest(this);
    }

    const char*        mName;
    const char*        mGroup;
    UInt32             mFlags;
    SizeT              mPriority;
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

#define REGISTER_TEST(NameM, ...)                          \
static void NameM##_TestFunction();                   \
::lf::TestRegristration (NameM)(#NameM, NameM##_TestFunction, __VA_ARGS__); \
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