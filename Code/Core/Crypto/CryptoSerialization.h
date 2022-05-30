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
#pragma once
#include "Core/Common/Types.h"
#include "Core/Common/API.h"

// This file provides the standard interface for serializing the cryptographic primitives.
// Currently Crypto is only compatible with binary streams.

namespace lf {
class Stream;
namespace Crypto {

class AESKey;
class AESIV;
class HMACKey;
class RSAKey;
class ECDHKey;


template<typename T>
struct TSerializeableCrypto
{
    TSerializeableCrypto() : mItem(nullptr) {}
    TSerializeableCrypto(T* item) : mItem(item) {}

    T* mItem;
};

class AES256KeySerialized : public TSerializeableCrypto<Crypto::AESKey>
{
public:
    using Super = TSerializeableCrypto<Crypto::AESKey>;
    AES256KeySerialized() : Super() {}
    AES256KeySerialized(Crypto::AESKey* key) : Super(key) {}
};
LF_CORE_API Stream& operator<<(Stream& s, AES256KeySerialized& o);

class AESIVSerialized : public TSerializeableCrypto<Crypto::AESIV>
{
public:
    using Super = TSerializeableCrypto<Crypto::AESIV>;
    AESIVSerialized() : Super() {}
    AESIVSerialized(Crypto::AESIV* iv) : Super(iv) {}
};
LF_CORE_API Stream& operator<<(Stream& s, AESIVSerialized& o);

class HMACKeySerialized : public TSerializeableCrypto<Crypto::HMACKey>
{
public:
    using Super = TSerializeableCrypto<Crypto::HMACKey>;
    HMACKeySerialized() : Super() {}
    HMACKeySerialized(Crypto::HMACKey* key) : Super(key) {}
};
LF_CORE_API Stream& operator<<(Stream& s, HMACKeySerialized& o);

class RSA2048PublicKeySerialized : public TSerializeableCrypto<Crypto::RSAKey>
{
public:
    using Super = TSerializeableCrypto<Crypto::RSAKey>;
    RSA2048PublicKeySerialized() : Super() {}
    RSA2048PublicKeySerialized(Crypto::RSAKey* key) : Super(key), mError(false) {}

    bool mError;
};
LF_CORE_API Stream& operator<<(Stream& s, RSA2048PublicKeySerialized & o);

class RSA2048PrivateKeySerialized : public TSerializeableCrypto<Crypto::RSAKey>
{
public:
    using Super = TSerializeableCrypto<Crypto::RSAKey>;
    RSA2048PrivateKeySerialized() : Super() {}
    RSA2048PrivateKeySerialized(Crypto::RSAKey* key) : Super(key), mError(false) {}

    bool mError;
};
LF_CORE_API Stream& operator<<(Stream& s, RSA2048PrivateKeySerialized & o);

class ECDHPublicKeySerialized : public TSerializeableCrypto<Crypto::ECDHKey>
{
public:
    using Super = TSerializeableCrypto<Crypto::ECDHKey>;
    ECDHPublicKeySerialized() : Super() {}
    ECDHPublicKeySerialized(Crypto::ECDHKey* key) : Super(key), mError(false) {}

    bool mError;
};
LF_CORE_API Stream& operator<<(Stream& s, ECDHPublicKeySerialized& o);

class ECDHPrivateKeySerialized : public TSerializeableCrypto<Crypto::ECDHKey>
{
public:
    using Super = TSerializeableCrypto<Crypto::ECDHKey>;
    ECDHPrivateKeySerialized() : Super() {}
    ECDHPrivateKeySerialized(Crypto::ECDHKey* key) : Super(key), mError(false) {};

    bool mError;
};
LF_CORE_API Stream& operator<<(Stream& s, ECDHPrivateKeySerialized& o);

} // namespace Crypto
} // namespace lf