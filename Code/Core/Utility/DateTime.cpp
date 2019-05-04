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
#include "DateTime.h"

#include "Core/String/String.h"
#include "Core/String/StringCommon.h"
#include "Core/Math/MathFunctions.h"

#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace lf {

enum Month
{
    JANUARY = 1,
    FEBRUARY,
    MARCH,
    APRIL,
    MAY,
    JUNE,
    JULY,
    AUGUST,
    SEPTEMBER,
    OCTOBER,
    NOVEMBER,
    DECEMBER
};

struct OSDateTime
{
    SizeT day;
    SizeT month;
    SizeT year;

    SizeT second;
    SizeT minute;
    SizeT hour;
};

static void GetTimeNow(OSDateTime& dt)
{
#if defined(LF_OS_WINDOWS)
    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);

    dt.day = static_cast<SizeT>(sysTime.wDay);
    dt.month = static_cast<SizeT>(sysTime.wMonth);
    dt.year = static_cast<SizeT>(sysTime.wYear);

    dt.second = static_cast<SizeT>(sysTime.wSecond);
    dt.minute = static_cast<SizeT>(sysTime.wMinute);
    dt.hour = static_cast<SizeT>(sysTime.wHour);
#else 
    LF_STATIC_CRASH("Missing implementation.");
    (dt);
#endif
}

static bool IsLeapYear(SizeT year)
{
    // from: https://support.microsoft.com/en-ca/help/214019/method-to-determine-whether-a-year-is-a-leap-year

    // To determine whether a year is a leap year, follow these steps :
    //     1. If the year is evenly divisible by 4, go to step 2. Otherwise, go to step 5.
    //     2. If the year is evenly divisible by 100, go to step 3. Otherwise, go to step 4.
    //     3. If the year is evenly divisible by 400, go to step 4. Otherwise, go to step 5.
    //     4. The year is a leap year(it has 366 days).
    //     5. The year is not a leap year(it has 365 days).

    if (year % 4 == 0)
    {
        if ((year % 100 != 0) || (year % 400 == 0))
        {
            return true;
        }
    }
    return false;
}

static SizeT GetDaysInMonth(SizeT month, SizeT year)
{
    switch (month)
    {
        case JANUARY: return 31;
        case FEBRUARY: return IsLeapYear(year) ? 29 : 28;
        case MARCH: return 31;
        case APRIL: return 30;
        case MAY: return 31;
        case JUNE: return 30;
        case JULY: return 31;
        case AUGUST: return 31;
        case SEPTEMBER: return 30;
        case OCTOBER: return 31;
        case NOVEMBER: return 30;
        case DECEMBER: return 31;
    }
    return INVALID;
}

DateTime::DateTime() :
    mDay(0),
    mMonth(0),
    mYear(0),
    mSecond(0),
    mMinute(0),
    mHour(0)
{

}

DateTime::DateTime(bool now) :
    mDay(0),
    mMonth(0),
    mYear(0),
    mSecond(0),
    mMinute(0),
    mHour(0)
{
    if (now)
    {
        OSDateTime dt;
        GetTimeNow(dt);
#if defined(MGF_DEBUG)
        Assert(dt.day <= 31);
        Assert(dt.month <= 12);
        Assert(dt.year <= 9999);
        Assert(dt.second <= 60);
        Assert(dt.minute <= 60);
        Assert(dt.hour <= 24);
#endif
        mDay = static_cast<UInt8>(dt.day);
        mMonth = static_cast<UInt8>(dt.month);
        mYear = static_cast<UInt16>(dt.year);

        mSecond = static_cast<UInt8>(dt.second);
        mMinute = static_cast<UInt8>(dt.minute);
        mHour = static_cast<UInt8>(dt.hour);
    }
}

DateTime::DateTime(DateTimeEncoded data) :
    mDay(0),
    mMonth(0),
    mYear(0),
    mSecond(0),
    mMinute(0),
    mHour(0)
{
    Decode(data);
}

DateTime::DateTime(const char* formattedString) :
    mDay(0),
    mMonth(0),
    mYear(0),
    mSecond(0),
    mMinute(0),
    mHour(0)
{
    String str(formattedString, COPY_ON_WRITE);
    InternalParse(str);
}

DateTime::DateTime(const String& formattedString) :
    mDay(0),
    mMonth(0),
    mYear(0),
    mSecond(0),
    mMinute(0),
    mHour(0)
{
    InternalParse(formattedString);
}

DateTime::DateTime(SizeT day, SizeT month, SizeT year, SizeT second, SizeT minute, SizeT hour) :
    mDay(0),
    mMonth(0),
    mYear(0),
    mSecond(0),
    mMinute(0),
    mHour(0)
{
    SetDate(day, month, year);
    SetTime(second, minute, hour);
}

bool DateTime::IsLeapYear() const
{
    return ::lf::IsLeapYear(static_cast<SizeT>(mYear));
}

String DateTime::GetFormattedDate() const
{
    String str;
    if (mDay < 10)
    {
        str.Append('0');
        str.Append('0' + mDay);
    }
    else
    {
        str.Append(ToString(mDay));
    }
    str.Append('/');

    if (mMonth < 10)
    {
        str.Append('0');
        str.Append('0' + mMonth);
    }
    else
    {
        str.Append(ToString(mMonth));
    }
    str.Append('/');

    String year = ToString(mYear);
    SizeT numZeroes = 4 - year.Size();
    for (SizeT i = 0; i < numZeroes; ++i)
    {
        str.Append('0');
    }
    str.Append(year);

    return str;
}

String DateTime::GetFormattedTime() const
{
    String str;
    if (mSecond < 10)
    {
        str.Append('0');
        str.Append('0' + mSecond);
    }
    else
    {
        str.Append(ToString(mSecond));
    }
    str.Append(':');

    if (mMinute < 10)
    {
        str.Append('0');
        str.Append('0' + mMinute);
    }
    else
    {
        str.Append(ToString(mMinute));
    }
    str.Append(':');

    if (mHour < 10)
    {
        str.Append('0');
        str.Append('0' + mHour);
    }
    else
    {
        str.Append(ToString(mHour));
    }

    return str;
}

void DateTime::SetDate(SizeT day, SizeT month, SizeT year)
{
    mYear = static_cast<UInt16>(Clamp<SizeT>(year, 0, 9999));
    mMonth = static_cast<UInt8>(Clamp<SizeT>(month, JANUARY, DECEMBER));
    SizeT maxDay = GetDaysInMonth(mMonth, mYear);
    mDay = static_cast<UInt8>(Clamp<SizeT>(day, 0, maxDay));
}

void DateTime::SetTime(SizeT second, SizeT minute, SizeT hour)
{
    mSecond = static_cast<UInt8>(Clamp<SizeT>(second, 0, 60));
    mMinute = static_cast<UInt8>(Clamp<SizeT>(minute, 0, 60));
    mHour = static_cast<UInt8>(Clamp<SizeT>(hour, 0, 24));
}

// Date: 23 bits
//   day : [1-31]   : 5 bits 
// month : [1-12]   : 4 bits 
//  year : [0-9999] : 14 bits

// Time: 17 bits
// second : [0-60] : 6 bits 
// minute : [0-60] : 6 bits 
//   hour : [0-24] : 5 bits
DateTimeEncoded DateTime::Encode() const
{
    DateTimeEncoded data;
    // Shift over the amount for the next write:
    data.value |= GetHour();   data.value = data.value << 6;
    data.value |= GetMinute(); data.value = data.value << 6;
    data.value |= GetSecond(); data.value = data.value << 14;
    data.value |= GetYear();   data.value = data.value << 4;
    data.value |= GetMonth();  data.value = data.value << 5;
    data.value |= GetDay();
    return data;
}
void DateTime::Decode(DateTimeEncoded data)
{
    // Read Date:
    SizeT values[3];
    values[0] = static_cast<SizeT>(data.value & 0x1F);  data.value = data.value >> 5; // day
    values[1] = static_cast<SizeT>(data.value & 0xF);   data.value = data.value >> 4; // month
    values[2] = static_cast<SizeT>(data.value & 0x7FF); data.value = data.value >> 14;// year
    SetDate(values[0], values[1], values[2]);

    values[0] = static_cast<SizeT>(data.value & 0x3F); data.value = data.value >> 6; // second
    values[1] = static_cast<SizeT>(data.value & 0x3F); data.value = data.value >> 6; // minute
    values[2] = static_cast<SizeT>(data.value & 0x1F); // hour
    SetTime(values[0], values[1], values[2]);
}

static SizeT SearchBegin(SizeT start, const String& str)
{
    for (SizeT i = start, size = str.Size(); i < size; ++i)
    {
        char c = str[i];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
        {
            return i;
        }
    }
    return INVALID;
}

static SizeT SearchEnd(SizeT start, const String& str)
{
    for (SizeT i = start, size = str.Size(); i < size; ++i)
    {
        char c = str[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            return i;
        }
    }
    return INVALID;
}

void DateTime::InternalParse(const String& formattedString)
{
    // Get 'Date' and 'Time' ffrom string based on "whitespace" markers

    // ParseTime(formattedString);
    // ParseDate(formattedString);

    SizeT dateBegin = SearchBegin(0, formattedString);
    SizeT dateEnd = SearchEnd(dateBegin + 1, formattedString);
    String dateStr;

    if (Invalid(dateBegin))
    {
        if (Invalid(dateEnd))
        {
            dateStr = formattedString;
        }
        else
        {
            formattedString.SubString(0, dateEnd, dateStr);
        }
    }
    else
    {
        if (Invalid(dateEnd))
        {
            formattedString.SubString(dateBegin, dateStr);
        }
        else
        {
            formattedString.SubString(dateBegin, dateEnd - dateBegin, dateStr);
        }
    }

    if (!ParseDate(dateStr))
    {
        return;
    }

    if (Invalid(dateEnd))
    {
        return; // no time component:
    }

    SizeT timeBegin = SearchBegin(dateEnd + 1, formattedString);
    SizeT timeEnd = SearchEnd(timeBegin + 1, formattedString);
    String timeStr;

    if (Valid(dateBegin)) // Has no valid "time"
    {
        if (Invalid(timeEnd))
        {
            formattedString.SubString(timeBegin, timeStr);
        }
        else
        {
            formattedString.SubString(timeBegin, timeEnd - timeBegin, timeStr);
        }
        ParseTime(timeStr);
    }
}

bool DateTime::ParseDate(const String& date)
{
    String dayStr;
    String monthStr;
    String yearStr;

    const SizeT PT_DAY = 0;
    const SizeT PT_MONTH = 1;
    const SizeT PT_YEAR = 2;
    const SizeT size = date.Size();
    SizeT parseType = 0;
    SizeT length = 0;
    SizeT start = 0;

    for (SizeT i = 0; i < size; ++i)
    {
        char c = date[i];
        if (c == '/')
        {
            // take substr
            switch (parseType)
            {
                case PT_DAY:
                    date.SubString(start, length, dayStr);
                    break;
                case PT_MONTH:
                    date.SubString(start, length, monthStr);
                    break;
                case PT_YEAR:
                    date.SubString(start, length, yearStr);
                    break;
                default:
                    Crash("Invalid parse type!", LF_ERROR_INTERNAL, ERROR_API_CORE);
                    return false;
            }
            start = i + 1;
            length = 0;
            ++parseType;
        }
        else if (i == size - 1)
        {
            switch (parseType)
            {
                case PT_DAY:
                    date.SubString(start, dayStr);
                    break;
                case PT_MONTH:
                    date.SubString(start, monthStr);
                    break;
                case PT_YEAR:
                    date.SubString(start, yearStr);
                    break;
                default:
                    Crash("Invalid parse type!", LF_ERROR_INTERNAL, ERROR_API_CORE);
                    return false;
            }
        }
        else
        {
            ++length;
        }
    }

    if (dayStr.Empty() || monthStr.Empty() || yearStr.Empty())
    {
        return false;
    }

    SizeT day = static_cast<SizeT>(ToUInt32(dayStr));
    SizeT month = Clamp<SizeT>(static_cast<SizeT>(ToUInt32(monthStr)), JANUARY, DECEMBER);
    SizeT year = Clamp<SizeT>(static_cast<SizeT>(ToUInt32(yearStr)), 0, 9999);

    SizeT maxDays = GetDaysInMonth(month, year);
    if (Invalid(maxDays))
    {
        return false;
    }
    day = Clamp<SizeT>(day, 1, maxDays);

    mDay = static_cast<UInt8>(day);
    mMonth = static_cast<UInt8>(month);
    mYear = static_cast<UInt16>(year);
    return true;
}

bool DateTime::ParseTime(const String& time)
{
    String secondStr;
    String minuteStr;
    String hourStr;

    const SizeT PT_SECOND = 0;
    const SizeT PT_MINUTE = 1;
    const SizeT PT_HOUR = 2;
    const SizeT size = time.Size();
    SizeT parseType = 0;
    SizeT length = 0;
    SizeT start = 0;

    for (SizeT i = 0; i < size; ++i)
    {
        char c = time[i];
        if (c == ':')
        {
            // take substr
            switch (parseType)
            {
                case PT_SECOND:
                    time.SubString(start, length, secondStr);
                    break;
                case PT_MINUTE:
                    time.SubString(start, length, minuteStr);
                    break;
                case PT_HOUR:
                    time.SubString(start, length, hourStr);
                    break;
                default:
                    Crash("Invalid parse type!", LF_ERROR_INTERNAL, ERROR_API_CORE);
                    return false;
            }
            start = i + 1;
            length = 0;
            ++parseType;
        }
        else if (i == size - 1)
        {
            switch (parseType)
            {
                case PT_SECOND:
                    time.SubString(start, secondStr);
                    break;
                case PT_MINUTE:
                    time.SubString(start, minuteStr);
                    break;
                case PT_HOUR:
                    time.SubString(start, hourStr);
                    break;
                default:
                    Crash("Invalid parse type!", LF_ERROR_INTERNAL, ERROR_API_CORE);
                    return false;
            }
        }
        else
        {
            ++length;
        }
    }

    if (secondStr.Empty() || minuteStr.Empty() || hourStr.Empty())
    {
        return false;
    }

    SizeT second = Clamp<SizeT>(static_cast<SizeT>(ToUInt32(secondStr)), 0, 60);
    SizeT minute = Clamp<SizeT>(static_cast<SizeT>(ToUInt32(minuteStr)), 0, 60);
    SizeT hour = Clamp<SizeT>(static_cast<SizeT>(ToUInt32(hourStr)), 0, 24);

    mSecond = static_cast<UInt8>(second);
    mMinute = static_cast<UInt8>(minute);
    mHour = static_cast<UInt8>(hour);
    return true;
}

} // namespace lf