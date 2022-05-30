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
#include "SaveDomainOp.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Utility/Log.h"
#include "Runtime/Asset/AssetCommon.h"
#include "Runtime/Asset/AssetTypeMap.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"

namespace lf {

SaveDomainOp::SaveDomainOp(const String& domain, const String& cachePath, const AssetOpDependencyContext& context)
    : Super(context)
    , mDomain(domain)
    , mCachePath(cachePath)
{}
AssetOpThread::Value SaveDomainOp::GetExecutionThread() const
{
    return AssetOpThread::MAIN_THREAD;
}
void SaveDomainOp::OnUpdate()
{
    if (mDomain.Empty())
    {
        SetFailed("Invalid argument 'domain'");
        return;
    }
    if (mCachePath.Empty())
    {
        SetFailed("Invalid argument 'cache path'");
        return;
    }

    AssetTypeMap typeMap;
    GetDataController().WriteDomain(mDomain, typeMap);

    String path;
    if (StrToLower(mDomain) == "engine")
    {
        path = FileSystem::PathJoin(FileSystem::PathJoin(mCachePath, "Content"), "cache.typemap");
    }
    else
    {
        path = FileSystem::PathJoin(FileSystem::PathJoin(mCachePath, "Mods\\" + mDomain), "modinfo.typemap");
    }
    gSysLog.Info(LogMessage("Saving domain ") << mDomain << " : " << path << "...");
    typeMap.Write(AssetTypeMap::JSON, path);
    SetComplete();
}

SaveDomainCacheOp::SaveDomainCacheOp(const String& domain, const AssetOpDependencyContext& context)
: Super(context)
, mDomain(domain)
{

}
AssetOpThread::Value SaveDomainCacheOp::GetExecutionThread() const { return AssetOpThread::MAIN_THREAD; }

void SaveDomainCacheOp::OnUpdate()
{
    if (mDomain.Empty())
    {
        SetFailed("Invalid argument 'domain'");
        return;
    }

    GetCacheController().SaveDomain(mDomain);
    SetComplete();
}

} // namespace 