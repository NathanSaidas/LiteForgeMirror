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

#include "Core/Common/API.h"
#include "Core/Concurrent/ConcurrentRingBuffer.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadFence.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/Test/Profiling.h" // technically dont even need this here, can be in cpp
#include "Core/Utility/StdMap.h"
#include "Core/Utility/StdVector.h"

#include "Core/String/SStream.h"

namespace lf {

class SStream;

class LF_SERVICE_API Profiler
{
public:
    Profiler();
    ~Profiler();

    void Initialize();
    void Shutdown();

    void EndFrame();

    void ProcessCaptures();

    bool CsvExportCaptureHeader(SStream& csvRows);
    bool CsvExportLabels(const String& label, SStream& csvRows, bool writeHeader = true);
    bool CsvExportObjects(const String& label, const String& objectName, SStream& csvRows, bool writeHeader = true);
    bool CsvExportObjects(const String& label, UInt32 objectID, SStream& csvRows, bool writeHeader = true);
    bool CsvExportLabelsWhere(const String& label, SStream& csvRows, bool writeHeader = true);
    bool CsvExportObjectsWhere(const String& label, const String& objectName, SStream& csvRows, bool writeHeader = true);
    bool CsvExportAllLabels(SStream& csvRows, bool writeHeader = true);
    bool CsvExportAllObjects(SStream& csvRows, bool writeHeader = true);
    bool CsvExportAllObjects(const String& label, SStream& csvRows, bool writeHeader = true);
    bool CsvExportAll(SStream& csvRows, bool writeHeader = true);

    SizeT Footprint() const;

private:
    enum CaptureType
    {
        CT_LABEL,
        CT_OBJECT
    };

    struct ProfileCapture
    {
        CaptureType mType;

        Int64 mBeginTick;
        Int64 mEndTick;
        UInt64 mFrame;
        UInt16 mThreadBeginCore;
        UInt16 mThreadEndCore;
        UInt16 mThreadID;
        UInt16 mThreadTag;
        const char* mLabel;
        char   mObjectName[64];
        UInt32 mObjectID;
    };

    struct ProfileLabelStorage
    {
        Int64 mBeginTick;
        Int64 mEndTick;
        UInt64 mFrame;
        UInt16 mThreadBeginCore;
        UInt16 mThreadEndCore;
        UInt16 mThreadID;
        UInt16 mThreadTag;
        const char* mLabel;
    };

    struct ProfileObjectStorage
    {
        Int64 mBeginTick;
        Int64 mEndTick;
        UInt64 mFrame;
        UInt16 mThreadBeginCore;
        UInt16 mThreadEndCore;
        UInt16 mThreadID;
        UInt16 mThreadTag;
        const char* mLabel;
        char   mObjectName[64];
        UInt32 mObjectID;
    };

    using ProfileLabelCollection = TVector<ProfileLabelStorage>;
    using ProfileObjectCollection = TVector<ProfileObjectStorage>;

    struct Capture
    {
        ProfileLabelCollection  mScopedLabels;
        ProfileObjectCollection mScopedObjects;
    };

    struct LabelLess
    {
        bool operator()(const char* a, const char* b) const
        {
            return a < b;
        }
    };
    using LabelDB = TMap<const char*, Capture, LabelLess>;

    struct FrameCapture
    {
        Int64  mBeginTick;
        Int64  mEndTick;
        UInt64 mFrame;
    };
    using FrameCaptureCollection = TStackVector<FrameCapture, 256>;
    using RingBufferType = ConcurrentRingBuffer<ProfileCapture>;

    void OnQueueCapture(const ProfileScopeCaptureData& capture);
    void OnQueueCapture(const ProfileScopeObjectCaptureData& capture);
    void Insert(ProfileLabelCollection& collection, const ProfileCapture& capture);
    void Insert(ProfileObjectCollection& collection, const ProfileCapture& capture);
    bool IsRunning();
    void SetIsRunning(bool value);

    Int64 mBeginFrameTick;

    RingBufferType          mRingBuffer;
    Thread                  mThread;
    ThreadFence             mFence;
    LabelDB                 mLabelDB;
    FrameCaptureCollection  mFrameDB;
    RWSpinLock              mDBLock;
    volatile Atomic32       mRunning;

    volatile Atomic64       mNumLabels;
    volatile Atomic64       mNumObjects;
};

} // namespace lf
