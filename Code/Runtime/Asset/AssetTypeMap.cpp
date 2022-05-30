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
#include "AssetTypeMap.h"
#include "Core/IO/BinaryStream.h"
#include "Core/IO/JsonStream.h"
#include "Core/Platform/File.h"

namespace lf {

AssetTypeMapping::AssetTypeMapping()
: mPath()
, mParent()
, mConcreteType()
, mCacheUID(0)
, mCacheBlobID(0)
, mCacheObjectID(0)
, mWeakReferences(0)
, mStrongReferences(0)
{

}

Stream& operator<<(Stream& s, AssetTypeMapping& obj)
{
    SERIALIZE(s, obj.mPath, "");
    SERIALIZE(s, obj.mParent, "");
    SERIALIZE(s, obj.mConcreteType, "");
    SERIALIZE(s, obj.mCacheUID, "");
    SERIALIZE(s, obj.mCacheBlobID, "");
    SERIALIZE(s, obj.mCacheObjectID, "");
    SERIALIZE(s, obj.mWeakReferences, "");
    SERIALIZE(s, obj.mStrongReferences, "");
    return s;
}

bool AssetTypeMap::Read(DataType dataType, const String& path)
{
    File file;
    if (!file.Open(path, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        return false;
    }
    SizeT fileSize = static_cast<SizeT>(file.GetSize());
    if (fileSize == 0)
    {
        return false;
    }
    switch (dataType)
    {
        case BINARY:
        {
            MemoryBuffer buffer;
            if (!buffer.Allocate(fileSize, 1))
            {
                return false;
            }

            if (file.Read(buffer.GetData(), buffer.GetCapacity()) != fileSize)
            {
                return false;
            }

            BinaryStream s(Stream::MEMORY, &buffer, Stream::SM_READ);
            return SerializeCommon(s);
        }
        case JSON:
        {
            String text;
            text.Resize(fileSize);

            if (file.Read(const_cast<char*>(text.CStr()), text.Size()) != fileSize)
            {
                return false;
            }

            JsonStream s(Stream::TEXT, &text, Stream::SM_READ);
            return SerializeCommon(s);
        }
        default:
            return false;
    }
}

bool AssetTypeMap::Write(DataType dataType, const String& path)
{
    File file;
    if (!file.Open(path, FF_WRITE, FILE_OPEN_CREATE_NEW))
    {
        return false;
    }
    switch (dataType)
    {
    case BINARY:
    {
        MemoryBuffer buffer;
        BinaryStream s(Stream::MEMORY, &buffer, Stream::SM_WRITE);
        if (!SerializeCommon(s))
        {
            return false;
        }
        s.Close();

        if (file.Write(buffer.GetData(), buffer.GetCapacity()) != buffer.GetSize())
        {
            return false;
        }
        return true;
    }
    case JSON:
    {
        String text;

        JsonStream s(Stream::TEXT, &text, Stream::SM_PRETTY_WRITE);
        if (!SerializeCommon(s))
        {
            return false;
        }
        s.Close();

        if (file.Write(text.CStr(), text.Size()) != text.Size())
        {
            return false;
        }
        return true;
    }
    default:
        return false;
    }
}

bool AssetTypeMap::SerializeCommon(Stream& s)
{
    if (s.BeginObject("TypeMap", "Object"))
    {
        SERIALIZE_STRUCT_ARRAY(s, mTypes, "");
        s.EndObject();
        return true;
    }
    return false;
}



} // namespace lf