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
#include "Core/PCH.h"
#include "JsonStream.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/String/Token.h"
#include "Core/Runtime/ReflectionHooks.h"
#include "Core/Reflection/Type.h"
#include "Core/Utility/Guid.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

namespace lf {

class JsonStreamContext : public StreamContext
{
public:
    JsonStreamContext()
    : StreamContext()
    , mOutputText(nullptr)
    {
    }
    virtual ~JsonStreamContext() {}
    String* mOutputText;
};

class JsonWriteStreamContext : public JsonStreamContext
{
public:
    JsonWriteStreamContext()
    : JsonStreamContext()
    , mBuffer()
    , mWriter(mBuffer)
    {}

    rapidjson::StringBuffer mBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> mWriter;
};

class JsonPrettyWriteStreamContext : public JsonStreamContext
{
public:
    JsonPrettyWriteStreamContext()
        : JsonStreamContext()
        , mBuffer()
        , mWriter(mBuffer)
    {}

    rapidjson::StringBuffer mBuffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> mWriter;
};

class JsonReadStreamContext : public JsonStreamContext
{
public:
    JsonReadStreamContext()
    : JsonStreamContext()
    , mDocument()
    , mTop()
    {
    }

    rapidjson::Document mDocument;
    TVector<rapidjson::Value*> mTop;
    bool mValueReady;

    rapidjson::Value* Top()
    {
        return mValueReady ? mTop.back() : End();
    }

    void Select(const char* key)
    {
        auto iter = mTop.empty() ? mDocument.FindMember(key) : mTop.back()->FindMember(key);
        auto end = mTop.empty() ? mDocument.MemberEnd() : mTop.back()->MemberEnd();
        if (iter != end)
        {
            mTop.push_back(&iter->value);
            mValueReady = true;
        }
        else
        {
            mValueReady = false;
        }
    }

    void Pop()
    {
        if (!mTop.empty())
        {
            mTop.pop_back();
        }
        mValueReady = false;
    }

    rapidjson::Value* End()
    {
        return nullptr;
    }

    /*
    rapidjson::Document::MemberIterator Top()
    {
        return mValueReady ? mTop.GetLast() : End();
    }

    void Select(const char* key)
    {
        auto iter = mTop.Empty() ? mDocument.FindMember(key) : mTop.GetLast()->value.FindMember(key);
        if (iter != mDocument.MemberEnd())
        {
            mTop.Add(iter);
            mValueReady = true;
        }
        else
        {
            mValueReady = false;
        }
    }

    void Pop()
    {
        if (!mTop.Empty())
        {
            mTop.Remove(mTop.rbegin().GetBase());
        }
        mValueReady = false;
    }

    rapidjson::Document::MemberIterator End()
    {
        return mDocument.MemberEnd();
    }*/

};


JsonStream::JsonStream()
: Stream()
, mMemory()
, mContext(nullptr)
{
    memset(mMemory, 0, sizeof(mMemory));
}
JsonStream::JsonStream(Stream::StreamText, String* text, StreamMode mode)
: Stream()
, mMemory()
, mContext(nullptr)
{
    Open(Stream::TEXT, text, mode);
}

JsonStream::~JsonStream()
{
    Close();
}

void JsonStream::Open(Stream::StreamText, String* text, StreamMode mode)
{
    LF_STATIC_ASSERT(sizeof(mMemory) >= sizeof(JsonWriteStreamContext));
    LF_STATIC_ASSERT(sizeof(mMemory) >= sizeof(JsonPrettyWriteStreamContext));
    LF_STATIC_ASSERT(sizeof(mMemory) >= sizeof(JsonReadStreamContext));
    if (!text || (text->Empty() && mode == SM_READ))
    {
        return;
    }
    if (mContext)
    {
        Close();
    }

    mContext = mode == SM_READ
        ? static_cast<JsonStreamContext*>(new(mMemory)JsonReadStreamContext())
        : mode == SM_PRETTY_WRITE
            ? static_cast<JsonStreamContext*>(new(mMemory)JsonPrettyWriteStreamContext())
            : static_cast<JsonStreamContext*>(new(mMemory)JsonWriteStreamContext());


    mContext->mType = StreamContext::TEXT;
    mContext->mMode = mode;
    mContext->mOutputText = nullptr;

    switch (mode)
    {
        case SM_READ:
        {
            if (Reader()->mDocument.Parse(text->CStr()).HasParseError())
            {
                Close();
                return;
            }
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.StartObject();
            mContext->mOutputText = text;
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.StartObject();
            mContext->mOutputText = text;
        } break;
    }

    return;
}
void JsonStream::Close()
{
    if (mContext)
    {
        switch (GetMode())
        {
            case SM_READ:
            {
            } break;
            case SM_PRETTY_WRITE:
            {
                PrettyWriter()->mWriter.EndObject();
                *PrettyWriter()->mOutputText = PrettyWriter()->mBuffer.GetString();
            } break;
            case SM_WRITE:
            {
                Writer()->mWriter.EndObject();
                *Writer()->mOutputText = Writer()->mBuffer.GetString();
            } break;
        }

        mContext->~JsonStreamContext();
        mContext = nullptr;
        memset(mMemory, 0, sizeof(mMemory));
    }
}
void JsonStream::Clear()
{

}

// Normal Value Serialization
void JsonStream::Serialize(UInt8& value)
{
    UInt32 v = static_cast<UInt32>(value);
    Serialize(v);
    value = static_cast<UInt8>(v);
}
void JsonStream::Serialize(UInt16& value)
{
    UInt16 v = static_cast<UInt16>(value);
    Serialize(v);
    value = static_cast<UInt8>(v);
}
void JsonStream::Serialize(UInt32& value)
{
    switch (GetMode())
    {
        case SM_READ:
        {
            auto iter = Reader()->Top();
            if (iter != Reader()->End())
            {
                if (iter->IsUint())
                {
                    value = iter->GetUint();
                }
                Reader()->Pop();
            }
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.Uint(value);
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.Uint(value);
        } break;
    }
} 
void JsonStream::Serialize(UInt64& value)
{
    switch (GetMode())
    {
        case SM_READ:
        {
            auto iter = Reader()->Top();
            if (iter != Reader()->End())
            {
                if (iter->IsUint64())
                {
                    value = iter->GetUint64();
                }
                Reader()->Pop();
            }
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.Uint64(value);
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.Uint64(value);
        } break;
    }
} 
void JsonStream::Serialize(Int8& value)
{
    UInt16 v = static_cast<UInt16>(value);
    Serialize(v);
    value = static_cast<UInt8>(v);
}
void JsonStream::Serialize(Int16& value)
{
    UInt16 v = static_cast<UInt16>(value);
    Serialize(v);
    value = static_cast<UInt8>(v);
} 
void JsonStream::Serialize(Int32& value)
{
    switch (GetMode())
    {
        case SM_READ:
        {
            auto iter = Reader()->Top();
            if (iter != Reader()->End())
            {
                if (iter->IsInt())
                {
                    value = iter->GetInt();
                }
                Reader()->Pop();
            }
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.Int(value);
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.Int(value);
        } break;
    }
} 
void JsonStream::Serialize(Int64& value)
{
    switch (GetMode())
    {
        case SM_READ:
        {
            auto iter = Reader()->Top();
            if (iter != Reader()->End())
            {
                if (iter->IsInt64())
                {
                    value = iter->GetInt64();
                }
                Reader()->Pop();
            }
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.Int64(value);
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.Int64(value);
        } break;
    }
} 
void JsonStream::Serialize(Float32& value)
{
    switch (GetMode())
    {
        case SM_READ:
        {
            auto iter = Reader()->Top();
            if (iter != Reader()->End())
            {
                if (iter->IsFloat())
                {
                    value = iter->GetFloat();
                }
                Reader()->Pop();
            }
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.Double(static_cast<Float32>(value));
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.Double(static_cast<Float32>(value));
        } break;
    }
} 
void JsonStream::Serialize(Float64& value)
{
    switch (GetMode())
    {
        case SM_READ:
        {
            auto iter = Reader()->Top();
            if (iter != Reader()->End())
            {
                if (iter->IsDouble())
                {
                    value = iter->GetDouble();
                }
                Reader()->Pop();
            }
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.Double(value);
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.Double(value);
        } break;
    }
} 
void JsonStream::Serialize(Vector2& value)
{
    if (IsReading())
    {
        String str;
        Serialize(str);
        ToVector2(str, value);
    }
    else
    {
        String str = ToString(value);
        Serialize(str);
    }
} 
void JsonStream::Serialize(Vector3& value)
{
    if (IsReading())
    {
        String str;
        Serialize(str);
        ToVector3(str, value);
    }
    else
    {
        String str = ToString(value);
        Serialize(str);
    }
} 
void JsonStream::Serialize(Vector4& value)
{
    if (IsReading())
    {
        String str;
        Serialize(str);
        ToVector4(str, value);
    }
    else
    {
        String str = ToString(value);
        Serialize(str);
    }
} 
void JsonStream::Serialize(Color& value)
{
    if (IsReading())
    {
        String str;
        Serialize(str);
        ToColor(str, value);
    }
    else
    {
        String str = ToString(value);
        Serialize(str);
    }
}
void JsonStream::Serialize(String& value)
{
        switch (GetMode())
    {
        case SM_READ:
        {
            auto iter = Reader()->Top();
            if (iter != Reader()->End())
            {
                value = iter->GetString();
                Reader()->Pop();
            }
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.String(value.CStr());
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.String(value.CStr());
        } break;
    }
}
void JsonStream::Serialize(Token& value)
{
    if (IsReading())
    {
        String str = value.CStr();
        Serialize(str);
        value = Token(str);
    }
    else
    {
        String str(value.Size(), value.CStr(), COPY_ON_WRITE);
        Serialize(str);
    }

}
void JsonStream::Serialize(const Type*& value)
{
    if (IsReading())
    {
        Token typeName;
        Serialize(typeName);
        if (typeName.Empty())
        {
            value = nullptr;
        }
        else
        {
            value = InternalHooks::gFindType(typeName);
        }
    }
    else
    {
        // todo: const_cast and ref hack would be slightly faster...
        Token typeName = value ? value->GetFullName() : Token();
        Serialize(typeName);
    }
}
void JsonStream::SerializeGuid(ByteT* value, SizeT size)
{
    if (IsReading())
    {
        String str;
        Serialize(str);
        ToGuid(str, value, size);
    }
    else
    {
        String str = ToString(value, size);
        Serialize(str);
    }
}
void JsonStream::SerializeAsset(Token& value, bool isWeak)
{
    (isWeak);
    Serialize(value);
}
void JsonStream::Serialize(const StreamPropertyInfo& info)
{
    switch (GetMode())
    {
        case SM_READ:
        {
            Reader()->Select(info.name.CStr());
        } break;
        case SM_WRITE:
        {
            Writer()->mWriter.Key(info.name.CStr());
        } break;
        case SM_PRETTY_WRITE:
        {
            PrettyWriter()->mWriter.Key(info.name.CStr());
        } break;
    }
}
void JsonStream::Serialize(const ArrayPropertyInfo& info)
{
    switch (GetMode())
    {
        case SM_READ:
        {
            auto iter = Reader()->mTop.empty() ? Reader()->End() : Reader()->mTop.back();
            if (iter != Reader()->End() && iter->IsArray())
            {
                Reader()->mTop.push_back(&iter->GetArray()[static_cast<rapidjson::SizeType>(info.index)]);
                Reader()->mValueReady = true;
            }
        } break;
        case SM_WRITE:
        {
        } break;
        case SM_PRETTY_WRITE:
        {
        } break;
    }
}

void JsonStream::Serialize(MemoryBuffer& value)
{
    if (IsReading())
    {
        String hexValue;
        Serialize(hexValue);

        value.Allocate(hexValue.Size() * 2, 1);
        value.SetSize(hexValue.Size() * 2);
        ToGuid(hexValue, reinterpret_cast<ByteT*>(value.GetData()), value.GetSize());
    }
    else
    {
        String hexValue = ToString(reinterpret_cast<ByteT*>(value.GetData()), value.GetSize());
        Serialize(hexValue);
    }
}

// Named objects not supported.
bool JsonStream::BeginObject(const String& name, const String& super)
{
    (name);
    (super);
    return true;
}
void JsonStream::EndObject()
{

}

bool JsonStream::BeginStruct()
{
    switch (GetMode())
    {
        case SM_READ:
        {
            bool isObject = Reader()->Top() != Reader()->End() ? Reader()->Top()->IsObject() : false;
            return isObject;
        } break;
        case SM_WRITE:
        {
            return Writer()->mWriter.StartObject();
        } break;
        case SM_PRETTY_WRITE:
        {
            return PrettyWriter()->mWriter.StartObject();
        } break;
    }
    return false;
}
void JsonStream::EndStruct()
{
    switch (GetMode())
    {
        case SM_READ:
        {
            Reader()->Pop();
        } break;
        case SM_WRITE:
        {
            ReportBug(Writer()->mWriter.EndObject());
        } break;
        case SM_PRETTY_WRITE:
        {
            ReportBug(PrettyWriter()->mWriter.EndObject());
        } break;
    }
}

bool JsonStream::BeginArray()
{
    switch (GetMode())
    {
        case SM_READ:
        {
            bool isArray = Reader()->Top() != Reader()->End() ? Reader()->Top()->IsArray() : false;
            return isArray;
        } break;
        case SM_WRITE:
        {
            return Writer()->mWriter.StartArray();
        } break;
        case SM_PRETTY_WRITE:
        {
            return PrettyWriter()->mWriter.StartArray();
        } break;
    }
    return false;
}
void JsonStream::EndArray()
{
    switch (GetMode())
    {
        case SM_READ:
        {
            Reader()->Pop();
        } break;
        case SM_WRITE:
        {
            ReportBug(Writer()->mWriter.EndArray());
        } break;
        case SM_PRETTY_WRITE:
        {
            ReportBug(PrettyWriter()->mWriter.EndArray());
        } break;
    }
}
size_t JsonStream::GetArraySize() const
{
    if (GetMode() == SM_READ)
    {
        auto iter = const_cast<JsonStream*>(this)->Reader()->Top();
        if (iter != const_cast<JsonStream*>(this)->Reader()->End() && iter->IsArray())
        {
            SizeT size = iter->GetArray().Size();
            return size;
        }
    }
    return 0;
}
void JsonStream::SetArraySize(size_t size)
{
    (size);
    LF_DEBUG_BREAK;
}
StreamContext* JsonStream::PopContext()
{
    return nullptr;
}
const StreamContext* JsonStream::GetContext() const
{
    return mContext;
}
void JsonStream::SetContext(StreamContext* context)
{
    (context);
}

JsonWriteStreamContext* JsonStream::Writer()
{
    Assert(GetMode() == SM_WRITE);
    return static_cast<JsonWriteStreamContext*>(mContext);
}
JsonPrettyWriteStreamContext* JsonStream::PrettyWriter()
{
    Assert(GetMode() == SM_PRETTY_WRITE);
    return static_cast<JsonPrettyWriteStreamContext*>(mContext);
}
JsonReadStreamContext* JsonStream::Reader()
{
    Assert(GetMode() == SM_READ);
    return static_cast<JsonReadStreamContext*>(mContext);
}

JsonObjectStream::JsonObjectStream() 
: JsonStream()
, mSerializingObject(false)
, mCurrentSuper()
{
}
JsonObjectStream::JsonObjectStream(Stream::StreamText, String* text, StreamMode mode)
: JsonStream(Stream::TEXT, text, mode)
, mSerializingObject(false)
, mCurrentSuper()
{}

bool JsonObjectStream::BeginObject(const String& name, const String& super)
{
    if (mSerializingObject)
    {
        return false;
    }

    StreamPropertyInfo objectProperty(name);

    mSerializingObject = (*this << objectProperty).BeginStruct();

    if (mSerializingObject)
    {
        mCurrentSuper = super;
        SERIALIZE_NAMED(*this, "__super", mCurrentSuper, "");
        StreamPropertyInfo objectData("__data", "");
        mSerializingObject = (*this << objectData).BeginStruct();
        if (!mSerializingObject)
        {
            mCurrentSuper.Clear();
            EndStruct(); // End the property serialize
        }
    }
    return mSerializingObject;
}

void JsonObjectStream::EndObject()
{
    if (mSerializingObject)
    {
        EndStruct(); // end the data serialization
        EndStruct(); // end the property serialization
        mSerializingObject = false;
        mCurrentSuper.Clear();
    }
}

} // namespace lf 
