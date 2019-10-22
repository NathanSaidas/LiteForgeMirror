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

#ifndef LF_CORE_AES_H
#define LF_CORE_AES_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/String/String.h"

namespace lf { namespace Crypto {

enum AESKeySize
{
    AES_KEY_128,
    AES_KEY_256,

    AES_KEY_Unknown
};

const SizeT AES_MAX_KEY_SIZE = 32;
const SizeT AES_IV_SIZE = 16;

class LF_CORE_API AESKey
{
public:
    AESKey();
    AESKey(const AESKey& other);
    AESKey(AESKey&& other);
    ~AESKey();

    AESKey& operator=(const AESKey& other);
    AESKey& operator=(AESKey&& other);

    bool Generate(AESKeySize keySize);
    bool Load(AESKeySize keySize, const ByteT* key);

    void Clear();
    AESKeySize GetKeySize() const { return mKeySize; }
    SizeT GetKeySizeBytes() const { return mKeySize == AES_KEY_256 ? (256 / 8) : (mKeySize == AES_KEY_128 ? (128 / 8) : 0); }

    const ByteT* GetKey() const { return mKey; }
private:
    AESKeySize mKeySize;
    ByteT mKey[256 / 8];
};

LF_CORE_API bool AESEncrypt(const AESKey* key, const ByteT iv[16], const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity);
LF_CORE_API bool AESDecrypt(const AESKey* key, const ByteT iv[16], const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity);
LF_CORE_API bool AESEncrypt(const AESKey* key, const ByteT iv[16], const String& inMessage, String& outMessage);
LF_CORE_API bool AESDecrypt(const AESKey* key, const ByteT iv[16], const String& inMessage, String& outMessage);
LF_CORE_API SizeT AESCipherTextLength(const AESKey* key, SizeT plainTextLength);

} // namespace Crypto
} // namespace lf

#endif // LF_CORE_AES_H