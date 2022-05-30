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
#include "AssetCacheController.h"
#include "Core/IO/JsonStream.h"
#include "Core/Platform/File.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Runtime/Asset/AssetTypeInfo.h"
#include "Runtime/Asset/CacheWriter.h"
#include "Runtime/Asset/CacheReader.h"

namespace lf {

AssetCacheController::AssetCacheController()
: mDomainContextsLock()
, mDomainContexts()
{

}

AssetCacheController::~AssetCacheController()
{

}


bool AssetCacheController::AddDomain(const String& domain, const String& root)
{
    if (GetDomainContext(domain))
    {
        return false;
    }

    DomainContextPtr context(LFNew<DomainContext>());
    context->mDomain = domain;
    context->mRoot = root;

    for (SizeT i = 0; i < CacheBlockType::MAX_VALUE; ++i)
    {
        Token blockName(CacheBlockType::GetName(static_cast<CacheBlockType::Value>(i)));
        context->mBlocks[i].Initialize(blockName);
        context->mBlocks[i].SetFilename(Token(root + blockName.CStr()));
    }
    LoadIndex(context);

    ScopeRWSpinLockWrite lock(mDomainContextsLock);
    mDomainContexts.push_back(context);
    return true;
}

void AssetCacheController::RemoveDomain(const String& domain)
{
    ScopeRWSpinLockWrite lock(mDomainContextsLock);
    for (auto it = mDomainContexts.begin(); it != mDomainContexts.end(); ++it)
    {
        if ((*it)->mDomain == domain)
        {
            SaveIndex(*it);
            mDomainContexts.swap_erase(it);
            return;
        }
    }
}

bool AssetCacheController::SaveDomain(const String& domain)
{
    DomainContextPtr context = GetDomainContext(domain);
    if (!context)
    {
        return false;
    }

    SaveIndex(context);
    return true;
}

bool AssetCacheController::FindIndex(const AssetTypeInfo* type, CacheIndex& index)
{
    DomainContextPtr context = GetDomainContext(type->GetPath().GetDomain());
    if (!context)
    {
        return false;
    }

    CacheBlockType::Value blockType = CacheBlockType::ToEnum(type->GetPath());
    CacheBlock& block = context->mBlocks[blockType];
    index = block.Find(type->GetCacheIndex().mUID);
    return index;
}
bool AssetCacheController::FindObject(const AssetTypeInfo* type, CacheObject& outObject, CacheIndex& outIndex)
{
    DomainContextPtr context = GetDomainContext(type->GetPath().GetDomain());
    if (!context)
    {
        return false;
    }

    CacheBlockType::Value blockType = CacheBlockType::ToEnum(type->GetPath());
    CacheBlock& block = context->mBlocks[blockType];
    CacheIndex index = block.Find(type->GetCacheIndex().mUID);
    bool result = index && block.GetObject(index, outObject);
    if (!result)
    {
        result = block.FindObject(type->GetCacheIndex().mUID, outObject, outIndex);
    }
    return result;
}

bool AssetCacheController::Write(const String& content, const AssetTypeInfo* type, CacheIndex& cacheIndex)
{
    return WriteBytes(const_cast<char*>(content.CStr()), content.Size(), type, cacheIndex);
}

bool AssetCacheController::Read(String& content, const AssetTypeInfo* type, CacheIndex& cacheIndex)
{
    return ReadBytes(const_cast<char*>(content.CStr()), content.Size(), type, cacheIndex);
}

bool AssetCacheController::Write(const MemoryBuffer& buffer, const AssetTypeInfo* type, CacheIndex& cacheIndex)
{
    return WriteBytes(buffer.GetData(), buffer.GetSize(), type, cacheIndex);
}

bool AssetCacheController::Read(MemoryBuffer& buffer, const AssetTypeInfo* type, CacheIndex& cacheIndex)
{
    return ReadBytes(buffer.GetData(), buffer.GetSize(), type, cacheIndex);
}

bool AssetCacheController::QuerySize(const AssetTypeInfo* type, SizeT& outSize)
{
    DomainContextPtr context = GetDomainContext(type->GetPath().GetDomain());
    if (!context)
    {
        return false;
    }

    CacheBlockType::Value blockType = CacheBlockType::ToEnum(type->GetPath());
    CacheBlock& block = context->mBlocks[blockType];

    CacheObject cacheObject;
    if (!block.GetObject(type->GetCacheIndex(), cacheObject))
    {
        return false;
    }
    outSize = static_cast<SizeT>(cacheObject.mSize);
    return true;
}

bool AssetCacheController::QueryInfo(const AssetTypeInfo* type, const AssetInfoQuery& query, AssetInfoQueryResult& result)
{
    if (!type || Invalid(type->GetCacheIndex()))
    {
        return false;
    }

    if (query.mHash)
    {
        SizeT size;
        if (!QuerySize(type, size))
        {
            return false;
        }

        MemoryBuffer buffer;
        if (!buffer.Allocate(size, 1))
        {
            return false;
        }

        CacheIndex index = type->GetCacheIndex();
        if (!Read(buffer, type, index))
        {
            return false;
        }

        result.mHash = AssetHash(Crypto::MD5Hash(static_cast<ByteT*>(buffer.GetData()), size));
    }

    // Cache Modify Date is whats on the type
    if (query.mModifyDate)
    {
        result.mModifyDate = type->GetModifyDate();
    }
    return true;
}

bool AssetCacheController::Delete(const AssetTypeInfo* type)
{
    if (Invalid(type->GetCacheIndex()))
    {
        return false;
    }

    DomainContextPtr context = GetDomainContext(type->GetPath().GetDomain());
    if (!context)
    {
        return false;
    }

    CacheBlockType::Value blockType = CacheBlockType::ToEnum(type->GetPath());
    CacheBlock& block = context->mBlocks[blockType];

    CacheObject cacheObject;
    if (!block.GetObject(type->GetCacheIndex(), cacheObject))
    {
        return false;
    }

    block.Destroy(type->GetCacheIndex());
    CacheWriter writer;
    return writer.Open(block, type->GetCacheIndex(), nullptr, cacheObject.mCapacity) && writer.Write();
}

bool AssetCacheController::DeleteObject(const AssetTypeInfo* type, const CacheObject& object, const CacheIndex& cacheIndex)
{
    if (Invalid(object.mUID))
    {
        return false;
    }

    DomainContextPtr context = GetDomainContext(type->GetPath().GetDomain());
    if (!context)
    {
        return false;
    }

    CacheBlockType::Value blockType = CacheBlockType::ToEnum(type->GetPath());
    CacheBlock& block = context->mBlocks[blockType];

    if (!block.DestroyObject(object.mUID))
    {
        return false;
    }
    CacheWriter writer;
    return writer.Open(block, cacheIndex, nullptr, object.mCapacity) && writer.Write();
}

bool AssetCacheController::DeleteIndex(const AssetTypeInfo* type, const CacheIndex& cacheIndex)
{
    if (!cacheIndex)
    {
        return false;
    }

    DomainContextPtr context = GetDomainContext(type->GetPath().GetDomain());
    if (!context)
    {
        return false;
    }

    CacheBlockType::Value blockType = CacheBlockType::ToEnum(type->GetPath());
    CacheBlock& block = context->mBlocks[blockType];

    if (!block.DestroyIndex(cacheIndex))
    {
        return false;
    }

    return true;
}


AssetCacheController::DomainContextPtr AssetCacheController::GetDomainContext(const String& domain) const
{
    if (domain.Empty())
    {
        return NULL_PTR;
    }

    ScopeRWSpinLockRead lock(mDomainContextsLock);
    for (const DomainContextPtr& context : mDomainContexts)
    {
        if (StrCompareAgnostic(context->mDomain, domain))
        {
            return context;
        }
    }
    return NULL_PTR;
}

bool AssetCacheController::WriteBytes(const void* buffer, SizeT numBytes, const AssetTypeInfo* type, CacheIndex& cacheIndex)
{
    DomainContextPtr context = GetDomainContext(type->GetPath().GetDomain());
    if (!context)
    {
        return false;
    }

    CacheBlockType::Value blockType = CacheBlockType::ToEnum(type->GetPath());
    CacheBlock& block = context->mBlocks[blockType];

    CacheObject cacheObject;
    UInt32 uid = type->GetCacheIndex().mUID;
    cacheIndex = block.Find(uid);
    if (!cacheIndex)
    {
        cacheIndex = block.Create(uid, static_cast<UInt32>(numBytes));
        Assert(block.GetObject(cacheIndex, cacheObject) && cacheObject.mCapacity >= numBytes);
    }
    else
    {
        // #TODO [Nathan] In what scenario can we have a valid cache index but invalid object?
        Assert(block.GetObject(cacheIndex, cacheObject));
        if (cacheObject.mCapacity < numBytes)
        {
            block.Destroy(cacheIndex);
            cacheIndex = block.Create(uid, static_cast<UInt32>(numBytes));
            Assert(block.GetObject(cacheIndex, cacheObject) && cacheObject.mCapacity >= numBytes);
        }
    }

    CacheWriter writer;
    if (!(writer.Open(block, cacheIndex, buffer, numBytes) && writer.Write()))
    {
        return false;
    }
    cacheIndex = block.Update(cacheIndex, static_cast<UInt32>(numBytes));
    return true;
}

bool AssetCacheController::ReadBytes(void* buffer, SizeT numBytes, const AssetTypeInfo* type, CacheIndex& cacheIndex)
{
    DomainContextPtr context = GetDomainContext(type->GetPath().GetDomain());
    if (!context)
    {
        return false;
    }

    CacheBlockType::Value blockType = CacheBlockType::ToEnum(type->GetPath());
    CacheBlock& block = context->mBlocks[blockType];

    CacheObject cacheObject;
    UInt32 uid = type->GetCacheIndex().mUID;
    cacheIndex = block.Find(uid);
    if (!cacheIndex || !block.GetObject(cacheIndex, cacheObject))
    {
        return false;
    }

    if (cacheObject.mSize > numBytes)
    {
        return false;
    }
    // content.Resize(static_cast<SizeT>(cacheObject.mSize));

    CacheReader reader;
    return reader.Open(block, cacheIndex, buffer, numBytes) && reader.Read();
}

void AssetCacheController::SaveIndex(DomainContext* context) 
{
    for (CacheBlock& block : context->mBlocks)
    {
        if (block.Empty())
        {
            continue;
        }

        String path(block.GetFilename().CStr());
        path += ".lfindex";

        String text;
        JsonStream s(Stream::TEXT, &text, Stream::SM_PRETTY_WRITE);
        if (s.BeginObject("CacheIndex", "Object"))
        {
            block.Serialize(s);
            s.EndObject();
        }
        s.Close();

        File file;
        if (file.Open(path, FF_WRITE, FILE_OPEN_CREATE_NEW))
        {
            file.Write(text.CStr(), text.Size());
            file.Close();
        }
    }
}

void AssetCacheController::LoadIndex(DomainContext* context)
{
    for (CacheBlock& block : context->mBlocks)
    {
        String path(block.GetFilename().CStr());
        path += ".lfindex";

        String text;
        File file;
        if (file.Open(path, FF_READ, FILE_OPEN_EXISTING))
        {
            text.Resize(static_cast<SizeT>(file.GetSize()));
            Assert(file.Read(const_cast<char*>(text.CStr()), text.Size()) == text.Size());
            file.Close();
        }

        JsonStream s(Stream::TEXT, &text, Stream::SM_READ);
        if (s.BeginObject("CacheIndex", "Object"))
        {
            block.Serialize(s);
            s.EndObject();
        }
        s.Close();
    }
}


} // namespace lf