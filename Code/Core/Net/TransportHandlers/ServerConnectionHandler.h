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
#ifndef LF_CORE_SERVER_CONNECTION_HANDLER_H
#define LF_CORE_SERVER_CONNECTION_HANDLER_H

#include "Core/Memory/DynamicPoolHeap.h"
#include "Core/Net/NetTransportHandler.h"

namespace lf {

namespace Crypto {
class RSAKey;
} // namespace Crypto
class TaskScheduler;
class NetConnectionController;
class NetServerController;

class LF_CORE_API ServerConnectionHandler : public NetTransportHandler
{
public:
    using Super = NetTransportHandler;

    // todo: We should define these 'packet data' types somewhere else? common header?
    // Connect Messages are anywhere from 750 bytes to 1000 bytes...
    // using ConnectPacketData = PacketData1024;
    struct ConnectPacketData : public PacketData1024
    {
        IPEndPointAny mSender;
    };


    // Basic overview of the structure:
    //
    // 1. Use the constructor to inject dependencies.. Maybe we'll use some sort of NetContext/NetDependencyManager to retrieve rather than arguments...
    // 
    // 2. This class receives and processes connection packets (no acks)
    // 
    // 3. The basic message protocol is 'Receive' -> 'Decode' -> 'Establish Connection' -> 'Generate Unique Key' -> 'Acknowledge'
    //
    //   Receive              [ Network Receiver Thread ]
    //   Decode               [ Network Task Thread ]
    //   Establish Connection [ Network Task Thread ]
    //   Generate Unique Key  [ Network Task Thread ]
    //   Acknowledge          [ Network Task Thread ]
    //  
    // 

    ServerConnectionHandler(TaskScheduler* taskScheduler, NetConnectionController* connectionController, NetServerController* serverController);
    ServerConnectionHandler(ServerConnectionHandler&& other);
    virtual ~ServerConnectionHandler();
    ServerConnectionHandler& operator=(ServerConnectionHandler&& other);

    void DecodePacket(ConnectPacketData* packetData);

protected:
    void OnInitialize() final;
    void OnShutdown() final;
    void OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender) final;
    void OnUpdateFrame() final;

private:
    ConnectPacketData* AllocatePacket();
    void FreePacket(ConnectPacketData* packet);

    // 'Context':
    TaskScheduler* mTaskScheduler;
    NetConnectionController* mConnectionController;
    NetServerController* mServerController;

    // Outputs:
    DynamicPoolHeap mPacketPool;

};

} // namespace lf

#endif // LF_CORE_SERVER_CONNECTION_HANDLER_H