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
#include "AssetDeleteOp.h"
#include "Core/IO/BinaryStream.h"
#include "Core/IO/DependencyStream.h"
#include "Runtime/Asset/AssetTypeInfo.h"
#include "Runtime/Asset/AssetProcessor.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
// #include "Runtime/Asset/Controllers/AssetOpController.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"
#include "Runtime/Reflection/ReflectionMgr.h"


namespace lf {

AssetDeleteOp::AssetDeleteOp(const AssetTypeInfoCPtr& type, const AssetOpDependencyContext& context)
: Super(context)
, mDeleteState(Validate)
, mType(type)
, mLocked(false)
{}

AssetOpThread::Value AssetDeleteOp::GetExecutionThread() const
{
    return AssetOpThread::MAIN_THREAD;
}

void AssetDeleteOp::OnUpdate()
{
    switch (mDeleteState)
    {
        case Validate:
        {
            if (mType == nullptr)
            {
                SetFailed("Invalid argument 'type'");
                return;
            }

            if (mType->GetParent() == nullptr)
            {
                SetFailed("Cannot delete concrete type!");
                return;
            }
            mDeleteState = AcquireWriteLock;
        } break;
        case AcquireWriteLock:
        {
            if (mType->GetLock().TryAcquireWrite())
            {
                mLocked = true;
                mDeleteState = UpdateDataController;
            }
        } break;
        case UpdateDataController:
        {
            DependencyStream::CollectionType weakDeps;
            DependencyStream::CollectionType strongDeps;
            DependencyStream ds(&weakDeps, &strongDeps);

            // Build a temporary object
            MemoryBuffer buffer;
            SizeT bufferSize;
            if (GetCacheController().QuerySize(mType, bufferSize))
            {
                CacheIndex cacheIndex;
                buffer.Allocate(bufferSize, 1);
                if (GetCacheController().Read(buffer, mType, cacheIndex))
                {
                    AssetProcessor* processor = GetDataController().GetProcessor(mType);
                    const Type* prototypeType = processor->GetPrototypeType(mType->GetConcreteType());
                    CriticalAssert(prototypeType != nullptr);

                    AssetObjectAtomicPtr object = GetReflectionMgr().CreateAtomic<AssetObject>(prototypeType);
                    if (object)
                    {
                        object->SetAssetType(mType);
                        processor->PrepareAsset(object, buffer, AssetLoadFlags::LF_ACQUIRE | AssetLoadFlags::LF_IMMEDIATE_PROPERTIES);
                        object->Serialize(ds);
                    }
                }
            }
            
            // Remove dependencies
            for (const Token& dependencyToken : weakDeps)
            {
                const AssetPath dependencyPath(dependencyToken);
                AssetDataController::QueryResult queryResult = GetDataController().Find(dependencyPath);
                if (queryResult)
                {
                    const bool isWeak = true;
                    GetDataController().RemoveDependency(queryResult.mType, mType, isWeak);
                }
            }

            for (const Token& dependencyToken : strongDeps)
            {
                const AssetPath dependencyPath(dependencyToken);
                AssetDataController::QueryResult queryResult = GetDataController().Find(dependencyPath);
                if (queryResult)
                {
                    const bool isWeak = false;
                    GetDataController().RemoveDependency(queryResult.mType, mType, isWeak);
                }
            }

            if (GetDataController().DeleteType(mType))
            {
                mDeleteState = UpdateSourceController;
            }
            else
            {
                SetFailed("Failed to delete type from data controller.");
                return;
            }
        } break;
        case UpdateSourceController:
        {
            if (GetSourceController().Delete(mType->GetPath()))
            {
                mDeleteState = UpdateCacheController;
            }
            else
            {
                SetFailed("Failed to delete source file from source controller.");
                return;
            }
        } break;
        case UpdateCacheController:
        {
            // We can have an issue where ObjectID/BlobID might not point to a correct object in the cache
            // In this case we should invalidate it, then delete the type.

            // CORRUPTION CHECK:
            bool corrupted = false;
            CacheIndex index;
            CacheObject object;
            if (!GetCacheController().FindIndex(mType, index))
            {
                corrupted = true;
                if (GetCacheController().FindObject(mType, object, index))
                {
                    // Cache has been deleted, best to delete the object and 'update cache data'

                    // Delete object by UID. (Other types shouldn't share same UID, unique to typemap)
                    GetCacheController().DeleteObject(mType, object, index);
                }
            }
            else 
            {
                CacheIndex dummyIndex;
                if (!GetCacheController().FindObject(mType, object, dummyIndex))
                {
                    GetCacheController().DeleteIndex(mType, index);
                    corrupted = true;
                }
            }

            // Only do this if all the data is correct.
            if (corrupted || GetCacheController().Delete(mType))
            {
                index = mType->GetCacheIndex();
                index.mObjectID = INVALID32;
                index.mBlobID = INVALID32;
                GetDataController().UpdateCacheIndex(mType, index);
                mDeleteState = ReleaseWriteLock;
            }
            else
            {
                SetFailed("Failed to delete source file from source controller.");
                return;
            }
        } break;
        case ReleaseWriteLock:
        {
            Unlock();
            mDeleteState = Done;
        } break;
        case Done:
        {
            SetComplete();
        } break;
    }
}

void AssetDeleteOp::OnCancelled()
{
    Unlock();
}
void AssetDeleteOp::OnFailure()
{
    Unlock();
}
void AssetDeleteOp::OnComplete()
{
    Unlock();
}
void AssetDeleteOp::Unlock()
{
    if (mLocked)
    {
        mType->GetLock().ReleaseWrite();
        mLocked = false;
    }
}

} // namespace lf