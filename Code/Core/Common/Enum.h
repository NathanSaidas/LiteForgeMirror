#ifndef LF_CORE_ENUM_H
#define LF_CORE_ENUM_H


#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/IO/Stream.h"
#include "Core/Utility/Array.h"

namespace lf {

// Declare/Define enums with these macros. 
// Note: Enums can be declared in headers
// 
// DECLARE_ENUM(enumName, ...)
// DECLARE_STRICT_ENUM(enumName, ...)
//
// Non-Strict enums can be used directly with 
// USING_ENUM(enumName)

#define ENUM_SIZE(enumName) static_cast<SizeT>(enumName::MAX_VALUE)


// **********************************
// Data about enums are stored in this data structure.
// **********************************
struct LF_CORE_API EnumData
{
    EnumData() :
        numberOfStrings(0),
        actualSize(0),
        invalidIndex(0),
        enumValues(nullptr),
        prettyStrings(nullptr),
        rawStrings(nullptr),
        enumName(nullptr),
        buffer(nullptr),
        bufferLength(0)
    {

    }

    EnumData(LazyTag)
    {

    }

    SizeT numberOfStrings;
    SizeT actualSize;
    SizeT invalidIndex;
    Int32* enumValues;
    char** prettyStrings;
    char** rawStrings;
    char* enumName;

    void* buffer;
    SizeT bufferLength;

    bool Initialized(const char* name, const char* details);
    bool TestInitialized()
    {
        return (enumValues != nullptr && prettyStrings != nullptr && rawStrings != nullptr);
    }
    void Release();
};

// **********************************
// Enum template class, holds data about the enum. 
// Provides a value > index, index > value table
// And provides name/pretty name > index and vice versa
// Use the DECLARE_ENUM macros to actually create a TEnum as it will initialize the enum data.
//
// Pretty String: This is a string that is stripped of the CamelCase prefix. (Max 4 prefix) 
// eg. BigDataBank == BDB_VALUE would be stripped to VALUE
// 
// Type Strict constructor, although you can use DECLARE_STRICT_ENUM to make an enum C++ strict
// You can use the TEnum equiv as well which is always type strict. 
// Error "Invalid enum assignment." is generated when non-enum / value type is assigned.
// **********************************
template<typename T, typename V = Int32>
class TEnum
{
public:
    typedef T t_EnumType;
    typedef V t_ValueType;

    LF_INLINE TEnum() : mValue(static_cast<t_EnumType>(0))
    {
        if (GetInternalData().Initialized(GetInternalName(), GetInternalDetails()))
        {
            mValue = static_cast<t_EnumType>(GetInternalData().enumValues[0]);
        }
    }
    LF_INLINE TEnum(t_EnumType value) : mValue(value) {}
    LF_INLINE explicit TEnum(t_ValueType value) : mValue(static_cast<t_EnumType>(value))
    {
    }


    LF_INLINE bool operator==(const TEnum& other) const { return mValue == other.mValue; }
    LF_INLINE bool operator!=(const TEnum& other) const { return mValue != other.mValue; }
    LF_INLINE bool operator==(t_EnumType other) const { return mValue == other; }
    LF_INLINE bool operator!=(t_EnumType other) const { return mValue != other; }

    LF_INLINE TEnum& operator=(t_EnumType other)
    {
        mValue = other;
        return *this;
    }
    // **********************************
    // Get string that matches the current value of the enum
    // **********************************
    LF_INLINE const char* GetString() const
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        SizeT index = GetIndex();
        Assert(Valid(index));
        return GetInternalData().rawStrings[index];
    }
    // **********************************
    // Get the pretty string that matches the current value of the enum.
    // **********************************
    LF_INLINE const char* GetPrettyString() const
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        SizeT index = GetIndex();
        Assert(Valid(index));
        return GetInternalData().prettyStrings[index];
    }
    // **********************************
    // Get the index into enum table associated with the current value.
    // **********************************
    LF_INLINE SizeT GetIndex() const
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        Int32 ival = static_cast<Int32>(mValue);
        for (SizeT i = 0; i < GetInternalData().numberOfStrings; ++i)
        {
            if (ival == GetInternalData().enumValues[i])
            {
                return i;
            }
        }
        return GetInternalData().invalidIndex;
    }
    // **********************************
    // Retrieve a string based on an index.
    // **********************************
    LF_INLINE static const char* GetString(SizeT index)
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        if (index >= GetInternalData().numberOfStrings)
        {
            return "INVALID_ENUM";
        }
        return GetInternalData().rawStrings[index];
    }
    LF_INLINE static const char* GetString(t_EnumType enumIndex)
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        Int32 ival = static_cast<Int32>(enumIndex);
        for (SizeT i = 0; i < GetInternalData().numberOfStrings; ++i)
        {
            if (ival == GetInternalData().enumValues[i])
            {
                return GetString(static_cast<SizeT>(i));
            }
        }
        return "INVALID_ENUM";
    }
    // **********************************
    // Retrieve a 'pretty string' based on index.
    // **********************************
    LF_INLINE static const char* GetPrettyString(SizeT index)
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        if (index >= GetInternalData().numberOfStrings)
        {
            return "INVALID_ENUM";
        }
        return GetInternalData().prettyStrings[index];
    }
    // **********************************
    // Retrieve the enum value based on index.
    // **********************************
    LF_INLINE static t_ValueType GetEnumValue(SizeT index)
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        if (index >= GetInternalData().numberOfStrings)
        {
            return  static_cast<t_ValueType>(GetInternalData().invalidIndex);
        }
        return static_cast<t_ValueType>(GetInternalData().enumValues[index]);
    }
    // **********************************
    // Retrieve the number of values (except MAX_VALUE and INVALID_ENUM)
    // **********************************
    LF_INLINE static SizeT GetNumberOfValues()
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        return GetInternalData().actualSize;
    }
    // **********************************
    // Retrieve the total number of strings (include MAX_VALUE and INVALID_ENUM)
    // **********************************
    LF_INLINE static SizeT GetNumberOfStrings()
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        return GetInternalData().numberOfStrings;
    }
    // **********************************
    // Retrieve all the strings.
    // **********************************
    LF_INLINE static const char* const* GetStrings()
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        return GetInternalData().rawStrings;
    }
    // **********************************
    // Retrieve all the pretty strings.
    // **********************************
    LF_INLINE static const char* const* GetPrettyStrings()
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        return const_cast<char**>(GetInternalData().prettyStrings);
    }
    // **********************************
    // Find a value associated with the name. Compares again pretty name && name
    // **********************************
    LF_INLINE static const t_ValueType FindValue(const char* name)
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        for (SizeT i = 0; i < GetNumberOfStrings(); ++i)
        {
            if (strcmp(name, GetString(i)) == 0 || strcmp(name, GetPrettyString(i)) == 0)
            {
                return GetEnumValue(i);
            }
        }
        return static_cast<t_ValueType>(GetInternalData().invalidIndex);
    }
    // **********************************
    // Retrieve all the values.
    // **********************************
    LF_INLINE static const Int32* GetValues()
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        return GetInternalData().enumValues;
    }
    // **********************************
    // Retrieve the name of the enum.
    // **********************************
    LF_INLINE static const char* GetName()
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        return GetInternalData().enumName;
    }
    // **********************************
    // Serialize value (string)
    // **********************************
    friend ::lf::Stream& operator<<(::lf::Stream& s, TEnum& value)
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        if (s.IsReading())
        {
            String enumName;
            s.Serialize(enumName);
            if (!enumName.Empty())
            {
                value.mValue = static_cast<t_EnumType>(FindValue(enumName.CStr()));
            }
        }
        else
        {
            String enumName = value.GetString();
            s.Serialize(enumName);
        }
        return s;
    }
    // **********************************
    // Serialize value (interger)
    // **********************************
    friend ::lf::Stream& operator<<(::lf::Stream& s, t_EnumType& value)
    {
        Assert(GetInternalData().Initialized(GetInternalName(), GetInternalDetails()));
        s.Serialize(reinterpret_cast<t_ValueType&>(value));
        return s;
    }

    // **********************************
    // Allow t_Enumtype = TEnum
    // **********************************
    operator t_EnumType() const
    {
        return mValue;
    }

    template<typename OtherT>
    TEnum(OtherT)
    {
        COMPILE_CRASH("Invalid enum assignment.");
    }

    template<typename OtherT>
    TEnum operator=(OtherT)
    {
        COMPILE_CRASH("Invalid enum assignment.");
        return *this;
    }

    template<typename OtherT>
    bool operator==(OtherT) const
    {
        COMPILE_CRASH("Invalid enum assignment.");
        return false;
    }

    template<typename OtherT>
    bool operator!=(OtherT) const
    {
        COMPILE_CRASH("Invalid enum assignment.");
        return false;
    }

    t_EnumType mValue;

    // Internal Enum Data, DO NOT MODIFY
    static EnumData& GetInternalData()
    {
        static EnumData sInternalData(LAZY);
        return sInternalData;
    }

    static const char* GetInternalName()
    {
        return InternalTypeName<T>();
    }

    static const char* GetInternalDetails()
    {
        return InternalTypeDetails<T>();
    }

};


// This is where all the enums 'registered' are stored.
struct LF_CORE_API EnumRegistry
{
public:
    typedef TArray<EnumData*> EnumDatas;
    EnumRegistry() : mEnumDatas() {}

    void Add(EnumData* data) { mEnumDatas.Add(data); }
    void Clear();

    EnumDatas& GetData() { return mEnumDatas; }
private:
    EnumDatas mEnumDatas;
};
LF_CORE_API EnumRegistry& GetEnumRegistry();

// **********************************
// A function that calculates enum data.
// Eg. Will parse 'args' and calcluate number of values, 
// names/pretty names in string form and values.
// **********************************
LF_CORE_API void BuildEnumData(const char* name, const char* args, EnumData* enumData);

template<typename T>
void RegisterEnumData(const char* name, const char* args)
{
    BuildEnumData(name, args, &T::GetInternalData());
    GetEnumRegistry().Add(&T::GetInternalData());
}

#define DECLARE_ENUM(Name, ...)                                                                         \
namespace Name                                                                                          \
{                                                                                                       \
    enum Value                                                                                          \
    {                                                                                                   \
        __VA_ARGS__,                                                                                    \
        MAX_VALUE,                                                                                      \
        INVALID_ENUM = MAX_VALUE                                                                        \
    };                                                                                                  \
                                                                                                        \
}                                                                                                       \
                                                                                                        \
    template<>                                                                                          \
    class InternalTypeName<Name::Value>                                                                 \
    {                                                                                                   \
    public:																								\
        operator const char*() const                                                                    \
        {                                                                                               \
            return #Name;                                                                               \
        }                                                                                               \
    };                                                                                                  \
                                                                                                        \
    template<>                                                                                          \
    class InternalTypeDetails<Name::Value>                                                              \
    {                                                                                                   \
    public:																								\
        operator const char*() const                                                                    \
        {                                                                                               \
            return #__VA_ARGS__ ## ", MAX_VALUE, INVALID_ENUM=MAX_VALUE";                               \
        }                                                                                               \
    };                                                                                                  \
    template<>                                                                                          \
    class InternalTypeName<TEnum<Name::Value>>                                                          \
    {                                                                                                   \
    public:																								\
        operator const char*() const                                                                    \
        {                                                                                               \
            return #Name;                                                                               \
        }                                                                                               \
    };                                                                                                  \
                                                                                                        \
    template<>                                                                                          \
    class InternalTypeDetails<TEnum<Name::Value>>                                                       \
    {                                                                                                   \
    public:																								\
        operator const char*() const                                                                    \
        {                                                                                               \
            return #__VA_ARGS__ ## ", MAX_VALUE, INVALID_ENUM=MAX_VALUE";                               \
        }                                                                                               \
    };                                                                                                  \
typedef TEnum<Name::Value> T##Name;                                                                     \
template class TEnum<Name::Value>;                                                                      \


#define DECLARE_STRICT_ENUM(Name, ...)                                                                  \
enum class Name                                                                                         \
{                                                                                                       \
    __VA_ARGS__,                                                                                        \
    MAX_VALUE,                                                                                          \
    INVALID_ENUM = MAX_VALUE                                                                            \
};                                                                                                      \
                                                                                                        \
    template<>                                                                                          \
    class InternalTypeName<Name>                                                                        \
    {                                                                                                   \
    public:																								\
        operator const char*() const                                                                    \
        {                                                                                               \
            return #Name;                                                                               \
        }                                                                                               \
    };                                                                                                  \
                                                                                                        \
    template<>                                                                                          \
    class InternalTypeDetails<Name>                                                                     \
    {                                                                                                   \
    public:																								\
        operator const char*() const                                                                    \
        {                                                                                               \
            return #__VA_ARGS__ ## ", MAX_VALUE, INVALID_ENUM=MAX_VALUE";                               \
        }                                                                                               \
    };                                                                                                  \
                                                                                                        \
    template<>                                                                                          \
    class InternalTypeName<TEnum<Name>>                                                                 \
    {                                                                                                   \
    public:																								\
        operator const char*() const                                                                    \
        {                                                                                               \
            return #Name;                                                                               \
        }                                                                                               \
    };                                                                                                  \
                                                                                                        \
    template<>                                                                                          \
    class InternalTypeDetails<TEnum<Name>>                                                              \
    {                                                                                                   \
    public:																								\
        operator const char*() const                                                                    \
        {                                                                                               \
            return #__VA_ARGS__ ## ", MAX_VALUE, INVALID_ENUM=MAX_VALUE";                               \
        }                                                                                               \
    };                                                                                                  \
typedef TEnum<Name> T##Name;                                                                            \
template class TEnum<Name>;                                                                             \



#define USING_ENUM(Name) using namespace Name;
    //SH_CORE_API StaticCall sInternalRegister##Name(RegisterEnumData<T##Name>, #Name, #__VA_ARGS__ ## ",\nMAX_VALUE,\nINVALID_ENUM=MAX_VALUE")  \

}

#endif // SHIELD_HEART_CORE_ENUM_H