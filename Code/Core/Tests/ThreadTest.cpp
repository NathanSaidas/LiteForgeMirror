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
#include "ThreadTest.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadFence.h"
#include "Core/Test/Test.h"
#include "Core/Utility/Log.h"

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

struct TestEventData
{
    ThreadFence mFence;
};
static volatile Atomic32 gTestValue = 0;

static void TestEventCallback(void* ptr)
{
    TestEventData* e = reinterpret_cast<TestEventData*>(ptr);
    gTestLog.Info(LogMessage("Waiting for fence..."));
    gTestLog.Sync();
    TEST(e->mFence.Wait() == ThreadFence::WS_SUCCESS);

    gTestLog.Info(LogMessage("Fence signal received!"));
    gTestLog.Sync();
    AtomicIncrement32(&gTestValue);

    gTestLog.Info(LogMessage("Waiting for event again..."));
    gTestLog.Sync();
    TEST(e->mFence.Wait() == ThreadFence::WS_SUCCESS);

    gTestLog.Info(LogMessage("Fence signal received!"));
    gTestLog.Sync();
    AtomicIncrement32(&gTestValue);

}

REGISTER_TEST(ThreadFenceTest)
{
    AtomicStore(&gTestValue, 0);

    TestEventData e;
    e.mFence.Initialize();


    gTestLog.Info(LogMessage("Forking threads..."));
    gTestLog.Sync();

    Thread thread;
    thread.Fork(TestEventCallback, &e);

    Thread threadB;
    threadB.Fork(TestEventCallback, &e);

    gTestLog.Info(LogMessage("Waiting for 3 seconds..."));
    gTestLog.Sync();
    SleepCallingThread(3000);

    gTestLog.Info(LogMessage("Signal fence."));
    gTestLog.Sync();
    TEST(e.mFence.Signal());

    gTestLog.Info(LogMessage("Waiting for 5 seconds..."));
    gTestLog.Sync();
    SleepCallingThread(5000);
    TEST(AtomicLoad(&gTestValue) == 2);


    gTestLog.Info(LogMessage("Signal fence."));
    gTestLog.Sync();
    TEST(e.mFence.Signal());

    gTestLog.Info(LogMessage("Waiting for thread to finish..."));
    gTestLog.Sync();

    thread.Join();
    SleepCallingThread(1000);
    TEST(AtomicLoad(&gTestValue) == 4);

}

}