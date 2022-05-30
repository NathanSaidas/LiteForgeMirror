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
#include "NetResponseArgs.h"
#include "Runtime/Net/NetResponseStatus.h"

namespace lf {
NetResponseArgs::NetResponseArgs()
: mStatus(NetResponseStatus::INVALID_ENUM)
, mSourcePacketUID(INVALID_PACKET_UID)
, mBody(nullptr)
, mBodyLength(0)
{
}

bool NetResponseArgs::Set(UInt16 status, UInt32 sourcePacketUID, const void* body, SizeT bodyLength)
{
    Clear();
    if (status == NetResponseStatus::INVALID_ENUM || Invalid(sourcePacketUID))
    {
        return false;
    }

    mStatus = status;
    mSourcePacketUID = sourcePacketUID;
    mBody = body;
    mBodyLength = body != nullptr ? bodyLength : 0;
    return true;
}

void NetResponseArgs::ReserveBody(void* body, SizeT bodyLength)
{
    mStatus = NetResponseStatus::INVALID_ENUM;
    mSourcePacketUID = INVALID_PACKET_UID;
    mBody = body;
    mBodyLength = bodyLength;
}

void NetResponseArgs::Clear()
{
    mStatus = NetResponseStatus::INVALID_ENUM;
    mSourcePacketUID = INVALID_PACKET_UID;
    mBody = nullptr;
    mBodyLength = 0;
}

SizeT NetResponseArgs::Write(ByteT* buffer, const SizeT bufferLength) const
{
    const SizeT bytesWritten = sizeof(mStatus) + sizeof(mSourcePacketUID) + mBodyLength;
    if (bytesWritten > bufferLength)
    {
        return 0;
    }
    SizeT cursor = 0;
    memcpy(&buffer[cursor], &mStatus, sizeof(mStatus));
    cursor += sizeof(mStatus);

    memcpy(&buffer[cursor], &mSourcePacketUID, sizeof(mSourcePacketUID));
    cursor += sizeof(mSourcePacketUID);
    
    if (mBody)
    {
        memcpy(&buffer[cursor], mBody, mBodyLength);
        cursor += mBodyLength;
    }

    CriticalAssert(cursor == bytesWritten);
    return cursor;
}

bool NetResponseArgs::Read(const ByteT* buffer, const SizeT bufferLength)
{
    if (bufferLength < (sizeof(mStatus) + sizeof(mSourcePacketUID)))
    {
        return false;
    }

    SizeT cursor = 0;
    memcpy(&mStatus, &buffer[cursor], sizeof(mStatus));
    cursor += sizeof(mStatus);

    memcpy(&mSourcePacketUID, &buffer[cursor], sizeof(mSourcePacketUID));
    cursor += sizeof(mSourcePacketUID);

    SizeT remaining = bufferLength - cursor;
    if (remaining == 0)
    {
        return true;
    }

    if (mBody == nullptr || mBodyLength < remaining)
    {
        return false;
    }

    memcpy(const_cast<void*>(mBody), &buffer[cursor], remaining);
    mBodyLength = remaining;
    return true;
}

SizeT NetResponseArgs::GetWriteSize() const
{
    return sizeof(mStatus) + sizeof(mSourcePacketUID) + mBodyLength;
}

} // namespace lf