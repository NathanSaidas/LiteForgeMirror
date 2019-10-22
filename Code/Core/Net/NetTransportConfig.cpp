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
#include "NetTransportConfig.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Net/NetTransportHandler.h"
#include "Core/Utility/ErrorCore.h"

namespace lf {

NetTransportConfig::NetTransportConfig()
: mPort(0)
, mAppId(0)
, mAppVersion(0)
, mEndPoint()
, mHandlers()
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        mHandlers[i] = nullptr;
    }
}
NetTransportConfig::NetTransportConfig(NetTransportConfig&& other)
: mPort(other.mPort)
, mAppId(other.mAppId)
, mAppVersion(other.mAppVersion)
, mEndPoint(std::move(other.mEndPoint))
, mHandlers()
{
    other.mPort = 0;
    other.mAppId = 0;
    other.mAppVersion = 0;
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        mHandlers[i] = other.mHandlers[i];
        other.mHandlers[i] = nullptr;
    }
}
NetTransportConfig::~NetTransportConfig()
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        // Must call close handlers before!
        CriticalAssertEx(!mHandlers[i], LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
    }
}

NetTransportConfig& NetTransportConfig::operator=(NetTransportConfig&& other)
{
    if (this == &other)
    {
        return *this;
    }

    mPort = other.mPort;
    mAppId = other.mAppId;
    mAppVersion = other.mAppVersion;
    mEndPoint = std::move(other.mEndPoint);
    other.mPort = 0;
    other.mAppId = 0;
    other.mAppVersion = 0;
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        // Must call close handlers before!
        CriticalAssertEx(!mHandlers[i], LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
        mHandlers[i] = other.mHandlers[i];
        other.mHandlers[i] = nullptr;
    }

    return *this;
}

void NetTransportConfig::SetTransportHandler(NetPacketType::Value packetType, NetTransportHandler* transportHandler)
{
    CriticalAssertEx(packetType >= 0 && packetType < LF_ARRAY_SIZE(mHandlers), LF_ERROR_OUT_OF_RANGE, ERROR_API_CORE);
    if (mHandlers[packetType])
    {
        LFDelete(mHandlers[packetType]);
    }
    mHandlers[packetType] = transportHandler;
}

void NetTransportConfig::CloseHandlers(bool unset)
{
    if (unset)
    {
        for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
        {
            mHandlers[i] = nullptr;
        }
    }
    else
    {
        for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
        {
            if (mHandlers[i])
            {
                LFDelete(mHandlers[i]);
                mHandlers[i] = nullptr;
            }
        }
    }
}

} // namespace lf