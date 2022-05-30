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
#include "AbstractEngine/PCH.h"
#include "GfxTextureBinaryProcessor.h"

#include "Core/Reflection/DynamicCast.h"
#include "Core/Utility/Log.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"
#include "Runtime/Asset/Controllers/AssetCacheController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Reflection/ReflectionMgr.h"
namespace lf {

DECLARE_ATOMIC_PTR(GfxTextureBinary);
static const String EXTENSIONS[] = { "png", "jpeg", "dds" };
static const Gfx::TextureFileFormat FORMATS[] = { Gfx::TextureFileFormat::PNG, Gfx::TextureFileFormat::INVALID_ENUM, Gfx::TextureFileFormat::DDS };

static Gfx::TextureFileFormat FormatFromExtension(const AssetPath& path)
{
    const String extension = path.GetExtension();
    for (SizeT i = 0; i < LF_ARRAY_SIZE(EXTENSIONS); ++i)
    {
        if (StrCompareAgnostic(extension, EXTENSIONS[i]))
        {
            return FORMATS[i];
        }
    }
    return Gfx::TextureFileFormat::INVALID_ENUM;
}

static bool ConvertToDDS(MemoryBuffer& data, Gfx::TextureFileFormat sourceFormat)
{
    (data);
    (sourceFormat);
    return false;
}

static bool ConvertToPNG(MemoryBuffer& data, Gfx::TextureFileFormat sourceFormat)
{
    (data);
    (sourceFormat);
    return false;
}

GfxTextureBinaryProcessor::GfxTextureBinaryProcessor(Gfx::TextureFileFormat targetFormat)
: Super()
, mTargetFormat(targetFormat)
{}


const Type* GfxTextureBinaryProcessor::GetTargetType() const
{
    return typeof(GfxTextureBinary);
}

SizeT GfxTextureBinaryProcessor::GetCacheBlockScore(CacheBlockType::Value) const
{
    return 0;
}

bool GfxTextureBinaryProcessor::AcceptImportPath(const AssetPath& path) const
{
    const String extension = path.GetExtension();
    for (SizeT i = 0; i < LF_ARRAY_SIZE(EXTENSIONS); ++i)
    {
        if (StrCompareAgnostic(extension, EXTENSIONS[i]))
        {
            return true;
        }
    }
    return false;
}

const Type* GfxTextureBinaryProcessor::GetPrototypeType(const Type* inputType) const
{
    return inputType;
}

const Type* GfxTextureBinaryProcessor::GetConcreteType(const Type* inputType) const
{
    return inputType;
}
AssetImportResult GfxTextureBinaryProcessor::Import(const AssetPath& assetPath) const
{
    AssetImportResult result;
    Gfx::TextureFileFormat fileFormat = FormatFromExtension(assetPath);
    if (InvalidEnum(fileFormat))
    {
        return result;
    }
    
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

    auto queryResult = GetDataController().Find(typeof(GfxTextureBinary));
    if (!queryResult)
    {
        return result;
    }

    switch (mTargetFormat)
    {
        case Gfx::TextureFileFormat::DDS:
        {
            if (fileFormat != Gfx::TextureFileFormat::DDS)
            {
                if (!ConvertToDDS(content, fileFormat))
                {
                    return result;
                }
            }
        } break;
        case Gfx::TextureFileFormat::PNG:
        {
            if (fileFormat != Gfx::TextureFileFormat::PNG)
            {
                if (!ConvertToPNG(content, fileFormat))
                {
                    return result;
                }
            }
        } break;
        default:
            CriticalAssertMsg("Invalid Enum - Gfx::TextureFileFormat GfxTextureBinaryAssetProcess::Import");
            break;
    }


    const Type* prototypeType = GetPrototypeType(queryResult->GetConcreteType());
    ReportBug(prototypeType == typeof(GfxTextureBinary));
    if (prototypeType != typeof(GfxTextureBinary))
    {
        return result;
    }

    GfxTextureBinaryAtomicPtr texture = GetReflectionMgr().CreateAtomic<GfxTextureBinary>();
    texture->SetBinary(mTargetFormat, std::move(content));

    result.mObject = texture;
    if (!result.mObject)
    {
        gSysLog.Error(LogMessage("Failed to import asset, could not create object of type. Type=") << prototypeType->GetFullName());
        return result;
    }
    result.mParentType = queryResult;
    result.mConcreteType = queryResult->GetConcreteType();
    return result;
}

AssetDataType::Value GfxTextureBinaryProcessor::Export(AssetObject* object, MemoryBuffer& buffer, bool , AssetDataType::Value ) const
{
    GfxTextureBinary* texture = DynamicCast<GfxTextureBinary>(object);
    if (!texture)
    {
        return AssetDataType::INVALID_ENUM;
    }

    buffer.Copy(texture->GetData());
    return AssetDataType::ADT_BINARY;
}

void GfxTextureBinaryProcessor::OnCreatePrototype(AssetObject* ) const
{

}

void GfxTextureBinaryProcessor::OnDestroyPrototype(AssetObject* ) const
{

}

bool GfxTextureBinaryProcessor::PrepareAsset(AssetObject* object, const MemoryBuffer& buffer, AssetLoadFlags::Value ) const
{
    GfxTextureBinary* texture = DynamicCast<GfxTextureBinary>(object);
    ReportBug(texture != nullptr);
    if (texture == nullptr)
    {
        return false;
    }

    if (buffer.GetSize() == 0)
    {
        return true;
    }

    texture->SetBinary(mTargetFormat, buffer);
    return true;
}

void GfxTextureBinaryProcessor::OnLoadAsset(AssetObject* ) const
{

}

void GfxTextureBinaryProcessor::OnUnloadAsset(AssetObject* ) const
{

}

} // namespace lf