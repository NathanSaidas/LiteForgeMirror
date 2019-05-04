// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
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