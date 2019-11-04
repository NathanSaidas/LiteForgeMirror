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
#include "NetEvent.h"

namespace lf {

NetEvent::NetEvent(NetEventType::Value type)
: mType(type)
{}
NetEvent::NetEvent(const NetEvent& other)
: mType(other.mType)
{}
NetEvent::NetEvent(NetEvent&& other)
: mType(other.mType)
{
    // We don't do this as mType represents our 'class type'
    // other.mType = 
}

NetEvent& NetEvent::operator=(const NetEvent&)
{
    // Type may only be initialized from constructor
    return *this;
}
NetEvent& NetEvent::operator=(NetEvent&&)
{
    // Type may only be initialized from constructor
    return *this;
}


NetConnectSuccessEvent::NetConnectSuccessEvent()
: Super(NetConnectSuccessEvent::EVENT_TYPE)
{
    memset(mServerNonce, 0, sizeof(mServerNonce));
}

NetConnectFailedEvent::NetConnectFailedEvent()
: Super(NetConnectFailedEvent::EVENT_TYPE)
, mReason(0)
{

}

NetConnectionCreatedEvent::NetConnectionCreatedEvent()
: Super(NetConnectionCreatedEvent::EVENT_TYPE)
, mConnectionID(INVALID_CONNECTION)
{

}

NetConnectionTerminatedEvent::NetConnectionTerminatedEvent()
: Super(NetConnectionTerminatedEvent::EVENT_TYPE)
, mReason(0)
{

}

NetHeartbeatReceivedEvent::NetHeartbeatReceivedEvent()
: Super(NetHeartbeatReceivedEvent::EVENT_TYPE)
{
    memset(mNonce, 0, sizeof(mNonce));
}

NetDataReceivedRequestEvent::NetDataReceivedRequestEvent()
: Super(NetDataReceivedRequestEvent::EVENT_TYPE)
{

}

NetDataReceivedResponseEvent::NetDataReceivedResponseEvent()
: Super(NetDataReceivedResponseEvent::EVENT_TYPE)
{

}

NetDataReceivedActionEvent::NetDataReceivedActionEvent()
: Super(NetDataReceivedActionEvent::EVENT_TYPE)
{

}

NetDataReceivedReplicationEvent::NetDataReceivedReplicationEvent()
: Super(NetDataReceivedReplicationEvent::EVENT_TYPE)
{

}

} // namespace lf 