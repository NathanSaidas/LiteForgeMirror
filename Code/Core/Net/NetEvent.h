#pragma once
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

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Net/NetTypes.h"

#if defined(LF_TEST) || defined(LF_DEBUG)
#define LF_NET_EVENT_DEBUG
#endif

// After allocating
#if defined(LF_NET_EVENT_DEBUG)
#define NET_EVENT_DEBUG_INFO(event) ::lf::NetDebugInfo(event, __FILE__, __LINE__)
#else
#define NET_EVENT_DEBUG_INFO(event)
#endif

namespace lf {

namespace NetEventType
{
    enum Value
    {
        NET_EVENT_CONNECT_SUCCESS,
        NET_EVENT_CONNECT_FAILED,
        NET_EVENT_CONNECTION_CREATED,
        NET_EVENT_CONNECTION_TERMINATED,
        NET_EVENT_HEARTBEAT_RECEIVED,
        NET_EVENT_DATA_RECEIVED_REQUEST,
        NET_EVENT_DATA_RECEIVED_RESPONSE,
        NET_EVENT_DATA_RECEIVED_ACTION,
        NET_EVENT_DATA_RECEIVED_REPLICATION,

        MAX_VALUE,
        INVALID_ENUM = NetEventType::MAX_VALUE
    };
}

class NetEvent;
#if defined(LF_NET_EVENT_DEBUG)
LF_INLINE void NetDebugInfo(NetEvent* event, const char* filename, SizeT line);
#endif

// Stub for future NetEvent types.
class LF_CORE_API NetEvent
{
public:
    NetEvent() = delete;
    NetEvent(NetEventType::Value type);
    NetEvent(const NetEvent& other);
    NetEvent(NetEvent&& other);

    NetEvent& operator=(const NetEvent& other);
    NetEvent& operator=(NetEvent&& other);

    NetEventType::Value GetType() const { return mType; }
protected:
    NetEventType::Value mType;
#if defined(LF_NET_EVENT_DEBUG)
private:
    friend void NetDebugInfo(NetEvent* event, const char* filename, SizeT line);;
    const char* mDebugFilename;
    SizeT       mDebugLine;
#endif
};

class LF_CORE_API NetConnectSuccessEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_CONNECT_SUCCESS;
    NetConnectSuccessEvent();

    ByteT mServerNonce[NET_HEARTBEAT_NONCE_SIZE];
};

class LF_CORE_API NetConnectFailedEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_CONNECT_FAILED;
    NetConnectFailedEvent();

    UInt32 mReason;
};

class LF_CORE_API NetConnectionCreatedEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_CONNECTION_CREATED;
    NetConnectionCreatedEvent();

    ConnectionID mConnectionID;
};

class LF_CORE_API NetConnectionTerminatedEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_CONNECTION_TERMINATED;
    NetConnectionTerminatedEvent();

    UInt32 mReason;
};

class LF_CORE_API NetHeartbeatReceivedEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_HEARTBEAT_RECEIVED;
    NetHeartbeatReceivedEvent();

    ConnectionID mSender;
    ByteT mNonce[NET_HEARTBEAT_NONCE_SIZE];
};

class LF_CORE_API NetDataReceivedRequestEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_DATA_RECEIVED_REQUEST;
    NetDataReceivedRequestEvent();
};

class LF_CORE_API NetDataReceivedResponseEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_DATA_RECEIVED_RESPONSE;
    NetDataReceivedResponseEvent();
};

class LF_CORE_API NetDataReceivedActionEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_DATA_RECEIVED_ACTION;
    NetDataReceivedActionEvent();
};

class LF_CORE_API NetDataReceivedReplicationEvent : public NetEvent
{
public:
    using Super = NetEvent;
    static const NetEventType::Value EVENT_TYPE = NetEventType::NET_EVENT_DATA_RECEIVED_REPLICATION;
    NetDataReceivedReplicationEvent();
};

#if defined(LF_NET_EVENT_DEBUG)
void NetDebugInfo(NetEvent* event, const char* filename, SizeT line)
{
    event->mDebugFilename = filename;
    event->mDebugLine = line;
}
#endif

} // namespace lf