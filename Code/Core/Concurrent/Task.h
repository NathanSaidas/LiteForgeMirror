// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "Core/Common/Enum.h"
#include "Core/Concurrent/TaskHandle.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Platform/ThreadFence.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/SmartCallback.h"
#include "Core/Utility/Time.h"

namespace lf {

// ********************************************************************
// Represents an asynchronous task that you can retrieve the result value of.
// ********************************************************************
template<typename ResultTypeT>
class Task
{
public:
    using TaskCallback = TCallback<ResultTypeT>;
private:
    enum class TaskState
    {
        None,
        Running,
        Complete
    };

    struct TaskData
    {
        TaskData() 
        : mState(EnumValue(TaskState::None))
        , mFence()
        , mTask()
        , mCallback()
        , mResultValue()
        { 
            CriticalAssert(mFence.Initialize()); 
        }
        ~TaskData() 
        { 
            mFence.Set(false);
            mFence.Destroy();  
        }
        TaskState GetState() const { return static_cast<TaskState>(AtomicLoad(&mState)); }
        void SetState(TaskState state) { AtomicStore(&mState, static_cast<Atomic32>(state)); }

        volatile Atomic32   mState;
        ThreadFence         mFence;
        TaskHandle          mTask;
        TaskCallback        mCallback;
        ResultTypeT         mResultValue;
    };
    using DataPtr = TAtomicStrongPointer<TaskData>;
public:
    Task() : mData() {}
    Task(const Task& other) : mData(other.mData) {}
    Task(Task&& other) : mData(std::move(other.mData)) {}

    explicit Task(const TaskCallback& callback) : mData()
    {
        SetCallback(callback);
    }

    explicit Task(const TaskCallback& callback, TaskSchedulerBase& scheduler) : mData()
    {
        SetCallback(callback);
        Run(scheduler);
    }

    Task& operator=(const Task& other)
    {
        if (this != &other)
        {
            mData = other.mData;
        }
        return *this;
    }
    Task& operator=(Task&& other)
    {
        if (this != &other)
        {
            mData = std::move(other.mData);
        }
        return *this;
    }

    void SetCallback(const TaskCallback& callback)
    {
        auto data = GetData();
        ReportBug(data->GetState() == TaskState::None);
        if (data->GetState() == TaskState::None)
        {
            data->mCallback = callback;
        }
    }

    void Run(TaskSchedulerBase& scheduler)
    {
        auto data = TryGetData();
        ReportBug(data != nullptr && data->GetState() == TaskState::None);
        if (data != nullptr && data->GetState() == TaskState::None)
        {
            data->mFence.Set(true);
            data->SetState(TaskState::Running);
            auto dataPtr = mData;
            data->mTask = scheduler.RunTask([dataPtr](void*)
                {
                    dataPtr->mResultValue = dataPtr->mCallback.Invoke();
                    dataPtr->SetState(TaskState::Complete);
                    dataPtr->mFence.Set(false);
                });
        }
    }

    const ResultTypeT& ResultValue() const
    {
        ReportBug(mData != NULL_PTR && mData->GetState() == TaskState::Complete);
        return GetData()->mResultValue;
    }

    bool IsRunning() const
    {
        return mData && GetData()->GetState() == TaskState::Running;
    }

    bool IsComplete() const
    {
        return mData && GetData()->GetState() == TaskState::Complete;
    }

    bool Wait(SizeT milliseconds = INVALID)
    {
        while(!IsComplete())
        {
            GetData()->mFence.Wait(milliseconds);
        }
        return IsComplete();
    }

private:
    TaskData* GetData()
    {
        if (!mData)
        {
            mData = DataPtr(LFNew<TaskData>());
        }
        return mData;
    }

    const TaskData* GetData() const
    {
        if (!mData)
        {
            mData = DataPtr(LFNew<TaskData>());
        }
        return mData;
    }

    TaskData* TryGetData()
    {
        return mData;
    }

    mutable DataPtr mData;
};

template<>
class Task<void>
{
public:
    using TaskCallback = TCallback<void>;
private:
    enum class TaskState
    {
        None,
        Running,
        Complete
    };

    struct TaskData
    {
        TaskData()
            : mState(EnumValue(TaskState::None))
            , mFence()
            , mTask()
            , mCallback()
        {
            CriticalAssert(mFence.Initialize());
        }
        ~TaskData()
        {
            mFence.Set(false);
            mFence.Destroy();
        }
        TaskState GetState() const { return static_cast<TaskState>(AtomicLoad(&mState)); }
        void SetState(TaskState state) { AtomicStore(&mState, static_cast<Atomic32>(state)); }

        volatile Atomic32   mState;
        ThreadFence         mFence; // TODO: This will become a hotspot, consider a pool
        TaskHandle          mTask;
        TaskCallback        mCallback;
        
    };
    using DataPtr = TAtomicStrongPointer<TaskData>;
public:
    Task() : mData() {}
    Task(const Task& other) : mData(other.mData) {}
    Task(Task&& other) : mData(std::move(other.mData)) {}

    explicit Task(const TaskCallback& callback) : mData()
    {
        SetCallback(callback);
    }

    explicit Task(const TaskCallback& callback, TaskSchedulerBase& scheduler) : mData()
    {
        SetCallback(callback);
        Run(scheduler);
    }

    Task& operator=(const Task& other)
    {
        if (this != &other)
        {
            mData = other.mData;
        }
        return *this;
    }
    Task& operator=(Task&& other)
    {
        if (this != &other)
        {
            mData = std::move(other.mData);
        }
        return *this;
    }

    void SetCallback(const TaskCallback& callback)
    {
        auto data = GetData();
        ReportBug(data->GetState() == TaskState::None);
        if (data->GetState() == TaskState::None)
        {
            data->mCallback = callback;
        }
    }

    void Run(TaskSchedulerBase& scheduler)
    {
        auto data = TryGetData();
        ReportBug(data != nullptr && data->GetState() == TaskState::None);
        if (data != nullptr && data->GetState() == TaskState::None)
        {
            data->mFence.Set(true);
            data->SetState(TaskState::Running);
            auto dataPtr = mData;
            data->mTask = scheduler.RunTask([dataPtr](void*)
                {
                    dataPtr->mCallback.Invoke();
                    dataPtr->SetState(TaskState::Complete);
                    dataPtr->mFence.Set(false);
                });
        }
    }

    bool IsRunning() const
    {
        return mData && GetData()->GetState() == TaskState::Running;
    }

    bool IsComplete() const
    {
        return mData && GetData()->GetState() == TaskState::Complete;
    }

    bool Wait(SizeT milliseconds = INVALID)
    {
        while (!IsComplete())
        {
            GetData()->mFence.Wait(milliseconds);
        }
        return IsComplete();
    }

private:
    TaskData* GetData()
    {
        if (!mData)
        {
            mData = DataPtr(LFNew<TaskData>());
        }
        return mData;
    }

    const TaskData* GetData() const
    {
        if (!mData)
        {
            mData = DataPtr(LFNew<TaskData>());
        }
        return mData;
    }

    TaskData* TryGetData()
    {
        return mData;
    }

    mutable DataPtr mData;
};

} // namespace lf