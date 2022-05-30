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
#include "ECDH.h"
#include "Core/Common/Assert.h"

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <memory>

namespace lf {
namespace Crypto {

// https://github.com/MonobitInc/Small-OpenSSL-ECDH-example/blob/master/OpenSSL_ECDH.cpp
// https://github.com/openssl/openssl/blob/master/crypto/ec/curve448/ed448.h
// https://wiki.openssl.org/index.php/Elliptic_Curve_Diffie_Hellman


ECDHKey::ECDHKey()
: mKey(nullptr)
{}
ECDHKey::ECDHKey(const ECDHKey& other)
: mKey(other.mKey)
{
    if (mKey)
    {
        Assert(EVP_PKEY_up_ref(static_cast<EVP_PKEY*>(mKey)) == 1);
    }
}
ECDHKey::ECDHKey(ECDHKey&& other)
: mKey(other.mKey)
{
    other.mKey = nullptr;
}
ECDHKey::~ECDHKey()
{
    Clear();
}

ECDHKey& ECDHKey::operator=(const ECDHKey& other)
{
    Clear();
    mKey = other.mKey;
    if (mKey)
    {
        Assert(EVP_PKEY_up_ref(static_cast<EVP_PKEY*>(mKey)) == 1);
    }
    return *this;
}
ECDHKey& ECDHKey::operator=(ECDHKey&& other)
{
    mKey = other.mKey;
    other.mKey = nullptr;
    return *this;
}

bool ECDHKey::Generate()
{
    EVP_PKEY_CTX* keygenContext = nullptr;
    keygenContext = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (keygenContext == nullptr)
    {
        return false;
    }

    // Generate the key
    EVP_PKEY* key = nullptr;
    if (EVP_PKEY_keygen_init(keygenContext) != 1)
    {
        EVP_PKEY_CTX_free(keygenContext);
        return false;
    }

    if (EVP_PKEY_keygen(keygenContext, &key) != 1)
    {
        EVP_PKEY_CTX_free(keygenContext);
        return false;
    }
    EVP_PKEY_CTX_free(keygenContext);

    mKey = key;
    return true;
}

bool ECDHKey::LoadPublicKey(const String& string)
{
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        return false;
    }

    int keySize = static_cast<int>(string.Size());
    if (BIO_write(bio, string.CStr(), keySize) != keySize)
    {
        BIO_free_all(bio);
        return false;
    }

    if(BIO_flush(bio) == 0)
    {
        return false;
    }

    EVP_PKEY* key = mKey ? static_cast<EVP_PKEY*>(mKey) : nullptr;
    bool result = PEM_read_bio_PUBKEY(bio, &key, NULL, NULL) == key;
    BIO_free_all(bio);
    mKey = static_cast<EVP_PKEY*>(key);
    return result;
}

bool ECDHKey::LoadPrivateKey(const String& string)
{
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        return false;
    }

    int keySize = static_cast<int>(string.Size());
    if (BIO_write(bio, string.CStr(), keySize) != keySize)
    {
        BIO_free_all(bio);
        return false;
    }

    if (BIO_flush(bio) == 0)
    {
        return false;
    }

    EVP_PKEY* key = mKey ? static_cast<EVP_PKEY*>(mKey) : nullptr;
    bool result = PEM_read_bio_PrivateKey(bio, &key, NULL, NULL) == key;
    BIO_free_all(bio);
    mKey = static_cast<EVP_PKEY*>(key);
    return result;
}

void ECDHKey::Clear()
{
    if (mKey)
    {
        EVP_PKEY_free(static_cast<EVP_PKEY*>(mKey));
    }
    mKey = nullptr;
}

String ECDHKey::GetPublicKey() const
{
    if (!mKey)
    {
        return String();
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        return String();
    }

    if (PEM_write_bio_PUBKEY(bio, static_cast<EVP_PKEY*>(mKey)) == 0)
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
String ECDHKey::GetPrivateKey() const
{
    if (!mKey)
    {
        return String();
    }

    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio)
    {
        return String();
    }

    if (PEM_write_bio_PrivateKey(bio, static_cast<EVP_PKEY*>(mKey), NULL, NULL, 0, NULL, NULL) == 0)
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

SizeT ECDHDerive(ECDHKey* localKey, ECDHKey* peerKey, ByteT* outBuffer, SizeT bufferSize)
{
    EVP_PKEY_CTX* context;
    context = EVP_PKEY_CTX_new(static_cast<EVP_PKEY*>(localKey->GetKey()), NULL);
    if (!context)
    {
        return 0;
    }

    if (EVP_PKEY_derive_init(context) != 1)
    {
        EVP_PKEY_CTX_free(context);
        return 0;
    }

    if (EVP_PKEY_derive_set_peer(context, static_cast<EVP_PKEY*>(peerKey->GetKey())) != 1)
    {   
        EVP_PKEY_CTX_free(context);
        return 0;
    }

    SizeT requiredBytes = 0;
    if (EVP_PKEY_derive(context, NULL, &requiredBytes) != 1)
    {
        EVP_PKEY_CTX_free(context);
        return 0;
    }

    if (outBuffer == nullptr)
    {
        EVP_PKEY_CTX_free(context);
        return requiredBytes;
    }

    if (requiredBytes > bufferSize)
    {
        EVP_PKEY_CTX_free(context);
        return 0;
    }
    
    if (EVP_PKEY_derive(context, outBuffer, &requiredBytes) != 1)
    {
        EVP_PKEY_CTX_free(context);
        return 0;
    }
    EVP_PKEY_CTX_free(context);
    return requiredBytes;
}

} // namespace Crypto
} // namespace lf