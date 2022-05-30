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
#include "DefaultAssetProcessor.h"
#include "Core/IO/BinaryStream.h"
#include "Core/IO/TextStream.h"
#include "Core/IO/JsonStream.h"
#include "Core/Utility/Log.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Reflection/ReflectionMgr.h"

namespace lf {

DefaultAssetProcessor::DefaultAssetProcessor()
: Super()
{
}
DefaultAssetProcessor::~DefaultAssetProcessor()
{
}

const Type* DefaultAssetProcessor::GetTargetType() const 
{
    return typeof(AssetObject);
}
SizeT DefaultAssetProcessor::GetCacheBlockScore(CacheBlockType::Value cacheBlock) const
{
    return cacheBlock == CacheBlockType::CBT_OBJECT ? 0 : INVALID;
}
bool DefaultAssetProcessor::AcceptImportPath(const AssetPath& ) const
{
    return false;
}
const Type* DefaultAssetProcessor::GetPrototypeType(const Type* inputType) const 
{
    return inputType;
}
const Type* DefaultAssetProcessor::GetConcreteType(const Type* inputType) const
{
    return inputType;
}
AssetImportResult DefaultAssetProcessor::Import(const AssetPath& assetPath) const 
{
    AssetImportResult result;

    String content;
    SizeT contentSize = 0;
    if (!GetSourceController().QuerySize(assetPath, contentSize))
    {
        gSysLog.Warning(LogMessage("Failed to import asset, could not query the source content size. Asset=") << assetPath.CStr());
        return result;
    }
    content.Resize(contentSize);

    if (!GetSourceController().Read(content, assetPath))
    {
        gSysLog.Error(LogMessage("Failed to import asset, could not read the source content. Asset=") << assetPath.CStr());
        return result;
    }

    if (content.Empty())
    {
        return result;
    }

    if (content.First() == '$') // Text
    {
        TextStream ts(Stream::TEXT, &content, Stream::SM_READ);
        if (ts.GetMode() != Stream::SM_READ)
        {
            gSysLog.Error(LogMessage("Failed to import asset, failed to parse the source content object. Asset=") << assetPath.CStr());
            return result;
        }

        if (ts.GetObjectCount() == 0)
        {
            gSysLog.Error(LogMessage("Failed to import asset, there are no objects in the source content. Asset=") << assetPath.CStr());
            return result;
        }

        if (assetPath.GetName() != ts.GetObjectName(0))
        {
            gSysLog.Error(LogMessage("Failed to import asset, the source content object name differs from source filename. Asset=") << assetPath.CStr());
            return result;
        }

        AssetPath superName(ts.GetObjectSuper(0));
        auto queryResult = GetDataController().Find(superName);
        if (!queryResult)
        {
            result.mDependencies.push_back(superName);
            return result;
        }

        const Type* prototypeType = GetPrototypeType(queryResult.mType->GetConcreteType());

        result.mObject = GetReflectionMgr().CreateAtomic<AssetObject>(prototypeType);
        if (!result.mObject)
        {
            gSysLog.Error(LogMessage("Failed to import asset, could not create object of type. Type=") << queryResult.mType->GetConcreteType()->GetFullName());
            return result;
        }
        result.mParentType = queryResult.mType;
        result.mConcreteType = result.mObject->GetType();

        ts.SetAssetLoadFlags(AssetLoadFlags::LF_ACQUIRE);
        Assert(ts.BeginObject(ts.GetObjectName(0), ts.GetObjectSuper(0)));
        result.mObject->Serialize(ts);
        ts.EndObject();
    }
    else // Json
    {
        JsonObjectStream js(Stream::TEXT, &content, Stream::SM_READ);
        if (js.GetMode() != Stream::SM_READ)
        {
            gSysLog.Error(LogMessage("Failed to import asset, failed to parse the source content object. Asset=") << assetPath.CStr());
            return result;
        }

        if (!js.BeginObject(assetPath.GetName(), ""))
        {
            gSysLog.Error(LogMessage("Failed to import asset, the source content object name differs from source filename. Asset=") << assetPath.CStr());
            js.Close();
            return result;
        }

        AssetPath superName(js.GetCurrentSuper());
        auto queryResult = GetDataController().Find(superName);
        if (!queryResult)
        {
            result.mDependencies.push_back(superName);
            return result;
        }

        const Type* prototypeType = GetPrototypeType(queryResult.mType->GetConcreteType());

        result.mObject = GetReflectionMgr().CreateAtomic<AssetObject>(prototypeType);
        if (!result.mObject)
        {
            gSysLog.Error(LogMessage("Failed to import asset, could not create object of type. Type=") << queryResult.mType->GetConcreteType()->GetFullName());
            return result;
        }
        result.mParentType = queryResult.mType;
        result.mConcreteType = result.mObject->GetType();

        js.SetAssetLoadFlags(AssetLoadFlags::LF_ACQUIRE);
        result.mObject->Serialize(js);
        js.EndObject();
    }
    return result;
}
AssetDataType::Value DefaultAssetProcessor::Export(AssetObject* object, MemoryBuffer& buffer, bool cache, AssetDataType::Value dataTypeHint) const 
{
    if (!object)
    {
        return AssetDataType::INVALID_ENUM;
    }

    const AssetTypeInfo* assetType = object->GetAssetType();
    if (!assetType)
    {
        return AssetDataType::INVALID_ENUM;
    }

    AssetDataType::Value dataType = dataTypeHint;
    if (InvalidEnum(dataType))
    {
        dataType = cache ? AssetDataType::ADT_BINARY : AssetDataType::ADT_JSON;
    }

    String name = assetType->GetPath().GetName();
    String super = assetType->GetParent()->GetPath().CStr();

    switch (dataType)
    {
    case AssetDataType::ADT_BINARY:
    {
        BinaryStream s(Stream::MEMORY, &buffer, Stream::SM_WRITE);
        if (s.BeginObject(name, super))
        {
            object->Serialize(s);
            s.EndObject();
        }
        s.Close();
    } break;
    case AssetDataType::ADT_TEXT:
    {
        String content;
        TextStream s(Stream::TEXT, &content, Stream::SM_WRITE);
        if (s.BeginObject(name, super))
        {
            object->Serialize(s);
            s.EndObject();
        }
        s.Close();

        buffer.Allocate(content.Size() + 1, 1);
        strcpy_s(reinterpret_cast<char*>(buffer.GetData()), buffer.GetSize(), content.CStr());
    } break;
    case AssetDataType::ADT_JSON:
    {
        String content;
        JsonObjectStream s(Stream::TEXT, &content, Stream::SM_PRETTY_WRITE);
        if (s.BeginObject(name, super))
        {
            object->Serialize(s);
            s.EndObject();
        }
        s.Close();

        buffer.Allocate(content.Size() + 1, 1);
        strcpy_s(reinterpret_cast<char*>(buffer.GetData()), buffer.GetSize(), content.CStr());
    } break;
    default:
        return AssetDataType::INVALID_ENUM;
    }
    return dataType;
}
void DefaultAssetProcessor::OnCreatePrototype(AssetObject* ) const 
{
}
void DefaultAssetProcessor::OnDestroyPrototype(AssetObject* ) const 
{
}
bool DefaultAssetProcessor::PrepareAsset(AssetObject* object, const MemoryBuffer& buffer, AssetLoadFlags::Value loadFlags) const
{
    ReportBug(object != nullptr);
    if (object == nullptr)
    {
        return false;
    }

    const bool fromSource = (loadFlags & AssetLoadFlags::LF_SOURCE) > 0;
    // Objects in 'source' form are in text format.
    // Objects in 'cache' form are in binary format.

    // When we serialize with streams, we need to disable asset loading with the LF_ACQUIRE flag.
    // We might not have all the required dependencies, so the calling AssetOp should then
    // serialize again with DepStream to calculate dependencies and load the dependencies
    // once the dependencies are loaded the asset will just be considered loaded. 
    // (Acquire acquires the handle so everything should just work)

    //
    // TAsset< ... > can acquire a reference to the type sure, but it cannot load the asset.

    if (buffer.GetSize() == 0)
    {
        return true;
    }

    const String name = object->GetAssetType()->GetPath().GetName();
    const String super(object->GetAssetType()->GetParent()->GetPath().CStr(), COPY_ON_WRITE);
    if (fromSource)
    {
        String text(buffer.GetSize(), reinterpret_cast<const char*>(buffer.GetData()), COPY_ON_WRITE);
        if (text.First() == '$')
        {
            TextStream ts(Stream::TEXT, &text, Stream::SM_READ);
            if (!ts.BeginObject(name, super))
            {
                return false;
            }
            ts.SetAssetLoadFlags(loadFlags | AssetLoadFlags::LF_ACQUIRE);
            object->Serialize(ts);
            ts.EndObject();
            ts.Close();
        }
        else
        {
            JsonObjectStream js(Stream::TEXT, &text, Stream::SM_READ);
            if (!js.BeginObject(name, super))
            {
                return false;
            }
            js.SetAssetLoadFlags(loadFlags | AssetLoadFlags::LF_ACQUIRE);
            object->Serialize(js);
            js.EndObject();
            js.Close();
        }
    }
    else
    {
        BinaryStream bs(Stream::MEMORY, &const_cast<MemoryBuffer&>(buffer), Stream::SM_READ);
        if (!bs.BeginObject(name, super))
        {
            return false;
        }
        bs.SetAssetLoadFlags(loadFlags | AssetLoadFlags::LF_ACQUIRE);
        object->Serialize(bs);
        bs.EndObject();
        bs.Close();
    }
    return true;
}

void DefaultAssetProcessor::OnLoadAsset(AssetObject* ) const
{

}

void DefaultAssetProcessor::OnUnloadAsset(AssetObject* ) const 
{
}

} // namespace lf