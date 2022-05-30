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
#include "Runtime/PCH.h"
#include "NetRequestArgs.h"
#include "Core/Common/Assert.h"
#include "Core/String/String.h"
#include <string.h>

namespace lf {

static const SizeT FORMAT_BYTE_LENGTH = 1;

NetRequestArgs::NetRequestArgs()
: mRouteName()
, mRouteIndex(INVALID_ROUTE)
, mRouteArgs()
, mBodyFormat()
, mBody(nullptr)
, mBodyLength(0)
{

}


bool NetRequestArgs::Set(
const String& routeName,
const String& routeArgs,
NetRequestBodyFormat::Value bodyFormat,
const void* body,
SizeT bodyLength)
{
    Clear();
    if (routeName.Size() > MAX_ROUTE_NAME) { return false; }
    if (routeArgs.Size() > MAX_ROUTE_ARGS) { return false; }
    if (bodyLength > MAX_ROUTE_BODY) { return false; }
    if (InvalidEnum(bodyFormat) && body != nullptr) { return false; }

    if (body == nullptr)
    {
        bodyFormat = NetRequestBodyFormat::INVALID_ENUM;
        bodyLength = 0;
    }

    strcpy_s(mRouteName, LF_ARRAY_SIZE(mRouteName), routeName.CStr());
    strcpy_s(mRouteArgs, LF_ARRAY_SIZE(mRouteArgs), routeArgs.CStr());
    mBodyFormat = bodyFormat;
    mBody = body;
    mBodyLength = bodyLength;
    return true;
}

bool NetRequestArgs::Set(
RouteIndex routeIndex,
const String& routeArgs,
NetRequestBodyFormat::Value bodyFormat,
const void* body,
SizeT bodyLength)
{
    Clear();
    if (Invalid(routeIndex)) { return false; }
    if (routeArgs.Size() > MAX_ROUTE_ARGS) { return false; }
    if (bodyLength > MAX_ROUTE_BODY) { return false; }
    if (InvalidEnum(bodyFormat) && body != nullptr) { return false; }

    if (body == nullptr)
    {
        bodyFormat = NetRequestBodyFormat::INVALID_ENUM;
        bodyLength = 0;
    }

    mRouteIndex = routeIndex;
    strcpy_s(mRouteArgs, LF_ARRAY_SIZE(mRouteArgs), routeArgs.CStr());
    mBodyFormat = bodyFormat;
    mBody = body;
    mBodyLength = bodyLength;
    return true;
}

void NetRequestArgs::Clear()
{
    memset(mRouteName, 0, sizeof(mRouteName));
    mRouteIndex = INVALID_ROUTE;
    memset(mRouteArgs, 0, sizeof(mRouteArgs));
    mBodyFormat = NetRequestBodyFormat::INVALID_ENUM;
    mBody = nullptr;
    mBodyLength = 0;
}
bool NetRequestArgs::Empty() const
{
    return Invalid(mRouteIndex) && mRouteName[0] == 0;
}

bool NetRequestArgs::UseRouteIndex() const                          
{
    return ::lf::Valid(mRouteIndex);
}
void NetRequestArgs::ReserveBody(void* body, SizeT bodyLength)
{
    Clear();
    mBody = body;
    mBodyLength = bodyLength;
}
String NetRequestArgs::GetRouteName() const                         
{
    return String(mRouteName, COPY_ON_WRITE);
}
RouteIndex NetRequestArgs::GetRouteIndex() const
{
    return mRouteIndex;
}
String NetRequestArgs::GetRouteArgs() const                         
{
    return String(mRouteArgs, COPY_ON_WRITE);
}
NetRequestBodyFormat::Value NetRequestArgs::GetBodyFormat() const   
{
    return mBodyFormat;
}
const void* NetRequestArgs::GetBody() const                         
{
    return mBody;
}
SizeT NetRequestArgs::GetBodyLength() const                         
{
    return mBodyLength;
}

SizeT NetRequestArgs::Write(ByteT* buffer, const SizeT bufferLength) const
{
    if (buffer == nullptr)
    {
        return 0;
    }

    String routeName = GetRouteName();
    String routeArgs = GetRouteArgs();
    SizeT  routeNameLength = UseRouteIndex() ? 2 : routeName.Size() + 1;
    SizeT requiredBufferLength = (1 + routeNameLength + routeArgs.Size() + 1 + mBodyLength);


    if (bufferLength < requiredBufferLength)
    {
        return 0;
    }


    SizeT cursor = 0;
    UInt8 bodyFormat = static_cast<UInt8>(mBodyFormat);
    buffer[cursor++] = (UseRouteIndex() ? 1 : 0) | (bodyFormat << 1);
    if (UseRouteIndex())
    {
        if (memcpy_s(&buffer[cursor], bufferLength - cursor, &mRouteIndex, sizeof(mRouteIndex)) != 0)
        {
            return 0;
        }
        cursor += sizeof(mRouteIndex);
    }
    else
    {
        if (memcpy_s(&buffer[cursor], bufferLength - cursor, routeName.CStr(), routeName.Size()) != 0)
        {
            return 0;
        }
        cursor += routeName.Size();
        buffer[cursor++] = 0;
    }

    if (memcpy_s(&buffer[cursor], bufferLength - cursor, routeArgs.CStr(), routeArgs.Size()) != 0)
    {
        return 0;
    }
    cursor += routeArgs.Size();
    buffer[cursor++] = 0;

    if (mBody)
    {
        if (memcpy_s(&buffer[cursor], bufferLength - cursor, mBody, mBodyLength) != 0)
        {
            return 0;
        }
        cursor += mBodyLength;
    }
    
    CriticalAssert(cursor <= bufferLength);
    return cursor;
}

bool DecodeString(const char* base, const SizeT maxBufferLength, char* outBuffer, const SizeT maxOutBufferLength, SizeT& writtenBytes)
{
    if (maxBufferLength == 0)
    {
        return false;
    }

    writtenBytes = 0;
    // find null byte
    const char* nullByte = base;
    while (*nullByte != '\0')
    {
        ++nullByte;
        if (static_cast<SizeT>(nullByte - base) >= maxBufferLength)
        {
            return false;
        }
        if (static_cast<SizeT>(nullByte - base) >= maxOutBufferLength)
        {
            return false;
        }
    }

    writtenBytes = (nullByte - base);
    memcpy(outBuffer, base, writtenBytes);
    return true;
}



bool NetRequestArgs::Read(const ByteT* buffer, const SizeT bufferLength)
{
    if (!buffer || bufferLength < 3) // Buffer not large enough
    {
        return false;
    }

    SizeT cursor = 0;
    UInt8 formatByte = buffer[cursor++];
    mBodyFormat = static_cast<NetRequestBodyFormat::Value>(formatByte >> 1);
    bool useRouteIndex = (formatByte & 0x01) != 0;

    SizeT bytesWritten = 0;
    
    if (useRouteIndex)
    {
        LF_STATIC_ASSERT(sizeof(mRouteIndex) == 2);
        reinterpret_cast<ByteT*>(&mRouteIndex)[0] = buffer[cursor++];
        reinterpret_cast<ByteT*>(&mRouteIndex)[1] = buffer[cursor++];
    }
    else
    {
        if (!DecodeString(reinterpret_cast<const char*>(buffer + cursor), bufferLength - cursor, mRouteName, MAX_ROUTE_NAME, bytesWritten))
        {
            Clear();
            return false;
        }
        cursor += bytesWritten + 1;
    }

    if (!DecodeString(reinterpret_cast<const char*>(buffer + cursor), bufferLength - cursor, mRouteArgs, MAX_ROUTE_ARGS, bytesWritten))
    {
        Clear();
        return false;
    }
    cursor += bytesWritten + 1;

    SizeT bodyLength = bufferLength - cursor;
    if (bodyLength == 0)
    {
        mBody = nullptr;
        mBodyLength = 0;
        return true;
    }
    // Buffer not large enough
    if (!mBody || bodyLength > mBodyLength)
    {
        Clear();
        return false;
    }

    memcpy(const_cast<void*>(mBody), &buffer[cursor], bodyLength);
    mBodyLength = bodyLength;
    return true;
}

bool NetRequestArgs::Valid() const
{
    if (InvalidEnum(mBodyFormat) && mBody != nullptr) 
    { 
        return false; 
    }
    if (Invalid(mRouteIndex) && mRouteName[0] == 0) // both invalid
    { 
        return false; 
    }
    if (::lf::Valid(mRouteIndex) && mRouteName[0] != 0) // both valid
    { 
        return false; 
    }

    return true;
}

} // namespace lf