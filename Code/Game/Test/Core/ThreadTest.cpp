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
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadFence.h"
#include "Core/Platform/RWLock.h"
#include "Core/Test/Test.h"
#include "Core/Utility/Log.h"

namespace lf {

void TestThreadProc(void* )
{
    LF_DEBUG_BREAK;
}

REGISTER_TEST(ThreadTest, "Core.Threading")
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

REGISTER_TEST(ThreadFenceTest, "Core.Threading")
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

REGISTER_TEST(AtomicIncrementTest, "Core.Threading")
{
    Atomic32 x = 0;
    auto y = AtomicIncrement32(&x);
    auto z = AtomicDecrement32(&x);
    TEST(y == 1);
    TEST(z == 0);
}

enum RWTestOperation
{
    TEST_OPERATION_READ,
    TEST_OPERATION_WRITE
};

struct RWTestingData
{
    TVector<RWTestOperation> mOperations;
    SpinLock                mLock;
    RWLock                  mRWLock;
};
struct RWTestingFuncData
{
    RWTestingFuncData() : mTestingData(nullptr), mSleepTime(0) {}
    RWTestingFuncData(RWTestingData* data, SizeT sleepTime) : mTestingData(data), mSleepTime(sleepTime) {}

    RWTestingData* mTestingData;
    SizeT          mSleepTime;
};


// Test to ensure that RWLock obeys locking priority.
REGISTER_TEST(RWLockTest, "Core.Threading")
{
    RWTestingData data;

    // We expect... R R R W W R R
    Thread threads[7];
    RWTestingFuncData threadDatas[LF_ARRAY_SIZE(threads)];

    threadDatas[0] = RWTestingFuncData(&data, 2000); // READ
    threadDatas[1] = RWTestingFuncData(&data, 2000); // READ
    threadDatas[2] = RWTestingFuncData(&data, 2000); // READ
    threadDatas[3] = RWTestingFuncData(&data, 5000); // WRITE
    threadDatas[4] = RWTestingFuncData(&data, 4000); // WRITE
    threadDatas[5] = RWTestingFuncData(&data, 2000); // READ
    threadDatas[6] = RWTestingFuncData(&data, 2000); // READ


    
    auto ReadFunc = [](void* param)
    {
        auto data = static_cast<RWTestingFuncData*>(param);
        ScopeRWLockRead readLock(data->mTestingData->mRWLock);
        SleepCallingThread(data->mSleepTime);
        ScopeLock lock(data->mTestingData->mLock);
        data->mTestingData->mOperations.push_back(TEST_OPERATION_READ);
    };

    auto WriteFunc = [](void* param)
    {
        auto data = static_cast<RWTestingFuncData*>(param);
        ScopeRWLockWrite writeLock(data->mTestingData->mRWLock);
        SleepCallingThread(data->mSleepTime);
        ScopeLock lock(data->mTestingData->mLock);
        data->mTestingData->mOperations.push_back(TEST_OPERATION_WRITE);
    };

    threads[0].Fork(ReadFunc, &threadDatas[0]);
    threads[1].Fork(ReadFunc, &threadDatas[1]);
    threads[2].Fork(ReadFunc, &threadDatas[2]);
    SleepCallingThread(1000);
    threads[3].Fork(WriteFunc, &threadDatas[3]);
    threads[4].Fork(WriteFunc, &threadDatas[4]);
    threads[5].Fork(ReadFunc, &threadDatas[5]);
    threads[6].Fork(ReadFunc, &threadDatas[6]);

    Thread::JoinAll(threads, LF_ARRAY_SIZE(threads));

    TEST(data.mOperations.size() == LF_ARRAY_SIZE(threads));
    TEST(data.mOperations[0] == TEST_OPERATION_READ);
    TEST(data.mOperations[1] == TEST_OPERATION_READ);
    TEST(data.mOperations[2] == TEST_OPERATION_READ);
    TEST(data.mOperations[3] == TEST_OPERATION_WRITE);
    TEST(data.mOperations[4] == TEST_OPERATION_WRITE);
    TEST(data.mOperations[5] == TEST_OPERATION_READ);
    TEST(data.mOperations[6] == TEST_OPERATION_READ);


}

}