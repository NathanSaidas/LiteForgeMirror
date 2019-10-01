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

#include "AES.h"
#include "Core/Crypto/SecureRandom.h"

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <utility>

namespace lf {

namespace Crypto
{
AESKey::AESKey() 
    : mKeySize(AES_KEY_Unknown)
{
    memset(mKey, 0, sizeof(mKey));
}
AESKey::AESKey(const AESKey& other)
    : mKeySize(other.mKeySize)
{
    memcpy(mKey, other.mKey, sizeof(mKey));
}
AESKey::AESKey(AESKey&& other)
    : mKeySize(other.mKeySize)
{
    memcpy(mKey, other.mKey, sizeof(mKey));
    memset(other.mKey, 0, sizeof(mKey));
    other.Clear();
}
AESKey::~AESKey()
{
    Clear();
}

AESKey& AESKey::operator=(const AESKey& other)
{
    mKeySize = other.mKeySize;
    memcpy(mKey, other.mKey, sizeof(mKey));
    return *this;
}
AESKey& AESKey::operator=(AESKey&& other)
{
    mKeySize = other.mKeySize;
    memcpy(mKey, other.mKey, sizeof(mKey));
    other.Clear();
    return *this;
}

bool AESKey::Generate(AESKeySize keySize)
{
    Clear();
    switch (keySize)
    {
        case AES_KEY_128:
            SecureRandomBytes(mKey, (128 / 8));
            mKeySize = keySize;
            break;
        case AES_KEY_256:
            SecureRandomBytes(mKey, (256 / 8));
            mKeySize = keySize;
            break;
        default:
            return false;
    }
    return true;
}

bool AESKey::Load(AESKeySize keySize, const ByteT* key)
{
    Clear();
    switch (keySize)
    {
        case AES_KEY_128:
            memcpy(mKey, key, 128 / 8);
            mKeySize = keySize;
            break;
        case AES_KEY_256:
            memcpy(mKey, key, 256 / 8);
            mKeySize = keySize;
            break;
        default:
            return false;
    }
    return true;
}

void AESKey::Clear()
{
    memset(mKey, 0, sizeof(mKey));
    mKeySize = AES_KEY_Unknown;
}

static SizeT CalculateCipherTextLength(const AESKey* key, SizeT bytes)
{
    SizeT N = 0;  

    switch (key->GetKeySize())
    {
        case AES_KEY_128:
        case AES_KEY_256: // AES 256 block size is 16 bytes as well.
            N = bytes / 16;
            return (N * 16) == bytes ? bytes : 16 * (N + 1);
        default:
            return 0;
    }
}

bool AESEncrypt(const AESKey* key, const ByteT iv[16], const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity)
{
    // Invalid arguments
    if (!key || !inBytes || inBytesLength == 0 || !outBytes || outBytesCapacity == 0)
    {
        return false;
    }

    // Invalid key type
    if (key->GetKeySize() == AES_KEY_Unknown)
    {
        return false;
    }

    // Not enough memory to encrypt
    // todo: If the inBytesLength == outBytesCapacity we might overflow as this adds an extra 16 bytes of padding for some reason
    // eg Encrypt(in_1024, out_1024)
    if (outBytesCapacity < CalculateCipherTextLength(key, inBytesLength))
    {
        return false;
    }
    
    EVP_CIPHER_CTX *ctx = nullptr;
    int len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, key->GetKeySize() == AES_KEY_128 ? EVP_aes_128_cbc() : EVP_aes_256_cbc(), NULL, key->GetKey(), iv) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_EncryptUpdate(ctx, outBytes, &len, inBytes, static_cast<int>(inBytesLength)) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    outBytesCapacity = len;

    if (EVP_EncryptFinal_ex(ctx, outBytes + len, &len) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    outBytesCapacity += len;

    EVP_CIPHER_CTX_free(ctx);

    return true;

}
bool AESDecrypt(const AESKey* key, const ByteT iv[16], const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity)
{
    if (!key || !inBytes || inBytesLength == 0 || !outBytes || outBytesCapacity == 0)
    {
        return false;
    }

    if (key->GetKeySize() == AES_KEY_Unknown)
    {
        return false;
    }

    // Not enough potential memory
    if (outBytesCapacity < inBytesLength)
    {
        return false;
    }

    EVP_CIPHER_CTX *ctx = nullptr;
    int len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, key->GetKeySize() == AES_KEY_128 ? EVP_aes_128_cbc() : EVP_aes_256_cbc(), NULL, key->GetKey(), iv) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (EVP_DecryptUpdate(ctx, outBytes, &len, inBytes, static_cast<int>(inBytesLength)) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    outBytesCapacity = len;

    if (EVP_DecryptFinal_ex(ctx, outBytes + len, &len) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    outBytesCapacity += len;

    EVP_CIPHER_CTX_free(ctx);
    return true;
}



bool AESEncrypt(const AESKey* key, const ByteT iv[16], const String& inMessage, String& outMessage)
{
    if (!key || inMessage.Empty())
    {
        return false;
    }

    const ByteT* inBytes = reinterpret_cast<const ByteT*>(inMessage.CStr());
    outMessage.Resize(CalculateCipherTextLength(key, inMessage.Size()));
    ByteT* outBytes = reinterpret_cast<ByteT*>(const_cast<char*>(outMessage.CStr()));
    SizeT returnedBytes = outMessage.Size();
    bool result = AESEncrypt(key, iv, inBytes, inMessage.Size(), outBytes, returnedBytes);
    return result;
}
bool AESDecrypt(const AESKey* key, const ByteT iv[16], const String& inMessage, String& outMessage)
{
    if (!key || inMessage.Empty())
    {
        return false;
    }

    const ByteT* inBytes = reinterpret_cast<const ByteT*>(inMessage.CStr());
    outMessage.Resize(CalculateCipherTextLength(key, inMessage.Size()));
    ByteT* outBytes = reinterpret_cast<ByteT*>(const_cast<char*>(outMessage.CStr()));
    SizeT returnedBytes = outMessage.Size();
    bool result = AESDecrypt(key, iv, inBytes, inMessage.Size(), outBytes, returnedBytes);
    outMessage.Resize(returnedBytes);
    return result;
}

SizeT AESCipherTextLength(const AESKey* key, SizeT plainTextLength)
{
    return CalculateCipherTextLength(key, plainTextLength);
}

} // namespace Crypto
} // namespace lf