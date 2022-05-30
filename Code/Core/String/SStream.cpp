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
#include "SStream.h"
#include "Core/String/Token.h"
#include "Core/String/StringCommon.h"
#include <utility>

namespace lf {

SStream::SStream() : 
mBuffer(),
mPrecision(5),
mFillAmount(0),
mOptions(OPTION_BOOL_ALPHA),
mFillMode(FM_LEFT),
mFillChar(' ')
{}
SStream::SStream(const SStream& other) :
mBuffer(other.mBuffer),
mPrecision(other.mPrecision),
mFillAmount(other.mFillAmount),
mOptions(other.mOptions),
mFillMode(other.mFillMode),
mFillChar(other.mFillChar)
{

}
SStream::SStream(SStream&& other) :
mBuffer(std::move(other.mBuffer)),
mPrecision(other.mPrecision),
mFillAmount(other.mFillAmount),
mOptions(other.mOptions),
mFillMode(other.mFillMode),
mFillChar(other.mFillChar)
{
    other.mPrecision    = 5;
    other.mFillAmount   = 0;
    other.mOptions      = OPTION_BOOL_ALPHA;
    other.mFillMode     = FM_LEFT;
    other.mFillChar     = ' ';
}
SStream::~SStream()
{
}

SStream& SStream::operator=(const SStream& other)
{
    mBuffer     = other.mBuffer;
    mPrecision  = other.mPrecision;
    mFillAmount = other.mFillAmount;
    mOptions    = other.mOptions;
    mFillMode   = other.mFillMode;
    mFillChar   = other.mFillChar;
    return *this;
}
SStream& SStream::operator=(SStream&& other)
{
    mBuffer     = std::move(other.mBuffer);
    mPrecision  = other.mPrecision;         other.mPrecision = 5;
    mFillAmount = other.mFillAmount;        other.mFillAmount = 0;
    mOptions    = other.mOptions;           other.mOptions = OPTION_BOOL_ALPHA;
    mFillMode   = other.mFillMode;          other.mFillMode = FM_LEFT;
    mFillChar   = other.mFillChar;          other.mFillChar = ' ';
    return *this;
}

SStream& SStream::operator<<(bool value)
{
    if ((mOptions & OPTION_BOOL_ALPHA) > 0)
    {
        if (value)
        {
            WriteCommon("true", 4);
        }
        else
        {
            WriteCommon("false", 5);
        }
    }
    else
    {
        WriteCommon(value ? "1" : "0", 1);
    }
    return *this;
}
SStream& SStream::operator<<(Int8 value)
{
    if ((mOptions & OPTION_CHAR_ALPHA) > 0)
    {
        char content[] = { value };
        WriteCommon(content, 1);
    }
    else
    {
        char buffer[5] = { 0 };
        sprintf_s(buffer, LF_ARRAY_SIZE(buffer), "%d", value);
        WriteCommon(buffer, StrLen(buffer));
    }
    return *this;
}
SStream& SStream::operator<<(Int16 value)
{
    char buffer[STR_INT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, LF_ARRAY_SIZE(buffer), "%d", value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(Int32 value)
{
    char buffer[STR_INT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, LF_ARRAY_SIZE(buffer), "%d", value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(Int64 value)
{
    char buffer[STR_INT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, LF_ARRAY_SIZE(buffer), "%lld", value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(UInt8 value)
{
    char buffer[STR_INT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, LF_ARRAY_SIZE(buffer), "%u", value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(UInt16 value)
{
    char buffer[STR_INT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, LF_ARRAY_SIZE(buffer), "%u", value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(UInt32 value)
{
    char buffer[STR_INT32_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, LF_ARRAY_SIZE(buffer), "%u", value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(UInt64 value)
{
    char buffer[STR_INT64_MAX_LENGTH] = { 0 };
    sprintf_s(buffer, LF_ARRAY_SIZE(buffer), "%llu", value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(Float32 value)
{
    static const char PRECISION_TABLE[] = { '0', '1', '2', '3','4','5','6','7','8','9' };

    char formatSingle[] = "%.5f";
    char formatDouble[] = "%.15f";
    char* formatString = nullptr;
    SizeT precision = mPrecision > 20 ? 20 : mPrecision;
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
    sprintf_s(buffer, STR_FLOAT32_MAX_LENGTH, formatString, value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(Float64 value)
{
    static const char PRECISION_TABLE[] = { '0', '1', '2', '3','4','5','6','7','8','9' };

    char formatSingle[] = "%.5f";
    char formatDouble[] = "%.15f";
    char* formatString = nullptr;
    SizeT precision = mPrecision > 20 ? 20 : mPrecision;
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
    sprintf_s(buffer, STR_FLOAT32_MAX_LENGTH, formatString, value);
    WriteCommon(buffer, StrLen(buffer));
    return *this;
}
SStream& SStream::operator<<(const char* value)
{
    WriteCommon(value, StrLen(value));
    return *this;
}
SStream& SStream::operator<<(const String& value)
{
    WriteCommon(value.CStr(), value.Size());
    return *this;
}
SStream& SStream::operator<<(const Token& value)
{
    WriteCommon(value.CStr(), value.Size());
    return *this;
}

SStream& SStream::operator<<(const StreamFillRight& fill)
{
    mFillMode = FM_RIGHT;
    mFillAmount = static_cast<UInt16>(fill.mWidth);
    return *this;
}
SStream& SStream::operator<<(const StreamFillLeft& fill)
{
    mFillMode = FM_LEFT;
    mFillAmount = static_cast<UInt16>(fill.mWidth);
    return *this;
}
SStream& SStream::operator<<(const StreamFillChar& fill)
{
    mFillChar = fill.mChar;
    return *this;
}
SStream& SStream::operator<<(const StreamPrecision& precision)
{
    mPrecision = static_cast<UInt16>(precision.mValue);
    return *this;
}
SStream& SStream::operator<<(const StreamBoolAlpha& option)
{
    if (option.mValue)
    {
        mOptions = static_cast<Options>(static_cast<UInt32>(mOptions) | static_cast<UInt32>(OPTION_BOOL_ALPHA));
    }
    else
    {
        mOptions = static_cast<Options>(static_cast<UInt32>(mOptions) & ~static_cast<UInt32>(OPTION_BOOL_ALPHA));
    }
    return *this;
}
SStream& SStream::operator<<(const StreamCharAlpha& option)
{
    if (option.mValue)
    {
        mOptions = static_cast<Options>(static_cast<UInt32>(mOptions) | static_cast<UInt32>(OPTION_CHAR_ALPHA));
    }
    else
    {
        mOptions = static_cast<Options>(static_cast<UInt32>(mOptions) & ~static_cast<UInt32>(OPTION_CHAR_ALPHA));
    }
    return *this;
}

void SStream::Clear(bool resetOptions)
{
    mBuffer.Clear();
    if (resetOptions)
    {
        mPrecision = 5;
        mFillAmount = 0;
        mOptions = OPTION_BOOL_ALPHA;
        mFillMode = FM_LEFT;
        mFillChar = ' ';
    }
}
void SStream::Reserve(SizeT amount)
{
    mBuffer.Reserve(amount);
}

SStream::State SStream::Push(bool preserve)
{
    State state;
    state.mPrecision = mPrecision;
    state.mFillAmount = mFillAmount;
    state.mOptions = mOptions;
    state.mFillMode = mFillMode;
    state.mFillChar = mFillChar;
    if (!preserve)
    {
        mPrecision = 5;
        mFillAmount = 0;
        mOptions = OPTION_BOOL_ALPHA;
        mFillMode = FM_LEFT;
        mFillChar = ' ';
    }
    return state;
}

void SStream::Pop(const State& state)
{
    mPrecision = state.mPrecision;
    mFillAmount = state.mFillAmount;
    mOptions = state.mOptions;
    mFillMode = state.mFillMode;
    mFillChar = state.mFillChar;
}

void SStream::WriteCommon(const char* content, SizeT length)
{
    SizeT fillAmount = length >= mFillAmount ? 0 : mFillAmount - length;
    if (fillAmount == 0)
    {
        mBuffer.Append(content);
        return;
    }
    // "    Fill-Content"
    if (mFillMode == FM_RIGHT)
    {
        for (SizeT i = 0; i < fillAmount; ++i)
        {
            mBuffer.Append(mFillChar);
        }
    }
    mBuffer.Append(content);
    // "Fill-Content    "
    if (mFillMode == FM_LEFT)
    {
        for (SizeT i = 0; i < fillAmount; ++i)
        {
            mBuffer.Append(mFillChar);
        }
    }
}

} // namespace lf