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

#include "Core/Net/NetTypes.h"
#include "Core/Net/UDPSocket.h"
#include "Core/Platform/Thread.h"

#include "Core/Memory/AtomicSmartPointer.h"

namespace lf {




// DECLARE_ATOMIC_WPTR(NetConnection);
// class LF_CORE_API NetConnection
// {
// public:
// 
// private:
//     NetConnectionAtomicWPtr mSelf;
//     IPEndPointAny mEndPoint;
//     UInt32        mConnectionUID;
// };

// class LF_CORE_API NetConnectTransportHandler : NetTransportHandler
// {
// protected:
//     virtual void OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender) override
//     {
//         if (flags.processNow)
//         {
//             immediate_update.
//         }
//         else
//         {
//             lock(frame_update_lock);
//             frame_update.add(...);
//             unlock(frame_update_lock);
//         }
//     }
// };



// PacketTypes:
//    Connection
//    Disconnect
//    Heartbeat
//    Ack
//    Message
class NetTransportHandler;
class NetTransportConfig;

class LF_CORE_API LF_IMPL_OPAQUE(NetTransport)
{
public:
    LF_IMPL_OPAQUE(NetTransport)(const LF_IMPL_OPAQUE(NetTransport)&) = delete;
    LF_IMPL_OPAQUE(NetTransport)& operator=(const LF_IMPL_OPAQUE(NetTransport)&) = delete;

    LF_IMPL_OPAQUE(NetTransport)();
    LF_IMPL_OPAQUE(NetTransport)(LF_IMPL_OPAQUE(NetTransport)&& other);
    ~LF_IMPL_OPAQUE(NetTransport)();

    LF_IMPL_OPAQUE(NetTransport)& operator=(LF_IMPL_OPAQUE(NetTransport) && other);



    void Start(NetTransportConfig&& config, const ByteT* clientConnectionBytes = nullptr, SizeT numBytes = 0);
    void Stop();

    bool IsRunning() const;
    // todo: 
    // IsClient
    // IsServer
    // IsConnected

    IPEndPointAny GetBoundEndPoint() const;
    bool Send(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint);

    // fn: HandlePacket( PacketType, Handler )
    // fn: SendPacket( ... )

    // In:
    //    
    //
    // Out:
    //      HashTable<Acknowledgements>
    // 
private:
    

    struct ServerData
    {
        // Connections  : array<connection_data_ptr>
        //
        // ConnectedIPs : map[IP => UID]
        // ConnectedUID : map[UID => Connection]
        // IPCriminals  : map[IP => NetCriminalRecord]
    };

    struct ClientData
    {
        // **********************************
        //
        // Server:
        // When a client calls 'Start' with client configuration they will attempt to
        // connect to the target server.
        // As we await a response back we cache the connection packet data here.
        // We might need to retransmit if the server responds [ACK | CORRUPT]
        // **********************************
        PacketData1024 mAckConnect;
    };


    static void ProcessReceiveThread(void*);
    void ProcessReceive();

    Thread        mInboundThread;
    UDPSocket     mInbound;
    IPEndPointAny mBoundEndPoint;
    UInt16        mAppId;
    UInt16        mAppVersion;
    NetTransportHandler* mHandlers[NetPacketType::MAX_VALUE];
    UDPSocket     mOutbound; // Clients only
    bool          mIsClient;

    // UDPSocket     mClientOutbound; // for client they use this socket to send data out, servers use 'connections'

    
    ClientData*   mClient;

    Atomic32      mRunning;
};

} // namespace lf