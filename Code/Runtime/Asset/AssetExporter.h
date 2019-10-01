#ifndef LF_RUNTIME_ASSET_EXPORTER_H
#define LF_RUNTIME_ASSET_EXPORTER_H

#include "Runtime/Asset/AssetTypes.h"

namespace lf {

struct AssetBundleExportName
{
    AssetBundleExportName() : mFullName(), mExportName() {}
    AssetBundleExportName(const String& fullName, const String& exportName) : mFullName(fullName), mExportName(exportName) {}
    // Full name of the bundle eg 'GameBase'
    String mFullName;
    // Abbreviation eg 'gb'
    String mExportName;
};

struct AssetExport
{
    Token  mFullName;
    String mCacheExtension;
};

struct AssetExportPackage
{
    // If not empty create a unique bin for the package'
    String mTag;
    // The bundle the package belongs to (for bins)
    String mBundle;
    // List of assets being exported with this package
    TArray<String> mAssets;
    // List of assets rejected from this package
    TArray<String> mBlacklist;
};

struct AssetExportInfo
{
    String mAssetName;
    String mCacheFile;
    String mTypeName;
    String mHash;
    UInt16 mVersion;
};

struct AssetExportManifest
{
    TArray<AssetExportInfo> mExports;
};

class LF_RUNTIME_API AssetExporter
{
public:
    void AddBundle(const AssetBundleExportName& bundle) { mBundles.Add(bundle); }
    void AddPackage(const AssetExportPackage& package) { mPackages.Add(package); }

    AssetExportManifest CreateManifest();

private:
    String GetBundleExportName(const String& name);

    TArray<AssetBundleExportName> mBundles;
    TArray<AssetExportPackage>    mPackages;
};

}

#endif // LF_RUNTIME_ASSET_EXPORTER_H