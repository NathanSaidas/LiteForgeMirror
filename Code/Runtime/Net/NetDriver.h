#pragma once
// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#include "Core/Utility/SmartCallback.h"
#include "Runtime/Net/NetEvent.h"

namespace lf {

class NetConnection;
class NetMessageController;

class LF_RUNTIME_API NetDriver
{
public:
    using OnSendSuccess = TCallback<void>;
    using OnSendFailed = TCallback<void>;

    enum Options
    {
        OPTION_ENCRYPT  = 1 << 0,
        OPTION_HMAC     = 1 << 1,
        OPTION_SIGNED   = 1 << 2,
        OPTION_RELIABLE = 1 << 3
    };
    friend static inline Options operator|(Options a, Options b) { return (Options)((int)a | (int)b); }
    friend static inline Options operator&(Options a, Options b) { return (Options)((int)a & (int)b); }

    enum MessageType
    {
        MESSAGE_REQUEST,
        MESSAGE_RESPONSE,
        MESSAGE_GENERIC,
        MESSAGE_FILE_TRANSFER,

        MAX_VALUE,
        INVALID_ENUM = MAX_VALUE
    };

    virtual void SetMessageController(MessageType messageType, NetMessageController* controller) = 0;

    virtual bool Send(
        MessageType message,
        Options options,
        const ByteT* bytes,
        const SizeT numBytes,
        OnSendSuccess onSuccess,
        OnSendFailed onFailed) = 0;

    virtual bool Send(
        MessageType message,
        Options options,
        const ByteT* bytes,
        const SizeT numBytes,
        NetConnection* connection,
        OnSendSuccess onSuccess,
        OnSendFailed onFailed) = 0;

    virtual bool IsServer() const = 0;
    virtual bool IsClient() const = 0;
};

} // namespace lf