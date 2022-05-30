// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

const SizeT LF_STRING_DEFAULT_STORAGE = 32;
const SizeT LF_STRING_STORAGE_SUB_1 = LF_STRING_DEFAULT_STORAGE - 1;
const UInt8 LF_STRING_STORAGE_FLAG = 1 << 7;
const UInt8 LF_STRING_COPY_ON_WRITE_FLAG = 1 << 6;
const UInt8 LF_STRING_SIZE_MASK = 0x3F;
const UInt8 LF_STRING_FLAG_MASK = 0xC0;

LF_CORE_API extern const Char8* gNullString;

// Char8 interface

// Compares to make sure two strings are exactly equal
LF_CORE_API bool StrEqual(const Char8* beginA, const Char8* beginB, const Char8* endA, const Char8* endB);
// Compares to make sure two strings are not equal
LF_CORE_API bool StrNotEqual(const Char8* beginA, const Char8* beginB, const Char8* endA, const Char8* endB);
// Does alpha-less compare on 2 strings
LF_CORE_API bool StrAlphaLess(const Char8* a, const Char8* b);
// Does alpha-greater compare on 2 strings
LF_CORE_API bool StrAlphaGreater(const Char8* a, const Char8* b);
// Calculates the length of the string
LF_CORE_API SizeT StrLen(const Char8* a);
// Finds the index of the first matching substring of 'character'/'token' in the string
LF_CORE_API SizeT StrFind(const Char8* first, const Char8* last, Char8 character);
LF_CORE_API SizeT StrFind(const Char8* first, const Char8* last, const Char8* token);
LF_CORE_API SizeT StrFind(const Char8* first, const Char8* last, const Char8* tokenBegin, const Char8* tokenEnd);
// Finds the index of the first matching substring of 'character'/'token' in the string while being indifferent to lower/upper case. (english only)
LF_CORE_API SizeT StrFindAgnostic(const Char8* first, const Char8* last, Char8 character);
LF_CORE_API SizeT StrFindAgnostic(const Char8* first, const Char8* last, const Char8* token);
LF_CORE_API SizeT StrFindAgnostic(const Char8* first, const Char8* last, const Char8* tokenBegin, const Char8* tokenEnd);
// Finds the index of the last matching substring of 'character'/'token' in the string
LF_CORE_API SizeT StrFindLast(const Char8* first, const Char8* last, Char8 character);
LF_CORE_API SizeT StrFindLast(const Char8* first, const Char8* last, const Char8* token);
LF_CORE_API SizeT StrFindLast(const Char8* first, const Char8* last, const Char8* tokenBegin, const Char8* tokenEnd);
// Finds the index of the last matching substring of 'character'/'token' in the string while being indifferent to lower/upper case. (english only)
LF_CORE_API SizeT StrFindLastAgnostic(const Char8* first, const Char8* last, Char8 character);
LF_CORE_API SizeT StrFindLastAgnostic(const Char8* first, const Char8* last, const Char8* token);
LF_CORE_API SizeT StrFindLastAgnostic(const Char8* first, const Char8* last, const Char8* tokenBegin, const Char8* tokenEnd);

LF_CORE_API void ToLower(Char8* string);
LF_CORE_API void ToUpper(Char8* string);

LF_FORCE_INLINE bool CharIsWhitespace(Char8 c) { return c == ' ' || c == '\t'; }
LF_FORCE_INLINE bool CharIsUpper(Char8 c) { return c > 64 && c < 91; }
LF_FORCE_INLINE bool CharIsLower(Char8 c) { return c > 96 && c < 123; }
LF_FORCE_INLINE bool CharIsHex(Char8 c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }

// Char16 interface
// Compares to make sure two strings are exactly equal
LF_CORE_API bool StrEqual(const Char16* beginA, const Char16* beginB, const Char16* endA, const Char16* endB);
// Compares to make sure two strings are not equal
LF_CORE_API bool StrNotEqual(const Char16* beginA, const Char16* beginB, const Char16* endA, const Char16* endB);
// Does alpha-less compare on 2 strings
LF_CORE_API bool StrAlphaLess(const Char16* a, const Char16* b);
// Does alpha-greater compare on 2 strings
LF_CORE_API bool StrAlphaGreater(const Char16* a, const Char16* b);
// Calculates the length of the string
LF_CORE_API SizeT StrLen(const Char16* a);
// Finds the index of the first matching substring of 'character'/'token' in the string
LF_CORE_API SizeT StrFind(const Char16* first, const Char16* last, Char16 character);
LF_CORE_API SizeT StrFind(const Char16* first, const Char16* last, const Char16* token);
LF_CORE_API SizeT StrFind(const Char16* first, const Char16* last, const Char16* tokenBegin, const Char16* tokenEnd);
// Finds the index of the first matching substring of 'character'/'token' in the string while being indifferent to lower/upper case. (english only)
LF_CORE_API SizeT StrFindAgnostic(const Char16* first, const Char16* last, Char16 character);
LF_CORE_API SizeT StrFindAgnostic(const Char16* first, const Char16* last, const Char16* token);
LF_CORE_API SizeT StrFindAgnostic(const Char16* first, const Char16* last, const Char16* tokenBegin, const Char16* tokenEnd);
// Finds the index of the last matching substring of 'character'/'token' in the string
LF_CORE_API SizeT StrFindLast(const Char16* first, const Char16* last, Char16 character);
LF_CORE_API SizeT StrFindLast(const Char16* first, const Char16* last, const Char16* token);
LF_CORE_API SizeT StrFindLast(const Char16* first, const Char16* last, const Char16* tokenBegin, const Char16* tokenEnd);
// Finds the index of the last matching substring of 'character'/'token' in the string while being indifferent to lower/upper case. (english only)
LF_CORE_API SizeT StrFindLastAgnostic(const Char16* first, const Char16* last, Char16 character);
LF_CORE_API SizeT StrFindLastAgnostic(const Char16* first, const Char16* last, const Char16* token);
LF_CORE_API SizeT StrFindLastAgnostic(const Char16* first, const Char16* last, const Char16* tokenBegin, const Char16* tokenEnd);

LF_CORE_API void ToLower(Char16* string);
LF_CORE_API void ToUpper(Char16* string);

} // namespace lf