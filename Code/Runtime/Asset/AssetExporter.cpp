#include "AssetExporter.h"
#include "Core/Platform/FileSystem.h"

#include <algorithm>

namespace lf {
    
AssetExportManifest AssetExporter::CreateManifest()
{
    AssetExportManifest manifest;
    for (const AssetExportPackage& pkg : mPackages)
    {
        String cacheTitle = GetBundleExportName(pkg.mBundle);
        if (cacheTitle.Empty())
        {
            continue;
        }

        if (!pkg.mTag.Empty())
        {
            cacheTitle.Append('_').Append(static_cast<char>(tolower(pkg.mTag.First())));
        }

        for (const String& asset : pkg.mAssets)
        {
            if (std::find(pkg.mBlacklist.begin(), pkg.mBlacklist.end(), asset) != pkg.mBlacklist.end())
            {
                continue;
            }

            String extension = FileSystem::PathGetExtension(asset);
            String cacheBlock = cacheTitle;
            AssetExportInfo info;

            if (extension == "png")
            {
                cacheBlock.Append('_').Append('t');
                info.mTypeName = "lf::GfxTexture";
            }
            else if (extension == "fbx")
            {
                cacheBlock.Append('_').Append('m');
                info.mTypeName = "lf::GfxMesh";
            }
            else if (extension == "wav")
            {
                cacheBlock.Append('_').Append('a');
                info.mTypeName = "lf::Sound";
            }
            else if (extension == "lua")
            {
                cacheBlock.Append('_').Append('s');
                info.mTypeName = "lf::Script";
            }

            info.mAssetName = asset;
            info.mCacheFile = cacheBlock;
            info.mHash = "ChickenHash";
            info.mVersion = 0;
            manifest.mExports.Add(info);
        }
    }
    return manifest;
}

String AssetExporter::GetBundleExportName(const String& name)
{
    auto iter = std::find_if(mBundles.begin(), mBundles.end(), [&name](const AssetBundleExportName& bundle) { return bundle.mFullName == name; });
    if (iter != mBundles.end())
    {
        return iter->mExportName;
    }
    return String();
}

}