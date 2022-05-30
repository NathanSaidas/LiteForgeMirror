#pragma once
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
#include "Core/Common/API.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/String/String.h"
#include "Core/Platform/RWSpinLock.h"
#include "Runtime/Async/PromiseImpl.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/AssetPath.h"
#include "Runtime/Asset/CacheBlockType.h"

namespace lf {

DECLARE_HASHED_CALLBACK(SourceControllerPromiseCallback, void);

// ********************************************************************
// The Source Controller is used to read/write and query info on assets
// at their source location.
// 
// All methods are thread safe.
// ********************************************************************
class LF_RUNTIME_API AssetSourceController
{
public:
    // ** The promise type used for async operations.
    using PromiseType = PromiseImpl<SourceControllerPromiseCallback, SourceControllerPromiseCallback>;

    AssetSourceController();
    ~AssetSourceController();

    // ********************************************************************
    // Adds a root filepath for a given domain. Call this for-each supported
    // domain.
    // 
    // note: Cannot have the same domain point to 2 different roots.
    // ********************************************************************
    bool AddDomain(const String& domain, const String& root);

    // ********************************************************************
    // Removes a domain from the source controller runtime.
    // ********************************************************************
    void RemoveDomain(const String& domain);

    // ********************************************************************
    // Use this method to query the size of a source file before trying to
    // read the data.
    // ********************************************************************
    bool QuerySize(const AssetPath& path, SizeT& size);

    // ********************************************************************
    // Use this method to check if the asset source file exists.
    // ********************************************************************
    bool QueryExist(const AssetPath& path);

    // ********************************************************************
    // This method writes the 'content' to the given asset path. The path
    // is resolved using the domain so if the domain is 'empty' then this is nop.
    // 
    // ********************************************************************
    bool Write(const String& content, const AssetPath& path);

    // ********************************************************************
    // This method reads the 'content' from the given asset path. The path
    // is resolved using the domain so if the domain is 'empty' then this is nop.
    // ********************************************************************
    bool Read(String& content, const AssetPath& path);

    // ********************************************************************
    // This method writes the 'buffer' to the given asset path. The path
    // is resolved using the domain so if the domain is 'empty' then this is nop.
    // ********************************************************************
    bool Write(const MemoryBuffer& buffer, const AssetPath& path);

    // ********************************************************************
    // This method reads the 'buffer' from the given asset path. The path
    // is resolved using the domain so if the domain is 'empty' then this is nop.
    // ********************************************************************
    bool Read(MemoryBuffer& buffer, const AssetPath& path);

    // ********************************************************************
    // This method is the same as Write except it now executes the write in
    // a background task. You can use the promise to receive callbacks
    // for when the task completes and whether or not it errors out.
    // ********************************************************************
    PromiseType WriteAsync(const String* content, const AssetPath& path);

    // ********************************************************************
    // This method is the same as Read except it now executes the write in
    // a background task. You can use the promise to receive callbacks
    // for when the task completes and whether or not it errors out.
    // ********************************************************************
    PromiseType ReadAsync(String* content, const AssetPath& path);

    // ********************************************************************
    // This method is the same as Write except it now executes the write in
    // a background task. You can use the promise to receive callbacks
    // for when the task completes and whether or not it errors out.
    // ********************************************************************
    PromiseType WriteAsync(const MemoryBuffer* content, const AssetPath& path);

    // ********************************************************************
    // This method is the same as Read except it now executes the write in
    // a background task. You can use the promise to receive callbacks
    // for when the task completes and whether or not it errors out.
    // ********************************************************************
    PromiseType ReadAsync(MemoryBuffer* buffer, const AssetPath& path);

    // ********************************************************************
    // This method returns the fully qualified path of an AssetPath
    // regardless of whether or not it exists.
    // ********************************************************************
    String GetFullPath(const AssetPath& path) const;

    // ********************************************************************
    // Deletes the source file at the given path.
    // ********************************************************************
    bool Delete(const AssetPath& path);

    TVector<AssetPath> GetSourcePaths(const AssetPath& path);

    bool QueryInfo(const AssetPath& path, const AssetInfoQuery& query, AssetInfoQueryResult& result);
private:
    struct ContentRootPair
    {
        String mDomain;
        String mRoot;
    };

    bool WriteBytes(const String& fullpath, const void* buffer, SizeT numBytes);
    bool ReadBytes(const String& fullpath, void* buffer, SizeT numBytes);
    String GetDomainRoot(const AssetPath& domainPath);

    mutable RWSpinLock      mContentRootsLock;
    TVector<ContentRootPair> mContentRoots;
};

} // namespace lf