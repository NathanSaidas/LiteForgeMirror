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
#ifndef LF_CORE_BINARY_STREAM_H
#define LF_CORE_BINARY_STREAM_H

#include "Core/IO/Stream.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Utility/Array.h"

namespace lf
{
    class LF_CORE_API BinaryStream : public Stream
    {
    public:
        BinaryStream();
        BinaryStream(Stream::StreamMemory, MemoryBuffer* buffer, StreamMode mode);
        BinaryStream(Stream::StreamFile, const String& filename, StreamMode mode);
        BinaryStream(StreamContext*& context);
        ~BinaryStream();

        void Open(Stream::StreamText, String* data, StreamMode mode) override;
        void Open(Stream::StreamMemory, MemoryBuffer* buffer, StreamMode mode) override;
        void Open(Stream::StreamFile, const String& filename, StreamMode mode) override;
        void Close() override;
        void Clear() override;

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
        void SerializeBuffer(MemoryBuffer& buffer);

        bool BeginObject(const String& name, const String& super) override;
        void EndObject() override;

        bool BeginStruct() override;
        void EndStruct() override;

        bool BeginArray() override;
        void EndArray() override;
        size_t GetArraySize() const override;
        void SetArraySize(size_t size) override;

        virtual StreamContext* PopContext() override;
        virtual const StreamContext* GetContext() const override;
        virtual void SetContext(StreamContext* context) override;

        size_t GetObjectCount() const override;
        const String& GetObjectName(const size_t index) const override;
        const String& GetObjectSuper(const size_t index) const override;

        size_t GetCursor() const;
    private:
        struct ObjectInfo
        {
            String name;
            String super;
            size_t location;
            size_t size;
        };

        class BinaryStreamContext : public StreamContext
        {
        public:
            TArray<ObjectInfo> objects;
            String filename;
            size_t cursor;
            size_t arraySize;
            MemoryBuffer* buffer;
            bool destroyBufferOnClear;
        };

        void WriteBytes(const void* buffer, size_t numBytes);
        void* ReadBytes(size_t numBytes);
        void* ReverseRead(size_t numBytes);

        BinaryStreamContext* mContext;
    };

}

#endif // LF_CORE_BINARY_STREAM_H
