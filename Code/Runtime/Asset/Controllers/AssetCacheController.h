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
#include "Core/Memory/SmartPointer.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/String/String.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/CacheBlockType.h"
#include "Runtime/Asset/CacheBlock.h"

namespace lf {

class MemoryBuffer;
class AssetTypeInfo;
struct CacheIndex;

class LF_RUNTIME_API AssetCacheController
{
    // Block -> Blob -> Object
    struct DomainContext
    {
        String mDomain;
        String mRoot;
        CacheBlock mBlocks[CacheBlockType::MAX_VALUE];
    };
    using DomainContextPtr = TAtomicStrongPointer<DomainContext>;

public:
    AssetCacheController();
    ~AssetCacheController();
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

    bool Write(const String& content, const AssetTypeInfo* type, CacheIndex& cacheIndex);
    bool Read(String& content, const AssetTypeInfo* type, CacheIndex& cacheIndex);

    bool Write(const MemoryBuffer& buffer, const AssetTypeInfo* type, CacheIndex& cacheIndex);
    bool Read(MemoryBuffer& buffer, const AssetTypeInfo* type, CacheIndex& cacheIndex);

    // ********************************************************************
    // Query the size of an asset in the cache.
    //
    // @threadsafe
    // ********************************************************************
    bool QuerySize(const AssetTypeInfo* type, SizeT& outSize);

    bool QueryInfo(const AssetTypeInfo* type, const AssetInfoQuery& query, AssetInfoQueryResult& result);

    bool Delete(const AssetTypeInfo* type);

    // ********************************************************************
    // Specifically deletes the object from the cache. You should only call 
    // this method if you're correcting corruption.
    //
    // @
    // ********************************************************************
    bool DeleteObject(const AssetTypeInfo* type, const CacheObject& cacheObjectID, const CacheIndex& cacheIndex);
    // ********************************************************************
    // Specifically deletes index from the cache. You should only call this
    // method if you're correcting corruption.
    //
    // 
    // ********************************************************************
    bool DeleteIndex(const AssetTypeInfo* type, const CacheIndex& cacheIndex);

    bool SaveDomain(const String& domain);

    bool FindIndex(const AssetTypeInfo* type, CacheIndex& index);
    bool FindObject(const AssetTypeInfo* type, CacheObject& outObject, CacheIndex& outIndex);
private:
    DomainContextPtr GetDomainContext(const String& domain) const;
    bool WriteBytes(const void* buffer, SizeT numBytes, const AssetTypeInfo* type, CacheIndex& cacheIndex);
    bool ReadBytes(void* buffer, SizeT numBytes, const AssetTypeInfo* type, CacheIndex& cacheIndex);
    
    void SaveIndex(DomainContext* context);
    void LoadIndex(DomainContext* context);

    mutable RWSpinLock       mDomainContextsLock;
    TVector<DomainContextPtr> mDomainContexts;
};

} // namespace lf