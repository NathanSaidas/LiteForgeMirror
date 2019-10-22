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
#ifndef LF_CORE_CLIENT_CONNECTION_HANDLER_H
#define LF_CORE_CLIENT_CONNECTION_HANDLER_H

#include "Core/Memory/DynamicPoolHeap.h"
#include "Core/Net/NetTransportHandler.h"

namespace lf {

class TaskScheduler;
class NetClientController;

class LF_CORE_API ClientConnectionHandler : public NetTransportHandler
{
public:
    using Super = NetTransportHandler;

    struct ConnectAckPacketData : public PacketData1024
    {
        IPEndPointAny mSender;
    };

    ClientConnectionHandler(TaskScheduler* taskScheduler, NetClientController* clientController);
    ClientConnectionHandler(ClientConnectionHandler&& other);
    virtual ~ClientConnectionHandler();
    ClientConnectionHandler& operator=(ClientConnectionHandler&& other);

    void DecodePacket(ConnectAckPacketData* packetData);

protected:
    void OnInitialize() final;
    void OnShutdown() final;
    void OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender) final;
    void OnUpdateFrame() final;

private:
    ConnectAckPacketData* AllocatePacket();
    void FreePacket(ConnectAckPacketData* packet);

    // 'Context':
    TaskScheduler* mTaskScheduler;
    NetClientController* mClientController;

    // Outputs:
    DynamicPoolHeap mPacketPool;
};

} // namespace lf

#endif // LF_CORE_CLIENT_CONNECTION_HANDLER_H