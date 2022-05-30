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
#include "UpdateCacheOp.h"
#include "Runtime/Asset/AssetProcessor.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"
#include "Runtime/Reflection/ReflectionMgr.h"


namespace lf {

UpdateCacheOp::UpdateCacheOp(const AssetTypeInfoCPtr& type, const AssetOpDependencyContext& context)
: Super(context)
, mState(Validate)
, mLocked(false)
, mType(type)
{}

AssetOpThread::Value UpdateCacheOp::GetExecutionThread() const
{
    // TODO: Could become ASYNC
    return AssetOpThread::MAIN_THREAD;
}

void UpdateCacheOp::OnUpdate()
{
    // TODO: Make async
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
            mState = AcquireLock;
        } break;
        case AcquireLock:
        {
            if (mType->GetLock().TryAcquireWrite())
            {
                mLocked = true;
                mState = UpdateCache;
            }
        } break;
        case UpdateCache:
        {
            AssetProcessor* processor = GetDataController().GetProcessor(mType);
            if (!processor)
            {
                SetFailed("Failed to get asset processor.");
                return;
            }

            auto result = processor->Import(mType->GetPath());
            result.mObject->SetAssetType(mType);

            MemoryBuffer content;
            if (InvalidEnum(processor->Export(result.mObject, content, true)))
            {
                SetFailed("Failed to export asset.");
                return;
            }

            CacheIndex index;
            if (!GetCacheController().Write(content, mType, index))
            {
                SetFailed("Failed to write the asset content to cache.");
                return;
            }
            GetDataController().UpdateCacheIndex(mType, index);
            mState = ReleaseLock;
        } break;
        case ReleaseLock:
        {
            Unlock();
            SetComplete();
            mState = Done;
        } break;
        case Done:
        {
            
        } break;
    }
}

void UpdateCacheOp::OnCancelled()
{
    Unlock();
}
void UpdateCacheOp::OnFailure()
{
    Unlock();
}
void UpdateCacheOp::OnComplete()
{
    Unlock();
}
void UpdateCacheOp::Unlock()
{
    if (mLocked)
    {
        mType->GetLock().ReleaseWrite();
        mLocked = false;
    }
}

} // namespace lf