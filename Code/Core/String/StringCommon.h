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
#ifndef LF_CORE_STRING_COMMON_H
#define LF_CORE_STRING_COMMON_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/String/String.h"
#include "Core/String/WString.h"
#include "Core/Utility/Array.h"

#include <cctype>



namespace lf {

class Vector2;
class Vector3;
class Vector4;
class Color;

const SizeT STR_INT32_MAX_LENGTH = 13;
const SizeT STR_UINT32_MAX_LENGTH = 12;
const SizeT STR_INT64_MAX_LENGTH = 21;
const SizeT STR_UINT64_MAX_LENGTH = 21;
const SizeT STR_FLOAT32_MAX_LENGTH = FLT_MAX_10_EXP + 2;
const SizeT STR_FLOAT64_MAX_LENGTH = DBL_MAX_10_EXP + 2;

const SizeT STR_HEX_32_MAX_LENGTH = 10;
const SizeT STR_HEX_64_MAX_LENGTH = 18;

LF_CORE_API String  StrStripWhitespace(const String& string, bool ignoreQuotes = false);
LF_CORE_API SizeT   StrSplit(const String& s, Char8 token, TArray<String>& output);
LF_CORE_API SizeT   StrSplit(const String& s, Char8 token, String* inOutArray, SizeT arraySize);
LF_CORE_API void    StrParseExtension(const String& str, String& outExtension);
LF_CORE_API WString StrConvert(const String& str);
LF_CORE_API String  StrConvert(const WString& str);
LF_CORE_API String  StrFormatAlignLeft(const String& str, SizeT length, Char8 fill = ' ');
LF_CORE_API String  StrFormatAlignRight(const String& str, SizeT length, Char8 fill = ' ');
LF_CORE_API bool    StrIsNumber(const Char8* string);
LF_INLINE   bool    StrIsNumber(const String& string);
LF_CORE_API String  StrToLower(const String& string);
LF_CORE_API String  StrToUpper(const String& string);
// Trim trailing whitespace
LF_CORE_API String  StrTrimRight(const String& string);

LF_INLINE ByteT HexToByte(Char8 c);
LF_INLINE Char8 ByteToHex(ByteT byte);
LF_CORE_API String BytesToHex(const ByteT* bytes, SizeT length);

LF_INLINE String ToString(Int32 number);
LF_INLINE String ToString(UInt32 number);
LF_INLINE String ToString(Int64 number);
LF_INLINE String ToString(UInt64 number);
LF_INLINE String ToString(Float32 number);
LF_INLINE String ToString(Float32 number, SizeT precision);
LF_INLINE String ToString(Float64 number);
LF_INLINE String ToString(Float64 number, SizeT precision);
LF_INLINE void   ToStringAppend(Int32 number, String& output);
LF_INLINE void   ToStringAppend(UInt32 number, String& output);
LF_INLINE void   ToStringAppend(Int64 number, String& output);
LF_INLINE void   ToStringAppend(UInt64 number, String& output);
LF_INLINE void   ToStringAppend(Float32 number, String& output);
LF_INLINE void   ToStringAppend(Float32 number, SizeT precision, String& output);
LF_INLINE void   ToStringAppend(Float64 number, String& output);
LF_INLINE void   ToStringAppend(Float64 number, SizeT precision, String& output);

LF_INLINE String ToHexString(UInt32 number, bool upper = true);
LF_INLINE String ToHexString(Int32 number, bool upper = true);
LF_INLINE String ToHexString(UInt64 number, bool upper = true);
LF_INLINE String ToHexString(Int64 number, bool upper = true);
LF_INLINE void   ToHexStringAppend(UInt32 number, String& output, bool upper = true);
LF_INLINE void   ToHexStringAppend(Int32 number, String& output, bool upper = true);
LF_INLINE void   ToHexStringAppend(UInt64 number, String& output, bool upper = true);
LF_INLINE void   ToHexStringAppend(Int64 number, String& output, bool upper = true);

LF_INLINE UInt32  ToUInt32(const String& str, bool hex = false);
LF_INLINE Int32   ToInt32(const String& str, bool hex = false);
LF_INLINE UInt64  ToUInt64(const String& str, bool hex = false);
LF_INLINE Int64   ToInt64(const String& str, bool hex = false);
LF_INLINE Float32 ToFloat32(const String& str);
LF_INLINE Float64 ToFloat64(const String& str);

LF_CORE_API SizeT ToVector2(const String& str, Vector2& out);
LF_CORE_API SizeT ToVector3(const String& str, Vector3& out);
LF_CORE_API SizeT ToVector4(const String& str, Vector4& out);
LF_CORE_API SizeT ToColor(const String& str, Color& out);
LF_CORE_API String ToString(const Vector2& value);
LF_CORE_API String ToString(const Vector3& value);
LF_CORE_API String ToString(const Vector4& value);
LF_CORE_API String ToString(const Vector2& value, SizeT precision);
LF_CORE_API String ToString(const Vector3& value, SizeT precision);
LF_CORE_API String ToString(const Vector4& value, SizeT precision);
LF_CORE_API String ToString(const Color& value);
LF_CORE_API String ToString(const Color& value, SizeT precision);

bool StrIsNumber(const String& string)
{
    return StrIsNumber(string.CStr());
}

ByteT HexToByte(Char8 c)
{
    switch (c)
    {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'A':
        case 'a':
            return 10;
        case 'B':
        case 'b':
            return 11;
        case 'C':
        case 'c':
            return 12;
        case 'D':
        case 'd':
            return 13;
        case 'E':
        case 'e':
            return 14;
        case 'F':
        case 'f':
            return 15;
    }
    return INVALID8;
}

Char8 ByteToHex(ByteT byte)
{
    switch (byte)
    {
        case 0: return '0';
        case 1: return '1';
        case 2: return '2';
        case 3: return '3';
        case 4: return '4';
        case 5: return '5';
        case 6: return '6';
        case 7: return '7';
        case 8: return '8';
        case 9: return '9';
        case 10: return 'A';
        case 11: return 'B';
        case 12: return 'C';
        case 13: return 'D';
        case 14: return 'E';
        case 15: return 'F';
    }
    return '\0';
}

String ToString(Int32 number)
{
    char buffer[STR_INT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_INT32_MAX_LENGTH, "%d", number);
    return String(buffer);
}
String ToString(UInt32 number)
{
    char buffer[STR_UINT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_UINT32_MAX_LENGTH, "%u", number);
    return String(buffer);
}
String ToString(Int64 number)
{
    char buffer[STR_INT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_INT64_MAX_LENGTH, "%lld", number);
    return String(buffer);
}
String ToString(UInt64 number)
{
    char buffer[STR_UINT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_UINT64_MAX_LENGTH, "%llu", number);
    return String(buffer);
}
String ToString(Float32 number)
{
    char buffer[STR_FLOAT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_FLOAT32_MAX_LENGTH, "%f", number);
    return String(buffer);
}
String ToString(Float32 number, SizeT precision)
{
    static const char PRECISION_TABLE[] = { '0', '1', '2', '3','4','5','6','7','8','9' };

    char formatSingle[] = "%.5f";
    char formatDouble[] = "%.15f";
    char* formatString = nullptr;
    precision = precision > 20 ? 20 : precision;
    if (precision < 10)
    {
        formatString = formatSingle;
        formatString[2] = PRECISION_TABLE[precision];
    }
    else
    {
        formatString = formatDouble;
        formatString[2] = PRECISION_TABLE[precision / 10];
        formatString[3] = PRECISION_TABLE[precision % 10];
    }
    char buffer[STR_FLOAT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_FLOAT32_MAX_LENGTH, formatString, number);
    return String(buffer);
}
String ToString(Float64 number)
{
    char buffer[STR_FLOAT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_FLOAT64_MAX_LENGTH, "%f", number);
    return String(buffer);
}
String ToString(Float64 number, SizeT precision)
{
    static const char PRECISION_TABLE[] = { '0', '1', '2', '3','4','5','6','7','8','9' };

    char formatSingle[] = "%.5f";
    char formatDouble[] = "%.15f";
    char* formatString = nullptr;
    precision = precision > 20 ? 20 : precision;
    if (precision < 10)
    {
        formatString = formatSingle;
        formatString[2] = PRECISION_TABLE[precision];
    }
    else
    {
        formatString = formatDouble;
        formatString[2] = PRECISION_TABLE[precision / 10];
        formatString[3] = PRECISION_TABLE[precision % 10];
    }
    char buffer[STR_FLOAT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_FLOAT64_MAX_LENGTH, formatString, number);
    return String(buffer);
}

void ToStringAppend(Int32 number, String& output)
{
    char buffer[STR_INT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_INT32_MAX_LENGTH, "%d", number);
    output.Append(buffer);
}
void ToStringAppend(UInt32 number, String& output)
{
    char buffer[STR_UINT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_UINT32_MAX_LENGTH, "%u", number);
    output.Append(buffer);
}
void ToStringAppend(Int64 number, String& output)
{
    char buffer[STR_INT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_INT64_MAX_LENGTH, "%lld", number);
    output.Append(buffer);
}
void ToStringAppend(UInt64 number, String& output)
{
    char buffer[STR_UINT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_UINT64_MAX_LENGTH, "%llu", number);
    output.Append(buffer);
}
void ToStringAppend(Float32 number, String& output)
{
    char buffer[STR_FLOAT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_FLOAT32_MAX_LENGTH, "%f", number);
    output.Append(buffer);
}
void ToStringAppend(Float32 number, SizeT precision, String& output)
{
    static const char PRECISION_TABLE[] = { '0', '1', '2', '3','4','5','6','7','8','9' };

    char formatSingle[] = "%.5f";
    char formatDouble[] = "%.15f";
    char* formatString = nullptr;
    precision = precision > 20 ? 20 : precision;
    if (precision < 10)
    {
        formatString = formatSingle;
        formatString[2] = PRECISION_TABLE[precision];
    }
    else
    {
        formatString = formatDouble;
        formatString[2] = PRECISION_TABLE[precision / 10];
        formatString[3] = PRECISION_TABLE[precision % 10];
    }
    char buffer[STR_FLOAT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_FLOAT32_MAX_LENGTH, formatString, number);
    output.Append(buffer);
}

void ToStringAppend(Float64 number, String& output)
{
    char buffer[STR_FLOAT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_FLOAT64_MAX_LENGTH, "%f", number);
    output.Append(buffer);
}
void ToStringAppend(Float64 number, SizeT precision, String& output)
{
    static const char PRECISION_TABLE[] = { '0', '1', '2', '3','4','5','6','7','8','9' };

    char formatSingle[] = "%.5f";
    char formatDouble[] = "%.15f";
    char* formatString = nullptr;
    precision = precision > 20 ? 20 : precision;
    if (precision < 10)
    {
        formatString = formatSingle;
        formatString[2] = PRECISION_TABLE[precision];
    }
    else
    {
        formatString = formatDouble;
        formatString[2] = PRECISION_TABLE[precision / 10];
        formatString[3] = PRECISION_TABLE[precision % 10];
    }
    char buffer[STR_FLOAT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, STR_FLOAT64_MAX_LENGTH, formatString, number);
    output.Append(buffer);
}

String ToHexString(UInt32 number, bool upper)
{
    char buffer[STR_HEX_32_MAX_LENGTH] = { 0 };
    if (upper)
    {
        sprintf_s(buffer, STR_HEX_32_MAX_LENGTH, "%X", number);
    }
    else
    {
        sprintf_s(buffer, STR_HEX_32_MAX_LENGTH, "%x", number);
    }
    return String(buffer);
}
String ToHexString(Int32 number, bool upper)
{
    return ToHexString(*reinterpret_cast<UInt32*>(&number), upper);
}
String ToHexString(UInt64 number, bool upper)
{
    char buffer[STR_HEX_64_MAX_LENGTH] = { 0 };
    if (upper)
    {
        sprintf_s(buffer, STR_HEX_64_MAX_LENGTH, "%llX", number);
    }
    else
    {
        sprintf_s(buffer, STR_HEX_64_MAX_LENGTH, "%llx", number);
    }
    return String(buffer);
}
String ToHexString(Int64 number, bool upper)
{
    return ToHexString(*reinterpret_cast<UInt64*>(&number), upper);
}

void ToHexStringAppend(UInt32 number, String& output, bool upper)
{
    char buffer[STR_HEX_32_MAX_LENGTH] = { 0 };
    if (upper)
    {
        sprintf_s(buffer, STR_HEX_32_MAX_LENGTH, "%02X", number);
    }
    else
    {
        sprintf_s(buffer, STR_HEX_32_MAX_LENGTH, "%02x", number);

    }
    output.Append(buffer);
}
void ToHexStringAppend(Int32 number, String& output, bool upper)
{
    ToHexStringAppend(*reinterpret_cast<UInt32*>(&number), output, upper);
}
void ToHexStringAppend(UInt64 number, String& output, bool upper)
{
    char buffer[STR_HEX_64_MAX_LENGTH] = { 0 };
    if (upper)
    {
        sprintf_s(buffer, STR_HEX_64_MAX_LENGTH, "%llX", number);
    }
    else
    {
        sprintf_s(buffer, STR_HEX_64_MAX_LENGTH, "%llx", number);
    }
    output.Append(buffer);
}
void ToHexStringAppend(Int64 number, String& output, bool upper)
{
    ToHexStringAppend(*reinterpret_cast<UInt64*>(&number), output, upper);
}

UInt32 ToUInt32(const String& str, bool hex)
{
    char* dummy;
    return static_cast<UInt32>(strtoul(str.CStr(), &dummy, hex ? 16 : 10));
}
Int32 ToInt32(const String& str, bool hex)
{
    char* dummy;
    return static_cast<Int32>(strtol(str.CStr(), &dummy, hex ? 16 : 10));
}
UInt64 ToUInt64(const String& str, bool hex)
{
    char* dummy;
    return static_cast<UInt64>(strtoull(str.CStr(), &dummy, hex ? 16 : 10));
}
Int64 ToInt64(const String& str, bool hex)
{
    char* dummy;
    return static_cast<Int64>(strtoll(str.CStr(), &dummy, hex ? 16 : 10));
}
Float32 ToFloat32(const String& str)
{
    char* dummy;
    return static_cast<Float32>(strtof(str.CStr(), &dummy));
}
Float64 ToFloat64(const String& str)
{
    char* dummy;
    return static_cast<Float64>(strtod(str.CStr(), &dummy));
}


} // namespace lf

#endif // LF_CORE_STRING_COMMON_H