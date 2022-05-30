#pragma once
// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/Time.h"
#include "Runtime/Net/NetDriver.h"
#include "Runtime/Net/NetConnection.h" // SetConnection(...); todo

namespace lf {

DECLARE_ATOMIC_WPTR(NetConnection);

// **********************************
// A NetMessage is a data structure that encapsulates the raw packet data to
// be sent in addition to some data about transmitting and retransmitting.
//
// A messages life-time is expected to last until
//    a) The message is acknowledged (if reliable)
//    b) The message times out trying to transmit (if reliable)
//    c) The message is sent (if unreliable)
// **********************************
class LF_RUNTIME_API NetMessage
{
public:
    enum State
    {
        SerializeData,
        Register,
        Transmit,
        Failed,
        Success,
        Garbage
    };

    NetMessage();

    bool Initialize(NetDriver::MessageType messageType, NetDriver::Options options, const ByteT* bytes, SizeT numBytes);
    bool Serialize(UInt32 packetUID, const NetKeySet& keySet, const NetServerDriverConfig& config);
    void OnTransmit();
    void OnSuccess();
    void OnFailed();

    void SetState(State value);
    State GetState() const;
    LF_INLINE SizeT GetTransmitRemaining() const { return mRetransmits; }
    LF_INLINE Float64 GetTransmitDelta() const { return mRetransmitTimer.PeekDelta(); }
    LF_INLINE bool HasTransmitStarted() const { return mRetransmitTimer.IsRunning(); }
    LF_INLINE UInt64 GetID() const { return mID; }
    LF_INLINE NetConnection* GetConnection() const { return mConnection; }
    LF_INLINE const ByteT* GetPacketBytes() const { return mPacketData.data(); }
    LF_INLINE SizeT GetPacketBytesSize() const { return mPacketData.size(); }

    LF_INLINE void SetSuccessCallback(NetDriver::OnSendSuccess value) { mSuccessCallback = value; }
    LF_INLINE void SetFailureCallback(NetDriver::OnSendFailed value) { mFailureCallback = value; }
    LF_INLINE void SetConnection(NetConnection* connection) { mConnection = GetAtomicPointer(connection); }

private:
    SizeT GetSizeEstimate() const;
    NetPacketType::Value GetPacketType() const;

    // ** The current state of the message
    volatile Atomic32 mState;
    // ** The raw packet bytes to be sent. (Created when the message is first serialized)
    TVector<ByteT>     mPacketData;
    // ** The remaining number of times we can transmit
    SizeT             mRetransmits;
    // ** A timer used to determine when we can retransmit the data
    Timer             mRetransmitTimer;
    // ** An id of the packet ( UID | Crc32 ) used for acks
    UInt64            mID;

    // ** A callback to be fired when the message has been sent successfully. 
    //    Reliable = (When it's been ack'ed)
    //    Unrelaible = (When it's been sent)
    NetDriver::OnSendSuccess mSuccessCallback;
    // ** A callback to be fired if there was an error sending the message (Serialization or Transmission)
    NetDriver::OnSendFailed  mFailureCallback;

    // ** The type of message being sent
    NetDriver::MessageType   mMessageType;
    // ** The options used for sending the message
    NetDriver::Options       mOptions;
    // ** The original application data sent. (This is cleared after serialization)
    TVector<ByteT>            mApplicationData;
    // ** The connection the message is being sent it.
    NetConnectionAtomicWPtr  mConnection;
};

} // namespace lf