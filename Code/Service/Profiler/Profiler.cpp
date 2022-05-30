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
#include "Profiler.h"
#include "Core/Common/Assert.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Platform/Atomic.h"
#include "Core/String/SStream.h"
#include "Core/Test/ProfilerHooks.h"
#include "Core/Utility/Time.h"

namespace lf {

Profiler::Profiler()
: mBeginFrameTick(0)
, mRingBuffer()
, mThread()
, mFence()
, mLabelDB()
, mFrameDB()
, mDBLock()
, mRunning(0)
, mNumLabels(0)
, mNumObjects(0)
{

}
Profiler::~Profiler()
{
    CriticalAssert(!IsRunning());
}

void Profiler::Initialize()
{
    // todo: We can probably allow 'fail initialization' in services.
    CriticalAssert(AtomicLoad(&Profiling::gEnabled) == 0);

    Profiling::gFrame = 0;
    mBeginFrameTick = GetClockTime();
    Profiling::gSubmitScope = Profiling::SubmitScopeCallback::Make([this](const ProfileScopeCaptureData& capture) { OnQueueCapture(capture); });
    Profiling::gSubmitScopeObject = Profiling::SubmitScopeObjectCallback::Make([this](const ProfileScopeObjectCaptureData& capture) { OnQueueCapture(capture); });
    AtomicStore(&Profiling::gEnabled, 1);

    SetIsRunning(true);
    mFence.Initialize();
    mFence.Set(true);
    mThread.Fork([](void* param)
    {
        static_cast<Profiler*>(param)->ProcessCaptures();
    }, this);
}
void Profiler::Shutdown()
{
    AtomicStore(&Profiling::gEnabled, 0);
    // Thread::Sleep(100); // todo: We might need to give some time depending on how we do initialize/shutdown and where we use profiling tags.
    Profiling::gSubmitScope = Profiling::SubmitScopeCallback::Make([](const ProfileScopeCaptureData&) {});
    Profiling::gSubmitScopeObject = Profiling::SubmitScopeObjectCallback::Make([](const ProfileScopeObjectCaptureData&) {});
    SetIsRunning(false);
    mFence.Set(false);
    mThread.Join();


}
void Profiler::EndFrame()
{
    FrameCapture capture;
    capture.mFrame = Profiling::gFrame;

    ++Profiling::gFrame;
    capture.mBeginTick = mBeginFrameTick;
    capture.mEndTick = GetClockTime();
    mBeginFrameTick = capture.mEndTick;

    // ScopeRWSpinLockWrite writeLock(mDBLock);
    mFrameDB.push_back(capture);
}

void Profiler::ProcessCaptures()
{
    while (IsRunning())
    {
        mFence.Wait(1000);
        if (auto result = mRingBuffer.TryPop())
        {
            ScopeRWSpinLockWrite writeLock(mDBLock);
            auto& capture = mLabelDB[result.mData.mLabel];
            switch (result.mData.mType)
            {
                case CT_LABEL: Insert(capture.mScopedLabels, result.mData); break;
                case CT_OBJECT: Insert(capture.mScopedObjects, result.mData);  break;
                default:
                    break;
            }
        }
    }
}

bool Profiler::CsvExportCaptureHeader(SStream& csvRows)
{
    csvRows << "Label,ObjectName,ObjectID,Frame,BeginTick,EndTick,ExecutionTime,ExecutionTimeUnit,ThreadID,ThreadTag,ThreadBeginCore,ThreadEndCore,\r\n";
    return true;
}
bool Profiler::CsvExportLabels(const String& label, SStream& csvRows, bool writeHeader)
{
    if (label.Empty())
    {
        return false;
    }

    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }

    Float64 freq = static_cast<Float64>(GetClockFrequency());
    ScopeRWSpinLockRead lock(mDBLock);
    for (auto it = mLabelDB.begin(); it != mLabelDB.end(); ++it)
    {
        String dbLabel(it->first, COPY_ON_WRITE);
        if (dbLabel != label)
        {
            continue;
        }

        for (auto capture = it->second.mScopedLabels.begin(); capture != it->second.mScopedLabels.end(); ++capture)
        {
            Float64 executionTime = Abs(capture->mEndTick - capture->mBeginTick) / freq;
            const char* executionTimeUnit = FormatTimeStr(executionTime);
            executionTime = FormatTime(executionTime);

            csvRows << dbLabel
                << "," // ObjectName
                << "," // ObjectID
                << "," << capture->mFrame
                << "," << capture->mBeginTick
                << "," << capture->mEndTick
                << "," << StreamPrecision(6) << executionTime << StreamPrecision()
                << "," << executionTimeUnit
                << "," << capture->mThreadID
                << "," << capture->mThreadTag
                << "," << capture->mThreadBeginCore
                << "," << capture->mThreadEndCore
                << ",\r\n";
        }

        return true;
    }
    return false;
}
bool Profiler::CsvExportObjects(const String& label, const String& objectName, SStream& csvRows, bool writeHeader)
{
    if (label.Empty() || objectName.Empty())
    {
        return false;
    }

    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }

    Float64 freq = static_cast<Float64>(GetClockFrequency());
    ScopeRWSpinLockRead lock(mDBLock);
    for (auto it = mLabelDB.begin(); it != mLabelDB.end(); ++it)
    {
        String dbLabel(it->first, COPY_ON_WRITE);
        if (dbLabel != label)
        {
            continue;
        }
        bool objectFound = false;
        for (auto capture = it->second.mScopedObjects.begin(); capture != it->second.mScopedObjects.end(); ++capture)
        {
            String captureObjectName(capture->mObjectName, COPY_ON_WRITE);
            if (captureObjectName != objectName)
            {
                continue;
            }

            Float64 executionTime = Abs(capture->mEndTick - capture->mBeginTick) / freq;
            const char* executionTimeUnit = FormatTimeStr(executionTime);
            executionTime = FormatTime(executionTime);

            csvRows << dbLabel
                << "," << captureObjectName
                << "," << capture->mObjectID
                << "," << capture->mFrame
                << "," << capture->mBeginTick
                << "," << capture->mEndTick
                << "," << StreamPrecision(6) << executionTime << StreamPrecision()
                << "," << executionTimeUnit
                << "," << capture->mThreadID
                << "," << capture->mThreadTag
                << "," << capture->mThreadBeginCore
                << "," << capture->mThreadEndCore
                << ",\r\n";
            objectFound = true;
        }
        return objectFound;
    }
    return false;
}
bool Profiler::CsvExportObjects(const String& label, UInt32 objectID, SStream& csvRows, bool writeHeader)
{
    if (label.Empty())
    {
        return false;
    }

    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }

    Float64 freq = static_cast<Float64>(GetClockFrequency());
    ScopeRWSpinLockRead lock(mDBLock);
    for (auto it = mLabelDB.begin(); it != mLabelDB.end(); ++it)
    {
        String dbLabel(it->first, COPY_ON_WRITE);
        if (dbLabel != label)
        {
            continue;
        }
        bool objectFound = false;
        for (auto capture = it->second.mScopedObjects.begin(); capture != it->second.mScopedObjects.end(); ++capture)
        {
            if (capture->mObjectID != objectID)
            {
                continue;
            }

            Float64 executionTime = Abs(capture->mEndTick - capture->mBeginTick) / freq;
            const char* executionTimeUnit = FormatTimeStr(executionTime);
            executionTime = FormatTime(executionTime);

            csvRows << dbLabel
                << "," << String(capture->mObjectName, COPY_ON_WRITE)
                << "," << capture->mObjectID
                << "," << capture->mFrame
                << "," << capture->mBeginTick
                << "," << capture->mEndTick
                << "," << StreamPrecision(6) << executionTime << StreamPrecision()
                << "," << executionTimeUnit
                << "," << capture->mThreadID
                << "," << capture->mThreadTag
                << "," << capture->mThreadBeginCore
                << "," << capture->mThreadEndCore
                << ",\r\n";
            objectFound = true;
        }
        return objectFound;
    }
    return false;

}
bool Profiler::CsvExportLabelsWhere(const String& label, SStream& csvRows, bool writeHeader)
{
    if (label.Empty())
    {
        return false;
    }

    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }

    bool success = false;
    Float64 freq = static_cast<Float64>(GetClockFrequency());
    ScopeRWSpinLockRead lock(mDBLock);
    for (auto it = mLabelDB.begin(); it != mLabelDB.end(); ++it)
    {
        String dbLabel(it->first, COPY_ON_WRITE);
        if (Valid(dbLabel.FindAgnostic(label)))
        {
            continue;
        }

        for (auto capture = it->second.mScopedLabels.begin(); capture != it->second.mScopedLabels.end(); ++capture)
        {
            Float64 executionTime = Abs(capture->mEndTick - capture->mBeginTick) / freq;
            const char* executionTimeUnit = FormatTimeStr(executionTime);
            executionTime = FormatTime(executionTime);

            csvRows << dbLabel
                << "," // ObjectName
                << "," // ObjectID
                << "," << capture->mFrame
                << "," << capture->mBeginTick
                << "," << capture->mEndTick
                << "," << StreamPrecision(6) << executionTime << StreamPrecision()
                << "," << executionTimeUnit
                << "," << capture->mThreadID
                << "," << capture->mThreadTag
                << "," << capture->mThreadBeginCore
                << "," << capture->mThreadEndCore
                << ",\r\n";
        }

        success = true;
    }
    return success;

}
bool Profiler::CsvExportObjectsWhere(const String& label, const String& objectName, SStream& csvRows, bool writeHeader)
{
    if (label.Empty() || objectName.Empty())
    {
        return false;
    }

    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }

    bool success = false;
    Float64 freq = static_cast<Float64>(GetClockFrequency());
    ScopeRWSpinLockRead lock(mDBLock);
    for (auto it = mLabelDB.begin(); it != mLabelDB.end(); ++it)
    {
        String dbLabel(it->first, COPY_ON_WRITE);
        if (Valid(dbLabel.FindAgnostic(label)))
        {
            continue;
        }
        bool objectFound = false;
        for (auto capture = it->second.mScopedObjects.begin(); capture != it->second.mScopedObjects.end(); ++capture)
        {
            String captureObjectName(capture->mObjectName, COPY_ON_WRITE);
            if (captureObjectName != objectName)
            {
                continue;
            }

            Float64 executionTime = Abs(capture->mEndTick - capture->mBeginTick) / freq;
            const char* executionTimeUnit = FormatTimeStr(executionTime);
            executionTime = FormatTime(executionTime);

            csvRows << dbLabel
                << "," << captureObjectName
                << "," << capture->mObjectID
                << "," << capture->mFrame
                << "," << capture->mBeginTick
                << "," << capture->mEndTick
                << "," << StreamPrecision(6) << executionTime << StreamPrecision()
                << "," << executionTimeUnit
                << "," << capture->mThreadID
                << "," << capture->mThreadTag
                << "," << capture->mThreadBeginCore
                << "," << capture->mThreadEndCore
                << ",\r\n";
            objectFound = true;
        }
        success = objectFound ? true : success;
    }
    return success;

}
bool Profiler::CsvExportAllLabels(SStream& csvRows, bool writeHeader)
{
    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }

    Float64 freq = static_cast<Float64>(GetClockFrequency());
    ScopeRWSpinLockRead lock(mDBLock);
    for (auto it = mLabelDB.begin(); it != mLabelDB.end(); ++it)
    {
        String dbLabel(it->first, COPY_ON_WRITE);

        for (auto capture = it->second.mScopedLabels.begin(); capture != it->second.mScopedLabels.end(); ++capture)
        {
            Float64 executionTime = Abs(capture->mEndTick - capture->mBeginTick) / freq;
            const char* executionTimeUnit = FormatTimeStr(executionTime);
            executionTime = FormatTime(executionTime);

            csvRows << dbLabel
                << "," // ObjectName
                << "," // ObjectID
                << "," << capture->mFrame
                << "," << capture->mBeginTick
                << "," << capture->mEndTick
                << "," << StreamPrecision(6) << executionTime << StreamPrecision()
                << "," << executionTimeUnit
                << "," << capture->mThreadID
                << "," << capture->mThreadTag
                << "," << capture->mThreadBeginCore
                << "," << capture->mThreadEndCore
                << ",\r\n";
        }
    }
    return true;
}
bool Profiler::CsvExportAllObjects(SStream& csvRows, bool writeHeader)
{
    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }

    Float64 freq = static_cast<Float64>(GetClockFrequency());
    ScopeRWSpinLockRead lock(mDBLock);
    for (auto it = mLabelDB.begin(); it != mLabelDB.end(); ++it)
    {
        String dbLabel(it->first, COPY_ON_WRITE);

        for (auto capture = it->second.mScopedObjects.begin(); capture != it->second.mScopedObjects.end(); ++capture)
        {
            String captureObjectName(capture->mObjectName, COPY_ON_WRITE);

            Float64 executionTime = Abs(capture->mEndTick - capture->mBeginTick) / freq;
            const char* executionTimeUnit = FormatTimeStr(executionTime);
            executionTime = FormatTime(executionTime);

            csvRows << dbLabel
                << "," << captureObjectName
                << "," << capture->mObjectID
                << "," << capture->mFrame
                << "," << capture->mBeginTick
                << "," << capture->mEndTick
                << "," << StreamPrecision(6) << executionTime << StreamPrecision()
                << "," << executionTimeUnit
                << "," << capture->mThreadID
                << "," << capture->mThreadTag
                << "," << capture->mThreadBeginCore
                << "," << capture->mThreadEndCore
                << ",\r\n";
        }
    }
    return true;

}
bool Profiler::CsvExportAllObjects(const String& label, SStream& csvRows, bool writeHeader)
{
    if (label.Empty())
    {
        return false;
    }

    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }

    bool success = false;
    Float64 freq = static_cast<Float64>(GetClockFrequency());
    ScopeRWSpinLockRead lock(mDBLock);
    for (auto it = mLabelDB.begin(); it != mLabelDB.end(); ++it)
    {
        String dbLabel(it->first, COPY_ON_WRITE);
        if (dbLabel == label)
        {
            continue;
        }
        for (auto capture = it->second.mScopedObjects.begin(); capture != it->second.mScopedObjects.end(); ++capture)
        {
            String captureObjectName(capture->mObjectName, COPY_ON_WRITE);

            Float64 executionTime = Abs(capture->mEndTick - capture->mBeginTick) / freq;
            const char* executionTimeUnit = FormatTimeStr(executionTime);
            executionTime = FormatTime(executionTime);

            csvRows << dbLabel
                << "," << captureObjectName
                << "," << capture->mObjectID
                << "," << capture->mFrame
                << "," << capture->mBeginTick
                << "," << capture->mEndTick
                << "," << StreamPrecision(6) << executionTime << StreamPrecision()
                << "," << executionTimeUnit
                << "," << capture->mThreadID
                << "," << capture->mThreadTag
                << "," << capture->mThreadBeginCore
                << "," << capture->mThreadEndCore
                << ",\r\n";
        }
        success = true;
    }
    return success;

}
bool Profiler::CsvExportAll(SStream& csvRows, bool writeHeader)
{
    if (writeHeader)
    {
        if (!CsvExportCaptureHeader(csvRows))
        {
            return false;
        }
    }
    return CsvExportAllLabels(csvRows, false) && CsvExportAllObjects(csvRows, false);
}

SizeT Profiler::Footprint() const
{
    SizeT db = 0;
    SizeT labels = 0;
    SizeT objects = 0;

    {
        ScopeRWSpinLockRead lock(const_cast<RWSpinLock&>(mDBLock));
        db = mLabelDB.size() * (sizeof(const char*) + sizeof(Capture));
    }

    labels = AtomicLoad(&mNumLabels) * sizeof(ProfileLabelStorage);
    objects = AtomicLoad(&mNumObjects) * sizeof(ProfileObjectStorage);
    return db + labels + objects;
}

void Profiler::OnQueueCapture(const ProfileScopeCaptureData& capture)
{
    RingBufferType::WorkItem item;
    item.mType = CT_LABEL;

    item.mBeginTick = capture.mBeginTick;
    item.mEndTick = capture.mEndTick;
    item.mFrame = capture.mFrame;
    item.mThreadBeginCore = capture.mThreadBeginCore;
    item.mThreadEndCore = capture.mThreadEndCore;
    item.mThreadID = capture.mThreadID;
    item.mThreadTag = capture.mThreadTag;
    item.mLabel = capture.mLabel;
    memset(item.mObjectName, 0, sizeof(item.mObjectName));
    item.mObjectID = INVALID32;
    
    while(!mRingBuffer.TryPush(item)) {}
    mFence.Signal();
}
void Profiler::OnQueueCapture(const ProfileScopeObjectCaptureData& capture)
{
    LF_STATIC_ASSERT(sizeof(RingBufferType::WorkItem::mObjectName) == sizeof(ProfileScopeObjectCaptureData::mObjectName));

    RingBufferType::WorkItem item;
    item.mType = CT_LABEL;

    item.mBeginTick = capture.mBeginTick;
    item.mEndTick = capture.mEndTick;
    item.mFrame = capture.mFrame;
    item.mThreadBeginCore = capture.mThreadBeginCore;
    item.mThreadEndCore = capture.mThreadEndCore;
    item.mThreadID = capture.mThreadID;
    item.mThreadTag = capture.mThreadTag;
    item.mLabel = capture.mLabel;
    memcpy(item.mObjectName, capture.mObjectName, sizeof(item.mObjectName));
    item.mObjectID = capture.mObjectID;

    while (!mRingBuffer.TryPush(item)) {}
    mFence.Signal();
}

void Profiler::Insert(ProfileLabelCollection& collection, const ProfileCapture& capture)
{
    if (capture.mType != CT_LABEL)
    {
        return;
    }

    ProfileLabelStorage stored;
    stored.mBeginTick = capture.mBeginTick;
    stored.mEndTick = capture.mEndTick;
    stored.mFrame = capture.mFrame;
    stored.mThreadBeginCore = capture.mThreadBeginCore;
    stored.mThreadEndCore = capture.mThreadEndCore;
    stored.mThreadID = capture.mThreadID;
    stored.mThreadTag = capture.mThreadTag;
    stored.mLabel = capture.mLabel;
    collection.push_back(stored);
    AtomicIncrement64(&mNumLabels);
}

void Profiler::Insert(ProfileObjectCollection& collection, const ProfileCapture& capture)
{
    if (capture.mType != CT_OBJECT)
    {
        return;
    }

    LF_STATIC_ASSERT(sizeof(RingBufferType::WorkItem::mObjectName) == sizeof(ProfileObjectStorage::mObjectName));

    ProfileObjectStorage stored;
    stored.mBeginTick = capture.mBeginTick;
    stored.mEndTick = capture.mEndTick;
    stored.mFrame = capture.mFrame;
    stored.mThreadBeginCore = capture.mThreadBeginCore;
    stored.mThreadEndCore = capture.mThreadEndCore;
    stored.mThreadID = capture.mThreadID;
    stored.mThreadTag = capture.mThreadTag;
    stored.mLabel = capture.mLabel;
    memcpy(stored.mObjectName, capture.mObjectName, sizeof(stored.mObjectName));
    stored.mObjectID = capture.mObjectID;

    collection.push_back(stored);
    AtomicIncrement64(&mNumObjects);
}

bool Profiler::IsRunning()
{
    return AtomicLoad(&mRunning) != 0;
}
void Profiler::SetIsRunning(bool value)
{
    AtomicStore(&mRunning, value ? 1 : 0);
}

} // namespace lf