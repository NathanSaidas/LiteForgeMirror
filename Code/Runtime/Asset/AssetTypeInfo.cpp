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
#include "AssetTypeInfo.h"
#include "Core/Reflection/Type.h"

namespace lf {

AssetTypeInfo::AssetTypeInfo()
: mPath()
, mParent(nullptr)
, mConcreteType(nullptr)
, mCacheIndex()
, mHandle(nullptr)
, mInstances(nullptr)
, mController(nullptr)
, mModifyHash()
, mModifyDate()
, mLoadState(AssetLoadState::ALS_UNLOADED)
, mOpState(AssetOpState::AOS_IDLE)
, mWeakReferences(0)
, mStrongReferences(0)
, mRefs(0)
{
}

AssetTypeInfo::AssetTypeInfo(AssetTypeInfo&& other)
: mPath(std::move(other.mPath))
, mParent(other.mParent)
, mConcreteType(other.mConcreteType)
, mCacheIndex(other.mCacheIndex)
, mHandle(other.mHandle)
, mInstances(other.mInstances)
, mController(other.mController)
, mModifyHash(other.mModifyHash)
, mModifyDate(other.mModifyDate)
, mLoadState(other.mLoadState)
, mOpState(other.mOpState)
, mWeakReferences(other.mWeakReferences)
, mStrongReferences(other.mStrongReferences)
, mRefs(other.mRefs)
// , mOpStateLock(std::move(other.mOpStateLock))
{
    other.mParent = nullptr;
    other.mConcreteType = nullptr;
    other.mCacheIndex = CacheIndex();
    other.mHandle = nullptr;
    other.mInstances = nullptr;
    other.mController = nullptr;
    other.mModifyHash = AssetHash();
    other.mModifyDate = DateTime();
    other.mLoadState = AssetLoadState::ALS_UNLOADED;
    other.mOpState = AssetOpState::AOS_IDLE;
    other.mWeakReferences = 0;
    other.mStrongReferences = 0;
    other.mRefs = 0;
}

AssetTypeInfo& AssetTypeInfo::operator=(AssetTypeInfo&& other)
{
    mPath = std::move(other.mPath);
    mParent = other.mParent; other.mParent = nullptr;
    mConcreteType = other.mConcreteType; other.mConcreteType = nullptr;
    mCacheIndex = other.mCacheIndex; other.mCacheIndex = CacheIndex();
    mHandle = other.mHandle; other.mHandle = nullptr;
    mInstances = other.mInstances; other.mInstances = nullptr;
    mController = other.mController; other.mController = nullptr;
    mModifyHash = other.mModifyHash; other.mModifyHash = AssetHash();
    mModifyDate = other.mModifyDate; other.mModifyDate = DateTime();
    mLoadState = other.mLoadState; other.mLoadState = AssetLoadState::ALS_UNLOADED;
    mOpState = other.mOpState; other.mOpState = AssetOpState::AOS_IDLE;
    mWeakReferences = other.mWeakReferences; other.mWeakReferences = 0;
    mStrongReferences = other.mStrongReferences; other.mStrongReferences = 0;
    mRefs = other.mRefs; other.mRefs = 0;
    // mOpStateLock = std::move(other.mOpStateLock);

    return *this;
}

bool AssetTypeInfo::IsA(const AssetTypeInfo* type) const
{
    const AssetTypeInfo* iter = this;
    while (iter)
    {
        if (iter == type)
        {
            return true;
        }
        iter = iter->GetParent();
    }
    return mConcreteType->IsA(type->GetConcreteType());
}

} // namespace lf