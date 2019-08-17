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
#include "AssetBundleController.h"

#include <algorithm>

namespace lf {
    
static const char* ASSET_CATEGORY_PREFIX[] = {
    "_t", // texture
    "_f", // font
    "_a", // audio
    "_m", // mesh
    "_s", // shader
    "_l", // level
    "_x"  // scripting
};

static bool CompareBundleUID(const AssetBundleInfo& a, const AssetBundleInfo& b)
{
    return a.mAssetUID < b.mAssetUID;
}

bool AssetBundleController::CreateBundle(const Token& name, const Token& prefix)
{
    BundlePair pair;
    pair.mName = name;
    pair.mPrefix = prefix;
    auto iter = std::lower_bound(mBundles.begin(), mBundles.end(), pair);
    if (iter == mBundles.end() || iter->mName != pair.mName)
    {
        mBundles.Insert(iter, pair);
        return true;
    }
    return false;
}

bool AssetBundleController::DestroyBundle(const Token& name)
{
    BundlePair pair;
    pair.mName = name;
    auto iter = std::lower_bound(mBundles.begin(), mBundles.end(), pair);
    if (iter != mBundles.end() && iter->mName == pair.mName)
    {
        mBundles.Remove(iter);
        return true;
    }
    return false;
}

bool AssetBundleController::CreateBundleLink(UInt32 assetUID, AssetCategory::Value assetCategory, const Token& bundleName)
{
    AssetBundleInfo bundle(assetUID, assetCategory, bundleName);
    auto iter = std::lower_bound(mBundleLinks.begin(), mBundleLinks.end(), bundle, CompareBundleUID);

    if (iter == mBundleLinks.end() || iter->mAssetUID != assetUID)
    {
        mBundleLinks.Insert(iter, bundle);
        return true;
    }
    return false;
}

bool AssetBundleController::DestroyBundleLink(UInt32 assetUID)
{
    AssetBundleInfo bundle(assetUID, AssetCategory::INVALID_ENUM,  Token());
    auto iter = std::lower_bound(mBundleLinks.begin(), mBundleLinks.end(), bundle, CompareBundleUID);

    if (iter != mBundleLinks.end() && iter->mAssetUID == assetUID)
    {
        mBundleLinks.Remove(iter);
        return true;
    }
    return false;
}

AssetBundleInfo AssetBundleController::FindBundleInfo(UInt32 assetUID) const
{
    AssetBundleInfo bundle(assetUID, AssetCategory::INVALID_ENUM, Token());
    auto iter = std::lower_bound(mBundleLinks.begin(), mBundleLinks.end(), bundle, CompareBundleUID);
    if (iter != mBundleLinks.end() && iter->mAssetUID == assetUID)
    {
        return *iter;
    }
    return AssetBundleInfo();
}
Token AssetBundleController::GetCacheName(const AssetBundleInfo& bundleInfo) const
{
    if (Invalid(bundleInfo.mAssetUID) || bundleInfo.mBundleName.Empty())
    {
        return Token();
    }
    Token prefix = GetPrefix(bundleInfo.mBundleName);
    if (prefix.Empty())
    {
        return Token();
    }

    String result(prefix.CStr());

    // Append category (if required)
    if(bundleInfo.mAssetCategory != AssetCategory::AC_SERIALIZED_OBJECT && ValidEnum(bundleInfo.mAssetCategory))
    {
        result.Append(ASSET_CATEGORY_PREFIX[bundleInfo.mAssetCategory]);
    }
    return Token(StrToLower(result));
}
Token AssetBundleController::GetPrefix(const Token& bundle) const
{
    BundlePair pair;
    pair.mName = bundle;
    auto iter = std::lower_bound(mBundles.begin(), mBundles.end(), pair);
    if (iter != mBundles.end() && iter->mName == bundle)
    {
        return iter->mPrefix;
    }
    return Token();
}

} // namespace lf