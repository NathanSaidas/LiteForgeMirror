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
#include "Engine/App/Application.h"
#include "Engine/App/Program.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Utility/Time.h"
#include "Core/Utility/Utility.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/Atomic.h"
#include "Core/Concurrent/ConcurrentRingBuffer.h"
#include "Core/Concurrent/IOCPQueue.h"
#include "Core/Concurrent/TaskScheduler.h"

#include <map>
#include <algorithm>

using namespace lf;

class AtomicIncTestApp;

// const SizeT THREAD_COUNT = 4;
// const SizeT THREAD_WORK =  12500000;

const SizeT THREAD_COUNT = 16;
const SizeT THREAD_WORK = 3125000;

struct TestData
{
    TestData() : mIds(), mThread(), mApp() {}
    TestData(AtomicIncTestApp* app) : mIds(), mThread(), mApp(app) {}

    TArray<Atomic32>    mIds;
    Thread              mThread;
    AtomicIncTestApp*   mApp;
};

template<typename ForwardIt, typename T, typename CompareT = std::less<>>
ForwardIt BinaryFind(ForwardIt first, ForwardIt last, const T& value, CompareT comp = {})
{
    first = std::lower_bound(first, last, value, comp);
    return first != last && !comp(value, *first) ? first : last;
}

static void ProcessData(void*);

class AtomicIncTestApp : public Application
{
    DECLARE_CLASS(AtomicIncTestApp, Application);
public:
    volatile Atomic32 mId;

    void TestAtomicIncrement();

    void OnStart() override
    {
        gSysLog.Info(LogMessage("MyApp::OnStart"));

        AtomicStore(&mId, 0);
        TestAtomicIncrement();

        gSysLog.Info(LogMessage("Done!"));
    }

    void OnExit() override
    {
        gSysLog.Info(LogMessage("MyApp::OnExit"));
    }
};
DEFINE_CLASS(AtomicIncTestApp)
{
    NO_REFLECTION;
}

static void ProcessData(void* param)
{
    TestData* self = static_cast<TestData*>(param);
    for (SizeT i = 0; i < THREAD_WORK; ++i)
    {
        Atomic32 id = AtomicIncrement32(&self->mApp->mId);
        self->mIds.Add(id);
    }
}

// In this test we can determine whether or not the AtomicInc will return a unique id per thread
// even at high contention.
void AtomicIncTestApp::TestAtomicIncrement()
{
    TArray<TestData> threads;
    gSysLog.Info(LogMessage("Starting") << THREAD_COUNT << " threads.");
    for (SizeT i = 0; i < THREAD_COUNT; ++i)
    {
        threads.Add(TestData(this));
    }

    for (TestData& thread : threads)
    {
        thread.mThread.Fork(ProcessData, &thread);
    }

    gSysLog.Info(LogMessage("Waiting for threads to finish..."));
    SizeT reserve = 0;
    for (TestData& thread : threads)
    {
        thread.mThread.Join();
        reserve += thread.mIds.Size();
    }


    gSysLog.Info(LogMessage("Sorting results..."));
    TArray<Atomic32> merged;
    merged.Reserve(reserve);
    for (TestData& thread : threads)
    {
        merged.Insert(merged.end(), thread.mIds.begin(), thread.mIds.end());
    }

    std::sort(merged.begin(), merged.end());

    gSysLog.Info(LogMessage("Validating results..."));
    for (SizeT i = 0; i < merged.Size(); ++i)
    {
        if (merged[i] != static_cast<Atomic32>(i + 1))
        {
            CriticalAssertMsgEx("Unexpected value", LF_ERROR_BAD_STATE, ERROR_API_GAME);
        }
    }
}

void SingleProducerEntry(void* param);
void SingleConsumerEntry(void* param);
void MultiProducerEntry(void* param);
void MultiConsumerEntry(void* param);
void ProfileProducerEntry(void* param);
void ProfileConsumerEntry(void* param);
void IOCPProducerEntry(void* param);
void IOCPConsumerEntry(void* param);
void SchedulerProducerEntry(void* param);
void SchedulerConsumerEntry(void* param);

struct TestOutputData
{
    Float64 mAverageLatency;
    Float64 mMinLatency;
    Float64 mMaxLatency;
    Int32   mSenderDistributionScore;
    Int32   mWorkerDistributionScore;
};

class WorkerTestApp;

struct CCData
{
    CCData() :
        mPushTicks(INVALID64),
        mPopTicks(INVALID64),
        mSenderId(INVALID32),
        mWorkerId(INVALID32),
        mWorkId(INVALID),
        mApp(nullptr)
    {
    }

    // What time were we pushed?
    Int64 mPushTicks;
    // What time were we popped?
    Int64 mPopTicks;
    // Who queued us for processing
    UInt32 mSenderId;
    // Who processed us?
    UInt32 mWorkerId;
    // What was processed?
    SizeT mWorkId;
    // For self contained tasks
    WorkerTestApp* mApp;
    // For TaskScheduler, tasks must be stored by client
    TaskTypes::TaskItemType mTask;
};

// Enforce defaults to be null
template<>
struct ConcurrentRingBufferTraits<CCData*>
{
    using WorkResult = ConcurrentRingBufferWorkValueResult<CCData*>;
    static CCData* Default() { return nullptr; }
    static void Reset(CCData*& item) { item = nullptr; }
    static void Push(CCData*& output, CCData*& input) { output = input; }
    static CCData* ToResultType(ConcurrentRingBufferSlot<CCData*>& slot) { return slot.mData; }
    static CCData* ToResultTypeDefault() { return Default(); }
};

const Atomic32 WORK_TO_SUBMIT = 1500000;
class WorkerTestApp : public Application
{
    DECLARE_CLASS(WorkerTestApp, Application);
public:
    ConcurrentRingBuffer<Atomic32> mRingBuffer;
    ConcurrentRingBuffer<CCData*> mProfileRingBuffer;

    volatile Atomic32 mWorkId;
    volatile Atomic32 mWorkSubmitted;
    volatile Atomic32 mWorkCompleted;
    volatile Atomic32 mWorkersRunning;

    TArray<CCData> mPendingWork;

    volatile Atomic32 mBenchSize;


    TaskScheduler* volatile mScheduler;

    IOCPQueue<CCData> mIOCP;

    WorkerTestApp()
    {}

    void TestReset()
    {
        AtomicStore(&mWorkId, 0);
        AtomicStore(&mWorkSubmitted, 0);
        AtomicStore(&mWorkCompleted, 0);
        AtomicStore(&mWorkersRunning, 1);
        AtomicStore(&mBenchSize, 0);
        AtomicStorePointer(&mScheduler, (TaskScheduler*)0);
    }

    void TestWorkers(SizeT numProducers, SizeT numConsumers, ThreadCallback producerEntry, ThreadCallback consumerEntry)
    {
        TestReset();
        gSysLog.Info(LogMessage("Producers=") << numProducers);
        gSysLog.Info(LogMessage("Consumers=") << numConsumers);

        TArray<Thread> producers;
        TArray<Thread> consumers;
        producers.Resize(numProducers);
        consumers.Resize(numConsumers);
        for (Thread& t : consumers)
        {
            t.Fork(consumerEntry, this);
        }
        for (Thread& t : producers)
        {
            t.Fork(producerEntry, this);
        }

        gSysLog.Info(LogMessage("Waiting for producers to finish..."));
        Thread::JoinAll(producers.GetData(), producers.Size());
        AtomicStore(&mWorkersRunning, 0);

        gSysLog.Info(LogMessage("Waiting for consumers to finish..."));
        Thread::JoinAll(consumers.GetData(), consumers.Size());

        auto completed = AtomicLoad(&mWorkCompleted);
        auto submitted = AtomicLoad(&mWorkSubmitted);

        gSysLog.Info(LogMessage("WorkCompleted=") << completed);
        gSysLog.Info(LogMessage("WorkSubmitted=") << submitted);
        Assert(completed == submitted);
    }

    void TestWorkersIOCP(SizeT numProducers, SizeT numConsumers, ThreadCallback producerEntry, ThreadCallback consumerEntry)
    {
        mIOCP.SetConsumers(numConsumers);
        TestWorkers(numProducers, numConsumers, producerEntry, consumerEntry);
    }

    void TestScheduler(SizeT numProducers, SizeT numConsumers, ThreadCallback producerEntry)
    {
        for (CCData& testData : mPendingWork)
        {
            testData.mApp = this;
        }

        TestReset();
        gSysLog.Info(LogMessage("Producers=") << numProducers);
        gSysLog.Info(LogMessage("Consumers=") << numConsumers);

        TaskScheduler::OptionsType options;
        options.mNumWorkerThreads = numConsumers;
        // options.mNumDeliveryThreads = numProducers; // todo: deprecated: MPMC_EXP only.
        TaskScheduler scheduler;
        AtomicStorePointer(&mScheduler, &scheduler);
        scheduler.Initialize(options, true);

        TArray<Thread> producers;
        producers.Resize(numProducers);
        for (Thread& t : producers)
        {
            t.Fork(producerEntry, this);
        }

        gSysLog.Info(LogMessage("Waiting for producers to finish..."));
        Thread::JoinAll(producers.GetData(), producers.Size());
        AtomicStore(&mWorkersRunning, 0);

        gSysLog.Info(LogMessage("Waiting for consumers to finish..."));
        while (AtomicLoad(&mWorkCompleted) < WORK_TO_SUBMIT)
        {
            SleepCallingThread(1000);
        }
        scheduler.Shutdown();
        AtomicStorePointer(&mScheduler, (TaskScheduler*)0);

        auto completed = AtomicLoad(&mWorkCompleted);
        auto submitted = AtomicLoad(&mWorkSubmitted);

        gSysLog.Info(LogMessage("WorkCompleted=") << completed);
        gSysLog.Info(LogMessage("WorkSubmitted=") << submitted);
        Assert(completed == submitted);

        for (CCData& testData : mPendingWork)
        {
            testData.mApp = nullptr;
        }
    }

    void TestSPSC()
    {
        gSysLog.Info(LogMessage("SPSC test running..."));
        TestWorkers(1, 1, SingleProducerEntry, SingleConsumerEntry);
        gSysLog.Info(LogMessage("SPSC (IOCP) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestWorkersIOCP(1, 1, IOCPProducerEntry, IOCPConsumerEntry);
        mPendingWork.Clear();
        gSysLog.Info(LogMessage("SPSC (TaskScheduler) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestScheduler(1, 1, SchedulerProducerEntry);
        mPendingWork.Clear();

    }
    void TestSPMC()
    {
        gSysLog.Info(LogMessage("SPMC test running..."));
        TestWorkers(1, 4, SingleProducerEntry, MultiConsumerEntry);
        gSysLog.Info(LogMessage("SPMC (IOCP) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestWorkersIOCP(1, 4, IOCPProducerEntry, IOCPConsumerEntry);
        mPendingWork.Clear();
        gSysLog.Info(LogMessage("SPMC (TaskScheduler) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestScheduler(1, 4, SchedulerProducerEntry);
        mPendingWork.Clear();
    }
    void TestMPSC()
    {
        gSysLog.Info(LogMessage("MPSC test running..."));
        TestWorkers(4, 1, MultiProducerEntry, SingleConsumerEntry);
        gSysLog.Info(LogMessage("MPSC (IOCP) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestWorkersIOCP(4, 1, IOCPProducerEntry, IOCPConsumerEntry);
        mPendingWork.Clear();
        gSysLog.Info(LogMessage("MPSC (TaskScheduler) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestScheduler(4, 1, SchedulerProducerEntry);
        mPendingWork.Clear();
    }
    void TestMPMC()
    {
        gSysLog.Info(LogMessage("MPMC test running..."));
        TestWorkers(4, 4, MultiProducerEntry, MultiConsumerEntry);
        gSysLog.Info(LogMessage("MPMC (IOCP) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestWorkersIOCP(4, 4, IOCPProducerEntry, IOCPConsumerEntry);
        mPendingWork.Clear();
        gSysLog.Info(LogMessage("MPMC (TaskScheduler) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestScheduler(4, 4, SchedulerProducerEntry);
        mPendingWork.Clear();
    }
    void TestMPMCEx(SizeT numProducers, SizeT numConsumers)
    {
        gSysLog.Info(LogMessage("MPMCEx test running..."));
        TestWorkers(numProducers, numConsumers, MultiProducerEntry, MultiConsumerEntry);
        gSysLog.Info(LogMessage("MPMCEx (IOCP) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestWorkersIOCP(numProducers, numConsumers, IOCPProducerEntry, IOCPConsumerEntry);
        mPendingWork.Clear();
        gSysLog.Info(LogMessage("MPMCEx (TaskScheduler) test running..."));
        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestScheduler(numProducers, numConsumers, SchedulerProducerEntry);
        mPendingWork.Clear();
    }

    void OutputResults(TestOutputData* output = nullptr)
    {
        if (mPendingWork.Empty())
        {
            gSysLog.Warning(LogMessage("No results to process!"));
            return;
        }
        using WorkerAccum = std::map<UInt32, SizeT>;
        Int64 clockFreq = GetClockFrequency();
        WorkerAccum senderStats;
        WorkerAccum workerStats;

        Float64 minLatency = DBL_MAX;
        Float64 maxLatency = DBL_MIN;
        Float64 totalLatency = 0.0;

        for (CCData& result : mPendingWork)
        {
            Assert(Valid(result.mSenderId));
            Assert(Valid(result.mWorkerId));

            ++senderStats[result.mSenderId];
            ++workerStats[result.mWorkerId];

            Float64 seconds = static_cast<Float64>(result.mPopTicks - result.mPushTicks) / clockFreq;
            Float64 us = seconds * 1000000.0;

            minLatency = Min(minLatency, us);
            maxLatency = Max(maxLatency, us);
            totalLatency += us;
        }

        Float64 avgLatency = totalLatency / mPendingWork.Size();

        Int32 totalScore = 0;
        Int32 perfectDistribution = 0;
        Int32 distributionScore = 0;
        gSysLog.Info(LogMessage("Distribution and Load Balancing Stats:"));
        gSysLog.Info(LogMessage("  Sender"));
        perfectDistribution = static_cast<Int32>((1.0 / senderStats.size()) * 100.0);
        for (auto it : senderStats)
        {
            Int32 pct = static_cast<Int32>((static_cast<Float64>(it.second) / mPendingWork.Size()) * 100.0);
            gSysLog.Info(LogMessage("    [") << it.first << "]: " << it.second << ", " << pct << "%");
            totalScore += Abs(pct - perfectDistribution);
        }
        distributionScore = static_cast<Int32>((totalScore / senderStats.size()));
        gSysLog.Info(LogMessage("    Perfect Distribution=") << perfectDistribution);
        gSysLog.Info(LogMessage("    Distribution Score=") << distributionScore);
        if (output)
        {
            output->mSenderDistributionScore = distributionScore;
        }

        totalScore = 0;
        perfectDistribution = static_cast<Int32>((1.0 / workerStats.size()) * 100.0);
        distributionScore = 0;
        gSysLog.Info(LogMessage("  Worker"));
        for (auto it : workerStats)
        {
            Int32 pct = static_cast<Int32>((static_cast<Float64>(it.second) / mPendingWork.Size()) * 100.0);
            gSysLog.Info(LogMessage("    [") << it.first << "]: " << it.second << ", " << pct << "%");
            totalScore += Abs(pct - perfectDistribution);
        }
        distributionScore = static_cast<Int32>((totalScore / senderStats.size()));
        gSysLog.Info(LogMessage("    Perfect Distribution=") << perfectDistribution);
        gSysLog.Info(LogMessage("    Distribution Score=") << distributionScore);
        if (output)
        {
            output->mWorkerDistributionScore = distributionScore;
        }

        gSysLog.Info(LogMessage("Timing (in microseconds):"));
        gSysLog.Info(LogMessage("  Average=") << avgLatency);
        gSysLog.Info(LogMessage("  Min=") << minLatency);
        gSysLog.Info(LogMessage("  Max=") << maxLatency);
        if (output)
        {
            output->mAverageLatency = avgLatency;
            output->mMinLatency = minLatency;
            output->mMaxLatency = maxLatency;
        }
    }

    void TestProfile(const char* testName, SizeT numProducers, SizeT numConsumers, TestOutputData* output = nullptr)
    {
        gSysLog.Info(LogMessage(testName) << " test running...");

        const SizeT memoryEstimate = WORK_TO_SUBMIT * sizeof(CCData);
        const SizeT GIGA_BYTE = 1024 * 1024 * 1024;
        LF_STATIC_ASSERT(memoryEstimate < GIGA_BYTE);

        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestWorkers(numProducers, numConsumers, ProfileProducerEntry, ProfileConsumerEntry);
        // Process Results:
        OutputResults(output);
        mPendingWork.Clear();
    }

    void TestIOCPProfile(const char* testName, SizeT numProducers, SizeT numConsumers, TestOutputData* output = nullptr)
    {
        gSysLog.Info(LogMessage(testName) << " test running...");

        const SizeT memoryEstimate = WORK_TO_SUBMIT * sizeof(CCData);
        const SizeT GIGA_BYTE = 1024 * 1024 * 1024;
        LF_STATIC_ASSERT(memoryEstimate < GIGA_BYTE);

        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestWorkersIOCP(numProducers, numConsumers, IOCPProducerEntry, IOCPConsumerEntry);
        // Process Results:
        OutputResults(output);
        mPendingWork.Clear();
    }

    void TestSchedulerProfile(const char* testName, SizeT numProducers, SizeT numConsumers, TestOutputData* output = nullptr)
    {
        gSysLog.Info(LogMessage(testName) << " test running...");

        const SizeT memoryEstimate = WORK_TO_SUBMIT * sizeof(CCData);
        const SizeT GIGA_BYTE = 1024 * 1024 * 1024;
        LF_STATIC_ASSERT(memoryEstimate < GIGA_BYTE);

        mPendingWork.Resize(WORK_TO_SUBMIT);
        TestScheduler(numProducers, numConsumers, SchedulerProducerEntry);
        // Process Results:
        OutputResults(output);
        mPendingWork.Clear();
    }

    void OnStart() override
    {
        const bool PROFILE = true;
        if (!PROFILE)
        {
            TestSPSC();
            TestSPMC();
            TestMPSC();
            TestMPMC();
            TestMPMCEx(4, 8);
        }
        else
        {
            const bool PROFILE_BASIC = false;

            if (PROFILE_BASIC)
            {
                TestProfile("Profile SPSC", 1, 1);
                TestIOCPProfile("Profile (IOCP) SPSC", 1, 1);
                TestSchedulerProfile("Profile (Scheduler) SPSC", 1, 1);

                TestProfile("Profile SPMC", 1, 4);
                TestIOCPProfile("Profile (IOCP) SPMC", 1, 4);
                TestSchedulerProfile("Profile (Scheduler) SPMC", 1, 4);

                TestProfile("Profile MPSC", 4, 1);
                TestIOCPProfile("Profile (IOCP) MPSC", 4, 1);
                TestSchedulerProfile("Profile (Scheduler) MPSC", 4, 1);

                TestProfile("Profile MPMC", 4, 4);
                TestIOCPProfile("Profile (IOCP) MPMC", 4, 4);
                TestSchedulerProfile("Profile (Scheduler) MPMC", 4, 4);

                TestProfile("Profile MPMC", 4, 16);
                TestIOCPProfile("Profile (IOCP) MPMC", 4, 16);
                TestSchedulerProfile("Profile (Scheduler) MPMC", 4, 16);

            }
            else
            {
                const SizeT PROFILE_ITERATIONS = 16;
                TArray<TestOutputData> CRBResults;
                TArray<TestOutputData> IOCPResults;
                TArray<TestOutputData> SchedulerResults;
                CRBResults.Resize(PROFILE_ITERATIONS);
                IOCPResults.Resize(PROFILE_ITERATIONS);
                SchedulerResults.Resize(PROFILE_ITERATIONS);

                for (SizeT i = 0; i < PROFILE_ITERATIONS; ++i)
                {
                    gSysLog.Info(LogMessage("Test Iteration ") << i << "\n\n");
                    // TestProfile("Profile MPMC", 4, 16, &CRBResults[i]);
                    // TestIOCPProfile("Profile (IOCP) MPMC", 4, 16, &IOCPResults[i]);
                    TestSchedulerProfile("Profile (Scheduler) MPMC", 4, 16, &SchedulerResults[i]);
                }

                gSysLog.Info(LogMessage("Format=[Average Latency(us), Sender Distribution Score/Worker Distribution Score"));
                gSysLog.Info(LogMessage("[i] -- [   crb    ] -- [   mpmc   ] -- [   iocp   ]"));

                for (SizeT i = 0; i < PROFILE_ITERATIONS; ++i)
                {
                    TestOutputData& crb = CRBResults[i];
                    TestOutputData& iocp = IOCPResults[i];
                    TestOutputData& scheduler = SchedulerResults[i];

                    // header:
                    auto logMessage = LogMessage("[") << StreamFillRight(2) << i << StreamFillRight() << "] -- [";
                    // crb
                    logMessage << StreamPrecision(5) << crb.mAverageLatency << ","
                        << StreamFillRight(2) << crb.mSenderDistributionScore << StreamFillRight() << "/"
                        << StreamFillRight(2) << crb.mWorkerDistributionScore << StreamFillRight() << "] -- [";
                    // iocp
                    logMessage << StreamPrecision(7) << iocp.mAverageLatency << ","
                        << StreamFillRight(2) << iocp.mSenderDistributionScore << StreamFillRight() << "/"
                        << StreamFillRight(2) << iocp.mWorkerDistributionScore << StreamFillRight() << "] -- [";
                    // scheduler
                    logMessage << StreamPrecision(7) << scheduler.mAverageLatency << ","
                        << StreamFillRight(2) << scheduler.mSenderDistributionScore << StreamFillRight() << "/"
                        << StreamFillRight(2) << scheduler.mWorkerDistributionScore << StreamFillRight() << "]";
                    gSysLog.Info(logMessage);
                }
            }
        }
    }

    void SingleProducer()
    {
        while (AtomicLoad(&mWorkSubmitted) < WORK_TO_SUBMIT)
        {
            Atomic32 work = AtomicIncrement32(&mWorkId);
            while (!mRingBuffer.TryPush(work))
            {

            }
            AtomicIncrement32(&mWorkSubmitted);
        }
    }
    void SingleConsumer()
    {
        while (AtomicLoad(&mWorkersRunning) > 0 || mRingBuffer.Size() > 0)
        {
            if (auto result = mRingBuffer.TryPop())
            {
                Assert(result);
                AtomicIncrement32(&mWorkCompleted);
            }
        }
    }
    void MultiProducer()
    {
        while (AtomicLoad(&mWorkSubmitted) < WORK_TO_SUBMIT)
        {
            Atomic32 work = AtomicIncrement32(&mWorkId);
            while (!mRingBuffer.TryPush(work))
            {

            }
            AtomicIncrement32(&mWorkSubmitted);
        }
    }
    void MultiConsumer()
    {
        while (AtomicLoad(&mWorkersRunning) > 0 || mRingBuffer.Size() > 0)
        {
            if (auto result = mRingBuffer.TryPop())
            {
                Assert(result);
                AtomicIncrement32(&mWorkCompleted);
            }
        }
    }

    void ProfileProducer()
    {
        while (AtomicLoad(&mWorkSubmitted) < WORK_TO_SUBMIT)
        {
            SizeT workId = static_cast<SizeT>(AtomicIncrement32(&mWorkId)) - 1;
            if (workId >= WORK_TO_SUBMIT)
            {
                return; // done:
            }

            CCData* work = &mPendingWork[workId];
            work->mPushTicks = GetClockTime();
            work->mSenderId = static_cast<UInt32>(GetPlatformThreadId());
            work->mWorkId = workId;

            while (!mProfileRingBuffer.TryPush(work)) {}
            AtomicIncrement32(&mWorkSubmitted);
        }
    }

    void ProfileConsumer()
    {
        while (AtomicLoad(&mWorkersRunning) > 0 || mProfileRingBuffer.Size() > 0)
        {
            if (auto result = mProfileRingBuffer.TryPop())
            {
                Assert(result);
                Assert(result.mData);
                result.mData->mPopTicks = GetClockTime();
                result.mData->mWorkerId = static_cast<UInt32>(GetPlatformThreadId());
                AtomicIncrement32(&mWorkCompleted);
            }
        }
    }

    void IOCPProducer()
    {
        while (AtomicLoad(&mWorkSubmitted) < WORK_TO_SUBMIT)
        {
            SizeT workId = static_cast<SizeT>(AtomicIncrement32(&mWorkId)) - 1;
            if (workId >= WORK_TO_SUBMIT)
            {
                return;
            }

            CCData* work = &mPendingWork[workId];
            work->mPushTicks = GetClockTime();
            work->mSenderId = static_cast<UInt32>(GetPlatformThreadId());
            work->mWorkId = workId;

            while(!mIOCP.TryPush(work)) {}
            AtomicIncrement32(&mWorkSubmitted);
            AtomicIncrement32(&mBenchSize);
        }
    }

    void IOCPConsumer()
    {
        while (AtomicLoad(&mWorkersRunning) > 0 || AtomicLoad(&mBenchSize) > 0)
        {
            if (auto result = mIOCP.TryPop())
            {
                Assert(result.mData);
                result.mData->mPopTicks = GetClockTime();
                result.mData->mWorkerId = static_cast<Int32>(GetPlatformThreadId());
                AtomicIncrement32(&mWorkCompleted);
                AtomicDecrement32(&mBenchSize);
            }
        }
    }

    void SchedulerProducer()
    {
        while (AtomicLoad(&mWorkSubmitted) < WORK_TO_SUBMIT)
        {
            SizeT workId = static_cast<SizeT>(AtomicIncrement32(&mWorkId)) - 1;
            if (workId >= WORK_TO_SUBMIT)
            {
                return;
            }

            CCData* work = &mPendingWork[workId];
            work->mPushTicks = GetClockTime();
            work->mSenderId = static_cast<UInt32>(GetPlatformThreadId());
            work->mWorkId = workId;

            mScheduler->RunTask(SchedulerConsumerEntry, work);

            AtomicIncrement32(&mWorkSubmitted);
            AtomicIncrement32(&mBenchSize);
        }
    }

    void SchedulerConsumer(void* param)
    {
        CCData* data = static_cast<CCData*>(param);
        data->mPopTicks = GetClockTime();
        data->mWorkerId = static_cast<Int32>(GetPlatformThreadId());
        AtomicIncrement32(&mWorkCompleted);
        AtomicDecrement32(&mBenchSize);
    }

};
DEFINE_CLASS(WorkerTestApp) { NO_REFLECTION; }

void SingleProducerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->SingleProducer();
}
void SingleConsumerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->SingleConsumer();
}
void MultiProducerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->MultiProducer();
}
void MultiConsumerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->MultiConsumer();
}
void ProfileProducerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->ProfileProducer();
}
void ProfileConsumerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->ProfileConsumer();
}
void IOCPProducerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->IOCPProducer();
}
void IOCPConsumerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->IOCPConsumer();
}
void SchedulerProducerEntry(void* param)
{
    static_cast<WorkerTestApp*>(param)->SchedulerProducer();
}
void SchedulerConsumerEntry(void* param)
{
    static_cast<CCData*>(param)->mApp->SchedulerConsumer(param);
}



// int main(int argc, const char** argv)
// {
//     Program::Execute(argc, argv);
//     // (argc);
//     // (argv);
//     // SetMainThread();
//     // gTokenTable = &gTokenTableInstance;
//     // gTokenTable->Initialize();
//     // GetReflectionMgr().BuildTypes();
//     // {
//     //     auto instance = GetReflectionMgr().Create<BreakType>();
//     //     instance->Run();
//     // }
//     // GetReflectionMgr().ReleaseTypes();
//     // gTokenTable->Release();
//     // gTokenTable = nullptr;
// 
//     return 0;
// }