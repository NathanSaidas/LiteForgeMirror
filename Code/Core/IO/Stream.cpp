// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#include "Stream.h"
#include "Core/String/StringUtil.h"
#include "Core/Memory/Memory.h"

namespace lf
{
// static void FormatString(const String& inString, String& outString)
// {
//     size_t trimFront = 0;
//     if (inString.GetSize() > 2)
//     {
//         if (inString[0] == 'm' && CharIsUpper(inString[1]))
//         {
//             ++trimFront;
//         }
//         if (inString[1] == '_')
//         {
//             ++trimFront;
//         }
//         outString = inString.SubString(trimFront);
//         if (!outString.Empty())
//         {
//             outString[0] = ToUpper(outString[0]);
//         }
//     }
//     else
//     {
//         outString = inString;
//         if (!outString.Empty())
//         {
//             outString[0] = ToUpper(outString[0]);
//         }
//     }
// }

StreamPropertyInfo::StreamPropertyInfo(const String& inName) : name(inName)
{
}
StreamPropertyInfo::StreamPropertyInfo(const char* inName, const String&) : name()
{
    String temp(inName, COPY_ON_WRITE);
    SizeT dot = temp.FindLast('.');
    SizeT offset = 0;
    if (Valid(dot))
    {
        offset = dot + 1;
        temp = String(inName + offset, COPY_ON_WRITE);
    }

    // change mColor => Color leave map as map though and m as m
    if (temp.Size() >= 2)
    {
        if (temp.First() == 'm' && CharIsUpper(temp[1]))
        {
            ++offset;
        }
    }

    name = String(inName + offset, COPY_ON_WRITE);
}

void StreamBufferObject::Clear()
{
    if (data)
    {
        LFFree(data);
        data = nullptr;
        length = 0;
    }
}

Stream::Stream()
{

}
Stream::~Stream()
{

}

void Stream::Open(Stream::StreamText, String*, StreamMode)
{

}

void Stream::Open(Stream::StreamMemory, MemoryBuffer*, StreamMode)
{

}
void Stream::Open(Stream::StreamFile, const String&, StreamMode)
{

}
void Stream::Close()
{

}
void Stream::Clear()
{

}

void Stream::Serialize(bool& value)
{
    UInt8 byte = value ? 1 : 0;
    Serialize(byte);
    value = byte ? true : false;
}

void Stream::Serialize(UInt8&)
{

}
void Stream::Serialize(UInt16&)
{

}
void Stream::Serialize(UInt32&)
{

}
void Stream::Serialize(UInt64&)
{

}
void Stream::Serialize(Int8&)
{

}
void Stream::Serialize(Int16&)
{

}
void Stream::Serialize(Int32&)
{

}
void Stream::Serialize(Int64&)
{

}
void Stream::Serialize(Float32&)
{

}
void Stream::Serialize(Float64&)
{

}
void Stream::Serialize(Vector2&)
{

}
void Stream::Serialize(Vector3&)
{

}
void Stream::Serialize(Vector4&)
{

}
void Stream::Serialize(Vector& )
{

}
void Stream::Serialize(Quaternion& )
{

}
void Stream::Serialize(Color&)
{

}
void Stream::Serialize(String&)
{

}
void Stream::Serialize(Token&)
{

}
void Stream::Serialize(const Type*&)
{

}
void Stream::SerializeGuid(ByteT*, SizeT)
{

}
void Stream::SerializeAsset(Token&, bool)
{

}
void Stream::Serialize(const StreamPropertyInfo&)
{

}

bool Stream::BeginObject(const String&, const String&)
{
    return(false);
}
void Stream::EndObject()
{

}

bool Stream::BeginStruct()
{
    return(false);
}
void Stream::EndStruct()
{

}

bool Stream::BeginArray()
{
    return(false);
}
void Stream::EndArray()
{

}
size_t Stream::GetArraySize() const
{
    return 0;
}

void Stream::SetArraySize(size_t)
{

}

bool Stream::IsReading() const
{
    StreamMode mode = GetMode();
    return mode == SM_READ;
}

Stream::StreamMode Stream::GetMode() const
{
    return GetContext() ? GetContext()->mMode : SM_CLOSED;
}

Stream::AssetLoadFlags Stream::GetAssetLoadFlags() const
{
    return GetContext() ? GetContext()->mFlags : 0;
}

size_t Stream::GetObjectCount() const
{
    return 0;
}
const String& Stream::GetObjectName(const size_t) const
{
    return EMPTY_STRING;
}
const String& Stream::GetObjectSuper(const size_t) const
{
    return EMPTY_STRING;
}

StreamContext* Stream::PopContext()
{
    return nullptr;
}

const StreamContext* Stream::GetContext() const
{
    return nullptr;
}

void Stream::SetContext(StreamContext*)
{

}

StreamContext* Stream::AllocContext(size_t size, size_t alignment)
{
    return static_cast<StreamContext*>(LFAlloc(size, alignment));
}
void Stream::FreeContext(StreamContext*& context)
{
    LFFree(context);
    context = nullptr;
}
}