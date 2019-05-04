// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_TOKEN_H
#define LF_CORE_TOKEN_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/String/StringUtil.h"
#include "Core/Utility/StaticCallback.h"

namespace lf {

#define STATIC_TOKEN(ArgName, Text) \
    static const ::lf::Token ArgName; \
    STATIC_INIT(LF_CONCAT(ArgName, _StaticConstructor), ::lf::SCP_PRE_INIT_CORE + 100) { const_cast<::lf::Token&>(ArgName) = ::lf::Token(Text); } \
    STATIC_DESTROY(LF_CONCAT(ArgName, _StaticDestructor), ::lf::SCP_PRE_INIT_CORE + 100) { const_cast<::lf::Token&>(ArgName).Clear(); }\

class String;

class LF_CORE_API Token
{
    friend class TokenTable;
public:
    using value_type = Char8;

    LF_FORCE_INLINE Token();
    LF_FORCE_INLINE Token(const Token& other);
    LF_FORCE_INLINE Token(Token&& other);
    LF_FORCE_INLINE Token(const value_type* string);
    LF_FORCE_INLINE Token(const value_type* string, AcquireTag);
    LF_FORCE_INLINE Token(const value_type* string, CopyOnWriteTag);
    LF_FORCE_INLINE Token(const String& string);
    LF_FORCE_INLINE Token(const String& string, AcquireTag);
    LF_FORCE_INLINE ~Token();

    LF_INLINE Token& operator=(const Token& other)
    {
        mString = other.mString;
        mKey = other.mKey;
        mSize = other.mSize;
        IncrementRef();
        return *this;
    }
    LF_INLINE Token& operator=(Token&& other)
    {
        mString = other.mString;
        mKey = other.mKey;
        mSize = other.mSize;
        other.mString = gNullString;
        other.mKey = INVALID16;
        other.mSize = 0;
        return *this;
    }
    LF_INLINE bool operator==(const Token& other) const
    {
        return mString == other.mString;
    }
    LF_INLINE bool operator!=(const Token& other) const
    {
        return mString != other.mString;
    }
    LF_INLINE bool operator<(const Token& other) const
    {
        return mString < other.mString;
    }
    LF_INLINE bool operator>(const Token& other) const
    {
        return mString > other.mString;
    }
    
    void Clear();
    const char* CStr() const;
    SizeT Size() const;
    bool Empty() const;

    bool AlphaLess(const String& string) const;
    bool AlphaLess(const value_type* string) const;
    bool AlphaGreater(const String& string) const;
    bool AlphaGreater(const value_type* string) const;
    bool Compare(const String& string) const;
    bool Compare(const value_type* string);

    SizeT Find(value_type character) const;
    SizeT Find(const String& string) const;
    SizeT Find(const value_type* string) const;
    SizeT FindAgnostic(value_type character) const;
    SizeT FindAgnostic(const String& string) const;
    SizeT FindAgnostic(const value_type* string) const;
    SizeT FindLast(value_type character) const;
    SizeT FindLast(const String& string) const;
    SizeT FindLast(const value_type* string) const;
    SizeT FindLastAgnostic(value_type character) const;
    SizeT FindLastAgnostic(const String& string) const;
    SizeT FindLastAgnostic(const value_type* string) const;

private:
    void DecrementRef();
    void IncrementRef();
    void LookUp(const value_type* string, AcquireTag);
    void LookUp(const value_type* string, CopyOnWriteTag);
    void LookUp(const value_type* string);
    void LookUp(const String& string);
    void LookUp(const String& string, AcquireTag);

    const value_type* mString;
    UInt16 mKey;
    UInt16 mSize;
};

Token::Token() :
mString(gNullString),
mKey(INVALID16),
mSize(0)
{

}
Token::Token(const Token& other) :
mString(other.mString),
mKey(other.mKey),
mSize(other.mSize)
{
    IncrementRef();
}
Token::Token(Token&& other) :
mString(other.mString),
mKey(other.mKey),
mSize(other.mSize)
{
    other.mString = gNullString;
    other.mKey = INVALID16;
    other.mSize = 0;
}
Token::Token(const value_type* string) :
mString(gNullString),
mKey(INVALID16),
mSize(0)
{
    LookUp(string);
}
Token::Token(const value_type* string, AcquireTag) :
mString(gNullString),
mKey(INVALID16),
mSize(0)
{
    LookUp(string, ACQUIRE);
}
Token::Token(const value_type* string, CopyOnWriteTag) :
mString(gNullString),
mKey(INVALID16),
mSize(0)
{
    LookUp(string, COPY_ON_WRITE);
}
Token::Token(const String& string) : 
mString(gNullString),
mKey(INVALID16),
mSize(0)
{
    LookUp(string);
}
Token::Token(const String& string, AcquireTag)
{
    LookUp(string, ACQUIRE);
}
Token::~Token()
{
    Clear();
}

} // namespace lf

#endif // LF_CORE_TOKEN_H