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
#ifndef LF_CORE_NET_TRANSPORT_HANDLER_H
#define LF_CORE_NET_TRANSPORT_HANDLER_H
#include "Core/Net/NetTypes.h"
#include "Core/Memory/Memory.h"

namespace lf {

struct NetTransportHandlerStats
{
    SizeT mBytesReceived;
    SizeT mPacketsReceived;

    SizeT mBytesReceivedFrame;
    SizeT mBytesReceivedSecond;
    SizeT mPacketsReceivedFrame;
    SizeT mPacketsReceivedSecond;
};

class LF_CORE_API NetTransportHandler
{
public:
    // **********************************
    // Copy constructor is not allowed.
    // **********************************
    NetTransportHandler(const NetTransportHandler&) = delete;
    // **********************************
    // Copy assignment is not allowed.
    // **********************************
    NetTransportHandler& operator=(const NetTransportHandler&) = delete;
    // **********************************
    // Default constructor initializes defaults
    // **********************************
    NetTransportHandler();
    // **********************************
    // Move constructor to move 'other' into 'this' while resetting 'other' to default
    // **********************************
    NetTransportHandler(NetTransportHandler&& other);
    // **********************************
    // Destructor ensures resources are released.
    // **********************************
    virtual ~NetTransportHandler();
    // **********************************
    // Move assignment to move 'other' into 'this' while resetting 'other to default
    // **********************************
    NetTransportHandler& operator=(NetTransportHandler&& other);
    // **********************************
    // Initializes the transport handler acquiring any resources it might need
    // **********************************
    void Initialize();
    // **********************************
    // Releases the transport handler
    // **********************************
    void Shutdown();
    // **********************************
    // Sends packet data to a transport handler for further processing
    // **********************************
    void ReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender);
    // **********************************
    // Updates the transport handler each frame
    // **********************************
    void UpdateFrame();
protected:
    // **********************************
    // Called once to initialize any resources the transport handler might require
    // [threading:unknown]
    // **********************************
    virtual void OnInitialize() = 0;
    // **********************************
    // Called once to release any resources the transport handler might hanging onto.
    // [threading:unknown]
    // **********************************
    virtual void OnShutdown() = 0;
    // **********************************
    // Called many times when the transport handler receives a packet. This method is called
    // on the network receiver thread and should be processed as quick as possible.
    // **********************************
    virtual void OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender) = 0;
    // **********************************
    // Called once every frame to pump any packets out that must be synced this frame.
    // **********************************
    virtual void OnUpdateFrame() = 0;
};
} // namespace lf 
#endif // LF_CORE_NET_TRANSPORT_HANDLER_H