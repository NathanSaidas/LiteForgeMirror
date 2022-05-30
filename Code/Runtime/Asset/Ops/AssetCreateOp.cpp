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
#include "AssetCreateOp.h"
#include "Core/IO/TextStream.h"
#include "Core/IO/DependencyStream.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetProcessor.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"

namespace lf {

AssetCreateOp::AssetCreateOp(const AssetPath& assetPath, const AssetObject* object, const AssetTypeInfoCPtr& parent, const AssetOpDependencyContext& context)
: Super(context)
, mCreateState(Validate)
, mAssetPath(assetPath)
, mParentAsset(parent)
, mObject(GetAtomicPointer(object))
, mAssetType()
, mCacheBlockType(CacheBlockType::INVALID_ENUM)
{
    ReportBug(!mAssetPath.Empty() && !mAssetPath.GetDomain().Empty()); // Must have correct asset path.
    ReportBug(mObject && mObject->GetType()); // Must have object and it must have a type
}

AssetOpThread::Value AssetCreateOp::GetExecutionThread() const
{
    return AssetOpThread::MAIN_THREAD;
}

void AssetCreateOp::OnUpdate()
{
    switch (mCreateState)
    {
        case Validate:
        {
            auto result = GetDataController().Find(mAssetPath);
            if (result)
            {
                SetFailed("An asset with that name already exists.");
                mCreateState = Done;
                return;
            }
            mCreateState = AllocateInitialize;
        } break;
        case AllocateInitialize:
        {
            AssetProcessor* processor = GetDataController().GetProcessor(mObject->GetType());
            if (!processor)
            {
                SetFailed("Failed to get asset processor.");
                mCreateState = Done;
                return;
            }

            auto result = GetDataController().CreateType(mAssetPath, processor->GetConcreteType(mObject->GetType()), mParentAsset);
            if (!result)
            {
                SetFailed("Failed to create asset type.");
                mCreateState = Done;
                return;
            }
            mAssetType = result.mType;
            // Since all asset type creation is done on 'main thread' we should never have conflicts.
            Assert(GetDataController().SetOp(mAssetType, AssetOpState::AOS_CREATING));
            mObject->SetAssetType(mAssetType);
            mCreateState = WriteContent;
            mCacheBlockType = CacheBlockType::ToEnum(mAssetType->GetPath());
        } break;
        case WriteContent:
        {
            AssetProcessor* processor = GetDataController().GetProcessor(mObject->GetType());
            if (!processor)
            {
                SetFailed("Failed to get asset processor.");
                mCreateState = Done;
                return;
            }

            MemoryBuffer content;
            if (InvalidEnum(processor->Export(mObject, content, false)))
            {
                SetFailed("Failed to export asset.");
                mCreateState = Done;
                return;
            }

            if (!GetSourceController().Write(content, mAssetPath))
            {
                SetFailed("Failed to write the asset content.");
                mCreateState = Done;
                return;
            }
            mCreateState = WriteCache;
        } break;
        case WriteCache:
        {
            AssetProcessor* processor = GetDataController().GetProcessor(mAssetType);
            if (!processor)
            {
                SetFailed("Failed to get asset processor.");
                mCreateState = Done;
                return;
            }

            MemoryBuffer content;
            if (InvalidEnum(processor->Export(mObject, content, true)))
            {
                SetFailed("Failed to export asset.");
                mCreateState = Done;
                return;
            }

            CacheIndex index;
            if (!GetCacheController().Write(content, mAssetType, index))
            {
                SetFailed("Failed to write the asset content to cache.");
                mCreateState = Done;
                return;
            }
            GetDataController().UpdateCacheIndex(mAssetType, index);
            GetDataController().ClearOp(mAssetType);
            mCreateState = WriteDependencies;
        } break;
        case WriteDependencies:
        {
            DependencyStream::CollectionType weakDeps;
            DependencyStream::CollectionType strongDeps;
            DependencyStream ds(&weakDeps, &strongDeps);
            mObject->Serialize(ds);

            for (const Token& dependencyToken : weakDeps)
            {
                const AssetPath dependencyPath(dependencyToken);
                AssetDataController::QueryResult queryResult = GetDataController().Find(dependencyPath);
                if (queryResult)
                {
                    const bool isWeak = true;
                    GetDataController().AddDependency(queryResult.mType, mAssetType, isWeak);
                }
            }

            for (const Token& dependencyToken : strongDeps)
            {
                const AssetPath dependencyPath(dependencyToken);
                AssetDataController::QueryResult queryResult = GetDataController().Find(dependencyPath);
                if (queryResult)
                {
                    const bool isWeak = false;
                    GetDataController().AddDependency(queryResult.mType, mAssetType, isWeak);
                }
            }

            SetComplete();
            mCreateState = Done;
        } break;
        case Done:
        {

        } break;
        default:
            CriticalAssertMsg("Invalid creation state.");
            break;
    }
}

void AssetCreateOp::OnCancelled()
{
    if (mObject)
    {
        mObject->SetAssetType(nullptr);
    }
}
void AssetCreateOp::OnComplete()
{
    if (mObject)
    {
        mObject->SetAssetType(nullptr);
    }
}

void AssetCreateOp::OnFailure()
{
    Super::OnFailure();
    GetDataController().ClearOp(mAssetType);
    if (mObject)
    {
        mObject->SetAssetType(nullptr);
    }
}

} // namespace lf