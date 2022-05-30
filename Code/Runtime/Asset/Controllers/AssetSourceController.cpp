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
#include "AssetSourceController.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/File.h"

namespace lf {

template<typename ContentT>
struct ReadWritePromiseData
{
    ContentT*   mContent;
    AssetPath   mPath;
};

template<typename ContentT>
TAtomicStrongPointer<ReadWritePromiseData<ContentT>> MakePromiseData(ContentT* content, const AssetPath& path)
{
    TAtomicStrongPointer<ReadWritePromiseData<ContentT>> data(LFNew<ReadWritePromiseData<ContentT>>());
    data->mContent = content;
    data->mPath = path;
    return data;
}

AssetSourceController::AssetSourceController()
: mContentRootsLock()
, mContentRoots()
{
}
AssetSourceController::~AssetSourceController()
{

}

bool AssetSourceController::AddDomain(const String& domain, const String& root)
{
    ScopeRWSpinLockWrite lock(mContentRootsLock);
    for (const ContentRootPair& pair : mContentRoots)
    {
        if (pair.mDomain == domain)
        {
            return false;
        }
    }

    mContentRoots.push_back({ domain, root });
    return true;
}

void AssetSourceController::RemoveDomain(const String& domain)
{
    ScopeRWSpinLockWrite lock(mContentRootsLock);
    for (auto it = mContentRoots.begin(); it != mContentRoots.end(); ++it)
    {
        if (it->mDomain == domain)
        {
            mContentRoots.swap_erase(it);
            return;
        }
    }
}

bool AssetSourceController::QuerySize(const AssetPath& path, SizeT& size)
{
    size = 0;
    FileSize fsize;
    if (!FileSystem::FileQuerySize(GetFullPath(path), fsize))
    {
        return false;
    }
    size = static_cast<SizeT>(fsize);
    return true;
}

bool AssetSourceController::QueryExist(const AssetPath& path)
{
    return FileSystem::FileExists(GetFullPath(path));
}

bool AssetSourceController::Write(const String& content, const AssetPath& path)
{
    return WriteBytes(GetFullPath(path), content.CStr(), content.Size());
}
bool AssetSourceController::Read(String& content, const AssetPath& path)
{
    return ReadBytes(GetFullPath(path), const_cast<char*>(content.CStr()), content.Size());
}
bool AssetSourceController::Write(const MemoryBuffer& buffer, const AssetPath& path)
{
    return WriteBytes(GetFullPath(path), buffer.GetData(), buffer.GetSize());
}
bool AssetSourceController::Read(MemoryBuffer& buffer, const AssetPath& path)
{
    return ReadBytes(GetFullPath(path), buffer.GetData(), buffer.GetSize());
}
AssetSourceController::PromiseType AssetSourceController::WriteAsync(const String* content, const AssetPath& path)
{
    auto promiseData = MakePromiseData(content, path);
    PromiseType promise([this, promiseData](Promise* self)
    {
        auto promise = static_cast<PromiseType*>(self);
        if (Write(*promiseData->mContent, promiseData->mPath))
        {
            promise->Resolve();
        }
        else
        {
            promise->Reject();
        }
    });
    return promise;
}
AssetSourceController::PromiseType AssetSourceController::ReadAsync(String* content, const AssetPath& path)
{
    auto promiseData = MakePromiseData(content, path);
    PromiseType promise([this, promiseData](Promise* self)
    {
        auto promise = static_cast<PromiseType*>(self);
        if (Read(*promiseData->mContent, promiseData->mPath))
        {
            promise->Resolve();
        }
        else
        {
            promise->Reject();
        }
    });
    return promise;
}
AssetSourceController::PromiseType AssetSourceController::WriteAsync(const MemoryBuffer* buffer, const AssetPath& path)
{
    auto promiseData = MakePromiseData(buffer, path);
    PromiseType promise([this, promiseData](Promise* self)
        {
            auto promise = static_cast<PromiseType*>(self);
            if (Write(*promiseData->mContent, promiseData->mPath))
            {
                promise->Resolve();
            }
            else
            {
                promise->Reject();
            }
        });
    return promise;
}
AssetSourceController::PromiseType AssetSourceController::ReadAsync(MemoryBuffer* buffer, const AssetPath& path)
{
    auto promiseData = MakePromiseData(buffer, path);
    PromiseType promise([this, promiseData](Promise* self)
        {
            auto promise = static_cast<PromiseType*>(self);
            if (Read(*promiseData->mContent, promiseData->mPath))
            {
                promise->Resolve();
            }
            else
            {
                promise->Reject();
            }
        });
    return promise;
}
String AssetSourceController::GetFullPath(const AssetPath& path) const
{
    String domain = path.GetDomain();
    if (domain.Empty())
    {
        return String();
    }
    ScopeRWSpinLockRead lock(mContentRootsLock);
    for (const ContentRootPair& pair : mContentRoots)
    {
        if (StrCompareAgnostic(pair.mDomain, domain))
        {
            return FileSystem::PathJoin(pair.mRoot, path.GetScopedName());
        }
    }
    return String();
}
bool AssetSourceController::Delete(const AssetPath& path)
{
    return FileSystem::FileDelete(GetFullPath(path));
}
TVector<AssetPath> AssetSourceController::GetSourcePaths(const AssetPath& path)
{
    TVector<AssetPath> paths;
    String fullpath = GetFullPath(path);
    if (fullpath.Empty())
    {
        return paths;
    }
    String root = GetDomainRoot(path);
    if (root.Empty())
    {
        return paths;
    }


    TVector<String> fullpaths;
    FileSystem::GetAllFiles(fullpath, fullpaths);

    for (const String& filepath : fullpaths)
    {
        SizeT findIndex = filepath.Find(root);
        Assert(Valid(findIndex));

        String localPath = filepath.SubString(findIndex + root.Size());
        localPath = path.GetDomain() + "//" + localPath;
        paths.push_back(AssetPath(localPath));
    }
    return paths;
}

bool AssetSourceController::QueryInfo(const AssetPath& path, const AssetInfoQuery& query, AssetInfoQueryResult& result)
{
    String fullpath = GetFullPath(path);

    if (query.mHash)
    {
        SizeT size;
        if (!QuerySize(path, size))
        {
            return false;
        }

        MemoryBuffer buffer;
        if (!buffer.Allocate(size, 1))
        {
            return false;
        }

        if (!ReadBytes(fullpath, buffer.GetData(), size))
        {
            return false;
        }

        result.mHash = AssetHash(Crypto::MD5Hash(static_cast<ByteT*>(buffer.GetData()), size));
    }

    if (query.mModifyDate)
    {
        if (!FileSystem::FileQueryModifyDate(fullpath, result.mModifyDate))
        {
            return false;
        }
    }
    return true;
}

bool AssetSourceController::WriteBytes(const String& fullpath, const void* buffer, SizeT numBytes)
{
    FileSystem::PathCreate(fullpath);

    File file;
    if (!file.Open(fullpath, FF_WRITE, FILE_OPEN_CREATE_NEW))
    {
        return false;
    }

    if (file.Write(buffer, numBytes) != numBytes)
    {
        return false;
    }
    return true;
}
bool AssetSourceController::ReadBytes(const String& fullpath, void* buffer, SizeT numBytes)
{
    File file;
    if (!file.Open(fullpath, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        return false;
    }

    if (file.Read(buffer, numBytes) != numBytes)
    {
        return false;
    }
    return true;
}

String AssetSourceController::GetDomainRoot(const AssetPath& domainPath)
{
    String domain = domainPath.GetDomain();
    if (domain.Empty())
    {
        return String();
    }
    ScopeRWSpinLockRead lock(mContentRootsLock);
    for (const ContentRootPair& pair : mContentRoots)
    {
        if (pair.mDomain == domain)
        {
            return pair.mRoot;
        }
    }
    return String();
}


} // namespace lf