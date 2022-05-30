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

namespace TimeTypes
{
struct Microseconds
{
    Microseconds() = default;
    explicit Microseconds(Float32 value) : mValue(value) {}
    explicit Microseconds(Float64 value) : mValue(static_cast<Float32>(value)) {}

    Float32 mValue;
};

struct Milliseconds
{
    Milliseconds() = default;
    explicit Milliseconds(Float32 value) : mValue(value) {}
    explicit Milliseconds(Float64 value) : mValue(static_cast<Float32>(value)) {}

    Float32 mValue;
};

struct Seconds
{
    Seconds() = default;
    explicit Seconds(Float32 value) : mValue(value) {}
    explicit Seconds(Float64 value) : mValue(static_cast<Float32>(value)) {}

    Float32 mValue;
};

}

LF_INLINE TimeTypes::Microseconds ToMicroseconds(TimeTypes::Microseconds value);
LF_INLINE TimeTypes::Microseconds ToMicroseconds(TimeTypes::Milliseconds value);
LF_INLINE TimeTypes::Microseconds ToMicroseconds(TimeTypes::Seconds value);
LF_INLINE TimeTypes::Milliseconds ToMilliseconds(TimeTypes::Microseconds value);
LF_INLINE TimeTypes::Milliseconds ToMilliseconds(TimeTypes::Milliseconds value);
LF_INLINE TimeTypes::Milliseconds ToMilliseconds(TimeTypes::Seconds value);
LF_INLINE TimeTypes::Seconds ToSeconds(TimeTypes::Microseconds value);
LF_INLINE TimeTypes::Seconds ToSeconds(TimeTypes::Milliseconds value);
LF_INLINE TimeTypes::Seconds ToSeconds(TimeTypes::Seconds value);


LF_CORE_API Int64 GetClockFrequency();
LF_CORE_API Int64 GetClockTime();

LF_INLINE Float64 FormatTime(Float64 time)
{

    if (time < 0.001)
    {
        return time * 1000000.0;
    }
    else if (time < 1.0)
    {
        return time * 1000.0;
    }
    return time;
}
LF_INLINE const char* FormatTimeStr(Float64 time)
{
    if (time < 0.001)
    {
        return "us";
    }
    else if (time < 1.0)
    {
        return "ms";
    }
    return "s";
}

class LF_CORE_API Timer
{
public:
    Timer()
    : mBegin(0)
    , mEnd(0)
    {
    }

    void Start()
    {
        mBegin = GetClockTime();
        mEnd = mBegin + 1;
    }

    void Stop()
    {
        mEnd = GetClockTime();
    }

    Float64 GetDelta() const
    {
        return AbsTime(mEnd - mBegin) / static_cast<Float64>(sFrequency);
    }

    Float64 PeekDelta() const
    {
        return AbsTime(GetClockTime() - mBegin) / static_cast<Float64>(sFrequency);
    }

    bool IsRunning() const { return mBegin != mEnd; }

private:
    static Int64 AbsTime(Int64 time) { return time < 0 ? -time : time; }

    static const Int64 sFrequency;
    Int64 mBegin;
    Int64 mEnd;
};


TimeTypes::Microseconds ToMicroseconds(TimeTypes::Microseconds value)
{
    return value;
}
TimeTypes::Microseconds ToMicroseconds(TimeTypes::Milliseconds value)
{
    return TimeTypes::Microseconds(value.mValue * 1000.0f);
}
TimeTypes::Microseconds ToMicroseconds(TimeTypes::Seconds value)
{
    return TimeTypes::Microseconds(value.mValue * 1000000.0f);
}
TimeTypes::Milliseconds ToMilliseconds(TimeTypes::Microseconds value)
{
    return TimeTypes::Milliseconds(value.mValue / 1000.0f);
}
TimeTypes::Milliseconds ToMilliseconds(TimeTypes::Milliseconds value)
{
    return value;
}
TimeTypes::Milliseconds ToMilliseconds(TimeTypes::Seconds value)
{
    return TimeTypes::Milliseconds(value.mValue * 1000.0f);
}
TimeTypes::Seconds ToSeconds(TimeTypes::Microseconds value)
{
    return TimeTypes::Seconds(value.mValue / 1000000.0f);
}
TimeTypes::Seconds ToSeconds(TimeTypes::Milliseconds value)
{
    return TimeTypes::Seconds(value.mValue / 1000.0f);
}
TimeTypes::Seconds ToSeconds(TimeTypes::Seconds value)
{
    return value;
}

}