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

#ifndef LF_CORE_STRING_H
#define LF_CORE_STRING_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/String/StringUtil.h"

namespace lf {

union StringStorage
{
    using value_type = Char8;
    struct local_type
    {
        value_type buffer[LF_STRING_DEFAULT_STORAGE];
    } local;
    struct heap_type
    {
        value_type* first;
        value_type* last;
        value_type* end;
        value_type  padding[LF_STRING_DEFAULT_STORAGE * sizeof(value_type) - sizeof(value_type*) * 3];
    } heap;
};

class LF_CORE_API String
{
public:
    using value_type = Char8;

    String();
    String(const String& other);
    String(String&& other);
    String(const value_type* string);
    String(const SizeT length, const value_type* string);
    String(value_type character);
    String(const value_type* string, CopyOnWriteTag);
    String(const SizeT length, const value_type* string, CopyOnWriteTag);
    ~String();

    LF_FORCE_INLINE SizeT               Size() const;
    LF_FORCE_INLINE SizeT               Capacity() const;
    LF_FORCE_INLINE bool                Empty() const;
    LF_FORCE_INLINE const value_type*   CStr() const;
     
    LF_FORCE_INLINE value_type First() const;
    LF_FORCE_INLINE value_type Last() const;
 
    void Clear();
    void Swap(String& other);
    void Resize(SizeT size, value_type fill = ' ');
    void Reserve(SizeT size);

    String& Assign(const String& other);
    String& Assign(const value_type* other);
    
    String& Append(value_type character);
    String& Append(const String& other);
    String& Append(const value_type* other);

    void Insert(value_type c, SizeT position);
    void Insert(const String& string, SizeT position);

    void SubString(SizeT start, String& outString) const;
    void SubString(SizeT start, SizeT length, String& outString) const;
    String SubString(SizeT start) const;
    String SubString(SizeT start, SizeT length) const;

    SizeT Replace(value_type find, value_type replace);
    SizeT Replace(const String& find, const String& replace);
    
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

    String& operator=(String&& other)
    {
        Clear();
        if (other.UseHeap())
        {
            mStorage.heap.first = other.mStorage.heap.first;
            mStorage.heap.last = other.mStorage.heap.last;
            mStorage.heap.end = other.mStorage.heap.end;
            mStorage.local.buffer[LF_STRING_STORAGE_SUB_1] = other.mStorage.local.buffer[LF_STRING_STORAGE_SUB_1];
        }
        else
        {
            for (size_t i = 0; i < LF_ARRAY_SIZE(mStorage.local.buffer); ++i)
            {
                mStorage.local.buffer[i] = other.mStorage.local.buffer[i];
            }
        }
        other.ZeroBuffer();
        return *this;
    }

    String& operator=(const String& other)
    {
        return Assign(other);
    }
    String& operator=(const value_type* other)
    {
        return Assign(other);
    }

    String& operator+=(value_type character)
    {
        return Append(character);
    }
    String& operator+=(const String& other)
    {
        return Append(other);
    }
    String& operator+=(const value_type* other)
    {
        return Append(other);
    }

    LF_INLINE String operator+(const String& other) const
    {
        String temp(*this);
        temp += other;
        return temp;
    }

    LF_INLINE String operator+(const value_type* other) const
    {
        String temp(*this);
        temp += other;
        return temp;
    }

    LF_INLINE String operator+(value_type c) const
    {
        String temp(*this);
        temp += c;
        return temp;
    }

    LF_INLINE friend String operator+(const value_type* other, const String& rhs)
    {
        String temp(other);
        temp.Append(rhs);
        return temp;
    }

    bool operator<(const String& other) const
    {
        return StrAlphaLess(CStr(), other.CStr());
    }

    bool operator>(const String& other) const
    {
        return StrAlphaGreater(CStr(), other.CStr());
    }

    bool operator==(const String& other) const
    {
        return StrEqual(begin(), other.begin(), end(), other.end());
    }

    bool operator!=(const String& other) const
    {
        return StrNotEqual(begin(), other.begin(), end(), other.end());
    }

    bool operator==(const value_type* other) const
    {
        const value_type* otherEnd = other + StrLen(other);
        return StrEqual(begin(), other, end(), otherEnd);
    }
    bool operator!=(const value_type* other) const
    {
        const value_type* otherEnd = other + StrLen(other);
        return StrNotEqual(begin(), other, end(), otherEnd);
    }

    const value_type& operator[](SizeT index) const
    {
        return GetBufferPointer()[index];
    }

    LF_INLINE friend bool operator==(const value_type* lhs, const String& rhs)
    {
        const value_type* cstrEnd = lhs + StrLen(lhs);
        return StrEqual(lhs, rhs.begin(), cstrEnd, rhs.end());
    }
    LF_INLINE friend bool operator!=(const value_type* lhs, const String& rhs)
    {
        const value_type* cstrEnd = lhs + StrLen(lhs);
        return StrNotEqual(lhs, rhs.begin(), cstrEnd, rhs.end());
    }
    
    LF_FORCE_INLINE const char* begin() const
    {
        return GetBufferPointer();
    }
    LF_FORCE_INLINE const char* end() const
    {
        return GetBufferPointer() + Size();
    }

    LF_FORCE_INLINE bool UseHeap() const
    {
        return HasFlag(LF_STRING_STORAGE_FLAG);
    }
    LF_FORCE_INLINE bool CopyOnWrite() const
    {
        return HasFlag(LF_STRING_COPY_ON_WRITE_FLAG);
    }
private:
    LF_FORCE_INLINE bool HasFlag(UInt8 flag) const
    {
        // todo: I think this should be `left == flag`
        return (reinterpret_cast<const UInt8&>(mStorage.local.buffer[LF_STRING_STORAGE_SUB_1]) & flag) > 0;
    }
    LF_FORCE_INLINE void SetFlag(UInt8 flag)
    {
#if defined(LF_DEBUG)
        SetLocalSize(0); // Debugger vis expects this to be a single value.
#endif
        reinterpret_cast<UInt8&>(mStorage.local.buffer[LF_STRING_STORAGE_SUB_1]) |= flag;
    }
    LF_FORCE_INLINE void UnsetFlag(UInt8 flag)
    {
        reinterpret_cast<UInt8&>(mStorage.local.buffer[LF_STRING_STORAGE_SUB_1]) &= ~(flag);
    }
    LF_FORCE_INLINE const SizeT GetHeapSize() const
    {
        return mStorage.heap.last - mStorage.heap.first;
    }
    LF_FORCE_INLINE const SizeT GetLocalSize() const
    {
        return reinterpret_cast<const UInt8&>(mStorage.local.buffer[LF_STRING_STORAGE_SUB_1]) & LF_STRING_SIZE_MASK;
    }
    LF_FORCE_INLINE void SetLocalSize(SizeT size)
    {
        (reinterpret_cast<UInt8&>(mStorage.local.buffer[LF_STRING_STORAGE_SUB_1]) &= LF_STRING_FLAG_MASK) |= size;
    }
    LF_FORCE_INLINE const SizeT GetHeapCapacity() const
    {
        return mStorage.heap.end - mStorage.heap.first;
    }
    LF_FORCE_INLINE const SizeT GetLocalCapacity() const
    {
        return LF_STRING_DEFAULT_STORAGE - 2;
    }
    LF_FORCE_INLINE void ZeroBuffer()
    {
        for (size_t i = 0; i < LF_ARRAY_SIZE(mStorage.local.buffer); ++i)
        {
            mStorage.local.buffer[i] = 0;
        }
    }
    LF_FORCE_INLINE value_type* GetBufferPointer()
    {
        return HasFlag(LF_STRING_STORAGE_FLAG | LF_STRING_COPY_ON_WRITE_FLAG) ? mStorage.heap.first : mStorage.local.buffer;
    }
    LF_FORCE_INLINE const value_type* GetBufferPointer() const
    {
        return HasFlag(LF_STRING_STORAGE_FLAG | LF_STRING_COPY_ON_WRITE_FLAG) ? mStorage.heap.first : mStorage.local.buffer;
    }
    void Grow(SizeT desiredCapacity);
    void MakeLocal();

    StringStorage mStorage;
};

LF_CORE_API extern String EMPTY_STRING;

// StringInline:

SizeT String::Size() const
{
    return HasFlag(LF_STRING_STORAGE_FLAG | LF_STRING_COPY_ON_WRITE_FLAG) ? GetHeapSize() : GetLocalSize();
}
SizeT String::Capacity() const
{
    return HasFlag(LF_STRING_STORAGE_FLAG | LF_STRING_COPY_ON_WRITE_FLAG) ? GetHeapCapacity() : GetLocalCapacity();
}
bool String::Empty() const
{
    return Size() == 0;
}
const String::value_type* String::CStr() const
{
    return GetBufferPointer();
}
String::value_type String::First() const { return GetBufferPointer()[0]; }
String::value_type String::Last() const { return GetBufferPointer()[Size() - 1]; }

} // namespace lf

#endif // LF_CORE_STIRNG_H