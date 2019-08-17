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
#ifndef LF_RUNTIME_ASSET_BUNDLE_CONTROLLER_H
#define LF_RUNTIME_ASSET_BUNDLE_CONTROLLER_H

#include "Runtime/Asset/AssetTypes.h"

namespace lf {

class AssetBundleController
{
public:
    bool CreateBundle(const Token& name, const Token& prefix);
    bool DestroyBundle(const Token& name);

    bool CreateBundleLink(UInt32 assetUID, AssetCategory::Value assetCategory, const Token& bundleName);
    bool DestroyBundleLink(UInt32 assetUID);

    AssetBundleInfo FindBundleInfo(UInt32 assetUID) const;
    Token           GetCacheName(const AssetBundleInfo& bundleInfo) const;
private:
    struct BundlePair
    {
        bool operator<(const BundlePair& other) const
        {
            return mName.CStr() < other.mName.CStr();
        }

        Token mName;
        Token mPrefix;
    };
    Token GetPrefix(const Token& bundle) const;

    TArray<BundlePair> mBundles;
    TArray<AssetBundleInfo> mBundleLinks;
};

}

#endif // LF_RUNTIME_ASSET_BUNDLE_CONTROLLER_H