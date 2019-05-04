#include "Core/Platform/Thread.h"
#include "Core/Test/Test.h"

namespace lf {

void TestThreadProc(void* )
{
    LF_DEBUG_BREAK;
}

REGISTER_TEST(ThreadTest)
{
    {
        Thread thread;
        thread.Fork(TestThreadProc, nullptr);
        thread.Join();
    }
    {
        Thread thread;
        thread.Fork(TestThreadProc, nullptr);
    }
    {
        Thread thread;
    }
}

}