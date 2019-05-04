// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "BinaryStream.h"


#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Color.h"
#include "Core/Reflection/Type.h"
#include "Core/String/String.h"
#include "Core/Platform/File.h"
#include "Core/Runtime/ReflectionHooks.h"
#include "Core/Utility/Utility.h"

#include <map>

namespace lf
{
    BinaryStream::BinaryStream() : Stream(), mContext(nullptr)
    {}
    BinaryStream::BinaryStream(Stream::StreamMemory, MemoryBuffer* buffer, StreamMode mode) : Stream(), mContext(nullptr)
    {
        Open(MEMORY, buffer, mode);
    }
    BinaryStream::BinaryStream(Stream::StreamFile, const String& filename, StreamMode mode) : Stream(), mContext(nullptr)
    {
        Open(FILE, filename, mode);
    }
    BinaryStream::BinaryStream(StreamContext*& context) : Stream(), mContext(nullptr)
    {
        SetContext(context);
        context = nullptr;
    }

    BinaryStream::~BinaryStream()
    {
        Close();
    }

    void BinaryStream::Open(Stream::StreamText, String* data, StreamMode mode)
    {
        (data);
        (mode);
        Crash("BinaryStream can only open from memory or file.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    }

    void BinaryStream::Open(Stream::StreamMemory, MemoryBuffer* buffer, StreamMode mode)
    {
        AssertError(buffer, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
        if (!mContext)
        {
            void* memory = AllocContext(sizeof(BinaryStreamContext), alignof(BinaryStreamContext));
            mContext = new(memory)BinaryStreamContext();
            mContext->mType = StreamContext::BINARY;
            mContext->arraySize = INVALID;
            mContext->buffer = buffer;
            mContext->destroyBufferOnClear = false;
            Assert(memory == mContext);
        }
        else
        {
            Clear();
        }

        mContext->mMode = mode;

        // Read Mode:
        if (mode == SM_READ)
        {
            mContext->buffer = buffer;
            mContext->destroyBufferOnClear = false;
            // TODO: Read Footer:

            // Seek End:
            Assert(mContext->buffer->GetCapacity() == mContext->buffer->GetSize());
            mContext->cursor = mContext->buffer->GetCapacity();
            // Read Footer:
            UInt32* footerSize = reinterpret_cast<UInt32*>(ReverseRead(4));
            for (SizeT i = 0, size = static_cast<SizeT>(*footerSize); i < size; ++i)
            {
                UInt32* nameSize = reinterpret_cast<UInt32*>(ReverseRead(4));
                UInt32* superSize = reinterpret_cast<UInt32*>(ReverseRead(4));
                UInt32* objLocation = reinterpret_cast<UInt32*>(ReverseRead(4));
                UInt32* objSize = reinterpret_cast<UInt32*>(ReverseRead(4));

                String name(static_cast<SizeT>(*nameSize), reinterpret_cast<char*>(ReverseRead(static_cast<SizeT>(*nameSize))));
                String super(static_cast<SizeT>(*superSize), reinterpret_cast<char*>(ReverseRead(static_cast<SizeT>(*superSize))));

                ObjectInfo info;
                info.name = name;
                info.super = super;
                info.location = static_cast<SizeT>(*objLocation);
                info.size = static_cast<SizeT>(*objSize);
                mContext->objects.Add(info);
            }
        }
        // Write Mode:
        else if (mode == SM_WRITE)
        {
            mContext->buffer = buffer;
            mContext->buffer->Allocate(4096, LF_SIMD_ALIGN);
            mContext->buffer->SetSize(0);
            mContext->cursor = 0;
        }
    }

    void BinaryStream::Open(Stream::StreamFile, const String& filename, StreamMode mode)
    {
        if (!mContext)
        {
            void* memory = AllocContext(sizeof(BinaryStreamContext), alignof(BinaryStreamContext));
            mContext = new(memory)BinaryStreamContext();
            mContext->mType = StreamContext::BINARY;
            mContext->arraySize = INVALID;
            mContext->buffer = nullptr;
            mContext->destroyBufferOnClear = false;
            Assert(memory == mContext);
        }
        else
        {
            Clear();
        }

        mContext->filename = filename;
        mContext->mMode = mode;

        if (!mContext->buffer)
        {
            mContext->buffer = LFNew<MemoryBuffer>();
            mContext->destroyBufferOnClear = true;
        }

        if (mode == SM_READ)
        {
            // File file(filename, io::FM_READ);
            File file;
            file.Open(filename, FF_SHARE_READ | FF_READ, FILE_OPEN_EXISTING);
            if (file.IsOpen())
            {
                mContext->buffer->Allocate(file.GetSize(), LF_SIMD_ALIGN);
                UInt8* bufferData = reinterpret_cast<UInt8*>(mContext->buffer->GetData());
                file.Read(bufferData, mContext->buffer->GetCapacity());
                file.Close();

                // Seek End:
                mContext->cursor = mContext->buffer->GetCapacity();
                // Read Footer:
                UInt32* footerSize = reinterpret_cast<UInt32*>(ReverseRead(4));
                for (SizeT i = 0, size = static_cast<SizeT>(*footerSize); i < size; ++i)
                {
                    UInt32* nameSize = reinterpret_cast<UInt32*>(ReverseRead(4));
                    UInt32* superSize = reinterpret_cast<UInt32*>(ReverseRead(4));
                    UInt32* objLocation = reinterpret_cast<UInt32*>(ReverseRead(4));
                    UInt32* objSize = reinterpret_cast<UInt32*>(ReverseRead(4));

                    String name(static_cast<SizeT>(*nameSize), reinterpret_cast<char*>(ReverseRead(static_cast<SizeT>(*nameSize))));
                    String super(static_cast<SizeT>(*superSize), reinterpret_cast<char*>(ReverseRead(static_cast<SizeT>(*superSize))));

                    ObjectInfo info;
                    info.name = name;
                    info.super = super;
                    info.location = static_cast<SizeT>(*objLocation);
                    info.size = static_cast<SizeT>(*objSize);
                    mContext->objects.Add(info);
                }

            }
        }
    }

    void BinaryStream::Close()
    {
        if (!mContext)
        {
            return;
        }

        if (!IsReading())
        {
            // Save Footer:
            for (SizeT i = 0, size = mContext->objects.Size(); i < size; ++i)
            {
                const ObjectInfo& info = mContext->objects[i];

                UInt32 superSize = static_cast<UInt32>(info.super.Size());
                WriteBytes(info.super.CStr(), superSize);

                UInt32 nameSize = static_cast<UInt32>(info.name.Size());
                WriteBytes(info.name.CStr(), nameSize);

                UInt32 objLocation = static_cast<UInt32>(info.location);
                UInt32 objSize = static_cast<UInt32>(info.size);
                WriteBytes(&objSize, 4);
                WriteBytes(&objLocation, 4);
                WriteBytes(&superSize, 4);
                WriteBytes(&nameSize, 4);
            }
            UInt32 footerSize = static_cast<UInt32>(mContext->objects.Size());
            WriteBytes(&footerSize, 4);

            // File file(mContext->filename, io::FM_WRITE);
            File file;
            file.Open(mContext->filename, FF_WRITE, FILE_OPEN_ALWAYS);
            if (file.IsOpen())
            {
                file.Write(mContext->buffer->GetData(), mContext->buffer->GetSize());
                file.Close();
            }
        }

        Clear();
        mContext->~BinaryStreamContext();
        FreeContext(reinterpret_cast<StreamContext*&>(mContext));

    }

    void BinaryStream::Clear()
    {
        if (!mContext)
        {
            return;
        }

        if (mContext->destroyBufferOnClear && mContext->buffer)
        {
            mContext->buffer->Free();
            LFDelete(mContext->buffer);
            mContext->buffer = nullptr;
        }

        mContext->objects.Clear();
        mContext->filename.Clear();
        mContext->arraySize = INVALID;
        mContext->cursor = 0;
    }


    void BinaryStream::Serialize(UInt8& value)
    {
        if (IsReading())
        {
            UInt8* bufferPtr = reinterpret_cast<UInt8*>(ReadBytes(1));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 1);
        }
    }
    void BinaryStream::Serialize(UInt16& value)
    {
        if (IsReading())
        {
            UInt16* bufferPtr = reinterpret_cast<UInt16*>(ReadBytes(2));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 2);
        }
    }
    void BinaryStream::Serialize(UInt32& value)
    {
        if (IsReading())
        {
            UInt32* bufferPtr = reinterpret_cast<UInt32*>(ReadBytes(4));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 4);
        }
    }
    void BinaryStream::Serialize(UInt64& value)
    {
        if (IsReading())
        {
            UInt64* bufferPtr = reinterpret_cast<UInt64*>(ReadBytes(8));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 8);
        }
    }
    void BinaryStream::Serialize(Int8& value)
    {
        if (IsReading())
        {
            Int8* bufferPtr = reinterpret_cast<Int8*>(ReadBytes(1));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 1);
        }
    }
    void BinaryStream::Serialize(Int16& value)
    {
        if (IsReading())
        {
            Int16* bufferPtr = reinterpret_cast<Int16*>(ReadBytes(2));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 2);
        }
    }
    void BinaryStream::Serialize(Int32& value)
    {
        if (IsReading())
        {
            Int32* bufferPtr = reinterpret_cast<Int32*>(ReadBytes(4));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 4);
        }
    }
    void BinaryStream::Serialize(Int64& value)
    {
        if (IsReading())
        {
            Int64* bufferPtr = reinterpret_cast<Int64*>(ReadBytes(8));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 8);
        }
    }
    void BinaryStream::Serialize(Float32& value)
    {
        if (IsReading())
        {
            Float32* bufferPtr = reinterpret_cast<Float32*>(ReadBytes(4));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 4);
        }
    }
    void BinaryStream::Serialize(Float64& value)
    {
        if (IsReading())
        {
            Float64* bufferPtr = reinterpret_cast<Float64*>(ReadBytes(8));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 8);
        }
    }
    void BinaryStream::Serialize(Vector2& value)
    {
        if (IsReading())
        {
            Vector2* bufferPtr = reinterpret_cast<Vector2*>(ReadBytes(8));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 8);
        }
    }
    void BinaryStream::Serialize(Vector3& value)
    {
        if (IsReading())
        {
            Vector3* bufferPtr = reinterpret_cast<Vector3*>(ReadBytes(12));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 12);
        }
    }
    void BinaryStream::Serialize(Vector4& value)
    {
        if (IsReading())
        {
            Vector4* bufferPtr = reinterpret_cast<Vector4*>(ReadBytes(16));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 16);
        }
    }
    void BinaryStream::Serialize(Color& value)
    {
        if (IsReading())
        {
            Color* bufferPtr = reinterpret_cast<Color*>(ReadBytes(16));
            value = *bufferPtr;
        }
        else
        {
            WriteBytes(&value, 16);
        }
    }
    void BinaryStream::Serialize(String& value)
    {
        if (IsReading())
        {
            UInt32* size = reinterpret_cast<UInt32*>(ReadBytes(4));
            value.Resize(*size);
            // Can't edit COW string
            if (value.CopyOnWrite())
            {
                value = value.CStr();
            }
            const char* bytes = reinterpret_cast<const char*>(ReadBytes(static_cast<size_t>(*size)));
            char* valueRaw = const_cast<char*>(value.CStr());
            for (UInt32 i = 0; i < *size; ++i)
            {
                valueRaw[i] = bytes[i];
            }
        }
        else
        {
            UInt32 size = static_cast<UInt32>(value.Size());
            WriteBytes(&size, 4);
            WriteBytes(value.CStr(), value.Size());
        }
    }
    void BinaryStream::Serialize(Token& value)
    {
        if (IsReading())
        {
            String tmp;
            Serialize(tmp);
            value = Token(tmp);
        }
        else
        {
            UInt32 size = static_cast<UInt32>(value.Size());
            WriteBytes(&size, 4);
            WriteBytes(value.CStr(), value.Size());
        }
    }
    void BinaryStream::Serialize(const Type*& value)
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

    void BinaryStream::SerializeGuid(ByteT* value, SizeT size)
    {
        if (IsReading())
        {
            ByteT* bufferPtr = reinterpret_cast<ByteT*>(ReadBytes(size));
            for (SizeT i = 0; i < size; ++i)
            {
                value[i] = bufferPtr[i];
            }
        }
        else
        {
            WriteBytes(value, size);
        }
    }

    void BinaryStream::SerializeAsset(Token& value, bool)
    {
        Serialize(value);
    }

    void BinaryStream::Serialize(const StreamPropertyInfo&)
    {
        // Do nothing, BinaryStream format doesn't care for names or anything from the property info.
    }

    void BinaryStream::SerializeBuffer(MemoryBuffer& buffer)
    {
        if (IsReading())
        {
            UInt32 size = 0;
            Serialize(size);
            if (size == 0)
            {
                buffer.Free();
            }
            else
            {
                buffer.Allocate(static_cast<SizeT>(size), LF_SIMD_ALIGN);
                void* data = ReadBytes(static_cast<SizeT>(size));
                memcpy(buffer.GetData(), data, static_cast<SizeT>(size));
            }
        }
        else
        {
            UInt32 size = static_cast<UInt32>(buffer.GetSize());
            Serialize(size);
            if (size != 0)
            {
                WriteBytes(buffer.GetData(), buffer.GetSize());
            }
        }
    }

    bool BinaryStream::BeginObject(const String& name, const String& super)
    {
        Assert(mContext)

            if (IsReading())
            {
                for (TArray<ObjectInfo>::iterator it = mContext->objects.begin(), end = mContext->objects.end(); it != end; ++it)
                {
                    if (it->name == name && it->super == super)
                    {
                        mContext->cursor = it->location;
                        return true;
                    }
                }
                return false;
            }
            else
            {
                ObjectInfo info;
                info.name = name;
                info.super = super;
                info.location = mContext->cursor;
                info.size = 0;
                mContext->objects.Add(info);
            }
        return true;
    }
    void BinaryStream::EndObject()
    {
        Assert(mContext);
        Assert(!mContext->objects.Empty()); // If this trips its because we forgot a BeginObject somewhere
        if (!IsReading())
        {
            ObjectInfo& obj = mContext->objects.GetLast();
            obj.size = GetCursor() - obj.location;
        }
    }

    bool BinaryStream::BeginStruct()
    {
        // Do nothing, BinaryStream format doesn't care for structs
        return true;
    }

    void BinaryStream::EndStruct()
    {
        // Do nothing, BinaryStream format doesn't care for structs
    }

    bool BinaryStream::BeginArray()
    {
        if (IsReading())
        {
            UInt32 size = 0;
            Serialize(size);
            mContext->arraySize = static_cast<size_t>(size);
        }
        return true;
    }

    void BinaryStream::EndArray()
    {
        // Do nothing, BinaryStream format doesn't care for arrays
    }

    size_t BinaryStream::GetArraySize() const
    {
#ifdef SH_DEBUG
        // Pop off and make invalid, easier to read errors.
        size_t tmp = mContext->arraySize;
        mContext->arraySize = INVALID;
        return tmp;
#else 
        return mContext->arraySize;
#endif

    }

    void BinaryStream::SetArraySize(size_t size)
    {
        if (!IsReading())
        {
            Serialize(reinterpret_cast<UInt32&>(size));
        }
    }

    StreamContext* BinaryStream::PopContext()
    {
        StreamContext* context = mContext;
        mContext = nullptr;
        return context;
    }
    const StreamContext* BinaryStream::GetContext() const
    {
        return mContext;
    }
    void BinaryStream::SetContext(StreamContext* context)
    {
        if (context && context->mType == StreamContext::BINARY)
        {
            if (mContext)
            {
                // mContext->data.clear();
                mContext->filename.Clear();
                mContext->~BinaryStreamContext();
                FreeContext(reinterpret_cast<StreamContext*&>(mContext));
            }

            mContext = static_cast<BinaryStreamContext*>(context);
        }
    }

    size_t BinaryStream::GetObjectCount() const
    {
        return mContext ? mContext->objects.Size() : 0;
    }
    const String& BinaryStream::GetObjectName(const size_t index) const
    {
        return mContext ? mContext->objects[index].name : EMPTY_STRING;
    }
    const String& BinaryStream::GetObjectSuper(const size_t index) const
    {
        return mContext ? mContext->objects[index].super : EMPTY_STRING;
    }

    size_t BinaryStream::GetCursor() const
    {
        return mContext ? mContext->cursor : INVALID;
    }

    void BinaryStream::WriteBytes(const void* buffer, size_t numBytes)
    {
        Assert(mContext);
        if (mContext->buffer->GetCapacity() < (mContext->buffer->GetSize() + numBytes))
        {
            mContext->buffer->Reallocate(Max(mContext->buffer->GetSize() + numBytes, mContext->buffer->GetCapacity() * 2), LF_SIMD_ALIGN);
        }
        UInt8* bufferData = reinterpret_cast<UInt8*>(mContext->buffer->GetData());
        memcpy(bufferData + mContext->cursor, buffer, numBytes);
        mContext->cursor += numBytes;
        mContext->buffer->SetSize(mContext->cursor);
    }
    void* BinaryStream::ReadBytes(size_t numBytes)
    {
        Assert(mContext);
        SizeT newCursor = mContext->cursor + numBytes;
        if (newCursor < mContext->cursor || newCursor > mContext->buffer->GetCapacity())
        {
            Crash("Reading off end of buffer.", LF_ERROR_INTERNAL, ERROR_API_CORE);
        }

        UInt8* bufferData = reinterpret_cast<UInt8*>(mContext->buffer->GetData()) + mContext->cursor;
        mContext->cursor += numBytes;
        return bufferData;
    }

    void* BinaryStream::ReverseRead(size_t numBytes)
    {
        Assert(mContext);
        SizeT newCursor = mContext->cursor - numBytes;
        if (newCursor > mContext->cursor)
        {
            Crash("Reading off end of buffer.", LF_ERROR_INTERNAL, ERROR_API_CORE);
        }

        mContext->cursor -= numBytes;
        UInt8* bufferData = reinterpret_cast<UInt8*>(mContext->buffer->GetData()) + mContext->cursor;
        return bufferData;
    }
}