#ifndef LF_CORE_STREAM_H
#define LF_CORE_STREAM_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/String/String.h"
#include "Core/String/StringCommon.h"
#include "Core/Utility/Bitfield.h"


// #include "core/system/String.h"
// #include "core/system/Token.h"
// #include "core/math/Vector.h"
// #include "core/math/Quaternion.h"
// #include "core/math/Vector4.h"
// #include "core/math/Vector3.h"
// #include "core/math/Vector2.h"
// #include "core/math/Color.h"
// #include "core/util/Bitfield.h"
// #include "core/util/Array.h"
// #include "core/util/MemoryBuffer.h"

// todo: Property Stream:


namespace lf {

class Type;
class Vector2;
class Vector3;
class Vector4;
class Vector;
class Quaternion;
class Color;
class MemoryBuffer;
class Token;

// @param s - The stream object doing the serializing.
// @param prop - The property getting serialized
// @param flags - Extra data using the following format. "Name=u8 Desc=\"Test Value\""
#define SERIALIZE(StreamArg, PropArg, FlagsArg)                                     \
    {                                                                                   \
        static const ::lf::StreamPropertyInfo PROP_INFO(#PropArg,FlagsArg);   \
        StreamArg << PROP_INFO << PropArg;                                              \
    }                                                                                                                                  


#define SERIALIZE_ARRAY(StreamArg, PropArg, FlagsArg)                               \
    {                                                                                   \
        static const ::lf::StreamPropertyInfo PROP_INFO(#PropArg, FlagsArg);  \
        TSerializeArray(StreamArg, PropArg, PROP_INFO);                                 \
    }                                                                                   \


#define SERIALIZE_STRUCT(StreamArg, PropArg, FlagsArg)                              \
    {                                                                                   \
        static const ::lf::StreamPropertyInfo PROP_INFO(#PropArg, FlagsArg);  \
        (StreamArg << PROP_INFO).BeginStruct();                                         \
        (StreamArg << PropArg).EndStruct();                                             \
    }                                                                                   \


#define SERIALIZE_STRUCT_ARRAY(StreamArg, PropArg, FlagsArg)                        \
    {                                                                                   \
        static const ::lf::StreamPropertyInfo PROP_INFO(#PropArg, FlagsArg);  \
        TSerializeStructArray(StreamArg, PropArg, PROP_INFO);                           \
    }                                                                                   \

class StreamContext;

// Extra information about the property
// Todo: If we want to support flags they will have to be actual flags.
struct LF_CORE_API StreamPropertyInfo
{
public:
    StreamPropertyInfo() : name() {}
    StreamPropertyInfo(const String& inName);
    StreamPropertyInfo(const char* inName, const String& inFlags);

    String name; // Note: Stored as COPY_ON_WRITE for static properties.
};

// StreamBufferObjects are limited to representing a single object.
struct LF_CORE_API StreamBufferObject
{
    // Write Ctor
    StreamBufferObject(const String& inName, const String& inSuper) :
        data(nullptr),
        length(0),
        name(inName),
        super(inSuper)
    {}
    // Read Ctor
    StreamBufferObject(const String& inName, const String& inSuper, void* inData, size_t inLength) :
        data(inData),
        length(inLength),
        name(inName),
        super(inSuper)
    {}

    ~StreamBufferObject()
    {
        Clear();
    }

    void Clear();

    void* data;
    size_t length;
    String name;
    String super;

private:
    // Not Allowed: Must have name
    StreamBufferObject() {}
};

class LF_CORE_API Stream
{
public:
    using AssetLoadFlags = UInt32;
    // Tags:
    enum StreamText { TEXT };
    enum StreamMemory { MEMORY };
    enum StreamFile { FILE };

    enum StreamMode
    {
        SM_CLOSED,
        SM_WRITE,
        SM_READ
    };

    Stream();
    virtual ~Stream();

    // **********************************
    // Open with text. (TextStream Only)
    // When mode is SM_READ, the text will be parsed.
    // When mode is SM_WRITE, the text will be kept and written to upon Close
    // **********************************
    virtual void Open(Stream::StreamText, String* text, StreamMode mode);
    // **********************************
    // Open with memory. (BinaryStream Only)
    // When mode is SM_READ, buffer object will be added to the stream.
    // When mode is SM_WRITE, the first object will be written to 'buffer' upon Close
    // **********************************
    virtual void Open(Stream::StreamMemory, MemoryBuffer* buffer, StreamMode mode);
    // **********************************
    // Open with file. (BinaryStream and TextStream)
    // When mode is SM_READ, contents from the file will be parsed/added to the stream.
    // When mode is SM_WRITE, objects contained within the stream will be written to the file.
    // **********************************
    virtual void Open(Stream::StreamFile, const String& filename, StreamMode mode);
    // **********************************
    // Closes the stream, submitting all content to the output buffer (text, memory or file)
    // **********************************
    virtual void Close();
    // **********************************
    // Removes all contents from the stream, does not output to any buffer (text, memory, or file)
    // **********************************
    virtual void Clear();

    // **********************************
    // Default serializeable values.
    // **********************************
    virtual void Serialize(bool& value);
    virtual void Serialize(UInt8& value);
    virtual void Serialize(UInt16& value);
    virtual void Serialize(UInt32& value);
    virtual void Serialize(UInt64& value);
    virtual void Serialize(Int8& value);
    virtual void Serialize(Int16& value);
    virtual void Serialize(Int32& value);
    virtual void Serialize(Int64& value);
    virtual void Serialize(Float32& value);
    virtual void Serialize(Float64& value);
    virtual void Serialize(Vector2& value);
    virtual void Serialize(Vector3& value);
    virtual void Serialize(Vector4& value);
    virtual void Serialize(Vector& value);
    virtual void Serialize(Quaternion& value);
    virtual void Serialize(Color& value);
    virtual void Serialize(String& value);
    virtual void Serialize(Token& value);
    virtual void Serialize(const Type*& value);
    virtual void SerializeGuid(ByteT* value, SizeT size);
    virtual void SerializeAsset(Token& value, bool isWeak);
    virtual void Serialize(const StreamPropertyInfo& info);

    // **********************************
    // Called to start writing an object. 
    // There must currently not be any object being written to
    // **********************************
    virtual bool BeginObject(const String& name, const String& super);
    // **********************************
    // Called to flush contents of the object into the stream. (Not to the output)
    // **********************************
    virtual void EndObject();

    // **********************************
    // Called in serialize macros to signal "begin struct" format
    // **********************************
    virtual bool BeginStruct();
    // **********************************
    // Called in serialize macros to signal "end struct" format
    // **********************************
    virtual void EndStruct();

    // **********************************
    // Called in serialize macros to signal "begin array" format
    // **********************************
    virtual bool BeginArray();
    // **********************************
    // Called in serialize macros to signal "end array" format
    // **********************************
    virtual void EndArray();

    // **********************************
    // Pops the array size associated with the current state of the stream.
    // **********************************
    virtual size_t GetArraySize() const;
    // **********************************
    // Pushes the array size associated with the current state of the stream.
    // **********************************
    virtual void SetArraySize(size_t size);

    bool IsReading() const;
    StreamMode GetMode() const;
    AssetLoadFlags GetAssetLoadFlags() const;

    

    // **********************************
    // Iterations helpers
    // **********************************
    virtual size_t GetObjectCount() const;
    virtual const String& GetObjectName(const size_t index) const;
    virtual const String& GetObjectSuper(const size_t index) const;

protected:
    // **********************************
    // Low level context access, use when you want to move/store contexts and avoid copy.
    // **********************************
    virtual StreamContext* PopContext();
    virtual const StreamContext* GetContext() const;
    virtual void SetContext(StreamContext* context);

    StreamContext* AllocContext(size_t size, size_t alignment);
    void FreeContext(StreamContext*& context);
};

// **********************************
// Base class for context. Implement to add per-stream custom data.
// **********************************
class LF_CORE_API StreamContext
{
public:
    enum ContextType
    {
        NONE,
        TEXT,
        BINARY,
        DATA,
        SHARED_BINARY,
        DEPENDENCY
    };

    StreamContext() : mType(NONE), mMode(Stream::SM_READ), mFlags(0), mLogWarnings(false) {}
    virtual ~StreamContext() {}

    ContextType mType;
    Stream::StreamMode mMode;
    Stream::AssetLoadFlags mFlags; // As the stream serializes assets it can optionally load data or just properties based on these flags.
    bool mLogWarnings;
};

template<typename TProp>
void TSerializeArray(Stream& s, TProp& prop, const StreamPropertyInfo& propInfo)
{
    (s << propInfo).BeginArray();
    size_t size = 0;
    if (s.IsReading())
    {
        size = s.GetArraySize();
        prop.Resize(size);
    }
    else
    {
        size = prop.Size();
        s.SetArraySize(size);
    }
    for (size_t i = 0; i < size; ++i)
    {
        StreamPropertyInfo arrayInfo(ToString(i));
        s << arrayInfo << prop[i];
    }
    s.EndArray();
}

template<typename TProp>
void TSerializeStructArray(Stream& s, TProp& prop, const StreamPropertyInfo& propInfo)
{
    if (!(s << propInfo).BeginArray())
    {
        return;
    }
    size_t size = 0;
    if (s.IsReading())
    {
        size = s.GetArraySize();
        prop.Resize(size);
    }
    else
    {
        size = prop.Size();
        s.SetArraySize(size);
    }
    for (size_t i = 0; i < size; ++i)
    {
        StreamPropertyInfo arrayInfo(ToString(i));
        (s << arrayInfo).BeginStruct();
        (s << prop[i]).EndStruct();
    }
    s.EndArray();
}

LF_INLINE Stream& operator<<(Stream& s, bool& v)
{
    s.Serialize(v);
    return s;
}

LF_INLINE Stream& operator<<(Stream& s, UInt8& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, UInt16& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, UInt32& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, UInt64& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, Int8& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, Int16& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, Int32& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, Int64& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, Float32& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, Float64& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, String& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, Token& v)
{
    s.Serialize(v);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, const Type*& type)
{
    s.Serialize(type);
    return s;
}
LF_INLINE Stream& operator<<(Stream& s, const StreamPropertyInfo& v)
{
    s.Serialize(v);
    return s;
}

LF_INLINE Stream& operator<<(Stream& s, Vector2& v)
{
    s.Serialize(v);
    return s;
}

LF_INLINE Stream& operator<<(Stream& s, Vector3& v)
{
    s.Serialize(v);
    return s;
}

LF_INLINE Stream& operator<<(Stream& s, Vector4& v)
{
    s.Serialize(v);
    return s;
}

LF_INLINE Stream& operator<<(Stream& s, Color& c)
{
    s.Serialize(c);
    return s;
}

LF_INLINE Stream& operator<<(Stream& s, Vector& v)
{
    s.Serialize(v);
    return s;
}

LF_INLINE Stream& operator<<(Stream& s, Quaternion& q)
{
    s.Serialize(q);
    return s;
}

template<typename T, typename V>
LF_INLINE Stream& operator<<(Stream& s, Bitfield<T, V>& b)
{
    s << b.value;
    return s;
}

}

#endif // LF_CORE_STREAM_H
