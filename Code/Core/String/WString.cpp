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
#include "WString.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Utility/Utility.h"
#include "Core/Utility/ErrorCore.h"
#include <string.h> // memcpy/memcmp
#include <wchar.h> // wmemcpy

namespace lf {

WString::WString() : 
mStorage()
{
    ZeroBuffer();
}
WString::WString(const WString& other) : 
mStorage()
{
    ZeroBuffer();
    Assign(other);
}
WString::WString(WString&& other) :
mStorage()
{
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
}
WString::WString(const value_type* string) :
mStorage()
{
    ZeroBuffer();
    Assign(string);
}
WString::WString(const SizeT length, const value_type* string) :
mStorage()
{
    ZeroBuffer();
    if (!string || *string == '\0')
    {
        return;
    }
    if (length > LF_STRING_STORAGE_SUB_1)
    {
        Grow(length);
        memcpy(mStorage.heap.first, string, length * sizeof(value_type));
        mStorage.heap.last = mStorage.heap.first + length;
        mStorage.heap.first[length] = '\0';
    }
    else
    {
        memcpy(mStorage.local.buffer, string, length * sizeof(value_type));
        mStorage.local.buffer[length] = '\0';
        SetLocalSize(length);
    }
}
WString::WString(value_type character) :
mStorage()
{
    ZeroBuffer();
    Append(character);
}
WString::WString(const value_type* string, CopyOnWriteTag) :
mStorage()
{
    ZeroBuffer();
    const SizeT otherSize = StrLen(string);
    mStorage.heap.first = const_cast<value_type*>(string);
    mStorage.heap.last = mStorage.heap.first + otherSize;
    mStorage.heap.end = mStorage.heap.last;
    SetFlag(LF_STRING_COPY_ON_WRITE_FLAG);
}
WString::~WString()
{
    Clear();
}

void WString::Clear()
{
    if (!CopyOnWrite() && UseHeap())
    {
        LFFree(mStorage.heap.first);
    }
    ZeroBuffer(); // TODO: This could be optimized out... But we'd still need to Unset(STORAGE_FLAG)
}
void WString::Swap(WString& other)
{
    if (&other == this)
    {
        return;
    }
    WStringStorage tmp;
    memcpy(tmp.local.buffer, mStorage.local.buffer, sizeof(tmp.local.buffer));
    memcpy(mStorage.local.buffer, other.mStorage.local.buffer, sizeof(tmp.local.buffer));
    memcpy(other.mStorage.local.buffer, tmp.local.buffer, sizeof(tmp.local.buffer));
}
void WString::Resize(SizeT size, value_type fill)
{
    SizeT currentSize = Size();
    if (currentSize == size)
    {
        return;
    }

    // Shrink
    if (size < currentSize)
    {
        if (CopyOnWrite())
        {
            const value_type* begin = mStorage.heap.first;
            if ((size + 1) >= LF_STRING_STORAGE_SUB_1)
            {
                Grow(size);
            }
            else
            {
                memcpy(mStorage.local.buffer, begin, size * sizeof(value_type));
                mStorage.local.buffer[size] = '\0';
                SetLocalSize(size);
                UnsetFlag(LF_STRING_COPY_ON_WRITE_FLAG);
            }
        }
        else
        {
            if (UseHeap())
            {
                mStorage.heap.last = mStorage.heap.first + size;
                *mStorage.heap.last = '\0';
            }
            else
            {
                SetLocalSize(size);
                mStorage.local.buffer[size] = '\0';
            }
        }
    }
    // Grow:
    else
    {
        if ((size + 1) > Capacity() || CopyOnWrite())
        {
            Grow(size);
        }
        value_type* buffer = GetBufferPointer();
        wmemset(buffer, fill, (size - currentSize));
        buffer[size] = '\0';
        if (UseHeap())
        {
            mStorage.heap.last = mStorage.heap.first + size;
        }
        else
        {
            SetLocalSize(size);
        }
    }
}
void WString::Reserve(SizeT size)
{
    const bool copyOnWrite = CopyOnWrite();
    if (copyOnWrite || (Capacity() < size + 1))
    {
        SizeT previousSize = Size();
        Grow(size + 1);
        SizeT newSize = copyOnWrite ? size : previousSize;
        if (UseHeap())
        {
            mStorage.heap.last = mStorage.heap.first + newSize;
            *mStorage.heap.last = '\0';
        }
        else
        {
            SetLocalSize(newSize);
            mStorage.local.buffer[newSize] = '\0';
        }

        return;
    }
}

WString& WString::Assign(const WString& other)
{
    if (this == &other)
    {
        return *this;
    }

    if (Empty() && other.Empty())
    {
        return *this;
    }

    // Cases: (cow = COPY ON WRITE )
    // this == local | other == local  | Simply copy LOCAL_STORAGE
    // this == heap  | other == heap   | Simply grow to fit and copy 'buffers'
    // this == cow   | other == cow    | Just assign the heap pointers and keep the flags
    // this == cow   | other == local  | Copy LOCAL_STORAGE and unmark the COW flag
    // this == cow   | other == heap   | Allocate a buffer and copy other 'buffer' and unmark the COW flag
    // this == heap  | other == local  | Copy other LOCAL_STORAGE to this 'buffer'
    // this == local | other == heap   | Grow to fit and copy 'buffers'
    // this == heap  | other == cow    | Free memory and assume COW state?

    const bool useHeap = UseHeap();
    const bool copyOnWrite = CopyOnWrite();
    const bool otherUseHeap = other.UseHeap();
    const bool otherCopyOnWrite = other.CopyOnWrite();

    const bool isLocal = !copyOnWrite && !useHeap;
    const bool isHeap = !copyOnWrite && useHeap;
    const bool otherIsLocal = !otherCopyOnWrite && !otherUseHeap;
    const bool otherIsHeap = !otherCopyOnWrite && otherUseHeap;

    if ((isLocal && otherIsLocal) || (copyOnWrite && otherIsLocal))
    {
        memcpy(mStorage.local.buffer, other.mStorage.local.buffer, sizeof(mStorage.local.buffer));
        SetLocalSize(other.GetLocalSize());
    }
    else if (isHeap && otherIsHeap)
    {
        const SizeT otherSize = other.GetHeapSize();
        if (otherSize > GetHeapCapacity())
        {
            Grow(otherSize); // todo: Grow no copy:
        }
        memcpy(mStorage.heap.first, other.mStorage.heap.first, (otherSize + 1) * sizeof(value_type)); // +1 for null terminator
        mStorage.heap.last = mStorage.heap.first + otherSize;
    }
    else if (copyOnWrite && otherCopyOnWrite)
    {
        mStorage.heap.first = other.mStorage.heap.first;
        mStorage.heap.last = other.mStorage.heap.last;
        mStorage.heap.end = other.mStorage.heap.end;
    }
    else if ((copyOnWrite && otherIsHeap) || (isLocal && otherIsHeap))
    {
        const SizeT otherSize = other.GetHeapSize();
        // Convert to heap:
        if (otherSize >= LF_STRING_STORAGE_SUB_1)
        {
            Grow(otherSize); // todo: Grow no copy:
            memcpy(mStorage.heap.first, other.mStorage.heap.first, (otherSize + 1) * sizeof(value_type)); // +1 for null terminator
            mStorage.heap.last = mStorage.heap.first + otherSize;
        }
        else // Convert to local:
        {
            memcpy(mStorage.local.buffer, other.mStorage.heap.first, (otherSize + 1) * sizeof(value_type)); // +1 for null terminator
            SetLocalSize(other.GetHeapSize());
        }
    }
    else if (isHeap && otherIsLocal)
    {
        const SizeT otherSize = other.GetLocalSize();
        memcpy(mStorage.heap.first, other.mStorage.local.buffer, (otherSize + 1) * sizeof(value_type)); // +1 for null terminator
        mStorage.heap.last = mStorage.heap.first + otherSize;
    }
    else if (isLocal && otherCopyOnWrite)
    {
        const SizeT otherSize = other.GetHeapSize();
        mStorage.heap.first = other.mStorage.heap.first;
        mStorage.heap.last = mStorage.heap.first + otherSize;
        mStorage.heap.end = mStorage.heap.last;
        SetFlag(LF_STRING_COPY_ON_WRITE_FLAG);
    }
    else if (isHeap && otherCopyOnWrite)
    {
        LFFree(mStorage.heap.first);
        const SizeT otherSize = other.GetHeapSize();
        mStorage.heap.first = other.mStorage.heap.first;
        mStorage.heap.last = mStorage.heap.first + otherSize;
        mStorage.heap.end = mStorage.heap.last;
        SetFlag(LF_STRING_COPY_ON_WRITE_FLAG);
        UnsetFlag(LF_STRING_STORAGE_FLAG);
    }
    else
    {
        CriticalAssertMsgEx("Unexpected condition.", LF_ERROR_INTERNAL, ERROR_API_CORE);
    }
    return *this;
}
WString& WString::Assign(const value_type* other)
{
    if (!other)
    {
        return *this;
    }

    if (Empty() && *other == '\0')
    {
        return *this;
    }

    // Cases: (cow = COPY ON WRITE )
    // this == local | other == heap  | Grow to fit and copy 'buffers'
    // this == heap  | other == heap  | Grow to fit and copy 'buffers'
    // this == cow   | other == heap  | Grow to fit and copy 'buffers'

    const SizeT otherSize = StrLen(other);
    const bool useHeap = UseHeap();
    const bool copyOnWrite = CopyOnWrite();
    const bool isLocal = !copyOnWrite && !useHeap;
    const bool isHeap = !copyOnWrite && useHeap;

    if (isLocal || copyOnWrite)
    {
        // Convert to heap:
        if (otherSize >= LF_STRING_STORAGE_SUB_1)
        {
            Grow(otherSize); // todo: Grow no copy:
            memcpy(mStorage.heap.first, other, (otherSize + 1) * sizeof(value_type)); // +1 for null terminator
            mStorage.heap.last = mStorage.heap.first + otherSize;
        }
        else // Convert to local:
        {
            memcpy(mStorage.local.buffer, other, (otherSize + 1) * sizeof(value_type)); // +1 for null terminator
            SetLocalSize(otherSize);
        }
    }
    else if (isHeap)
    {
        if (otherSize > GetHeapCapacity())
        {
            Grow(otherSize); // todo: Grow no copy:
        }
        memcpy(mStorage.heap.first, other, (otherSize + 1) * sizeof(value_type)); // +1 for null terminator
        mStorage.heap.last = mStorage.heap.first + otherSize;
    }
    else
    {
        CriticalAssertMsgEx("Unexpected condition.", LF_ERROR_INTERNAL, ERROR_API_CORE);
    }

    return(*this);
}

WString& WString::Append(value_type character)
{
    // if Empty > Just Assign
    // 
    // local -> local size = local, no free
    // local -> heap size = heap, no free
    // heap -> local size = heap, free on grow
    // heap -> heap size = heap, free on grow
    // cor -> local size = local, no free, no heap alloc
    // cor -> heap size = heap, no free, no heap alloc

    const SizeT newSize = Size() + 1;
    if ((newSize >= LF_STRING_STORAGE_SUB_1) || UseHeap())
    {
        if (newSize > Capacity() || CopyOnWrite())
        {
            Grow(Capacity() * 2);
        }
        AssertEx(UseHeap(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        value_type* buffer = mStorage.heap.first;
        buffer[newSize - 1] = character;
        buffer[newSize] = '\0';
        mStorage.heap.last = mStorage.heap.first + newSize;
    }
    else
    {
        if (CopyOnWrite())
        {
            MakeLocal();
        }
        AssertEx(!UseHeap(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        value_type* buffer = mStorage.local.buffer;
        buffer[newSize - 1] = character;
        buffer[newSize] = '\0';
        SetLocalSize(newSize);
    }
    return *this;
}
WString& WString::Append(const WString& other)
{
    if (Empty())
    {
        Assign(other);
        return *this;
    }

    const SizeT originalSize = Size();
    const SizeT otherSize = other.Size();
    const SizeT newSize = originalSize + otherSize;
    if ((newSize >= LF_STRING_STORAGE_SUB_1) || UseHeap())
    {
        if (newSize > Capacity() || CopyOnWrite())
        {
            Grow(Max(Capacity() * 2, newSize));
        }
        AssertEx(UseHeap(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        value_type* buffer = mStorage.heap.first + originalSize;
        memcpy(buffer, other.GetBufferPointer(), otherSize * sizeof(value_type));
        buffer[otherSize] = '\0';
        mStorage.heap.last = buffer + otherSize;
    }
    else
    {
        if (CopyOnWrite())
        {
            MakeLocal();
        }
        AssertEx(!UseHeap(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        value_type* buffer = mStorage.local.buffer + originalSize;
        memcpy(buffer, other.GetBufferPointer(), otherSize * sizeof(value_type));
        buffer[otherSize] = '\0';
        SetLocalSize(newSize);
    }
    return *this;
}
WString& WString::Append(const value_type* other)
{
    if (Empty())
    {
        Assign(other);
        return *this;
    }
    WString temp(other, COPY_ON_WRITE);
    Append(temp);
    return *this;
}

void WString::Insert(value_type c, SizeT position)
{
    if (position >= Size())
    {
        Append(c);
        return;
    }
    else
    {
        SizeT originalSize = Size();
        if (originalSize + 1 > Capacity())
        {
            Grow(Capacity() * 2);
        }
        value_type* buffer = GetBufferPointer();
        value_type* oldEnd = buffer + originalSize;
        value_type* newEnd = buffer + originalSize + 1;
        value_type* positionPtr = buffer + position;
        if (UseHeap())
        {
            mStorage.heap.last = newEnd;
        }
        else
        {
            SetLocalSize(originalSize + 1);
        }
        // [ ] [ ] [ a ] [ b ]
        // [ ] [ ] [ a/e ] [ b/c ] [ d ]
        // [ ] [ ] [ e ] [ c ] [ d ]

        // Copy Shift:
        value_type* oldCursor = oldEnd;
        value_type* newCursor = newEnd;
        for (SizeT i = 0, size = oldEnd - positionPtr; i < size; ++i)
        {
            *newCursor = *oldCursor;
            --newCursor;
            --oldCursor;
        }
        // Insert Single:
        *positionPtr = c;
    }
}
void WString::Insert(const WString& string, SizeT position)
{
    if (position >= Size())
    {
        Append(string);
        return;
    }
    else
    {
        const SizeT otherSize = string.Size();
        const value_type* otherBuffer = string.GetBufferPointer();

        const SizeT originalSize = Size();
        if (originalSize + otherSize > Capacity())
        {
            Grow(originalSize + otherSize);
        }
        value_type* buffer = GetBufferPointer();
        value_type* oldEnd = buffer + originalSize;
        value_type* newEnd = buffer + originalSize + otherSize;
        value_type* positionPtr = buffer + position;
        if (UseHeap())
        {
            mStorage.heap.last = newEnd;
        }
        else
        {
            SetLocalSize(originalSize + 1);
        }
        // [ ] [ ] [ a ] [ b ]
        // [ ] [ ] [ a/e ] [ b/c ] [ d ]
        // [ ] [ ] [ e ] [ c ] [ d ]

        // Copy Shift:
        value_type* oldCursor = oldEnd;
        value_type* newCursor = newEnd;
        for (SizeT i = 0, size = (oldEnd - positionPtr) + 1; i < size; ++i)
        {
            *newCursor = *oldCursor;
            --newCursor;
            --oldCursor;
        }
        for (SizeT i = 0; i < otherSize; ++i)
        {
            positionPtr[i] = otherBuffer[i];
        }
    }
}

void WString::SubString(SizeT start, WString& outString) const
{
    SubString(start, Size() - start, outString);
}
void WString::SubString(SizeT start, SizeT length, WString& outString) const
{
    AssertEx(&outString != this, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    if (&outString == this)
    {
        return;
    }

    outString.Resize(0);

    SizeT size = Size();
    if (start > size)
    {
        return;
    }
    SizeT correctLength = Min((size - start), length);
    if (correctLength == 0)
    {
        return;
    }

    outString.Resize(correctLength);
    memcpy(outString.GetBufferPointer(), GetBufferPointer() + start, correctLength * sizeof(value_type));
}
WString WString::SubString(SizeT start) const
{
    WString result;
    SubString(start, result);
    return result;
}
WString WString::SubString(SizeT start, SizeT length) const
{
    WString result;
    SubString(start, length, result);
    return result;
}

SizeT WString::Replace(value_type find, value_type replace)
{
    if (Empty())
    {
        return 0;
    }
    value_type* buffer = GetBufferPointer();
    SizeT size = Size();
    SizeT occurances = 0;
    for (SizeT i = 0; i < size; ++i)
    {
        if (buffer[i] == find)
        {
            buffer[i] = replace;
            ++occurances;
        }
    }
    return occurances;
}
SizeT WString::Replace(const WString& find, const WString& replace)
{
    if (Empty() || find.Empty()) // Use strip instead.
    {
        return 0;
    }
    SizeT replacedCount = 0;
    WString resultBuffer;

    const value_type* buffer = GetBufferPointer();
    const value_type* findBuffer = find.GetBufferPointer();
    const value_type* replaceBuffer = replace.GetBufferPointer();
    const SizeT findSize = find.Size();
    const SizeT replaceSize = replace.Size();

    for (SizeT i = 0, size = Size(); i < size; ++i)
    {
        if (buffer[i] == findBuffer[0])
        {
            // try match
            bool matched = false;
            SizeT findCursor = 0;
            SizeT thisCursor = i;
            SizeT findMinusOne = findSize - 1;
            while (thisCursor < size && findCursor < findSize)
            {
                if (buffer[thisCursor] != findBuffer[findCursor])
                {
                    break;
                }

                if (findCursor == findMinusOne)
                {
                    matched = true;
                }
                ++thisCursor;
                ++findCursor;
            }

            // replace:
            if (matched)
            {
                SizeT replaceCursor = 0;
                thisCursor = i;
                ++replacedCount;
                while (replaceCursor < replaceSize)
                {
                    resultBuffer.Append(replaceBuffer[replaceCursor]);
                    ++replaceCursor;
                }
                i += findMinusOne;
            }
            else
            {
                resultBuffer.Append(buffer[i]);
            }
        }
        else
        {
            resultBuffer.Append(buffer[i]);
        }
    }
    Swap(resultBuffer);
    return replacedCount;
}

SizeT WString::Find(value_type character) const
{
    return StrFind(begin(), end(), character);
}
SizeT WString::Find(const WString& string) const
{
    return StrFind(begin(), end(), string.begin(), string.end());
}
SizeT WString::Find(const value_type* string) const
{
    return StrFind(begin(), end(), string);
}
SizeT WString::FindAgnostic(value_type character) const
{
    return StrFindAgnostic(begin(), end(), character);
}
SizeT WString::FindAgnostic(const WString& string) const
{
    return StrFindAgnostic(begin(), end(), string.begin(), string.end());
}
SizeT WString::FindAgnostic(const value_type* string) const
{
    return StrFindAgnostic(begin(), end(), string);
}
SizeT WString::FindLast(value_type character) const
{
    return StrFindLast(begin(), end(), character);
}
SizeT WString::FindLast(const WString& string) const
{
    return StrFindLast(begin(), end(), string.begin(), string.end());
}
SizeT WString::FindLast(const value_type* string) const
{
    return StrFindLast(begin(), end(), string);
}
SizeT WString::FindLastAgnostic(value_type character) const
{
    return StrFindLastAgnostic(begin(), end(), character);
}
SizeT WString::FindLastAgnostic(const WString& string) const
{
    return StrFindLastAgnostic(begin(), end(), string.begin(), string.end());
}
SizeT WString::FindLastAgnostic(const value_type* string) const
{
    return StrFindLastAgnostic(begin(), end(), string);
}

void WString::Grow(SizeT desiredCapacity)
{
    const bool freeBuffer = !CopyOnWrite() && UseHeap();
    // 32 characters == buffer size
    // 30 characters == just enough for string + null terminator
    if ((desiredCapacity + 1) >= LF_STRING_STORAGE_SUB_1)
    {
        // Allocate Heap:
        value_type* newBuffer = static_cast<value_type*>(LFAlloc((desiredCapacity + 1) * sizeof(value_type) , 16));
        value_type* buffer = GetBufferPointer();
        memcpy(newBuffer, buffer, Min(Size(), desiredCapacity) * sizeof(value_type));
        if (freeBuffer)
        {
            LFFree(buffer);
        }
        mStorage.heap.first = newBuffer;
        mStorage.heap.end = newBuffer + desiredCapacity;
        *(mStorage.heap.end) = L'\0';
        SetFlag(LF_STRING_STORAGE_FLAG);
    }
    else
    {
        // Use local-buffer:
        value_type newBuffer[LF_STRING_DEFAULT_STORAGE];
        value_type* buffer = GetBufferPointer();
        memcpy(newBuffer, buffer, Min(Size(), desiredCapacity) * sizeof(value_type));
        if (freeBuffer)
        {
            LFFree(buffer);
        }
        memcpy(mStorage.local.buffer, newBuffer, desiredCapacity * sizeof(value_type));
        mStorage.local.buffer[desiredCapacity + 1] = '\0';
        UnsetFlag(LF_STRING_STORAGE_FLAG);
    }
    UnsetFlag(LF_STRING_COPY_ON_WRITE_FLAG);
}

void WString::MakeLocal()
{
    AssertEx(CopyOnWrite(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertEx(Size() < (LF_STRING_DEFAULT_STORAGE - 2), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    value_type* first = mStorage.heap.first;
    value_type* last = mStorage.heap.last;
    const SizeT size = last - first;
    memcpy(mStorage.local.buffer, first, size * sizeof(value_type));
    SetLocalSize(size);
    UnsetFlag(LF_STRING_COPY_ON_WRITE_FLAG);
}

} // namespace lf