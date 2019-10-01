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
#include "Core/Test/Test.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"
#include "Engine/App/Application.h"
#include "Engine/App/Program.h"

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
        config.mEngineConfig = GetConfig();
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

int main(int argc, const char** argv)
{
    lf::Program::Execute(argc, argv);
}
