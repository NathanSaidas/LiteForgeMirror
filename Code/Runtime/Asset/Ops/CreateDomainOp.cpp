// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "CreateDomainOp.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Utility/Log.h"
#include "Runtime/Asset/AssetTypeMap.h"
#include "Runtime/Asset/AssetPath.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"

namespace lf {

    CreateDomainOp::CreateDomainOp(const AssetPath& domain, const String& contentCachePath, const String& contentSourcePath, const AssetOpDependencyContext& context)
    : Super(context)
    , mDomain(domain.GetDomain())
    , mContentCachePath(contentCachePath)
    , mContentSourcePath(contentSourcePath)
    {
    }
    
    AssetOpThread::Value CreateDomainOp::GetExecutionThread() const
    {
        return AssetOpThread::MAIN_THREAD;
    }
    
    void CreateDomainOp::OnUpdate()
    {
        if (GetDataController().HasDomain(mDomain))
        {
            SetComplete();
            return;
        }

        String path;
        String cacheDir;
        String sourceDir;

        // TODO [Nathan] When we start loading actual mods we'll need to parse modinfo.json for the typemap format. 'TypeMapFormat': [Json|Binary]
        if (StrToLower(mDomain) == "engine")
        {
            path = FileSystem::PathJoin(FileSystem::PathJoin(mContentCachePath, "Content"), "cache.typemap");
            cacheDir = FileSystem::PathJoin(mContentCachePath, "Content");
            sourceDir = mContentSourcePath;
        }
        else
        {
            path = FileSystem::PathJoin(FileSystem::PathJoin(mContentCachePath, "Mods\\" + mDomain), "modinfo.typemap");
            cacheDir = FileSystem::PathJoin(mContentCachePath, "Mods\\" + mDomain);
            sourceDir = FileSystem::PathJoin(mContentSourcePath, "Mods\\" + mDomain);
        }
        gSysLog.Info(LogMessage("Loading domain ") << mDomain << " : " << path << "...");
        AssetTypeMap typeMap;
        if (!typeMap.Read(AssetTypeMap::JSON, path))
        {
            gSysLog.Warning(LogMessage("Failed to load the domain... It must be rebuilt from source."));
        }

        if (!GetCacheController().AddDomain(mDomain, cacheDir))
        {
            SetFailed("Failed to add domain to cache controller.");
            return;
        }

        if (!GetSourceController().AddDomain(mDomain, sourceDir))
        {
            SetFailed("Failed to add domain to source controller.");
            return;
        }

        if (!GetDataController().LoadDomain(mDomain, typeMap))
        {
            SetFailed("Failed to add doamin to data controller.");
            return;
        }

        SetComplete();
    }

}