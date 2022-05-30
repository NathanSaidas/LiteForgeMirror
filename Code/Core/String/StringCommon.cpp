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
#include "Core/PCH.h"
#include "StringCommon.h"

#include "Core/Math/MathCombined.h"
#include "Core/Math/Color.h"
namespace lf {

String StrStripWhitespace(const String& string, bool ignoreQuotes)
{
    const UInt8 STORAGE_FLAG = 1 << 7;
    const Char8 SPACE = ' ';
    const Char8 TAB = '\t';
    const Char8 QUOTE = '\"';
    const Char8 SLASH = '\\';

    String storage;
    if (string.Empty())
    {
        return storage;
    }

    storage.Resize(string.Size());
    StringStorage& outStorage = reinterpret_cast<StringStorage&>(storage);

    Char8* outBuffer = const_cast<Char8*>(storage.CStr());
    const Char8* inBuffer = string.CStr();
    bool useHeap = storage.UseHeap();
    SizeT size = string.Size();
    char* begin = outBuffer;
    bool inQuote = false;
    bool inSlash = false;
    for (SizeT i = 0; i < size; ++i)
    {
        Char8 c = inBuffer[i];
        if (ignoreQuotes)
        {
            if (!inSlash)
            {
                if (c == QUOTE)
                {
                    inQuote = !inQuote;
                }
            }
            if (inQuote)
            {
                if (c == SLASH)
                {
                    inSlash = !inSlash;
                }
                *outBuffer = c;
                ++outBuffer;
            }
            else
            {
                inSlash = false;
                if (c != SPACE && c != TAB)
                {
                    *outBuffer = c;
                    ++outBuffer;
                }
            }
        }
        else
        {
            if (c != SPACE && c != TAB)
            {
                *outBuffer = c;
                ++outBuffer;
            }
        }
    }
    *outBuffer = 0;

    SizeT outSize = outBuffer - begin;
    if (useHeap)
    {
        outStorage.heap.last = outBuffer;
    }
    else
    {
        const UInt8 LOCAL_STORAGE_MINUS_ONE = sizeof(StringStorage) - 1;
        const UInt8 FLAG_MASK = 0xC0;
        (reinterpret_cast<UInt8&>(outStorage.local.buffer[LOCAL_STORAGE_MINUS_ONE]) &= FLAG_MASK) |= outSize;
    }
    return storage;
}
SizeT StrSplit(const String& s, Char8 token, TVector<String>& output)
{
    if (s.Empty())
    {
        return 0;
    }

    SizeT sizeBefore = output.size();
    SizeT firstIndex = 0;
    SizeT lastIndex = 0;
    bool ignoringDelimiter = false;
    const String::value_type* buffer = s.CStr();
    SizeT size = s.Size();
    for (SizeT i = 0; i < size; ++i)
    {
        if (buffer[i] == token && !ignoringDelimiter)
        {
            if (firstIndex == lastIndex)
            {
                ++firstIndex;
                lastIndex = firstIndex;
                continue;
            }
            output.push_back(s.SubString(firstIndex, (lastIndex - firstIndex)));
            firstIndex = i + 1;
            lastIndex = firstIndex;
        }
        else
        {
            ++lastIndex;
        }
    }
    if (firstIndex != lastIndex)
    {
        output.push_back(s.SubString(firstIndex, (lastIndex - firstIndex) + 1));
    }
    return output.size() - sizeBefore;;
}

SizeT StrSplit(const String& s, Char8 token, String* inOutArray, SizeT arraySize)
{
    if (s.Empty() || arraySize == 0)
    {
        return 0;
    }

    SizeT arrayIndex = 0;
    SizeT firstIndex = 0;
    SizeT lastIndex = 0;
    bool ignoringDelimiter = false;
    const String::value_type* buffer = s.CStr();
    SizeT size = s.Size();
    for (SizeT i = 0; i < size; ++i)
    {
        if (buffer[i] == token && !ignoringDelimiter)
        {
            if (firstIndex == lastIndex)
            {
                ++firstIndex;
                lastIndex = firstIndex;
                continue;
            }
            inOutArray[arrayIndex++] = s.SubString(firstIndex, (lastIndex - firstIndex));
            if (arrayIndex >= arraySize)
            {
                return arrayIndex;
            }
            firstIndex = i + 1;
            lastIndex = firstIndex;
        }
        else
        {
            ++lastIndex;
        }
    }
    if (firstIndex != lastIndex)
    {
        inOutArray[arrayIndex++] = s.SubString(firstIndex, (lastIndex - firstIndex) + 1);
    }
    return arrayIndex;
}

void StrParseExtension(const String& str, String& outExtension)
{
    SizeT ext = str.FindLast('.');
    if (Valid(ext))
    {
        str.SubString(ext + 1, outExtension);
    }
}
WString StrConvert(const String& str)
{
    // If byte > 1111 0000 (xF0, 240) then 4 bytes in character
    // If byte > 1110 0000 (xE0, 224) then 3 bytes in character
    // If byte > 1100 0000 (xC0, 192) then 2 bytes in character
    // If byte > 1000 0000 (x80, 128) then error
    // else 0 bytes
    const Char16 CP_4 = 0xF0;
    const Char16 CP_3 = 0xE0;
    const Char16 CP_2 = 0xC0;
    const Char16 CP_INVALID = 0x80;

    WString s;
    s.Reserve(str.Size());
    const Char8* c = str.CStr();
    UInt32 n = 0;
    UInt32 i = 0;
    Char16 reserved[2];
    for (; *c != 0; ++c)
    {
        Char16 character = static_cast<Char16>(*c);
        if (n == 0)
        {
            if (character > CP_4)
            {
                CriticalAssertMsgEx("Not implemented yet.", LF_ERROR_INTERNAL, ERROR_API_CORE);
                //n = 4;
                //i = 0;
            }
            else if (character > CP_3)
            {
                CriticalAssertMsgEx("Not implemented yet.", LF_ERROR_INTERNAL, ERROR_API_CORE);
                //n = 3;
                //i = 0;
            }
            else if (character > CP_2)
            {
                n = 2;
                i = 0;
            }
            else if (character > CP_INVALID)
            {
                CriticalAssertMsgEx("Invalid code point parsing ", LF_ERROR_INTERNAL, ERROR_API_CORE);
            }
            else // Just add this single byte..
            {
                Char16 wc = Char16(character);
                s.Append(static_cast<Char16>(wc));
            }
        }
        else
        {
            if (i < LF_ARRAY_SIZE(reserved))
            {
                reserved[i] = character;
                ++i;
                if (i == n)
                {
                    Char16 wc = (Char16(reserved[0]) << 8) | Char16(reserved[1]);
                    s.Append(static_cast<Char16>(wc));
                    n = 0;
                }
            }
        }

    }
    return s;
}
String StrConvert(const WString& str)
{
    // If byte > 1111 0000 (xF0, 240) then 4 bytes in character
    // If byte > 1110 0000 (xE0, 224) then 3 bytes in character
    // If byte > 1100 0000 (xC0, 192) then 2 bytes in character
    // If byte > 1000 0000 (x80, 128) then error
    // else 0 bytes

    //const Char16 CP_4 = 0xF1;
    //const Char16 CP_3 = 0xE1;
    const unsigned char CP_2 = 0xC1;
    //const Char16 CP_INVALID = 0x80;

    String s;
    s.Reserve(str.Size() * 2);
    const Char16* c = str.CStr();
    for (; *c != 0; ++c)
    {
        Char16 character = *c;
        Char8 a = 0, b = 0;
        a = static_cast<Char8>(character & 0xFF);
        b = static_cast<Char8>((character >> 8) & 0xFF);
        if (b == 0)
        {
            s.Append(a);
        }
        else
        {
            s.Append(CP_2);
            s.Append(a);
            s.Append(b);
        }
    }
    return s;
}
String StrFormatAlignLeft(const String& str, SizeT length, Char8 fill)
{
    String result;
    if (length == 0)
    {
        return result;
    }

    if (str.Size() > length)
    {
        str.SubString(0, length, result);
        return result;
    }
    result = str;
    result.Resize(length, fill);
    return result;
}
String StrFormatAlignRight(const String& str, SizeT length, Char8 fill)
{
    String result;
    if (length == 0)
    {
        return result;
    }

    if (str.Size() > length)
    {
        str.SubString(str.Size() - length, result);
        return result;
    }
    SizeT numFill = length - str.Size();
    result.Resize(numFill, fill);
    result.Append(str);
    return result;
}

bool StrIsNumber(const Char8* string)
{
    // Examples:
    // 3
    // -3
    // 3.4123231
    // -3.443132
    // -.5 
    // -0.5
    // -5.

    // Numbers do not have whitespace
    // Numbers can have 0 or 1 -
    // Numbers must have - as first character
    // Numbers can have 0 or 1 .

    bool hasMinus = false;
    bool hasDot = false;
    bool numberDetected = false;

    while (*string)
    {
        if (*string == '-')
        {
            if (!hasMinus && !numberDetected && !hasDot)
            {
                hasMinus = true;
            }
            else
            {
                return false;
            }
        }
        else if (*string == '.')
        {
            if (hasDot)
            {
                return false;
            }
            hasDot = true;
        }
        else if (!std::isdigit(*string))
        {
            return false;
        }
        else
        {
            numberDetected = true;
        }
        ++string;
    }
    return numberDetected;
}

String StrToLower(const String& string)
{
    String result(string.CStr());
    ToLower(const_cast<Char8*>(result.CStr()));
    return result;
}
String StrToUpper(const String& string)
{
    String result(string.CStr());
    ToUpper(const_cast<Char8*>(result.CStr()));
    return result;
}
bool StrCompareAgnostic(const String& a, const String& b)
{
    return _stricmp(a.CStr(), b.CStr()) == 0;
}

String StrTrimRight(const String& string)
{
    if (string.Empty() || (string.Last() != ' ' && string.Last() != '\t'))
    {
        return string;
    }

    SizeT i = string.Size() - 1;
    for (; i > 0; ++i)
    {
        if (string[i] != ' ' && string[i] != '\t')
        {
            return string.SubString(0, i - 1);
        }
    }
    return String();
}

LF_CORE_API String BytesToHex(const ByteT* bytes, SizeT length)
{
    String buf;
    buf.Reserve(length * 2);
    for (SizeT i = 0; i < length; ++i)
    {
        char hex0 = ByteToHex((bytes[i] >> 4) & 0x0F);
        char hex1 = ByteToHex(bytes[i] & 0x0F);
        buf.Append(hex0);
        buf.Append(hex1);
    }

    return buf;
}

SizeT ToVector2(const String& str, Vector2& out)
{
    String numbers[2];
    SizeT numbersSize = StrSplit(str, ',', numbers, LF_ARRAY_SIZE(numbers));

    if (numbersSize == 2)
    {
        out.x = ToFloat32(numbers[0]);
        out.y = ToFloat32(numbers[1]);
    }
    else if (numbersSize == 1)
    {
        out.x = ToFloat32(numbers[0]);
    }
    return numbersSize;
}
SizeT ToVector3(const String& str, Vector3& out)
{
    String numbers[3];
    SizeT numbersSize = StrSplit(str, ',', numbers, LF_ARRAY_SIZE(numbers));

    if (numbersSize == 3)
    {
        out.x = ToFloat32(numbers[0]);
        out.y = ToFloat32(numbers[1]);
        out.z = ToFloat32(numbers[2]);
    }
    else if (numbersSize == 2)
    {
        out.x = ToFloat32(numbers[0]);
        out.y = ToFloat32(numbers[1]);
    }
    else if (numbersSize == 1)
    {
        out.x = ToFloat32(numbers[0]);
    }
    return numbersSize;
}
SizeT ToVector4(const String& str, Vector4& out)
{
    String numbers[4];
    SizeT numbersSize = StrSplit(str, ',', numbers, LF_ARRAY_SIZE(numbers));

    if (numbersSize == 4)
    {
        out.x = ToFloat32(numbers[0]);
        out.y = ToFloat32(numbers[1]);
        out.z = ToFloat32(numbers[2]);
        out.w = ToFloat32(numbers[3]);
    }
    else if (numbersSize == 3)
    {
        out.x = ToFloat32(numbers[0]);
        out.y = ToFloat32(numbers[1]);
        out.z = ToFloat32(numbers[2]);
    }
    else if (numbersSize == 2)
    {
        out.x = ToFloat32(numbers[0]);
        out.y = ToFloat32(numbers[1]);
    }
    else if (numbersSize == 1)
    {
        out.x = ToFloat32(numbers[0]);
    }
    return numbersSize;
}
SizeT ToColor(const String& str, Color& out)
{
    String numbers[4];
    SizeT numbersSize = StrSplit(str, ',', numbers, LF_ARRAY_SIZE(numbers));

    if (numbersSize == 4)
    {
        out.r = ToFloat32(numbers[0]);
        out.g = ToFloat32(numbers[1]);
        out.b = ToFloat32(numbers[2]);
        out.a = ToFloat32(numbers[3]);
    }
    else if (numbersSize == 3)
    {
        out.r = ToFloat32(numbers[0]);
        out.g = ToFloat32(numbers[1]);
        out.b = ToFloat32(numbers[2]);
    }
    else if (numbersSize == 2)
    {
        out.r = ToFloat32(numbers[0]);
        out.g = ToFloat32(numbers[1]);
    }
    else if (numbersSize == 1)
    {
        out.r = ToFloat32(numbers[0]);
    }
    return numbersSize;
}
String ToString(const Vector2& value)
{
    const SizeT reserve = (STR_FLOAT32_MAX_LENGTH) * 2;
    String result;
    result.Reserve(reserve);
    ToStringAppend(value.x, result);
    result.Append(',');
    ToStringAppend(value.y, result);
    return result;
}
String ToString(const Vector3& value)
{
    const SizeT reserve = (STR_FLOAT32_MAX_LENGTH) * 3;
    String result;
    result.Reserve(reserve);
    ToStringAppend(value.x, result);
    result.Append(',');
    ToStringAppend(value.y, result);
    result.Append(',');
    ToStringAppend(value.z, result);
    return result;
}
String ToString(const Vector4& value)
{
    const SizeT reserve = (STR_FLOAT32_MAX_LENGTH) * 4;
    String result;
    result.Reserve(reserve);
    ToStringAppend(value.x, result);
    result.Append(',');
    ToStringAppend(value.y, result);
    result.Append(',');
    ToStringAppend(value.z, result);
    result.Append(',');
    ToStringAppend(value.w, result);
    return result;
}

String ToString(const Vector2& value, SizeT precision)
{
    const SizeT reserve = (STR_FLOAT32_MAX_LENGTH) * 2;
    String result;
    result.Reserve(reserve);
    ToStringAppend(value.x, precision, result);
    result.Append(',');
    ToStringAppend(value.y, precision, result);
    return result;
}
String ToString(const Vector3& value, SizeT precision)
{
    const SizeT reserve = (STR_FLOAT32_MAX_LENGTH) * 3;
    String result;
    result.Reserve(reserve);
    ToStringAppend(value.x, precision, result);
    result.Append(',');
    ToStringAppend(value.y, precision, result);
    result.Append(',');
    ToStringAppend(value.z, precision, result);
    return result;
}
String ToString(const Vector4& value, SizeT precision)
{
    const SizeT reserve = (STR_FLOAT32_MAX_LENGTH) * 4;
    String result;
    result.Reserve(reserve);
    ToStringAppend(value.x, precision, result);
    result.Append(',');
    ToStringAppend(value.y, precision, result);
    result.Append(',');
    ToStringAppend(value.z, precision, result);
    result.Append(',');
    ToStringAppend(value.w, precision, result);
    return result;
}
String ToString(const Color& value)
{
    const SizeT reserve = (STR_FLOAT32_MAX_LENGTH) * 4;
    String result;
    result.Reserve(reserve);
    ToStringAppend(value.r, result);
    result.Append(',');
    ToStringAppend(value.g, result);
    result.Append(',');
    ToStringAppend(value.b, result);
    result.Append(',');
    ToStringAppend(value.a, result);
    return result;
}
String ToString(const Color& value, SizeT precision)
{
    const SizeT reserve = (STR_FLOAT32_MAX_LENGTH) * 4;
    String result;
    result.Reserve(reserve);
    ToStringAppend(value.r, precision, result);
    result.Append(',');
    ToStringAppend(value.g, precision, result);
    result.Append(',');
    ToStringAppend(value.b, precision, result);
    result.Append(',');
    ToStringAppend(value.a, precision, result);
    return result;
}

} // namespace lf