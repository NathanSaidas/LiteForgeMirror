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
#pragma once
#include "Core/IO/Stream.h"

namespace lf {

class JsonStreamContext;
class JsonWriteStreamContext;
class JsonPrettyWriteStreamContext;
class JsonReadStreamContext;

class LF_CORE_API JsonStream : public Stream
{
public:
    JsonStream();
    JsonStream(Stream::StreamText, String* text, StreamMode mode);
    ~JsonStream() override;


    void Open(Stream::StreamText, String* text, StreamMode mode);
    void Close() override;
    void Clear() override;

    // Normal Value Serialization
    void Serialize(UInt8& value) override;
    void Serialize(UInt16& value) override;
    void Serialize(UInt32& value) override;
    void Serialize(UInt64& value) override;
    void Serialize(Int8& value) override;
    void Serialize(Int16& value) override;
    void Serialize(Int32& value) override;
    void Serialize(Int64& value) override;
    void Serialize(Float32& value) override;
    void Serialize(Float64& value) override;
    void Serialize(Vector2& value) override;
    void Serialize(Vector3& value) override;
    void Serialize(Vector4& value) override;
    void Serialize(Color& value) override;
    void Serialize(String& value) override;
    void Serialize(Token& value) override;
    void Serialize(const Type*& value) override;
    void SerializeGuid(ByteT* value, SizeT size) override;
    void SerializeAsset(Token& value, bool isWeak) override;
    void Serialize(const StreamPropertyInfo& info) override;
    void Serialize(const ArrayPropertyInfo& info) override;
    void Serialize(MemoryBuffer& value) override;

    // Named objects not supported.
    bool BeginObject(const String& name, const String& super) override;
    void EndObject() override;

    bool BeginStruct() override;
    void EndStruct() override;

    bool BeginArray() override;
    void EndArray() override;
    size_t GetArraySize() const override;
    void SetArraySize(size_t size) override;
protected:
    StreamContext* PopContext() override;
    const StreamContext* GetContext() const override;
    void SetContext(StreamContext* context) override;

private:
    JsonWriteStreamContext* Writer();
    JsonPrettyWriteStreamContext* PrettyWriter();
    JsonReadStreamContext* Reader();



    ByteT      mMemory[200];
    JsonStreamContext* mContext;
};

/// JsonObjectStream is similar to Json stream
/// 1. Reads/Writes json
/// 2. Formatted object json (all meta data is prefixed __) 
/// 
/// <object type> : {
///     __super : <object super> 
///     __data : { <object data> } 
/// }
/// 
class LF_CORE_API JsonObjectStream : public JsonStream
{
public:
    JsonObjectStream();
    JsonObjectStream(Stream::StreamText, String* text, StreamMode mode);

    bool BeginObject(const String& name, const String& super) override;
    void EndObject();

    const String& GetCurrentSuper() const { return mCurrentSuper; }
private:
    bool mSerializingObject;
    String mCurrentSuper;
};

}