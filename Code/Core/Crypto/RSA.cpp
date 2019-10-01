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
#include "RSA.h"
#include "Core/Common/Assert.h"
#include "Core/Common/Types.h"
#include "Core/String/String.h"
#include "Core/Memory/Memory.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Utility/ErrorCore.h"

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/sha.h>
#include <utility>

namespace lf {

namespace Crypto
{
static bool GenerateSHA256Hash(ByteT messageDigest[64], const ByteT* data, SizeT dataLength, bool generateSalt = false)
{
    SHA256_CTX ctx;
    if (SHA256_Init(&ctx) == 0)
    {
        return false;
    }

    if (generateSalt)
    {
        SecureRandomBytes(messageDigest, 32);
    }

    if (SHA256_Update(&ctx, messageDigest, 32) == 0)
    {
        return false;
    }

    if (SHA256_Update(&ctx, data, dataLength) == 0)
    {
        return false;
    }

    if (SHA256_Final(&messageDigest[32], &ctx) == 0)
    {
        return false;
    }
    return true;
}

RSAKey::RSAKey() 
    : mContext(nullptr)
{

}
RSAKey::RSAKey(const RSAKey& other)
    : mContext(nullptr)
{
    Copy(other);
}
RSAKey::RSAKey(RSAKey&& other)
    : mContext(nullptr)
{
    Move(std::forward<RSAKey&&>(other));
}
RSAKey::~RSAKey()
{
    Clear();
}

RSAKey& RSAKey::operator=(const RSAKey& other)
{
    Copy(other);
    return *this;
}

RSAKey& RSAKey::operator=(RSAKey&& other)
{
    Move(std::forward<RSAKey&&>(other));
    return *this;
}

bool RSAKey::GeneratePair(RSAKeySize keySize)
{
    int bits = 0;
    switch (keySize)
    {
        case RSA_KEY_1024: bits = 1024; break;
        case RSA_KEY_2048: bits = 2048; break;
        case RSA_KEY_4096: bits = 4096; break;
        default:
            return false;
    }

    // Allocate Context:
    RSA* context = reinterpret_cast<RSA*>(mContext);
    if (!context)
    {
        context = RSA_new();
        mContext = reinterpret_cast<RSAKeyContext*>(context);
    }

    if (!context)
    {
        return false;
    }

    // Create our big num
    BIGNUM* bn = BN_new();
    if (BN_set_word(bn, RSA_F4) == 0)
    {
        BN_free(bn);
        return false;
    }

    // Generate
    int result = RSA_generate_key_ex(context, bits, bn, NULL);

    // Cleanup
    BN_free(bn);
    return result == 1;
}

String RSAKey::GetPublicKey() const
{
    if (!HasPublicKey())
    {
        return String();
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        return String();
    }

    if (PEM_write_bio_RSAPublicKey(bio, reinterpret_cast<RSA*>(mContext)) == 0)
    {
    BIO_free_all(bio);
    return String();
    }

    BUF_MEM* memory = nullptr;
    if (BIO_flush(bio) == 0)
    {
        BIO_free_all(bio);
        return String();
    }

    if (BIO_get_mem_ptr(bio, &memory) == 0)
    {
        BIO_free_all(bio);
        return String();
    }

    String result;
    result.Resize(memory->length);
    memcpy(const_cast<char*>(result.CStr()), memory->data, memory->length);

    BIO_free_all(bio);
    return result;
}

String RSAKey::GetPrivateKey() const
{
    if (!HasPrivateKey())
    {
        return String();
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        return String();
    }

    if (PEM_write_bio_RSAPrivateKey(bio, reinterpret_cast<RSA*>(mContext), NULL, NULL, 0, NULL, NULL) == 0)
    {
        BIO_free_all(bio);
        return String();
    }

    BUF_MEM* memory = nullptr;
    if (BIO_flush(bio) == 0)
    {
        BIO_free_all(bio);
        return String();
    }

    if (BIO_get_mem_ptr(bio, &memory) == 0)
    {
        BIO_free_all(bio);
        return String();
    }

    String result;
    result.Resize(memory->length);
    memcpy(const_cast<char*>(result.CStr()), memory->data, memory->length);

    BIO_free_all(bio);
    return result;
}

bool RSAKey::LoadPublicKey(const String& key)
{
    RSA* context = reinterpret_cast<RSA*>(mContext);
    if (context)
    {
        RSA_free(context);
        mContext = nullptr;
    }

    if (key.Empty())
    {
        return false;
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        return false;
    }

    int keySize = static_cast<int>(key.Size());
    if (BIO_write(bio, key.CStr(), keySize) != keySize)
    {
        BIO_free_all(bio);
        return false;
    }

    if (BIO_flush(bio) == 0)
    {
        BIO_free_all(bio);
        return false;
    }

    context = RSA_new();
    mContext = reinterpret_cast<RSAKeyContext*>(context);
    bool result = PEM_read_bio_RSAPublicKey(bio, &context, NULL, NULL) == context;
    BIO_free_all(bio);
    return result && HasPublicKey();
}

bool RSAKey::LoadPrivateKey(const String& key)
{
    RSA* context = reinterpret_cast<RSA*>(mContext);
    if (context)
    {
        RSA_free(context);
        mContext = nullptr;
        context = nullptr;
    }

    if (key.Empty())
    {
        return false;
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        return false;
    }

    int keySize = static_cast<int>(key.Size());
    if (BIO_write(bio, key.CStr(), keySize) != keySize)
    {
        BIO_free_all(bio);
        return false;
    }

    if (BIO_flush(bio) == 0)
    {
        BIO_free_all(bio);
        return false;
    }


    // Read in the private key
    EVP_PKEY* pktmp;
    pktmp = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
    context = EVP_PKEY_get1_RSA(pktmp);
    mContext = reinterpret_cast<RSAKeyContext*>(context);
    EVP_PKEY_free(pktmp);
    BIO_free_all(bio);
    return HasPublicKey() && HasPrivateKey();
}

void RSAKey::Clear()
{
    if (mContext)
    {
        RSA_free(reinterpret_cast<RSA*>(mContext));
        mContext = nullptr;
    }
}

bool RSAKey::HasPublicKey() const
{
    if (!mContext)
    {
        return false;
    }

    const BIGNUM* n = nullptr;
    const BIGNUM* e = nullptr;
    const BIGNUM* d = nullptr;
    RSA_get0_key(reinterpret_cast<const RSA*>(mContext), &n, &e, &d);
    return e != nullptr;
}

bool RSAKey::HasPrivateKey() const
{
    if (!mContext)
    {
        return false;
    }

    const BIGNUM* n = nullptr;
    const BIGNUM* e = nullptr;
    const BIGNUM* d = nullptr;
    RSA_get0_key(reinterpret_cast<const RSA*>(mContext), &n, &e, &d);
    return d != nullptr;
}

RSAKeySize RSAKey::GetKeySize() const
{
    if (!mContext)
    {
        return RSA_KEY_Unknown;
    }

    int bits = RSA_size(reinterpret_cast<const RSA*>(mContext)) * 8;
    switch (bits)
    {
        case 1024: return RSA_KEY_1024;
        case 2048: return RSA_KEY_2048;
        case 4096: return RSA_KEY_4096;
        default:
            return RSA_KEY_Unknown;
    }
}

SizeT RSAKey::GetKeySizeBytes() const
{
    if (!mContext)
    {
        return 0;
    }

    int bytes = RSA_size(reinterpret_cast<const RSA*>(mContext));
    return bytes < 0 ? SizeT(0) : static_cast<SizeT>(bytes);
}

void RSAKey::Copy(const RSAKey& other)
{
    if (this == &other)
    {
        return;
    }

    Clear();
    LoadPublicKey(other.GetPublicKey());
    LoadPrivateKey(other.GetPrivateKey());
}

void RSAKey::Move(RSAKey&& other)
{
    mContext = other.mContext;
    other.mContext = nullptr;
}


bool RSAEncryptPublic(const RSAKey* key, const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity)
{
    if (!key || !inBytes || !outBytes || outBytesCapacity == 0)
    {
        return false;
    }

    // Must have public key:
    if (!key->HasPublicKey())
    {
        return false;
    }

    const int PADDING_BYTES = 42; // RSA_PKCS1_OAEP_PADDING
    const SizeT RSA_SIZE = key->GetKeySizeBytes();

    // Impossible to encrypt to output, increase the capacity
    if (outBytesCapacity < RSA_SIZE)
    {
        return false;
    }

    // Impossible to encrypt this message (too large)
    if (inBytesLength > (RSA_SIZE - PADDING_BYTES))
    {
        return false;
    }

    RSA* rsa = const_cast<RSA*>(reinterpret_cast<const RSA*>(key->GetContext()));
    int result = RSA_public_encrypt(static_cast<int>(inBytesLength), inBytes, outBytes, rsa, RSA_PKCS1_OAEP_PADDING);
    if (result == -1)
    {
        return false;
    }

    outBytesCapacity = static_cast<SizeT>(result);
    return true;
}

bool RSAEncryptPrivate(const RSAKey* key, const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity)
{
    if (!key || !inBytes || !outBytes || outBytesCapacity == 0)
    {
        return false;
    }

    // Must have public key:
    if (!key->HasPrivateKey())
    {
        return false;
    }

    const int PADDING_BYTES = 42; // RSA_PKCS1_OAEP_PADDING
    const SizeT RSA_SIZE = key->GetKeySizeBytes();

    // Impossible to encrypt to output, increase the capacity
    if (outBytesCapacity < RSA_SIZE)
    {
        return false;
    }

    // Impossible to encrypt this message (too large)
    if (inBytesLength >(RSA_SIZE - PADDING_BYTES))
    {
        return false;
    }

    RSA* rsa = const_cast<RSA*>(reinterpret_cast<const RSA*>(key->GetContext()));
    int result = RSA_private_encrypt(static_cast<int>(inBytesLength), inBytes, outBytes, rsa, RSA_PKCS1_PADDING);
    if (result == -1)
    {
        return false;
    }

    outBytesCapacity = static_cast<SizeT>(result);
    return true;
}

bool RSADecryptPublic(const RSAKey* key, const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity)
{
    if (!key || !inBytes || inBytesLength == 0 || !outBytes || outBytesCapacity == 0)
    {
        return false;
    }

    // Must have public key:
    if (!key->HasPublicKey())
    {
        return false;
    }

    const int PADDING_BYTES = 42; // RSA_PKCS1_OAEP_PADDING
    const SizeT RSA_SIZE = key->GetKeySizeBytes();
    const SizeT MAX_MESSAGE_SIZE = RSA_SIZE - static_cast<SizeT>(PADDING_BYTES);

    // Not enough room to allocate the possible message
    if (outBytesCapacity < MAX_MESSAGE_SIZE)
    {
        return false;
    }

    // Message is corrupted?
    if (inBytesLength != RSA_SIZE)
    {
        return false;
    }

    RSA* rsa = const_cast<RSA*>(reinterpret_cast<const RSA*>(key->GetContext()));
    int result = RSA_public_decrypt(static_cast<int>(inBytesLength), inBytes, outBytes, rsa, RSA_PKCS1_PADDING);
    if (result == -1)
    {
        return false;
    }
    outBytesCapacity = static_cast<SizeT>(result);
    return true;
}
bool RSADecryptPrivate(const RSAKey* key, const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity)
{
    if (!key || !inBytes || inBytesLength == 0 || !outBytes || outBytesCapacity == 0)
    {
        return false;
    }

    // Must have public key:
    if (!key->HasPrivateKey())
    {
        return false;
    }

    const int PADDING_BYTES = 42; // RSA_PKCS1_OAEP_PADDING
    const SizeT RSA_SIZE = key->GetKeySizeBytes();
    const SizeT MAX_MESSAGE_SIZE = RSA_SIZE - static_cast<SizeT>(PADDING_BYTES);

    // Not enough room to allocate the possible message
    if (outBytesCapacity < MAX_MESSAGE_SIZE)
    {
        return false;
    }

    // Message is corrupted?
    if (inBytesLength != RSA_SIZE)
    {
        return false;
    }

    RSA* rsa = const_cast<RSA*>(reinterpret_cast<const RSA*>(key->GetContext()));
    int result = RSA_private_decrypt(static_cast<int>(inBytesLength), inBytes, outBytes, rsa, RSA_PKCS1_OAEP_PADDING);
    if (result == -1)
    {
        return false;
    }
    outBytesCapacity = static_cast<SizeT>(result);
    return true;
}

bool RSAEncryptPublic(const RSAKey* key, const String& input, String& output)
{
    if (!key || !key->HasPublicKey())
    {
        return false;
    }

    output.Resize(key->GetKeySizeBytes());
    const ByteT* inBytes = reinterpret_cast<const ByteT*>(input.CStr());
    SizeT inBytesLength = input.Size() + 1;
    ByteT* outBytes = reinterpret_cast<ByteT*>(const_cast<char*>(output.CStr()));
    SizeT outBytesLength = output.Size();
    bool result = RSAEncryptPublic(key, inBytes, inBytesLength, outBytes, outBytesLength);
    output.Resize(outBytesLength);
    return result;
}
bool RSAEncryptPrivate(const RSAKey* key, const String& input, String& output)
{
    if (!key || !key->HasPrivateKey())
    {
        return false;
    }

    output.Resize(key->GetKeySizeBytes());
    const ByteT* inBytes = reinterpret_cast<const ByteT*>(input.CStr());
    SizeT inBytesLength = input.Size() + 1;
    ByteT* outBytes = reinterpret_cast<ByteT*>(const_cast<char*>(output.CStr()));
    SizeT outBytesLength = output.Size();
    bool result = RSAEncryptPrivate(key, inBytes, inBytesLength, outBytes, outBytesLength);
    output.Resize(outBytesLength);
    return result;
}
bool RSADecryptPublic(const RSAKey* key, const String& input, String& output)
{
    if (!key || !key->HasPublicKey())
    {
        return false;
    }

    output.Resize(key->GetKeySizeBytes());
    const ByteT* inBytes = reinterpret_cast<const ByteT*>(input.CStr());
    SizeT inBytesLength = input.Size();
    ByteT* outBytes = reinterpret_cast<ByteT*>(const_cast<char*>(output.CStr()));
    SizeT outBytesLength = output.Size();
    bool result = RSADecryptPublic(key, inBytes, inBytesLength, outBytes, outBytesLength);
    if (result)
    {
        output.Resize(outBytesLength - 1);
    }
    return result;
}
bool RSADecryptPrivate(const RSAKey* key, const String& input, String& output)
{
    if (!key || !key->HasPrivateKey())
    {
        return false;
    }

    output.Resize(key->GetKeySizeBytes());
    const ByteT* inBytes = reinterpret_cast<const ByteT*>(input.CStr());
    SizeT inBytesLength = input.Size();
    ByteT* outBytes = reinterpret_cast<ByteT*>(const_cast<char*>(output.CStr()));
    SizeT outBytesLength = output.Size();
    bool result = RSADecryptPrivate(key, inBytes, inBytesLength, outBytes, outBytesLength);
    if (result)
    {
        output.Resize(outBytesLength - 1);
    }
    return result;
}

bool RSASignPublic(const RSAKey* key, const ByteT* data, SizeT dataLength, String& outSignature)
{
    const SizeT DIGEST_LENGTH = 64;

    ByteT messageDigest[DIGEST_LENGTH];
    if (!GenerateSHA256Hash(messageDigest, data, dataLength, true))
    {
        return false;
    }

    // Encrypt:
    outSignature.Resize(key->GetKeySizeBytes());
    ByteT* outBytes = reinterpret_cast<ByteT*>(const_cast<char*>(outSignature.CStr()));
    SizeT outBytesLength = outSignature.Size();
    bool result = RSAEncryptPublic(key, messageDigest, DIGEST_LENGTH, outBytes, outBytesLength);
    if (result)
    {
        outSignature.Resize(outBytesLength);
    }
    return result;
}

bool RSASignPrivate(const RSAKey* key, const ByteT* data, SizeT dataLength, String& outSignature)
{
    const SizeT DIGEST_LENGTH = 64;
    ByteT messageDigest[DIGEST_LENGTH];
    if (!GenerateSHA256Hash(messageDigest, data, dataLength, true))
    {
        return false;
    }

    // Encrypt:
    outSignature.Resize(key->GetKeySizeBytes());
    ByteT* outBytes = reinterpret_cast<ByteT*>(const_cast<char*>(outSignature.CStr()));
    SizeT outBytesLength = outSignature.Size();
    bool result = RSAEncryptPrivate(key, messageDigest, DIGEST_LENGTH, outBytes, outBytesLength);
    if (result)
    {
        outSignature.Resize(outBytesLength);
    }
    return result;
}

bool RSAVerifyPublic(const RSAKey* key, const ByteT* data, SizeT dataLength, const String& signature)
{
    const SizeT DIGEST_LENGTH = 64;
    const SizeT MAX_KEY_BYTES = 512;

    ByteT hash[MAX_KEY_BYTES];
    const ByteT* inBytes = reinterpret_cast<const ByteT*>(signature.CStr());
    SizeT outByteLength = LF_ARRAY_SIZE(hash);
    if (!RSADecryptPublic(key, inBytes, signature.Size(), hash, outByteLength) || outByteLength != DIGEST_LENGTH)
    {
        return false;
    }

    ByteT messageDigest[DIGEST_LENGTH];
    // copy salt
    memcpy(messageDigest, hash, 32);
    if (!GenerateSHA256Hash(messageDigest, data, dataLength))
    {
        return false;
    }
    
    return memcmp(messageDigest, hash, DIGEST_LENGTH) == 0;
}

bool RSAVerifyPrivate(const RSAKey* key, const ByteT* data, SizeT dataLength, const String& signature)
{
    const SizeT DIGEST_LENGTH = 64;
    const SizeT MAX_KEY_BYTES = 512;

    ByteT hash[MAX_KEY_BYTES];
    const ByteT* inBytes = reinterpret_cast<const ByteT*>(signature.CStr());
    SizeT outByteLength = LF_ARRAY_SIZE(hash);
    if (!RSADecryptPrivate(key, inBytes, signature.Size(), hash, outByteLength) || outByteLength != DIGEST_LENGTH)
    {
        return false;
    }

    ByteT messageDigest[DIGEST_LENGTH];
    // copy salt
    memcpy(messageDigest, hash, 32);
    if (!GenerateSHA256Hash(messageDigest, data, dataLength))
    {
        return false;
    }
    
    return memcmp(messageDigest, hash, DIGEST_LENGTH) == 0;
}

bool RSASignPublic(const RSAKey* key, const String& input, String& outSignature)
{
    return RSASignPublic(key, reinterpret_cast<const ByteT*>(input.CStr()), input.Size(), outSignature);
}
bool RSASignPrivate(const RSAKey* key, const String& input, String& outSignature)
{
    return RSASignPrivate(key, reinterpret_cast<const ByteT*>(input.CStr()), input.Size(), outSignature);
}
bool RSAVerifyPublic(const RSAKey* key, const String& input, const String& signature)
{
    return RSAVerifyPublic(key, reinterpret_cast<const ByteT*>(input.CStr()), input.Size(), signature);
}
bool RSAVerifyPrivate(const RSAKey* key, const String& input, const String& signature)
{
    return RSAVerifyPrivate(key, reinterpret_cast<const ByteT*>(input.CStr()), input.Size(), signature);
}

} // namespace Crypto
} // namespace lf