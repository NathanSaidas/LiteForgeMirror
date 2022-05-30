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
#include "TextAssetProcessor.h"
#include "Core/Utility/Log.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Reflection/ReflectionMgr.h"

namespace lf {

TextAssetProcessor::TextAssetProcessor()
{

}
TextAssetProcessor::~TextAssetProcessor()
{

}

SizeT TextAssetProcessor::GetCacheBlockScore(CacheBlockType::Value cacheBlock) const
{
    return cacheBlock == CacheBlockType::CBT_TEXT_DATA ? 0 : INVALID;
}
bool TextAssetProcessor::AcceptImportPath(const AssetPath&) const
{
    return false;
}
const Type* TextAssetProcessor::GetPrototypeType(const Type* inputType) const
{
    return inputType;
}
const Type* TextAssetProcessor::GetConcreteType(const Type* inputType) const
{
    return inputType;
}
AssetImportResult TextAssetProcessor::Import(const AssetPath& assetPath) const
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

    const Type* prototypeType = GetTargetType();
    if (!prototypeType)
    {
        return result;
    }

    AssetTypeInfoCPtr parentType = GetDataController().Find(prototypeType);
    if (!parentType)
    {
        return result;
    }

    result.mObject = GetReflectionMgr().CreateAtomic<AssetObject>(prototypeType);
    if (!result.mObject)
    {
        gSysLog.Error(LogMessage("Failed to import asset, could not create object of type. Type=") << prototypeType->GetFullName());
        return result;
    }
    result.mParentType = parentType;
    result.mConcreteType = prototypeType;
   
    ReadText(result.mObject, content);

    return result;
}

AssetDataType::Value TextAssetProcessor::Export(AssetObject* object, MemoryBuffer& buffer, bool , AssetDataType::Value ) const
{
    if (!object)
    {
        return AssetDataType::INVALID_ENUM;
    }

    String text;
    WriteText(object, text);

    if (text.Empty())
    {
        return AssetDataType::ADT_TEXT;
    }

    buffer.Allocate(text.Size() + 1, 1);
    strcpy_s(reinterpret_cast<char*>(buffer.GetData()), buffer.GetSize(), text.CStr());
    
    return AssetDataType::ADT_TEXT;
}
void TextAssetProcessor::OnCreatePrototype(AssetObject*) const
{

}
void TextAssetProcessor::OnDestroyPrototype(AssetObject*) const
{

}
bool TextAssetProcessor::PrepareAsset(AssetObject* object, const MemoryBuffer& buffer, AssetLoadFlags::Value ) const
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

    String text(buffer.GetSize() - 1, reinterpret_cast<const char*>(buffer.GetData()), COPY_ON_WRITE);
    ReadText(object, text);
    return true;
}
void TextAssetProcessor::OnLoadAsset(AssetObject*) const
{

}
void TextAssetProcessor::OnUnloadAsset(AssetObject*) const
{

}

} // namespace lf