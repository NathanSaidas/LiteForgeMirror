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
#include "StringUtil.h"
#include "Core/Common/Assert.h"
#include <cctype>
#include <emmintrin.h> // SSE 2

namespace lf {

using SimdCharacter = __m128i;
const SizeT SIMD_ALIGN = 16;

const Char8* gNullString = "";

LF_FORCE_INLINE static UIntPtrT AlignForwardAdjustment(void * address, SizeT alignment)
{
    UIntPtrT adjustment = alignment - (reinterpret_cast<UIntPtrT>(address)& static_cast<UIntPtrT>(alignment - 1));
    if (adjustment == alignment)
    {
        return 0;
    }
    return adjustment;
}

LF_FORCE_INLINE static Char8 ToLower(Char8 c)
{
    return static_cast<Char8>(std::tolower(c));
}
LF_FORCE_INLINE static Char8 ToUpper(Char8 c)
{
    return static_cast<Char8>(std::toupper(c));
}
LF_FORCE_INLINE static Char16 ToLower(Char16 c)
{
    return static_cast<Char16>(towlower(c));
}
LF_FORCE_INLINE static Char16 ToUpper(Char16 c)
{
    return static_cast<Char16>(towupper(c));
}

bool StrEqual(const Char8* beginA, const Char8* beginB, const Char8* endA, const Char8* endB)
{
    if (beginA == beginB)
    {
        return true;
    }

    const SizeT sizeA = endA - beginA;
    const SizeT sizeB = endB - beginB;
    if (sizeA != sizeB)
    {
        return false;
    }

    UIntPtrT alignA = AlignForwardAdjustment(const_cast<char*>(beginA), SIMD_ALIGN);
    UIntPtrT alignB = AlignForwardAdjustment(const_cast<char*>(beginB), SIMD_ALIGN);
    if (alignA != alignB || (sizeA - alignA) < 16 || sizeA < 16)
    {
        const char* itA = beginA;
        const char* itB = beginB;

        while (itA != endA && itB != endB)
        {
            if (*itA != *itB)
            {
                return false;
            }
            ++itA;
            ++itB;
        }
        return true;
    }
    else
    {
        const SizeT numPreCompare = alignA;
        const SizeT numSimdCompare = (sizeA - alignA) / SIMD_ALIGN;
        const SizeT numPostCompare = (sizeA - alignA) - (numSimdCompare * SIMD_ALIGN);
        Assert(numSimdCompare >= 1);

        const char* itA = beginA;
        const char* itB = beginB;
        for (SizeT i = 0; i < numPreCompare; ++i)
        {
            if (*itA != *itB)
            {
                return false;
            }
            ++itA;
            ++itB;
        }

        const SimdCharacter* simdItA = reinterpret_cast<const SimdCharacter*>(itA);
        const SimdCharacter* simdItB = reinterpret_cast<const SimdCharacter*>(itB);
        for (SizeT i = 0; i < numSimdCompare; ++i)
        {
            // simd a != b
            int r = _mm_movemask_epi8(_mm_cmpeq_epi8(*simdItA, *simdItB));
            if (r != 0xFFFF)
            {
                return false;
            }
            ++simdItA;
            ++simdItB;
        }

        itA += numSimdCompare * SIMD_ALIGN;
        itB += numSimdCompare * SIMD_ALIGN;
        for (SizeT i = 0; i < numPostCompare; ++i)
        {
            if (*itA != *itB)
            {
                return false;
            }
            ++itA;
            ++itB;
        }
        return true;
    }
}
bool StrNotEqual(const Char8* beginA, const Char8* beginB, const Char8* endA, const Char8* endB)
{
    if (beginA == beginB)
    {
        return false;
    }

    const SizeT sizeA = endA - beginA;
    const SizeT sizeB = endB - beginB;
    if (sizeA != sizeB)
    {
        return true;
    }

    UIntPtrT alignA = AlignForwardAdjustment(const_cast<char*>(beginA), SIMD_ALIGN);
    UIntPtrT alignB = AlignForwardAdjustment(const_cast<char*>(beginB), SIMD_ALIGN);
    if (alignA != alignB || (sizeA - alignA) < 16 || sizeA < 16)
    {
        const char* itA = beginA;
        const char* itB = beginB;

        while (itA != endA && itB != endB)
        {
            if (*itA != *itB)
            {
                return true;
            }
            ++itA;
            ++itB;
        }
        return false;
    }
    else
    {
        const SizeT numPreCompare = alignA;
        const SizeT numSimdCompare = (sizeA - alignA) / SIMD_ALIGN;
        const SizeT numPostCompare = (sizeA - alignA) - (numSimdCompare * SIMD_ALIGN);
        Assert(numSimdCompare >= 1);

        const char* itA = beginA;
        const char* itB = beginB;
        for (SizeT i = 0; i < numPreCompare; ++i)
        {
            if (*itA != *itB)
            {
                return true;
            }
            ++itA;
            ++itB;
        }

        const SimdCharacter* simdItA = reinterpret_cast<const SimdCharacter*>(itA);
        const SimdCharacter* simdItB = reinterpret_cast<const SimdCharacter*>(itB);
        for (SizeT i = 0; i < numSimdCompare; ++i)
        {
            // simd a == b
            int r = _mm_movemask_epi8(_mm_cmpeq_epi8(*simdItA, *simdItB));
            if (r != 0xFFFF)
            {
                return true;
            }
            ++simdItA;
            ++simdItB;
        }

        itA += numSimdCompare * SIMD_ALIGN;
        itB += numSimdCompare * SIMD_ALIGN;
        for (SizeT i = 0; i < numPostCompare; ++i)
        {
            if (*itA != *itB)
            {
                return true;
            }
            ++itA;
            ++itB;
        }
        return false;
    }
}
bool StrAlphaLess(const Char8* a, const Char8* b)
{
    if (a == b)
    {
        return false;
    }
    else if (*a == '\0')
    {
        return true;
    }
    else if (*b == '\0')
    {
        return false;
    }

    do
    {
        if (*a < *b)
        {
            return true;
        }
        else if (*a > *b)
        {
            return false;
        }
        ++a;
        ++b;

        if (*a == '\0')
        {
            return *b != '\0';
        }
        else if (*b == '\0')
        {
            return *a == '\0';
        }

    } while (*a != '\0' && *b != '\0');

    return false;
}
bool StrAlphaGreater(const Char8* a, const Char8* b)
{
    if (a == b)
    {
        return false;
    }
    else if (*a == '\0')
    {
        return false;
    }
    else if (*b == '\0')
    {
        return true;
    }

    do
    {
        if (*a < *b)
        {
            return false;
        }
        else if (*a > *b)
        {
            return true;
        }
        ++a;
        ++b;

        if (*a == '\0')
        {
            return *b == '\0';
        }
        else if (*b == '\0')
        {
            return *a != '\0';
        }

    } while (*a != '\0' && *b != '\0');

    return true;
}
SizeT StrLen(const Char8* a)
{
    const Char8* first = a;
    while (*a)
    {
        ++a;
    }
    return a - first;
}
SizeT StrFind(const Char8* first, const Char8* last, Char8 character)
{
    const Char8* it = first;
    while (it != last)
    {
        if (*it == character)
        {
            return it - first;
        }
        ++it;
    }
    return INVALID;
}
SizeT StrFind(const Char8* first, const Char8* last, const Char8* token)
{
    const Char8* end = token;
    SizeT strSize = last - first;
    SizeT tokenSize= 0;
    while (*end != '\0')
    {
        ++end;
        ++tokenSize;
        if (tokenSize > strSize)
        {
            return INVALID; // can't possibly fit in the string
        }
    }
    return StrFind(first, last, token, end);
}
SizeT StrFind(const Char8* first, const Char8* last, const Char8* tokenBegin, const Char8* tokenEnd)
{
    if ((tokenEnd - tokenBegin) > (last - first))
    {
        return INVALID;
    }
    if (first == last)
    {
        return INVALID;
    }

    const Char8* cursor = first;
    const Char8* otherCursor = tokenBegin;
    const Char8* fallback = nullptr;
    const Char8* saved = nullptr;

    while (cursor != last)
    {
        if (*cursor == *otherCursor)
        {
            ++cursor;
            ++otherCursor;
            saved = cursor;
            for (; otherCursor != tokenEnd && cursor != last; ++cursor, ++otherCursor)
            {
                if (saved == nullptr && *cursor == *tokenBegin)
                {
                    fallback = cursor;
                }

                if (*cursor != *otherCursor)
                {
                    cursor = fallback ? fallback : saved;
                    otherCursor = tokenBegin;
                    break;
                }
            }
            if (otherCursor == tokenEnd)
            {
                return (saved - first) - 1;
            }
        }
        else
        {
            ++cursor;
        }
    }
    return INVALID;
}

SizeT StrFindAgnostic(const Char8* first, const Char8* last, Char8 character)
{
    Char8 lower = ToLower(character);
    Char8 upper = ToUpper(character);
    const Char8* it = first;
    while (it != last)
    {
        if (*it == lower || *it == upper)
        {
            return it - first;
        }
        ++it;
    }
    return INVALID;
}
SizeT StrFindAgnostic(const Char8* first, const Char8* last, const Char8* token)
{
    const Char8* end = token;
    SizeT strSize = last - first;
    SizeT tokenSize = 0;
    while (*end != '\0')
    {
        ++end;
        ++tokenSize;
        if (tokenSize > strSize)
        {
            return INVALID; // can't possibly fit in the string
        }
    }
    return StrFindAgnostic(first, last, token, end);
}
SizeT StrFindAgnostic(const Char8* first, const Char8* last, const Char8* tokenBegin, const Char8* tokenEnd)
{
    if ((tokenEnd - tokenBegin) > (last - first))
    {
        return INVALID;
    }
    if (first == last)
    {
        return INVALID;
    }

    const Char8* cursor = first;
    const Char8* otherCursor = tokenBegin;
    const Char8* fallback = nullptr;
    const Char8* saved = nullptr;
    Char8 lowerTokenBegin = ToLower(*tokenBegin);

    while (cursor != last)
    {
        Char8 lowerCursor = ToLower(*cursor);
        Char8 lowerOtherCursor = ToLower(*otherCursor);
        if (lowerCursor == lowerOtherCursor)
        {
            ++cursor;
            ++otherCursor;
            saved = cursor;
            for (; otherCursor != tokenEnd && cursor != last; ++cursor, ++otherCursor)
            {
                lowerCursor = ToLower(*cursor);
                if (saved == nullptr && lowerCursor == lowerTokenBegin)
                {
                    fallback = cursor;
                }

                lowerOtherCursor = ToLower(*otherCursor);
                if (lowerCursor != lowerOtherCursor)
                {
                    cursor = fallback ? fallback : saved;
                    otherCursor = tokenBegin;
                    break;
                }
            }
            if (otherCursor == tokenEnd)
            {
                return (saved - first) - 1;
            }
        }
        else
        {
            ++cursor;
        }
    }
    return INVALID;
}

SizeT StrFindLast(const Char8* first, const Char8* last, Char8 character)
{
    const Char8* it = last - 1;
    const Char8* rend = first - 1;
    while (it != rend)
    {
        if (*it == character)
        {
            return it - first;
        }
        --it;
    }
    return INVALID;
}
SizeT StrFindLast(const Char8* first, const Char8* last, const Char8* token)
{
    const Char8* end = token;
    SizeT strSize = last - first;
    SizeT tokenSize = 0;
    while (*end != '\0')
    {
        ++end;
        ++tokenSize;
        if (tokenSize > strSize)
        {
            return INVALID; // can't possibly fit in the string
        }
    }
    return StrFindLast(first, last, token, end);
}
SizeT StrFindLast(const Char8* first, const Char8* last, const Char8* tokenBegin, const Char8* tokenEnd)
{
    if ((tokenEnd - tokenBegin) > (last - first))
    {
        return INVALID;
    }
    if (first == last)
    {
        return INVALID;
    }

    const Char8* cursor = last - 1;
    const Char8* otherCursor = tokenEnd - 1;
    const Char8* fallback = nullptr;
    const Char8* saved = nullptr;
    const Char8* rend = first - 1;
    const Char8* tokenRend = tokenBegin - 1;

    while (cursor != rend)
    {
        if (*cursor == *otherCursor)
        {
            --cursor;
            --otherCursor;
            saved = cursor;
            for (; otherCursor != tokenRend && cursor != rend; --cursor, --otherCursor)
            {
                if (saved == nullptr && *cursor == *tokenBegin)
                {
                    fallback = cursor;
                }

                if (*cursor != *otherCursor)
                {
                    cursor = fallback ? fallback : saved;
                    otherCursor = tokenEnd - 1;
                    break;
                }
            }
            if (otherCursor == tokenRend)
            {
                return (cursor - first) + 1;
            }
        }
        else
        {
            --cursor;
        }
    }
    return INVALID;
}

SizeT StrFindLastAgnostic(const Char8* first, const Char8* last, Char8 character)
{
    Char8 lower = ToLower(character);
    Char8 upper = ToUpper(character);
    const Char8* it = last - 1;
    const Char8* rend = first - 1;
    while (it != rend)
    {
        if (*it == lower || *it == upper)
        {
            return it - first;
        }
        --it;
    }
    return INVALID;
}
SizeT StrFindLastAgnostic(const Char8* first, const Char8* last, const Char8* token)
{
    const Char8* end = token;
    SizeT strSize = last - first;
    SizeT tokenSize = 0;
    while (*end != '\0')
    {
        ++end;
        ++tokenSize;
        if (tokenSize > strSize)
        {
            return INVALID; // can't possibly fit in the string
        }
    }
    return StrFindLastAgnostic(first, last, token, end);
}
SizeT StrFindLastAgnostic(const Char8* first, const Char8* last, const Char8* tokenBegin, const Char8* tokenEnd)
{
    if ((tokenEnd - tokenBegin) > (last - first))
    {
        return INVALID;
    }
    if (first == last)
    {
        return INVALID;
    }

    const Char8* cursor = last - 1;
    const Char8* otherCursor = tokenEnd - 1;
    const Char8* fallback = nullptr;
    const Char8* saved = nullptr;
    const Char8* rend = first - 1;
    const Char8* tokenRend = tokenBegin - 1;
    Char8 lowerTokenBegin = ToLower(*tokenBegin);

    while (cursor != rend)
    {
        Char8 lowerCursor = ToLower(*cursor);
        Char8 lowerOtherCursor = ToLower(*otherCursor);
        if (lowerCursor == lowerOtherCursor)
        {
            --cursor;
            --otherCursor;
            saved = cursor;
            for (; otherCursor != tokenRend && cursor != rend; --cursor, --otherCursor)
            {
                lowerCursor = ToLower(*cursor);
                if (saved == nullptr && lowerCursor == lowerTokenBegin)
                {
                    fallback = cursor;
                }

                lowerOtherCursor = ToLower(*otherCursor);
                if (lowerCursor != lowerOtherCursor)
                {
                    cursor = fallback ? fallback : saved;
                    otherCursor = tokenEnd - 1;
                    break;
                }
            }
            if (otherCursor == tokenRend)
            {
                return (cursor - first) + 1;
            }
        }
        else
        {
            --cursor;
        }
    }
    return INVALID;
}

void ToLower(Char8* string)
{
    while (*string)
    {
        *string = ToLower(*string);
        ++string;
    }
}
void ToUpper(Char8* string)
{
    while (*string)
    {
        *string = ToUpper(*string);
        ++string;
    }
}

bool StrEqual(const Char16* beginA, const Char16* beginB, const Char16* endA, const Char16* endB)
{
    if (beginA == beginB)
    {
        return true;
    }

    const SizeT sizeA = endA - beginA;
    const SizeT sizeB = endB - beginB;
    if (sizeA != sizeB)
    {
        return false;
    }

    const Char16* itA = beginA;
    const Char16* itB = beginB;

    while (itA != endA && itB != endB)
    {
        if (*itA != *itB)
        {
            return false;
        }
        ++itA;
        ++itB;
    }
    return true;
}

bool StrNotEqual(const Char16* beginA, const Char16* beginB, const Char16* endA, const Char16* endB)
{
    if (beginA == beginB)
    {
        return false;
    }

    const SizeT sizeA = endA - beginA;
    const SizeT sizeB = endB - beginB;
    if (sizeA != sizeB)
    {
        return true;
    }

    const Char16* itA = beginA;
    const Char16* itB = beginB;

    while (itA != endA && itB != endB)
    {
        if (*itA != *itB)
        {
            return true;
        }
        ++itA;
        ++itB;
    }
    return false;
}
bool StrAlphaLess(const Char16* a, const Char16* b)
{
    if (a == b)
    {
        return false;
    }
    else if (*a == L'\0')
    {
        return true;
    }
    else if (*b == L'\0')
    {
        return false;
    }

    do
    {
        if (*a < *b)
        {
            return true;
        }
        else if (*a > *b)
        {
            return false;
        }
        ++a;
        ++b;

        if (*a == L'\0')
        {
            return *b != L'\0';
        }
        else if (*b == L'\0')
        {
            return *a == L'\0';
        }

    } while (*a != L'\0' && *b != L'\0');

    return false;
}
bool StrAlphaGreater(const Char16* a, const Char16* b)
{
    if (a == b)
    {
        return false;
    }
    else if (*a == L'\0')
    {
        return false;
    }
    else if (*b == L'\0')
    {
        return true;
    }

    do
    {
        if (*a < *b)
        {
            return false;
        }
        else if (*a > *b)
        {
            return true;
        }
        ++a;
        ++b;

        if (*a == L'\0')
        {
            return *b == L'\0';
        }
        else if (*b == L'\0')
        {
            return *a != L'\0';
        }

    } while (*a != L'\0' && *b != L'\0');

    return true;
}
SizeT StrLen(const Char16* a)
{
    const Char16* first = a;
    while (*a)
    {
        ++a;
    }
    return a - first;
}

SizeT StrFind(const Char16* first, const Char16* last, Char16 character)
{
    const Char16* it = first;
    while (it != last)
    {
        if (*it == character)
        {
            return it - first;
        }
        ++it;
    }
    return INVALID;
}
SizeT StrFind(const Char16* first, const Char16* last, const Char16* token)
{
    const Char16* end = token;
    SizeT strSize = last - first;
    SizeT tokenSize = 0;
    while (*end != L'\0')
    {
        ++end;
        ++tokenSize;
        if (tokenSize > strSize)
        {
            return INVALID; // can't possibly fit in the string
        }
    }
    return StrFind(first, last, token, end);
}
SizeT StrFind(const Char16* first, const Char16* last, const Char16* tokenBegin, const Char16* tokenEnd)
{
    if ((tokenEnd - tokenBegin) > (last - first))
    {
        return INVALID;
    }
    if (first == last)
    {
        return INVALID;
    }

    const Char16* cursor = first;
    const Char16* otherCursor = tokenBegin;
    const Char16* fallback = nullptr;
    const Char16* saved = nullptr;

    while (cursor != last)
    {
        if (*cursor == *otherCursor)
        {
            ++cursor;
            ++otherCursor;
            saved = cursor;
            for (; otherCursor != tokenEnd && cursor != last; ++cursor, ++otherCursor)
            {
                if (saved == nullptr && *cursor == *tokenBegin)
                {
                    fallback = cursor;
                }

                if (*cursor != *otherCursor)
                {
                    cursor = fallback ? fallback : saved;
                    otherCursor = tokenBegin;
                    break;
                }
            }
            if (otherCursor == tokenEnd)
            {
                return (saved - first) - 1;
            }
        }
        else
        {
            ++cursor;
        }
    }
    return INVALID;
}

SizeT StrFindAgnostic(const Char16* first, const Char16* last, Char16 character)
{
    Char16 lower = ToLower(character);
    Char16 upper = ToUpper(character);
    const Char16* it = first;
    while (it != last)
    {
        if (*it == lower || *it == upper)
        {
            return it - first;
        }
        ++it;
    }
    return INVALID;
}
SizeT StrFindAgnostic(const Char16* first, const Char16* last, const Char16* token)
{
    const Char16* end = token;
    SizeT strSize = last - first;
    SizeT tokenSize = 0;
    while (*end != L'\0')
    {
        ++end;
        ++tokenSize;
        if (tokenSize > strSize)
        {
            return INVALID; // can't possibly fit in the string
        }
    }
    return StrFindAgnostic(first, last, token, end);
}
SizeT StrFindAgnostic(const Char16* first, const Char16* last, const Char16* tokenBegin, const Char16* tokenEnd)
{
    if ((tokenEnd - tokenBegin) > (last - first))
    {
        return INVALID;
    }
    if (first == last)
    {
        return INVALID;
    }

    const Char16* cursor = first;
    const Char16* otherCursor = tokenBegin;
    const Char16* fallback = nullptr;
    const Char16* saved = nullptr;
    Char16 lowerTokenBegin = ToLower(*tokenBegin);

    while (cursor != last)
    {
        Char16 lowerCursor = ToLower(*cursor);
        Char16 lowerOtherCursor = ToLower(*otherCursor);
        if (lowerCursor == lowerOtherCursor)
        {
            ++cursor;
            ++otherCursor;
            saved = cursor;
            for (; otherCursor != tokenEnd && cursor != last; ++cursor, ++otherCursor)
            {
                lowerCursor = ToLower(*cursor);
                if (saved == nullptr && lowerCursor == lowerTokenBegin)
                {
                    fallback = cursor;
                }

                lowerOtherCursor = ToLower(*otherCursor);
                if (lowerCursor != lowerOtherCursor)
                {
                    cursor = fallback ? fallback : saved;
                    otherCursor = tokenBegin;
                    break;
                }
            }
            if (otherCursor == tokenEnd)
            {
                return (saved - first) - 1;
            }
        }
        else
        {
            ++cursor;
        }
    }
    return INVALID;
}

SizeT StrFindLast(const Char16* first, const Char16* last, Char16 character)
{
    const Char16* it = last - 1;
    const Char16* rend = first - 1;
    while (it != rend)
    {
        if (*it == character)
        {
            return it - first;
        }
        --it;
    }
    return INVALID;
}
SizeT StrFindLast(const Char16* first, const Char16* last, const Char16* token)
{
    const Char16* end = token;
    SizeT strSize = last - first;
    SizeT tokenSize = 0;
    while (*end != L'\0')
    {
        ++end;
        ++tokenSize;
        if (tokenSize > strSize)
        {
            return INVALID; // can't possibly fit in the string
        }
    }
    return StrFindLast(first, last, token, end);
}
SizeT StrFindLast(const Char16* first, const Char16* last, const Char16* tokenBegin, const Char16* tokenEnd)
{
    if ((tokenEnd - tokenBegin) > (last - first))
    {
        return INVALID;
    }
    if (first == last)
    {
        return INVALID;
    }

    const Char16* cursor = last - 1;
    const Char16* otherCursor = tokenEnd - 1;
    const Char16* fallback = nullptr;
    const Char16* saved = nullptr;
    const Char16* rend = first - 1;
    const Char16* tokenRend = tokenBegin - 1;

    while (cursor != rend)
    {
        if (*cursor == *otherCursor)
        {
            --cursor;
            --otherCursor;
            saved = cursor;
            for (; otherCursor != tokenRend && cursor != rend; --cursor, --otherCursor)
            {
                if (saved == nullptr && *cursor == *tokenBegin)
                {
                    fallback = cursor;
                }

                if (*cursor != *otherCursor)
                {
                    cursor = fallback ? fallback : saved;
                    otherCursor = tokenEnd - 1;
                    break;
                }
            }
            if (otherCursor == tokenRend)
            {
                return (cursor - first) + 1;
            }
        }
        else
        {
            --cursor;
        }
    }
    return INVALID;
}

SizeT StrFindLastAgnostic(const Char16* first, const Char16* last, Char16 character)
{
    Char16 lower = ToLower(character);
    Char16 upper = ToUpper(character);
    const Char16* it = last - 1;
    const Char16* rend = first - 1;
    while (it != rend)
    {
        if (*it == lower || *it == upper)
        {
            return it - first;
        }
        --it;
    }
    return INVALID;
}
SizeT StrFindLastAgnostic(const Char16* first, const Char16* last, const Char16* token)
{
    const Char16* end = token;
    SizeT strSize = last - first;
    SizeT tokenSize = 0;
    while (*end != L'\0')
    {
        ++end;
        ++tokenSize;
        if (tokenSize > strSize)
        {
            return INVALID; // can't possibly fit in the string
        }
    }
    return StrFindLastAgnostic(first, last, token, end);
}
SizeT StrFindLastAgnostic(const Char16* first, const Char16* last, const Char16* tokenBegin, const Char16* tokenEnd)
{
    if ((tokenEnd - tokenBegin) > (last - first))
    {
        return INVALID;
    }
    if (first == last)
    {
        return INVALID;
    }

    const Char16* cursor = last - 1;
    const Char16* otherCursor = tokenEnd - 1;
    const Char16* fallback = nullptr;
    const Char16* saved = nullptr;
    const Char16* rend = first - 1;
    const Char16* tokenRend = tokenBegin - 1;
    Char16 lowerTokenBegin = ToLower(*tokenBegin);

    while (cursor != rend)
    {
        Char16 lowerCursor = ToLower(*cursor);
        Char16 lowerOtherCursor = ToLower(*otherCursor);
        if (lowerCursor == lowerOtherCursor)
        {
            --cursor;
            --otherCursor;
            saved = cursor;
            for (; otherCursor != tokenRend && cursor != rend; --cursor, --otherCursor)
            {
                lowerCursor = ToLower(*cursor);
                if (saved == nullptr && lowerCursor == lowerTokenBegin)
                {
                    fallback = cursor;
                }

                lowerOtherCursor = ToLower(*otherCursor);
                if (lowerCursor != lowerOtherCursor)
                {
                    cursor = fallback ? fallback : saved;
                    otherCursor = tokenEnd - 1;
                    break;
                }
            }
            if (otherCursor == tokenRend)
            {
                return (cursor - first) + 1;
            }
        }
        else
        {
            --cursor;
        }
    }
    return INVALID;
}

void ToLower(Char16* string)
{
    while (*string)
    {
        *string = ToLower(*string);
        ++string;
    }
}
void ToUpper(Char16* string)
{
    while (*string)
    {
        *string = ToUpper(*string);
        ++string;
    }
}

} // namespace lf