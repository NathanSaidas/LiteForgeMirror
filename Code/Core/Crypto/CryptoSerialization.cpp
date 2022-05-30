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
#include "Core/PCH.h"
#include "CryptoSerialization.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/ECDH.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Crypto/RSA.h"
#include "Core/IO/Stream.h"

namespace lf {
namespace Crypto {

Stream& operator<<(Stream& s, AES256KeySerialized& o)
{
    if (o.mItem)
    {
        if (!s.IsReading())
        {
            Assert(o.mItem->GetKeySize() == Crypto::AES_KEY_256);
            s.SerializeGuid(o.mItem->Bytes(), o.mItem->Size());
        }
        else
        {
            ByteT bytes[32];// todo: Crypto::AES figure out better way to get static keysize
            s.SerializeGuid(bytes, sizeof(bytes));
            Assert(o.mItem->Load(Crypto::AES_KEY_256, bytes));
            Assert(o.mItem->GetKeySize() == Crypto::AES_KEY_256);
        }
    }
    return s;
}

Stream& operator<<(Stream& s, AESIVSerialized& o)
{
    if (o.mItem)
    {
        s.SerializeGuid(o.mItem->mBytes, sizeof(o.mItem->mBytes));
    }
    return s;
}

Stream& operator<<(Stream& s, HMACKeySerialized& o)
{
    if (o.mItem)
    {
        s.SerializeGuid(o.mItem->Bytes(), o.mItem->Size());
    }
    return s;
}

Stream& operator<<(Stream& s, RSA2048PublicKeySerialized & o)
{
    if (o.mItem)
    {
        String keyString;
        if (s.IsReading())
        {
            s.Serialize(keyString);
            o.mError = !o.mItem->LoadPublicKey(keyString);
            Assert(o.mItem->GetKeySize() == Crypto::RSA_KEY_2048);
        }
        else
        {
            Assert(o.mItem->GetKeySize() == Crypto::RSA_KEY_2048);
            keyString = o.mItem->GetPublicKey();
            s.Serialize(keyString);
        }
    }
    return s;
}

Stream& operator<<(Stream& s, RSA2048PrivateKeySerialized & o)
{
    if (o.mItem)
    {
        String keyString;
        if (s.IsReading())
        {
            s.Serialize(keyString);
            o.mError = !o.mItem->LoadPrivateKey(keyString);
            Assert(o.mItem->GetKeySize() == Crypto::RSA_KEY_2048);
        }
        else
        {
            Assert(o.mItem->GetKeySize() == Crypto::RSA_KEY_2048);
            keyString = o.mItem->GetPrivateKey();
            s.Serialize(keyString);
        }
    }
    return s;
}

Stream& operator<<(Stream& s, ECDHPublicKeySerialized& o)
{
    if (o.mItem)
    {
        String keyString;
        if (s.IsReading())
        {
            s.Serialize(keyString);
            o.mError = !o.mItem->LoadPublicKey(keyString);
        }
        else
        {
            keyString = o.mItem->GetPublicKey();
            s.Serialize(keyString);
        }
    }
    return s;
}

Stream& operator<<(Stream& s, ECDHPrivateKeySerialized& o)
{
    if (o.mItem)
    {
        String keyString;
        if (s.IsReading())
        {
            s.Serialize(keyString);
            o.mError = !o.mItem->LoadPrivateKey(keyString);
        }
        else
        {
            keyString = o.mItem->GetPrivateKey();
            s.Serialize(keyString);
        }
    }
    return s;
}

} // namespace Crypto
} // namespace lf
