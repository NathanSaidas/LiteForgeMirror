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
#include "Core/Math/Random.h"
#include "Core/Memory/DynamicPoolHeap.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/PoolHeap.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadSignal.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/String/StringCommon.h"
#include "Core/String/SStream.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/StdMap.h"

#include <utility>
#include <Windows.h>

namespace lf {

struct SampleObject
{
    UInt32 a;
    UInt32 b;
    ByteT bytes[32];
};

REGISTER_TEST(PoolHeapTest)
{
    SStream ss;

    const SizeT NUM_OBJECTS = 10;
    const SizeT BUFFER_SIZE = sizeof(SampleObject) * (NUM_OBJECTS - 1);
    const SizeT NUM_POOL_OBJECTS = NUM_OBJECTS - 2;

    // Create heap
    PoolHeap heap;
    heap.Initialize(sizeof(SampleObject), alignof(SampleObject), NUM_POOL_OBJECTS, PoolHeap::PHF_DOUBLE_FREE);

    TStaticArray<SampleObject*, NUM_POOL_OBJECTS> objects;
    TStaticArray<SampleObject*, NUM_POOL_OBJECTS> ordered;
    objects.Resize(NUM_POOL_OBJECTS);
    // Allocate objects in order.
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        objects[i] = static_cast<SampleObject*>(heap.Allocate());
        TEST(objects[i] != nullptr);
        ordered.Add(objects[i]);
        memset(objects[i], static_cast<int>(i), sizeof(SampleObject));
        ss << "Allocated 0x" << ToHexString(reinterpret_cast<UIntPtrT>(objects[i])) << "\n";
    }

    // Free objects in same order. (note the next allocations will occur in reverse order)
    TEST(heap.Allocate() == nullptr);
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        memset(objects[i], 0xFF, sizeof(SampleObject));
        ss << "Free 0x" << ToHexString(reinterpret_cast<UIntPtrT>(objects[i])) << "\n";
        heap.Free(objects[i]);
        objects[i] = nullptr;
    }

    // Allocate objects
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        objects[i] = static_cast<SampleObject*>(heap.Allocate());
        TEST(objects[i] != nullptr);
        ss << "Allocated 0x" << ToHexString(reinterpret_cast<UIntPtrT>(objects[i])) << "\n";
    }
    TEST(heap.Allocate() == nullptr);

    // Free objects in 'random' order.
    Int32 seed = 0xCADE1337;
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        SizeT index = static_cast<SizeT>(Random::Mod(seed, static_cast<UInt32>(objects.Size())));

        memset(objects[index], static_cast<int>(i), sizeof(SampleObject));
        ss << "Free 0x" << ToHexString(reinterpret_cast<UIntPtrT>(objects[index])) << "\n";
        heap.Free(objects[index]);
        objects[index] = nullptr;
        objects.Remove(objects.begin() + index);
    }

    // Allocate objects in reverse order they were deallocated.
    objects.Resize(NUM_POOL_OBJECTS);
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        objects[i] = static_cast<SampleObject*>(heap.Allocate());
        TEST(objects[i] != nullptr);
        ss << "Allocated 0x" << ToHexString(reinterpret_cast<UIntPtrT>(objects[i])) << "\n";
    }
    TEST(heap.Allocate() == nullptr);

    // Free all objects.
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        ss << "Free 0x" << ToHexString(reinterpret_cast<UIntPtrT>(objects[i])) << "\n";
        heap.Free(objects[i]);
        objects[i] = nullptr;
    }

    // Release all memory.
    heap.Release();
    gTestLog.Info(LogMessage("\n") << ss.Str());
}

REGISTER_TEST(PoolHeapTestLarge)
{
    const SizeT NUM_OBJECTS = 35000;
    const SizeT BUFFER_SIZE = sizeof(SampleObject) * (NUM_OBJECTS - 1);
    const SizeT NUM_POOL_OBJECTS = NUM_OBJECTS - 2;

    // Create heap
    PoolHeap heap;
    heap.Initialize(sizeof(SampleObject), alignof(SampleObject), NUM_POOL_OBJECTS);

    TArray<SampleObject*> objects;
    TArray<SampleObject*> ordered;
    objects.Reserve(NUM_POOL_OBJECTS);
    ordered.Reserve(NUM_POOL_OBJECTS);
    objects.Resize(NUM_POOL_OBJECTS);
    // Allocate objects in order.
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        objects[i] = static_cast<SampleObject*>(heap.Allocate());
        TEST(objects[i] != nullptr);
        ordered.Add(objects[i]);
        memset(objects[i], static_cast<int>(i), sizeof(SampleObject));
    }

    // Free objects in same order. (note the next allocations will occur in reverse order)
    TEST(heap.Allocate() == nullptr);
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        memset(objects[i], 0xFF, sizeof(SampleObject));
        heap.Free(objects[i]);
        objects[i] = nullptr;
    }

    // Allocate objects
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        objects[i] = static_cast<SampleObject*>(heap.Allocate());
        TEST(objects[i] != nullptr);
    }
    TEST(heap.Allocate() == nullptr);

    // Free objects in 'random' order.
    Int32 seed = 0xCADE1337;
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        SizeT index = static_cast<SizeT>(Random::Mod(seed, static_cast<UInt32>(objects.Size())));

        memset(objects[index], static_cast<int>(i), sizeof(SampleObject));
        heap.Free(objects[index]);
        objects[index] = nullptr;
        objects.Remove(objects.begin() + index);
    }

    // Allocate objects in reverse order they were deallocated.
    objects.Resize(NUM_POOL_OBJECTS);
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        objects[i] = static_cast<SampleObject*>(heap.Allocate());
        TEST(objects[i] != nullptr);
    }
    TEST(heap.Allocate() == nullptr);

    // Free all objects.
    for (SizeT i = 0; i < NUM_POOL_OBJECTS; ++i)
    {
        heap.Free(objects[i]);
        objects[i] = nullptr;
    }

    // Release all memory.
    heap.Release();
}

struct PoolHeapTestContext;

struct PoolHeapThreadData
{
    SizeT mIndex;
    PoolHeapTestContext* mContext;
    TArray<SampleObject*> mIn;
    TArray<SampleObject*> mOut;
    Thread mThread;
};

struct PoolHeapTestContext
{
    SizeT mPoolObjects;
    SizeT mObjectAllocations;
    ThreadSignal mSignal;
    PoolHeap mHeap;
    TArray<PoolHeapThreadData> mThreads;
};

static void ProcessPoolHeapThreadTest(void* data)
{
    PoolHeapThreadData* threadData = reinterpret_cast<PoolHeapThreadData*>(data);
    threadData->mContext->mSignal.Wait();

    const SizeT capacity = threadData->mOut.Capacity();
    for (SizeT i = 0; i < capacity; ++i) 
    {
        SampleObject* object = reinterpret_cast<SampleObject*>(threadData->mContext->mHeap.Allocate());
        memset(object, static_cast<int>(i), sizeof(SampleObject));
        object->a = static_cast<UInt32>(threadData->mIndex);
        threadData->mOut.Add(object);
    }
}

REGISTER_TEST(PoolHeapMultithreaded)
{

    // If we have 4 threads... and 200 objects..
    // We can allocate 50 objects foreach thread.
    // From this we should be able to verify that each thread has their own object.
    // And the pool heap cannot allocate anymore objects.
    //
    {
        SizeT NUM_THREADS = 4;
        SizeT NUM_OBJECTS = 100000;

        PoolHeapTestContext context;
        context.mObjectAllocations = NUM_OBJECTS / NUM_THREADS;
        context.mPoolObjects = (NUM_OBJECTS + 2) - 2;

        gTestLog.Info(LogMessage(""));

        TEST(context.mHeap.Initialize(
            sizeof(SampleObject),
            alignof(SampleObject),
            context.mPoolObjects));

        context.mThreads.Resize(NUM_THREADS);
        for (SizeT i = 0; i < context.mThreads.Size(); ++i)
        {
            context.mThreads[i].mIndex = i;
            context.mThreads[i].mContext = &context;
            context.mThreads[i].mIn.Reserve(context.mObjectAllocations);
            context.mThreads[i].mOut.Reserve(context.mObjectAllocations);
        }

        for (SizeT i = 0; i < context.mThreads.Size(); ++i)
        {
            context.mThreads[i].mThread.Fork(ProcessPoolHeapThreadTest, &context.mThreads[i]);
        }

        SleepCallingThread(1000);
        context.mSignal.WakeAll();

        for (SizeT i = 0; i < context.mThreads.Size(); ++i)
        {
            context.mThreads[i].mThread.Join();
        }

        // Verify all objects are owned by their creator threads
        // Verify all objects are unique

        ByteT expectedMemory[sizeof(SampleObject)];

        TArray<SampleObject*> objects;
        objects.Reserve(context.mPoolObjects);
        for (SizeT i = 0; i < context.mThreads.Size(); ++i)
        {
            for (SizeT k = 0; k < context.mThreads[i].mOut.Size(); ++k)
            {
                SampleObject* object = context.mThreads[i].mOut[k];
                TEST(object->a == static_cast<UInt32>(context.mThreads[i].mIndex));
                TEST(std::find(objects.begin(), objects.end(), object) == objects.end());
                

                ByteT* bytes = &reinterpret_cast<ByteT*>(object)[4];
                memset(expectedMemory, static_cast<int>(k), sizeof(SampleObject));
                TEST(memcmp(bytes, expectedMemory, sizeof(SampleObject) - 4) == 0);

                objects.Add(object);
            }
        }


        context.mHeap.Release();
    }
    

    // We can even have 8 threads and 1000 objects
    // Threads can allocate until the pool returns null
    // We should then be able to verify that all threads allocates unique objects
    // and that the pool is exhausted.
    //
    //
    // If we allocate 500 objects on main thread, pass 250 each to 2 threads to free
    // and allocate 250 each on 2 other threads.
    // We should be ableto allocate 500 objects on main thread
    // We should be able to verify all objects are unique.
    //
}

#if defined(LF_USE_EXCEPTIONS)
REGISTER_TEST(PoolHeapDoubleFreeTest)
{
    PoolHeap heap;
    heap.Initialize(40, 8, 10, PoolHeap::PHF_DOUBLE_FREE);
    void* ptr = heap.Allocate();
    heap.Free(ptr);
    TEST_CRITICAL_EXCEPTION(heap.Free(ptr));
    heap.Release();
}
#endif

struct ReaderWriterState
{
    RWSpinLock mLock;
    Atomic32 mReaders;
    Atomic32 mWriters;

    Atomic32 mMultiReaders;

    Atomic32 mExecute;
};

const SizeT LOOP_ITERATIONS = 10000000;

static void Readers(void* data)
{
    ReaderWriterState* state = reinterpret_cast<ReaderWriterState*>(data);

    while (AtomicLoad(&state->mExecute) == 0)
    {
        
    }

    for (SizeT i = 0; i < LOOP_ITERATIONS; ++i)
    {
        ScopeRWLockRead lock(state->mLock);
        Atomic32 numReaders = AtomicIncrement32(&state->mReaders);
        TEST(AtomicLoad(&state->mWriters) == 0);
        TEST(numReaders >= 0);
        if (numReaders > 1)
        {
            AtomicIncrement32(&state->mMultiReaders);
        }
        AtomicDecrement32(&state->mReaders);
    }
}

static void Writers(void* data)
{
    ReaderWriterState* state = reinterpret_cast<ReaderWriterState*>(data);

    while (AtomicLoad(&state->mExecute) == 0)
    {

    }

    for (SizeT i = 0; i < LOOP_ITERATIONS; ++i)
    {
        ScopeRWLockWrite lock(state->mLock);
        Atomic32 numWriters = AtomicIncrement32(&state->mWriters);
        TEST(AtomicLoad(&state->mReaders) == 0);
        TEST(numWriters == 1);
        AtomicDecrement32(&state->mWriters);
    }
}

REGISTER_TEST(ReaderWriteLockTest)
{
    ReaderWriterState state;
    AtomicStore(&state.mExecute, 0);
    AtomicStore(&state.mMultiReaders, 0);
    AtomicStore(&state.mReaders, 0);
    AtomicStore(&state.mWriters, 0);

    Thread readers[12];
    Thread writers[3];

    for (SizeT i = 0; i < LF_ARRAY_SIZE(readers); ++i)
    {
        readers[i].Fork(Readers, &state);
        readers[i].SetDebugName("ReaderThread");
    }

    for (SizeT i = 0; i < LF_ARRAY_SIZE(writers); ++i)
    {
        writers[i].Fork(Writers, &state);
        writers[i].SetDebugName("WriterThread");
    }


    AtomicStore(&state.mExecute, 1);

    for (SizeT i = 0; i < LF_ARRAY_SIZE(readers); ++i)
    {
        readers[i].Join();
    }

    for (SizeT i = 0; i < LF_ARRAY_SIZE(writers); ++i)
    {
        writers[i].Join();
    }
}

REGISTER_TEST(DynamicPoolHeapTest)
{
    TArray<void*> objects;

    DynamicPoolHeap heap;
    TEST(heap.GetHeapCount() == 0);
    TEST(heap.GetGarbageHeapCount() == 0);
    heap.Initialize(64, 8, 4);
    TEST(heap.GetHeapCount() == 1);
    TEST(heap.GetGarbageHeapCount() == 0);

    // 0 1 2 3 | 4 5 6 7 | 8 9 10 11
    for (SizeT i = 0; i < 3 * 4; ++i)
    {
        objects.Add(heap.Allocate());
    }

    TEST(heap.GetHeapCount() == 3);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[1]);
    heap.GCCollect();
    TEST(heap.GetHeapCount() == 3);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[4]);
    heap.GCCollect();
    TEST(heap.GetHeapCount() == 3);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[5]);
    heap.GCCollect();
    TEST(heap.GetHeapCount() == 3);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[6]);
    heap.GCCollect();
    TEST(heap.GetHeapCount() == 3);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[7]);
    TEST(heap.GetHeapCount() == 3);
    TEST(heap.GetGarbageHeapCount() == 1);

    heap.GCCollect();
    TEST(heap.GetHeapCount() == 2);
    TEST(heap.GetGarbageHeapCount() == 0);


    heap.Free(objects[8]);
    heap.GCCollect();
    TEST(heap.GetHeapCount() == 2);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[9]);
    heap.GCCollect();
    TEST(heap.GetHeapCount() == 2);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[10]);
    heap.GCCollect();
    TEST(heap.GetHeapCount() == 2);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[11]);
    TEST(heap.GetHeapCount() == 2);
    TEST(heap.GetGarbageHeapCount() == 1);

    heap.GCCollect();
    TEST(heap.GetHeapCount() == 1);
    TEST(heap.GetGarbageHeapCount() == 0);
    heap.Release();
    objects.Clear();

    // Round 2:
    TEST(heap.GetHeapCount() == 0);
    TEST(heap.GetGarbageHeapCount() == 0);
    heap.Initialize(64, 8, 4);
    TEST(heap.GetHeapCount() == 1);
    TEST(heap.GetGarbageHeapCount() == 0);

    // 0 1 2 3 | 4 5 6 7 | 8 9 10 11
    for (SizeT i = 0; i < 3 * 4; ++i)
    {
        objects.Add(heap.Allocate());
    }

    TEST(heap.GetHeapCount() == 3);
    TEST(heap.GetGarbageHeapCount() == 0);

    heap.Free(objects[4]);
    heap.Free(objects[5]);
    heap.Free(objects[6]);
    heap.Free(objects[7]);
    heap.Free(objects[8]);
    heap.Free(objects[9]);
    heap.Free(objects[10]);
    heap.Free(objects[11]);
    TEST(heap.GetHeapCount() == 3);
    TEST(heap.GetGarbageHeapCount() == 2);
    heap.GCCollect();
    TEST(heap.GetHeapCount() == 1);
    TEST(heap.GetGarbageHeapCount() == 0);



    

}

struct ConcurrentDynamicPoolHeapTestState;

struct ConcurrentDynamicPoolHeapTestSharedState
{
    DynamicPoolHeap mHeap;
    TArray<ConcurrentDynamicPoolHeapTestState> mStates;
    volatile Atomic32 mExecute;

};

struct ConcurrentDynamicPoolHeapTestState
{
    Thread mThread;
    TArray<void*> mObjects;
    SizeT mID;
    ConcurrentDynamicPoolHeapTestSharedState* mState;
};

struct LF_ALIGN(8) ConcurrentObject
{
    ByteT mData[64];
};

static void ConcurrentDynamicPoolAllocate(void* param)
{
    ConcurrentDynamicPoolHeapTestState* self = reinterpret_cast<ConcurrentDynamicPoolHeapTestState*>(param);
    while (AtomicLoad(&self->mState->mExecute) == 0)
    {

    }

    DynamicPoolHeap& heap = self->mState->mHeap;
    SizeT numObjects = self->mObjects.Size();
    for (SizeT i = 0; i < numObjects; ++i)
    {
        self->mObjects[i] = heap.Allocate();
    }
}

static void ConcurrentDynamicPoolFree(void* param)
{
    ConcurrentDynamicPoolHeapTestState* self = reinterpret_cast<ConcurrentDynamicPoolHeapTestState*>(param);
    while (AtomicLoad(&self->mState->mExecute) == 0)
    {

    }

    DynamicPoolHeap& heap = self->mState->mHeap;
    SizeT numObjects = self->mObjects.Size();
    for (SizeT i = 0; i < numObjects; ++i)
    {
        if (self->mObjects[i])
        {
            heap.Free(self->mObjects[i]);
            self->mObjects[i] = nullptr;
        }
    }
}

static void ConcurrentDynamicPoolStableAllocate(void* param)
{
    ConcurrentDynamicPoolHeapTestState* self = reinterpret_cast<ConcurrentDynamicPoolHeapTestState*>(param);
    while (AtomicLoad(&self->mState->mExecute) == 0)
    {

    }

    SizeT numAllocs = self->mObjects.Size();
    SizeT objectsAllocated = 0;
    while (objectsAllocated < numAllocs)
    {
        void* object = self->mState->mHeap.Allocate();
        if (object)
        {
            self->mObjects[objectsAllocated++] = object;
        }
    }

    // Reserve ( N )
    // Allocate ( 1/2 * N )
    // Multithreaded Allocate (3/4 * N)
    // Multithreaded Free ( 1/4 * N )
    // Result: Exhausted Heap
    // Free ( 1/4 * N )
    // Multithreaded Allocate (1/4 * N)
    // Multithreaded Free(3/4 * N)
    // Result: 1/4 Allocated
    // 
}

static void ConcurrentExecuteTest()
{
    const SizeT NUM_OBJECTS_PER_THREAD = 750;
    const SizeT NUM_HEAPS = 4;
    const SizeT MAX_HEAPS = 3;
    const SizeT MAX_OBJECTS = (NUM_OBJECTS_PER_THREAD * NUM_HEAPS) / MAX_HEAPS;

    ConcurrentDynamicPoolHeapTestSharedState sharedState;
    AtomicStore(&sharedState.mExecute, 0);
    sharedState.mHeap.Initialize(sizeof(ConcurrentObject), alignof(ConcurrentObject), MAX_OBJECTS, MAX_HEAPS, PoolHeap::PHF_DOUBLE_FREE);
    sharedState.mStates.Resize(NUM_HEAPS);

    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        sharedState.mStates[i].mState = &sharedState;
        sharedState.mStates[i].mID = i;
        sharedState.mStates[i].mObjects.Reserve(NUM_OBJECTS_PER_THREAD);
        sharedState.mStates[i].mObjects.Resize(NUM_OBJECTS_PER_THREAD);
        sharedState.mStates[i].mThread.Fork(ConcurrentDynamicPoolAllocate, &sharedState.mStates[i]);
    }

    // Allocate
    AtomicStore(&sharedState.mExecute, 1);

    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        sharedState.mStates[i].mThread.Join();
    }

    AtomicStore(&sharedState.mExecute, 0);

    TMap<UIntPtrT, SizeT> addresses;
    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        for (SizeT k = 0; k < sharedState.mStates[i].mObjects.Size(); ++k)
        {
            UIntPtrT object = reinterpret_cast<UIntPtrT>(sharedState.mStates[i].mObjects[k]);
            SizeT& value = addresses[object];
            ++value;
            TEST(value == 1);
        }
    }
    TEST(sharedState.mHeap.GetHeapCount() == MAX_HEAPS);
    TEST(sharedState.mHeap.GetGarbageHeapCount() == 0);
    TEST(sharedState.mHeap.GetAllocations() == NUM_OBJECTS_PER_THREAD * NUM_HEAPS);

    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        sharedState.mStates[i].mThread.Fork(ConcurrentDynamicPoolFree, &sharedState.mStates[i]);
    }

    // Free
    AtomicStore(&sharedState.mExecute, 1);

    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        sharedState.mStates[i].mThread.Join();
    }


    TEST(sharedState.mHeap.GetHeapCount() == 3);
    TEST(sharedState.mHeap.GetGarbageHeapCount() == 2);
    TEST(sharedState.mHeap.GetAllocations() == 0);

    // Allocate:
    AtomicStore(&sharedState.mExecute, 0);
    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        sharedState.mStates[i].mThread.Fork(ConcurrentDynamicPoolAllocate, &sharedState.mStates[i]);
    }
    AtomicStore(&sharedState.mExecute, 1);
    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        sharedState.mStates[i].mThread.Join();
    }
    TEST(sharedState.mHeap.GetHeapCount() == 3);
    TEST(sharedState.mHeap.GetGarbageHeapCount() == 0);
    TEST(sharedState.mHeap.GetAllocations() == NUM_OBJECTS_PER_THREAD * NUM_HEAPS);
    // Free:
    AtomicStore(&sharedState.mExecute, 0);
    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        sharedState.mStates[i].mThread.Fork(ConcurrentDynamicPoolFree, &sharedState.mStates[i]);
    }
    AtomicStore(&sharedState.mExecute, 1);
    for (SizeT i = 0; i < NUM_HEAPS; ++i)
    {
        sharedState.mStates[i].mThread.Join();
    }
    TEST(sharedState.mHeap.GetHeapCount() == 3);
    TEST(sharedState.mHeap.GetGarbageHeapCount() == 2);
    TEST(sharedState.mHeap.GetAllocations() == 0);

    // Collect
    sharedState.mHeap.GCCollect();
    TEST(sharedState.mHeap.GetGarbageHeapCount() == 0);
    TEST(sharedState.mHeap.GetHeapCount() == 1);

    sharedState.mHeap.Release();
}

static void ConcurrentExecuteExhaustiveTest()
{
    const SizeT NUM_OBJECTS_PER_THREAD = 750;
    const SizeT NUM_NULL_OBJECTS = 200;
    const SizeT NUM_THREADS = 4;
    const SizeT MAX_HEAPS = 3;
    const SizeT MAX_OBJECTS_PER_HEAP = (NUM_OBJECTS_PER_THREAD * NUM_THREADS) / MAX_HEAPS;
    const SizeT MAX_OBJECTS = (NUM_OBJECTS_PER_THREAD * NUM_THREADS) + NUM_NULL_OBJECTS;

    ConcurrentDynamicPoolHeapTestSharedState sharedState;
    AtomicStore(&sharedState.mExecute, 0);
    sharedState.mHeap.Initialize(sizeof(ConcurrentObject), alignof(ConcurrentObject), MAX_OBJECTS_PER_HEAP, MAX_HEAPS, PoolHeap::PHF_DOUBLE_FREE);
    sharedState.mStates.Resize(NUM_THREADS);

    for (SizeT i = 0; i < NUM_THREADS; ++i)
    {
        sharedState.mStates[i].mState = &sharedState;
        sharedState.mStates[i].mID = i;
        sharedState.mStates[i].mObjects.Reserve(MAX_OBJECTS / NUM_THREADS);
        sharedState.mStates[i].mObjects.Resize(MAX_OBJECTS / NUM_THREADS);
        sharedState.mStates[i].mThread.Fork(ConcurrentDynamicPoolAllocate, &sharedState.mStates[i]);
    }

    // Allocate
    AtomicStore(&sharedState.mExecute, 1);
    for (SizeT i = 0; i < NUM_THREADS; ++i)
    {
        sharedState.mStates[i].mThread.Join();
    }
    AtomicStore(&sharedState.mExecute, 0);

    TMap<UIntPtrT, SizeT> addresses;
    for (SizeT i = 0; i < NUM_THREADS; ++i)
    {
        for (SizeT k = 0; k < sharedState.mStates[i].mObjects.Size(); ++k)
        {
            UIntPtrT object = reinterpret_cast<UIntPtrT>(sharedState.mStates[i].mObjects[k]);
            SizeT& value = addresses[object];
            ++value;
            if (object != 0)
            {
                TEST(value == 1);
            }
        }
    }

    TEST(sharedState.mHeap.GetHeapCount() == MAX_HEAPS);
    TEST(sharedState.mHeap.GetGarbageHeapCount() == 0);
    TEST(sharedState.mHeap.GetAllocations() == MAX_OBJECTS_PER_HEAP * MAX_HEAPS);
    TEST(addresses[0] == NUM_NULL_OBJECTS);

    sharedState.mHeap.Release();

}

static void ConcurrentAllocateFreeTest()
{
    const SizeT MAX_OBJECTS_PER_HEAP = 1000;
    const SizeT MAX_HEAPS = 3;

    const SizeT Q1 = (MAX_OBJECTS_PER_HEAP * MAX_HEAPS) / 4;
    const SizeT Q2 = Q1 * 2;
    const SizeT Q3 = Q1 * 3;

    ConcurrentDynamicPoolHeapTestSharedState sharedState;
    sharedState.mHeap.Initialize(sizeof(ConcurrentObject), alignof(ConcurrentObject), MAX_OBJECTS_PER_HEAP, MAX_HEAPS, PoolHeap::PHF_DOUBLE_FREE);
    
    // Allocate ( 1/2 * N )
    sharedState.mStates.Resize(5);
    for (ConcurrentDynamicPoolHeapTestState& state : sharedState.mStates)
    {
        state.mState = &sharedState;
        state.mID = 0;
    }
    
    TArray<ConcurrentDynamicPoolHeapTestState*> freeStates;
    freeStates.Add(&sharedState.mStates[0]);
    freeStates.Add(&sharedState.mStates[1]);
    TArray<ConcurrentDynamicPoolHeapTestState*> allocateStates;
    allocateStates.Add(&sharedState.mStates[2]);
    allocateStates.Add(&sharedState.mStates[3]);
    allocateStates.Add(&sharedState.mStates[4]);

    DynamicPoolHeap& heap = sharedState.mHeap;
    TArray<void*> reserved;
    for (SizeT i = 0; i < Q1; ++i)
    {
        void* pointer = heap.Allocate();
        TEST(pointer != nullptr);
        reserved.Add(pointer);
    }

    for (SizeT i = 0; i < Q1; ++i)
    {
        void* pointer = heap.Allocate();
        TEST(pointer != nullptr);
        freeStates[i % freeStates.Size()]->mObjects.Add(pointer);
    }

    AtomicStore(&sharedState.mExecute, 0);
    // Launch Stablealloc
    // Launch Free
    for (ConcurrentDynamicPoolHeapTestState* state : freeStates)
    {
        state->mThread.Fork(ConcurrentDynamicPoolFree, state);
    }
    for (ConcurrentDynamicPoolHeapTestState* state : allocateStates)
    {
        state->mObjects.Resize(Q1);
        state->mThread.Fork(ConcurrentDynamicPoolStableAllocate, state);
    }
    AtomicStore(&sharedState.mExecute, 1);

    for (ConcurrentDynamicPoolHeapTestState& state : sharedState.mStates)
    {
        state.mThread.Join();
    }


    TMap<UIntPtrT, SizeT> map;
    for (ConcurrentDynamicPoolHeapTestState* state : allocateStates)
    {
        for (void* obj : state->mObjects)
        {
            TEST(obj != nullptr);
            SizeT& result = map[reinterpret_cast<UIntPtrT>(obj)];
            ++result;
            TEST(result == 1);
        }
    }
    for (ConcurrentDynamicPoolHeapTestState* state : freeStates)
    {
        for (void* obj : state->mObjects)
        {
            TEST(obj == nullptr);
        }
        state->mObjects.Clear();
    }

    TEST(heap.GetAllocations() == heap.GetMaxAllocations());
    map.clear();

    for (void* obj : reserved)
    {
        heap.Free(obj);
    }
    reserved.Clear();


    AtomicStore(&sharedState.mExecute, 0);

    for (ConcurrentDynamicPoolHeapTestState* state : allocateStates)
    {
        state->mThread.Fork(ConcurrentDynamicPoolFree, state);
    }

    for (ConcurrentDynamicPoolHeapTestState* state : freeStates)
    {
        state->mObjects.Resize(Q1 / 2);
        state->mThread.Fork(ConcurrentDynamicPoolAllocate, state);
    }

    AtomicStore(&sharedState.mExecute, 1);

    for (ConcurrentDynamicPoolHeapTestState& state : sharedState.mStates)
    {
        state.mThread.Join();
    }

    TEST(heap.GetAllocations() == Q1);

    sharedState.mHeap.Release();
}


REGISTER_TEST(DynamicPoolHeapTestMultithreaded)
{
    // We should be able to guarantee...
    // All allocated pointers are unique
    // writes to pointers of size do not overlap
    // All allocated pointers are owned by a heap inside the dynamic heap
    
    ConcurrentExecuteExhaustiveTest();
    ConcurrentAllocateFreeTest();

    for (SizeT i = 0; i < 1; ++i)
    {
        ConcurrentExecuteTest();
    }

    // multithreaded_allocate
    // multithreaded_free
    // multithreaded_allocate_free
    // multithreaded_allocate_free_gc

}


} // namespace lf
