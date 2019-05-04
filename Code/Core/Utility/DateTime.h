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
#ifndef LF_CORE_DATE_TIME_H
#define LF_CORE_DATE_TIME_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

class String;

// This is just a wrapper around the value. There is no real Runtime benefit to the encoded
// value since they are both the same however different serialization interfaces may benefit 
// from the 2 byte savings we can achieve with an encoded value.
struct DateTimeEncoded
{
    LF_FORCE_INLINE DateTimeEncoded() : value(0) { }
    UInt64 value;
};

// A utility class that provides a way of storing/parsing and retrieving current time in UTC
// to the second. 
// 
// All inputs are clamped to their min/max value so if there is an invalid date when parsing it will be clamped to its
// max possible value.. Eg month=18 will clamp to December (12)
class LF_CORE_API DateTime
{
public:
    // Default Constructor: Initializes to 0
    DateTime();
    // Initializes to whatever the time is currently. UTC
    explicit DateTime(bool now);
    // Initializes based on an encoded value
    explicit DateTime(DateTimeEncoded data);
    // Initializes from a formatted string, acceptable formats are. If the format is not correct initializes to default DateTime
    //   dd/mm/yyyy           23/06/1766 -> June 23rd Year 1766
    //   dd/mm/yyyy ss:mm:hh  23/06/2046 46:06:03 -> June 23rd Year 2046 at 46 seconds 6 minutes and 3 hours
    explicit DateTime(const char* formattedString);
    explicit DateTime(const String& formattedString);
    // Explicit constructor, all values are validated and clamped.
    DateTime(SizeT day, SizeT month, SizeT year, SizeT second, SizeT minute, SizeT hour);


    LF_INLINE SizeT GetDay() const { return static_cast<SizeT>(mDay); }
    LF_INLINE SizeT GetMonth() const { return static_cast<SizeT>(mMonth); }
    LF_INLINE SizeT GetYear() const { return static_cast<SizeT>(mYear); }

    LF_INLINE SizeT GetSecond() const { return static_cast<SizeT>(mSecond); }
    LF_INLINE SizeT GetMinute() const { return static_cast<SizeT>(mMinute); }
    LF_INLINE SizeT GetHour() const { return static_cast<SizeT>(mHour); }

    bool IsLeapYear() const;
    String GetFormattedDate() const;
    String GetFormattedTime() const;

    void SetDate(SizeT day, SizeT month, SizeT year);
    void SetTime(SizeT second, SizeT minute, SizeT hour);

    // Encodes into a compact value
    DateTimeEncoded Encode() const;
    void Decode(DateTimeEncoded data);

    bool operator==(const DateTime& other) const
    {
        return mDay == other.mDay && mMonth == other.mMonth && mYear == other.mYear &&
            mSecond == other.mSecond && mMinute == other.mMinute && mHour == other.mHour;
    }

    bool operator!=(const DateTime& other) const
    {
        return !(*this == other);
    }

private:
    void InternalParse(const String& formattedString);

    bool ParseDate(const String& date);
    bool ParseTime(const String& time);

    UInt8 mDay;   // 1-30
    UInt8 mMonth; // 1-12
    UInt16 mYear; // 0-9999

    UInt8 mSecond; // 0-60
    UInt8 mMinute; // 0-60
    UInt8 mHour;   // 0-24
};

}

#endif // LF_CORE_DATE_TIME_H