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

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Memory/SmartPointer.h"

namespace lf {

DECLARE_ATOMIC_WPTR(NetConnection);
class NetDriver;

namespace NetMessageDataError
{
    enum Value
    {
        // ** The NetDriver could not validate the header hmac
        DATA_ERROR_INVALID_HEADER_HMAC,
        // ** The NetDriver could not validate the data signature
        DATA_ERROR_INVALID_SIGNATURE,
        // ** The NetDriver could not validate the data hmac
        DATA_ERROR_INVALID_HMAC,
        // ** The NetDriver could not retrieve the data. (Perhaps invalid buffer size)
        DATA_ERROR_DATA_RETRIEVAL,
        // ** The NetDriver could not decrypt the data.
        DATA_ERROR_DATA_DECRYPTION,

        MAX_VALUE,
        INVALID_ENUM = MAX_VALUE
    };
}

struct NetMessageDataArgs
{
    LF_INLINE NetMessageDataArgs() 
    : mAppData(nullptr)
    , mAppDataSize(0)
    , mConnection()
    , mSignatureVerified(false)
    , mHmacVerified(false)
    , mEncrypted(false)
    {}

    // ** A pointer to the application data in the request. The data provided is already decrypted and ready to go.
    const ByteT*             mAppData;
    // ** The number of bytes contained in the 'mAppData' variable
    SizeT                    mAppDataSize;
    // ** Pointer to the connection of who sent the data.
    NetConnectionAtomicWPtr  mConnection;
    // ********************************** 
    // This variable is true if the NetDriver has verified the signature
    // If the packet flags did not contain the SIGNED flag then the driver will not
    // make an attempt to verify it.
    // ********************************** 
    bool                     mSignatureVerified;
    // ********************************** 
    // This variable is true if the NetDriver has verified the hmac
    // If the packet flags did not contain the HMAC flag then the driver will not
    // make an attempt to verify it.
    // ********************************** 
    bool                     mHmacVerified;
    // ********************************** 
    // This variable is true if the NetDriver decrypted the data.
    // If the packet flags did not contain the %TEMP:SECURE% flag then driver will
    // not make an attempt to decrypt it.
    // ********************************** 
    bool                     mEncrypted;
};

struct NetMessageDataErrorArgs
{
    // ** A pointer to the raw packet data in the request.
    const ByteT*            mPacketData;
    // ** The number of bytes contained in the 'mPacketData' variable
    SizeT                   mPacketDataLength;
    // ** A pointer to the connection of who sent the data.
    NetConnectionAtomicWPtr mConnection;
    // ** The type of error that occured.
    NetMessageDataError::Value mError;
};

// ********************************** 
// The NetMessageController class is charged with the role of processing 
// all of the standard network message requests.
//
// ********************************** 
class LF_RUNTIME_API NetMessageController : public TWeakPointerConvertible<NetMessageController>
{
public:
    virtual void OnInitialize(NetDriver* driver) = 0;
    virtual void OnShutdown() = 0;
    // ********************************** 
    // This event is fired when a new connection has been registered with the NetDriver
    // ********************************** 
    virtual void OnConnect(NetConnection* connection) = 0;
    // ********************************** 
    // This event is fired when a connection has been terminated by the NetDriver
    // ********************************** 
    virtual void OnDisconnect(NetConnection* connection) = 0;
    // ********************************** 
    // This event is fired anytime there is a message sent on behalf of an existing NetConnection
    // ********************************** 
    virtual void OnMessageData(NetMessageDataArgs& args) = 0;
    // ********************************** 
    // This event is fired if the NetDriver fails to process a message. (OnMessageData will not be called)
    // ********************************** 
    virtual void OnMessageDataError(NetMessageDataErrorArgs& args) = 0;
};

}