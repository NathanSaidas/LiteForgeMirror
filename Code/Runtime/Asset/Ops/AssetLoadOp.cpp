// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "Runtime/PCH.h"
#include "AssetLoadOp.h"
#include "Core/IO/BinaryStream.h"
#include "Core/IO/TextStream.h"
#include "Core/IO/DependencyStream.h"
#include "Core/Utility/Log.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetProcessor.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"

namespace lf {

bool HasProperties(AssetLoadFlags::Value flags)
{
    return (flags & AssetLoadFlags::LF_IMMEDIATE_PROPERTIES) > 0;
}

bool IsRecursive(AssetLoadFlags::Value flags)
{
    return (flags & AssetLoadFlags::LF_RECURSIVE_PROPERTIES) > 0;
}

bool IsFullLoad(AssetLoadFlags::Value flags)
{
    return (flags & (AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES)) == 0;
}

AssetLoadState::Value LoadFlagsToState(AssetLoadFlags::Value flags)
{
    

    AssetLoadState::Value state = AssetLoadState::ALS_UNLOADED;
    if(HasProperties(flags))
    {
        state = AssetLoadState::ALS_SERIALIZED_PROPERTIES;
    }
    if (IsRecursive(flags))
    {
        state = AssetLoadState::ALS_SERIALIZED_DEPENDENCIES;
    }
    if (IsFullLoad(flags))
    {
        state = AssetLoadState::ALS_LOADED;
    }
    return state;
}

bool CompareLoadState(AssetLoadState::Value current, AssetLoadState::Value target)
{
    if (target == AssetLoadState::ALS_SERIALIZED_PROPERTIES)
    {
        return current == AssetLoadState::ALS_SERIALIZED_PROPERTIES
            || current == AssetLoadState::ALS_SERIALIZED_DEPENDENCIES
            || current == AssetLoadState::ALS_LOADED;
    }
    else if (target == AssetLoadState::ALS_SERIALIZED_DEPENDENCIES)
    {
        return current == AssetLoadState::ALS_SERIALIZED_DEPENDENCIES
            || current == AssetLoadState::ALS_LOADED;
    }
    return current == target;
}

AssetLoadOp::AssetLoadOp(const AssetTypeInfoCPtr& assetType, AssetLoadFlags::Value flags, bool loadCache, const AssetOpDependencyContext& context)
: Super(context)
, mState(Validate)
, mType(assetType)
, mFlags(flags)
, mLoadCache(loadCache)
, mLocked(false)
, mHandle(nullptr)
, mLatencyState(Validate)
, mLatencyTimer()
, mLatencyTimings{0.0f}
{
}

AssetOpThread::Value AssetLoadOp::GetExecutionThread() const { return AssetOpThread::WORKER_THREAD; }

void AssetLoadOp::OnUpdate()
{
    switch (mState)
    {
        case Validate:
        {
            if (!mType)
            {
                SetFailed("Invalid argument 'AssetType'");
                return;
            }
            if (mType->GetLoadState() == AssetLoadState::ALS_DELETED)
            {
                SetFailed("AssetType is deleted.");
                return;
            }
            // todo: More elaborate check
            if (mType->GetLoadState() == AssetLoadState::ALS_LOADED)
            {
                SetComplete();
                return;
            }
            mState = AcquireLock;

            mLatencyState = Validate;
            mLatencyTimer.Start();
        } break;
        case AcquireLock:
        {
            mLatencyTimer.Stop();
            mLatencyTimings[mLatencyState] += mLatencyTimer.GetDelta();
            const bool acquire = (mFlags & AssetLoadFlags::LF_ACQUIRE) > 0;

            if (acquire)
            {
                if(mType->GetLock().TryAcquireRead())
                {
                    mLocked = true;
                    mState = ReleaseLock;
                }
            }
            else
            {
                if (mType->GetLock().TryAcquireWrite())
                {
                    mLocked = true;
                    mState = CreatePrototype;
                }
            }

            mLatencyState = AcquireLock;
            mLatencyTimer.Start();
        } break;
        case CreatePrototype:
        {
            mLatencyTimer.Stop();
            mLatencyTimings[mLatencyState] += mLatencyTimer.GetDelta();

            const bool acquire = (mFlags & AssetLoadFlags::LF_ACQUIRE) > 0;
            ReportBug(acquire == false);

            if (!AssetLoadState::IsCreated(mType->GetLoadState()))
            {
                if (!GetDataController().CreatePrototype(mType, mHandle) || !mHandle)
                {
                    SetFailed("Failed to create prototype for asset.");
                    return;
                }
            }

            const bool loadImmediate = (mFlags & AssetLoadFlags::LF_IMMEDIATE_PROPERTIES) > 0;
            const bool loadRecursive = (mFlags & AssetLoadFlags::LF_RECURSIVE_PROPERTIES) > 0;
            if (loadImmediate || loadRecursive)
            {
                mState = LoadBinary;
                GetDataController().SetLoadState(mType, AssetLoadState::ALS_CREATED);
            }
            else
            {
                mState = ReleaseLock;
            }

            mLatencyState = CreatePrototype;
            mLatencyTimer.Start();
        } break;
        case LoadBinary:
        {
            mLatencyTimer.Stop();
            mLatencyTimings[mLatencyState] += mLatencyTimer.GetDelta();

            // TODO: If a write operation was happening on the file where the content is cached then this would fail.
            // We should acquire a read-file lock on the cache block itself before reading.

            if (!AssetLoadState::IsPropertyLoaded(mType->GetLoadState()))
            {
                // Load the data from the cache or the source.
                MemoryBuffer buffer;
                Timer loadTimer;
                loadTimer.Start();
                const bool success = mLoadCache ? ReadCache(buffer) : ReadSource(buffer);
                loadTimer.Stop();
                mLoadTime = loadTimer.GetDelta();
                if (!success)
                {
                    return;
                }

                // Hand the data over to the processor to actually load it into the object correctly.
                AssetProcessor* processor = GetDataController().GetProcessor(mType);
                processor->PrepareAsset(mHandle->mPrototype, buffer, mFlags);
                GetDataController().SetLoadState(mType, AssetLoadState::ALS_SERIALIZED_PROPERTIES);
            }

            const bool loadRecursive = (mFlags & AssetLoadFlags::LF_RECURSIVE_PROPERTIES) > 0;
            if (!loadRecursive)
            {
                mState = ReleaseLock;
                return;
            }

            if (!AssetLoadState::IsDependencyLoaded(mType->GetLoadState()))
            {
                GetDataController().SetLoadState(mType, AssetLoadState::ALS_SERIALIZED_DEPENDENCIES);
                TVector<Token> weak;
                TVector<Token> strong;
                DependencyStream ds(&weak, &strong);
                mHandle->mPrototype->Serialize(ds);
                ds.Close();

                if (!strong.empty())
                {
                    for (const Token& token : strong)
                    {
                        AssetDataController::QueryResult result = GetDataController().Find(AssetPath(token));
                        if (!result)
                        {
                            SetFailed("Failed to query dependency.");
                            return;
                        }

                        if (result.mType->GetLoadState() != AssetLoadState::ALS_SERIALIZED_DEPENDENCIES)
                        {
                            auto op = MakeConvertibleAtomicPtr<AssetLoadOp>(result.mType, mFlags, mLoadCache, GetContext());
                            op->Start();
                            WaitFor(op);
                            mDependencies.push_back(result.mType);
                        }
                    }

                    if (!mDependencies.empty())
                    {
                        mState = WaitingForDependencies;
                        mLatencyState = WaitingForDependencies;
                        mLatencyTimer.Start();
                        return;
                    }
                }
            }
            GetDataController().SetLoadState(mType, AssetLoadState::ALS_LOADED);
            mState = LoadComplete;

            mLatencyState = LoadBinary;
            mLatencyTimer.Start();
        } break;
        case WaitingForDependencies:
        {
            
        } break;
        case LoadComplete:
        {
            mLatencyTimer.Stop();
            mLatencyTimings[mLatencyState] += mLatencyTimer.GetDelta();

            AssetProcessor* processor = GetDataController().GetProcessor(mType);
            processor->OnLoadAsset(mHandle->mPrototype);
            mState = ReleaseLock;
            mLatencyState = LoadComplete;
            mLatencyTimer.Start();
        } break;
        case ReleaseLock:
        {
            mLatencyTimer.Stop();
            mLatencyTimings[mLatencyState] += mLatencyTimer.GetDelta();

            Unlock();
            SetComplete();
            mState = Done;

            mLatencyState = ReleaseLock;
            mLatencyTimer.Start();
        } break;
        case Done:
        {

        } break;
    }
}

void AssetLoadOp::OnCancelled()
{
    Unlock();
    LogStats();
}
void AssetLoadOp::OnFailure()
{
    Unlock();
    LogStats();
}
void AssetLoadOp::OnComplete()
{
    Unlock();
    LogStats();
}

void AssetLoadOp::OnWaitComplete(AssetOp* op)
{
    if (!op || op->IsFailed())
    {
        SetFailed("Failed to load dependency.");
    }
    else
    {
        auto it = std::find(mDependencies.begin(), mDependencies.end(), static_cast<AssetLoadOp*>(op)->mType);
        if (it != mDependencies.end())
        {
            mDependencies.swap_erase(it);
        }

        if (mDependencies.empty())
        {
            GetDataController().SetLoadState(mType, AssetLoadState::ALS_LOADED);
            mState = LoadComplete;
        }
    }
}

bool AssetLoadOp::ReadCache(MemoryBuffer& buffer)
{
    Timer timer;
    SizeT size;

    timer.Start();
    if (!GetCacheController().QuerySize(mType, size))
    {
        mLoadSizeTime = timer.PeekDelta();
        SetFailed("Failed to query the size of the type.");
        return false;
    }
    mLoadSizeTime = timer.PeekDelta();
    buffer.Allocate(size, 1);
    buffer.SetSize(size);

    CacheIndex cacheIndex;
    timer.Start();
    if (!GetCacheController().Read(buffer, mType, cacheIndex))
    {
        mLoadDataTime = timer.PeekDelta();
        SetFailed("Failed to read from the cache controller");
        return false;
    }
    mLoadDataTime = timer.PeekDelta();

    return true;
}
bool AssetLoadOp::ReadSource(MemoryBuffer& buffer)
{
    SizeT size;
    if (!GetSourceController().QuerySize(mType->GetPath(), size))
    {
        SetFailed("Failed to query the size of the type.");
        return false;
    }
    buffer.Allocate(size, 1);
    buffer.SetSize(size);

    if (!GetSourceController().Read(buffer, mType->GetPath()))
    {
        SetFailed("Failed to read from the source controller.");
        return false;
    }
    return true;
}

void AssetLoadOp::Unlock()
{
    if (mLocked)
    {
        const bool acquire = (mFlags & AssetLoadFlags::LF_ACQUIRE);
        if (acquire)
        {
            mType->GetLock().ReleaseRead();
        }
        else
        {
            mType->GetLock().ReleaseWrite();
        }
        mLocked = false;
    }
}

void AssetLoadOp::LogStats()
{
    auto timeFormat = [](Float64 value)
    {
        return ToMicroseconds(TimeTypes::Seconds(value)).mValue;
    };

    auto msg = LogMessage("Loading Stats: [") << mType->GetPath().CStr()  << "]";
    msg << "\n  Latency:";
    msg << "\n               Validate:" << timeFormat(mLatencyTimings[Validate]) << " (us)";
    msg << "\n            AcquireLock:" << timeFormat(mLatencyTimings[AcquireLock]) << " (us)";
    msg << "\n        CreatePrototype:" << timeFormat(mLatencyTimings[CreatePrototype]) << " (us)";
    msg << "\n             LoadBinary:" << timeFormat(mLatencyTimings[LoadBinary]) << " (us)";
    msg << "\n    WaitingDependencies:" << timeFormat(mLatencyTimings[WaitingForDependencies]) << " (us)";
    msg << "\n            ReleaseLock:" << timeFormat(mLatencyTimings[ReleaseLock]) << " (us)";
    msg << "\n  Other:";
    msg << "\n                   Load:" << timeFormat(mLoadTime) << " (us)";
    msg << "\n               LoadData:" << timeFormat(mLoadDataTime) << " (us)";
    msg << "\n               LoadSize:" << timeFormat(mLoadSizeTime) << " (us)";

    gSysLog.Info(msg);

}


} // namespace