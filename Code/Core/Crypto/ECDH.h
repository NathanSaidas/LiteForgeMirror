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
#include "Core/String/String.h"

namespace lf {

namespace Crypto {

// This class provides the key generation/loading implementation for Elliptic curve diffie hellman
class LF_CORE_API ECDHKey
{
public:
    ECDHKey();
    ECDHKey(const ECDHKey& other);
    ECDHKey(ECDHKey&& other);
    ~ECDHKey();

    ECDHKey& operator=(const ECDHKey& other);
    ECDHKey& operator=(ECDHKey&& other);

    bool Generate();
    bool LoadPublicKey(const String& string);
    bool LoadPrivateKey(const String& string);
    void Clear();

    String GetPublicKey() const;
    String GetPrivateKey() const;

    void* GetKey() { return mKey; }
private:
    void* mKey;
};

// This method is used to derive a set of bytes to use in the AES 256 key.
LF_CORE_API SizeT ECDHDerive(ECDHKey* localKey, ECDHKey* peerKey, ByteT* outBuffer, SizeT bufferSize);

} // namespace Crypto
} // namespace lf