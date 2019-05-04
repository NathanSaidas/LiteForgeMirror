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
#ifndef LF_CORE_STREAM_TYPES_H
#define LF_CORE_STREAM_TYPES_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

// Helper type used to specify the stream to fill from the right with a certain width.
struct LF_CORE_API StreamFillRight
{
    StreamFillRight() :
        mWidth(0)
    {}
    StreamFillRight(SizeT width) :
        mWidth(width)
    {}
    SizeT mWidth;
};
// Helper type used to specify the stream to fill from the left with a certain width
struct LF_CORE_API StreamFillLeft
{
    StreamFillLeft() :
        mWidth(0)
    {}
    StreamFillLeft(SizeT width) :
        mWidth(width)
    {}
    SizeT mWidth;
};
// Helper type used to specify the stream to fill with a certain character.
struct LF_CORE_API StreamFillChar
{
    StreamFillChar() :
        mChar(' ')
    {}
    StreamFillChar(char character) :
        mChar(character)
    {}
    char mChar;
};
// Helper type used to specify the precision to use for logging numerical values.
struct LF_CORE_API StreamPrecision
{
    StreamPrecision() :
        mValue(5)
    {}
    StreamPrecision(SizeT value) :
        mValue(value)
    {}
    SizeT mValue;
};
// Helper type used to specify OPTION_BOOL_ALPHA stream option.
struct LF_CORE_API StreamBoolAlpha
{
    StreamBoolAlpha() :
        mValue(true)
    {}
    StreamBoolAlpha(bool value) :
        mValue(value)
    {}
    bool mValue;
};
// Helper type used to specify OPTION_CHAR_ALPHA stream option
struct LF_CORE_API StreamCharAlpha
{
    StreamCharAlpha() :
        mValue(false)
    {}
    StreamCharAlpha(bool value) :
        mValue(value)
    {}
    bool mValue;
};

} // namespace lf

#endif // LF_CORE_STREAM_TYPES_H