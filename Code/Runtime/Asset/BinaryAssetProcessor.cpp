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
#include "BinaryAssetProcessor.h"
#include "Core/Utility/Log.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Reflection/ReflectionMgr.h"

namespace lf {

    BinaryAssetProcessor::BinaryAssetProcessor()
    {

    }
    BinaryAssetProcessor::~BinaryAssetProcessor()
    {

    }

    SizeT BinaryAssetProcessor::GetCacheBlockScore(CacheBlockType::Value cacheBlock) const
    {
        return cacheBlock == CacheBlockType::CBT_BINARY_DATA ? 0 : INVALID;
    }

    bool BinaryAssetProcessor::AcceptImportPath(const AssetPath& ) const
    {
        return false;
    }

    const Type* BinaryAssetProcessor::GetPrototypeType(const Type* inputType) const
    {
        return inputType;
    }
    const Type* BinaryAssetProcessor::GetConcreteType(const Type* inputType) const
    {
        return inputType;
    }
    AssetImportResult BinaryAssetProcessor::Import(const AssetPath& assetPath) const
    {
        AssetImportResult result;

        MemoryBuffer content;
        SizeT contentSize = 0;
        if (!GetSourceController().QuerySize(assetPath, contentSize))
        {
            gSysLog.Warning(LogMessage("Failed to import asset, could not query the source content size. Asset=") << assetPath.CStr());
            return result;
        }
        content.Allocate(contentSize, 1);
        content.SetSize(contentSize);

        if (!GetSourceController().Read(content, assetPath))
        {
            gSysLog.Error(LogMessage("Failed to import asset, could not read the source content. Asset=") << assetPath.CStr());
            return result;
        }

        auto queryResult = GetDataController().Find(assetPath);
        if (!queryResult)
        {
            return result;
        }

        const Type* prototypeType = GetPrototypeType(queryResult.mType->GetConcreteType());

        result.mObject = GetReflectionMgr().CreateAtomic<AssetObject>(prototypeType);
        if (!result.mObject)
        {
            gSysLog.Error(LogMessage("Failed to import asset, could not create object of type. Type=") << prototypeType->GetFullName());
            return result;
        }
        result.mParentType = queryResult.mType->GetParent();
        result.mConcreteType = queryResult.mType->GetConcreteType();

        ReadBinary(result.mObject, content);

        return result;
    }

    AssetDataType::Value BinaryAssetProcessor::Export(AssetObject* object, MemoryBuffer& buffer, bool, AssetDataType::Value) const
    {
        if (!object)
        {
            return AssetDataType::INVALID_ENUM;
        }

        WriteBinary(object, buffer);
        
        return AssetDataType::ADT_BINARY;
    }
    void BinaryAssetProcessor::OnCreatePrototype(AssetObject*) const
    {

    }
    void BinaryAssetProcessor::OnDestroyPrototype(AssetObject*) const
    {

    }
    bool BinaryAssetProcessor::PrepareAsset(AssetObject* object, const MemoryBuffer& buffer, AssetLoadFlags::Value) const
    {
        ReportBug(object != nullptr);
        if (object == nullptr)
        {
            return false;
        }

        if (buffer.GetSize() == 0)
        {
            return true;
        }

        ReadBinary(object, buffer);
        return true;
    }
    void BinaryAssetProcessor::OnLoadAsset(AssetObject*) const
    {

    }
    void BinaryAssetProcessor::OnUnloadAsset(AssetObject*) const
    {

    }

} // namespace lf