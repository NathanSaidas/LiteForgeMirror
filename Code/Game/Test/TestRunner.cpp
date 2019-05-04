#include "Core/Test/Test.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"
#include "Engine/App/Application.h"

namespace lf {


class TestRunner : public Application
{
DECLARE_CLASS(TestRunner, Application);
public:
    void OnStart() override
    {
        // Requirements:
        // -test /single=<test_name>
        // -test /all
        // -test /batch=<test_name>,<test_name>,<test_name>
        TestConfig config;
        if (CmdLine::HasArgOption("test", "opt_no_break"))
        {
            config.mTriggerBreakpoint = false;
        }
        if (CmdLine::HasArgOption("test", "opt_debug"))
        {
            gTestLog.SetLogLevel(LOG_DEBUG);
        }
        else
        {
            gTestLog.SetLogLevel(LOG_INFO);
        }

        String arg;
        if (CmdLine::HasArgOption("test", "all"))
        {
            TestFramework::ExecuteAllTests(config);
        }
        else if (CmdLine::GetArgOption("test", "batch", arg))
        {
            TArray<String> tests;
            StrSplit(arg, ',', tests);
            for (const String& test : tests)
            {
                TestFramework::ExecuteTest(test.CStr(), config);
            }
        }
        else if (CmdLine::GetArgOption("test", "single", arg))
        {
            TestFramework::ExecuteTest(arg.CStr(), config);
        }

    }
private:
};
DEFINE_CLASS(TestRunner) { NO_REFLECTION; }


}