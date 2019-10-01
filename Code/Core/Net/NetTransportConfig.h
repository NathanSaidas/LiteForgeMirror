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
#ifndef LF_CORE_NET_TRANSPORT_CONFIG_H
#define LF_CORE_NET_TRANSPORT_CONFIG_H

#include "Core/Net/NetTypes.h"

namespace lf {
class NetTransportHandler;

class LF_CORE_API NetTransportConfig
{
public:
    NetTransportConfig(const NetTransportConfig&) = delete;
    NetTransportConfig& operator=(const NetTransportConfig&) = delete;

    NetTransportConfig();
    NetTransportConfig(NetTransportConfig&& other);
    ~NetTransportConfig();

    NetTransportConfig& operator=(NetTransportConfig&& other);

    void SetPort(UInt16 value) { mPort = value; }
    void SetAppId(UInt16 value) { mAppId = value; }
    void SetAppVersion(UInt16 value) { mAppVersion = value; }
    void SetTransportHandler(NetPacketType::Value packetType, NetTransportHandler* transportHandler);

    UInt16 GetPort() const { return mPort; }
    UInt16 GetAppId() const { return mAppId; }
    UInt16 GetAppVersion() const { return mAppVersion; }
    NetTransportHandler* GetTransportHandler(NetPacketType::Value packetType) { return mHandlers[packetType]; }

    void CloseHandlers(bool unset = true);
private:
    UInt16 mPort;
    UInt16 mAppId;
    UInt16 mAppVersion;
    NetTransportHandler* mHandlers[NetPacketType::MAX_VALUE];
};

} // namespace lf 

#endif // 