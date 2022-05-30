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
#include "AssetImportOp.h"
#include "Core/Common/Assert.h"
#include "Core/IO/TextStream.h"
#include "Runtime/Asset/AssetProcessor.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"

namespace lf {

AssetImportOp::AssetImportOp(const AssetPath& assetPath, bool allowRawData, const AssetOpDependencyContext& context)
: Super(context)
, mImportState(Validate)
, mAssetPath(assetPath)
, mAllowRawData(allowRawData)
, mCurrentAsset()
, mCurrentAssetType()
, mDependencies()
, mLockedAssets()
{}

AssetOpThread::Value AssetImportOp::GetExecutionThread() const { return AssetOpThread::MAIN_THREAD; }

void AssetImportOp::OnUpdate()
{
    switch (mImportState)
    {
        case Validate:
        {
            auto result = GetDataController().Find(GetCurrentAssetPath());
            if (result)
            {
                SetFailed("The asset already exists.");
                mImportState = Done;
                return;
            }

            if (!GetSourceController().QueryExist(GetCurrentAssetPath()))
            {
                SetFailed("An asset at that path does not exist.");
                mImportState = Done;
                return;
            }

            mImportState = AllocateInitialize;

        } break;
        case AllocateInitialize:
        {
            AssetProcessor* processor = nullptr;
            const String extension = GetCurrentAssetPath().GetExtension();
            if (extension == "lob")
            {
                String fullpath = GetSourceController().GetFullPath(GetCurrentAssetPath());
                TextStream ts(Stream::FILE, fullpath, Stream::SM_READ);
                if (ts.GetMode() == Stream::SM_READ && ts.GetObjectCount() >= 1)
                {
                    const String& super = ts.GetObjectSuper(0);
                    auto queryResult = GetDataController().Find(AssetPath(super));
                    if (queryResult)
                    {
                        processor = GetDataController().GetProcessor(queryResult.mType);
                    }
                    else
                    {
                        SetFailed("Failed to import asset. Could not find type " + super);
                        mImportState = Done;
                        return;
                    }
                }
                else
                {
                    SetFailed("Failed to import asset. Could not read types from object file.");
                    mImportState = Done;
                    return;
                }
            }
            else
            {
                processor = GetDataController().GetProcessor(GetCurrentAssetPath());
            }

            if (!processor)
            {
                SetFailed("Cannot import asset there is no processor.");
                mImportState = Done;
                return;
            }
            AssetImportResult result = processor->Import(GetCurrentAssetPath());
            // If we failed to import because there are dependencies that must be imported first
            // add them to the list of imports and try again
            if (!result.mDependencies.empty())
            {
                mDependencies.insert(mDependencies.end(), result.mDependencies.begin(), result.mDependencies.end());
                mImportState = Validate;
                return;
            }

            auto type = GetDataController().CreateType(GetCurrentAssetPath(), result.mConcreteType, result.mParentType);
            if (!type)
            {
                SetFailed("Failed to import asset, could not create a type.");
                mImportState = Done;
                return;
            }

            mCurrentAssetType = type.mType;
            // Since we run single threaded we should be able to set this op always
            Assert(GetDataController().SetOp(mCurrentAssetType, AssetOpState::AOS_CREATING));
            mLockedAssets.push_back(mCurrentAssetType);
            mCurrentAsset = result.mObject;
            mCurrentAsset->SetAssetType(type.mType);
            mImportState = WriteCache;
        } break;
        case WriteCache:
        {
            AssetProcessor* processor = GetDataController().GetProcessor(mCurrentAssetType);
            if (!processor)
            {
                SetFailed("Failed to get asset processor.");
                mImportState = Done;
                return;
            }

            MemoryBuffer content;
            if (InvalidEnum(processor->Export(mCurrentAsset, content, true)))
            {
                SetFailed("Failed to export asset.");
                mImportState = Done;
                return;
            }

            CacheIndex index;
            if (!GetCacheController().Write(content, mCurrentAssetType, index))
            {
                SetFailed("Failed to write the asset content to cache.");
                mImportState = Done;
                return;
            }
            GetDataController().UpdateCacheIndex(mCurrentAssetType, index);
            GetDataController().ClearOp(mCurrentAssetType);

            mCurrentAsset = NULL_PTR;
            mCurrentAssetType = nullptr;
            if (mDependencies.empty())
            {
                SetComplete();
                mImportState = Done;
            }
            else
            {
                mDependencies.erase(mDependencies.rend().base());
                mImportState = Validate;
            }
        } break;
        case Done:
        {

        } break;
        default:
            CriticalAssertMsg("Invalid import state.");
            break;
    }
}

void AssetImportOp::OnFailure()
{
    Super::OnFailure();
    for (const AssetTypeInfoCPtr& assetTypeInfo : mLockedAssets)
    {
        GetDataController().ClearOp(assetTypeInfo);
    }
    mLockedAssets.clear();
}

const AssetPath& AssetImportOp::GetCurrentAssetPath() const
{
    return mDependencies.empty() ? mAssetPath : mDependencies.back();
}


} // namespace lf