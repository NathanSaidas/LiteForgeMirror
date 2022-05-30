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

#include "Core/Crypto/CryptoSerialization.h"
#include "Core/Net/NetTypes.h"

namespace lf {

class SessionIDSerialized
{
public:
    SessionIDSerialized() : mItem(nullptr) {}
    SessionIDSerialized(SessionID* item) : mItem(item) {}

    SessionID* mItem;
};
LF_RUNTIME_API Stream& operator<<(Stream& s, SessionIDSerialized& o);

struct NetOneTimeKeyMsg
{
    Crypto::AES256KeySerialized mOneTimeKey;
    Crypto::AESIVSerialized     mOneTimeIV;
};
LF_RUNTIME_API Stream& operator<<(Stream& s, NetOneTimeKeyMsg& o);

struct NetClientHelloMsg
{
    Crypto::ECDHPublicKeySerialized mClientHandshakeKey;
    Crypto::ECDHPublicKeySerialized mClientHandshakeHmac;
    Crypto::RSA2048PublicKeySerialized mClientSigningKey;
};
LF_RUNTIME_API Stream& operator<<(Stream& s, NetClientHelloMsg& o);

struct NetServerHelloRSAMsg
{
    Crypto::ECDHPublicKeySerialized mServerHandshakeKey;
    Crypto::AESIVSerialized         mIV;
};
LF_RUNTIME_API Stream& operator<<(Stream& s, NetServerHelloRSAMsg& o);

struct NetServerHelloMsg
{
    SessionIDSerialized mSessionID;
    Crypto::ECDHPublicKeySerialized mServerHandshakeHMAC;
    Crypto::RSA2048PublicKeySerialized mServerSigningKey;
};
LF_RUNTIME_API Stream& operator<<(Stream& s, NetServerHelloMsg& o);


namespace NetSerialization
{
    LF_RUNTIME_API bool ReadAllBytes(const ByteT* bytes, SizeT numBytes, NetOneTimeKeyMsg& msg);
    LF_RUNTIME_API bool ReadAllBytes(const ByteT* bytes, SizeT numBytes, NetClientHelloMsg& msg);
    LF_RUNTIME_API bool ReadAllBytes(const ByteT* bytes, SizeT numBytes, NetServerHelloRSAMsg& msg);
    LF_RUNTIME_API bool ReadAllBytes(const ByteT* bytes, SizeT numBytes, NetServerHelloMsg& msg);


    LF_RUNTIME_API bool WriteAllBytes(ByteT* bytes, SizeT& numBytes, NetOneTimeKeyMsg& msg);
    LF_RUNTIME_API bool WriteAllBytes(ByteT* bytes, SizeT& numBytes, NetClientHelloMsg& msg);
    LF_RUNTIME_API bool WriteAllBytes(ByteT* bytes, SizeT& numBytes, NetServerHelloRSAMsg& msg);
    LF_RUNTIME_API bool WriteAllBytes(ByteT* bytes, SizeT& numBytes, NetServerHelloMsg& msg);
}

}