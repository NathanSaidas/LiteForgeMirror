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
// todo: (Nathan) RSASign/Verify should support 2 modes (bcrypt/sha256) currently it signs sha256 with 'random' data.
// ********************************************************************
#ifndef LF_CORE_RSA_H
#define LF_CORE_RSA_H

#include "Core/Common/API.h"
#include "Core/String/String.h"

namespace lf {

namespace Crypto
{
using RSAKeyContext = void*;

enum RSAKeySize
{
    RSA_KEY_1024,
    RSA_KEY_2048,
    RSA_KEY_4096,

    RSA_KEY_Unknown
};

// note:
//   Uses RSA_F4 for key generation
//   Uses PEM format for string storage, === BEGIN RSA PRIVATE/PUBLIC KEY ===
class LF_CORE_API RSAKey
{
public:
    // **********************************
    // Default Constructor: (Does not generate key)
    // **********************************
    RSAKey();
    // **********************************
    // Copy Constructor: Allocates a whole new RSAKeyContext
    // **********************************
    RSAKey(const RSAKey& other);
    // **********************************
    // Move Constructor: Moves the RSAKeyContext from 'other'
    // **********************************
    RSAKey(RSAKey&& other);
    ~RSAKey();

    RSAKey& operator=(const RSAKey& other);
    RSAKey& operator=(RSAKey&& other);

    // **********************************
    // Generates a new public/private key pair
    // of the specified keySize
    // 
    // @param keySize -- The size of the key to generate
    // **********************************
    bool  GeneratePair(RSAKeySize keySize);
    // **********************************
    // Extracts the public key information as string in PEM format.
    // **********************************
    String GetPublicKey() const;
    // **********************************
    // Extracts the private key information as string in PEM format.
    // **********************************
    String GetPrivateKey() const;
    // **********************************
    // Loads the public key information into this key, assumes 'key' is in PEM format.
    // **********************************
    bool LoadPublicKey(const String& key);
    // **********************************
    // Loads the private key information into this key, assumes 'key' is in PEM format.
    // **********************************
    bool LoadPrivateKey(const String& key);
    // **********************************
    // Releases all keys.
    // **********************************
    void Clear();
    // **********************************
    // Returns true if the public key is present. (ie can be used for Public RSA operations)
    // **********************************
    bool HasPublicKey() const;
    // **********************************
    // Returns true if the private key is present. (ie can be used for Private RSA operations)
    // **********************************
    bool HasPrivateKey() const;
    // **********************************
    // Returns the KeySize
    //
    // note: This returns RSA_KEY_Unknown if the key is not initialized yet.
    // **********************************
    RSAKeySize GetKeySize() const;
    // **********************************
    // Returns the size of the key in bytes.
    // **********************************
    SizeT GetKeySizeBytes() const;

    const RSAKeyContext* GetContext() const { return mContext; }
private:
    void Copy(const RSAKey& other);
    void Move(RSAKey&& other);

    RSAKeyContext* mContext;
};

// **********************************
// Encrypts the 'inBytes' to 'outBytes' using RSA public key encryption.
// 
// @param key              -- The public key used for the encryption
// @param inBytes          -- The source data to encrypt. 
// @param inBytesLength    -- The source data length. (Must be less than (KeySize - Padding) where Padding = 42
// @param outBytes         -- The result buffer where the cipher text is written.
// @param outBytesCapacity -- Specify the capacity of the 'outBytes' buffer. [Must be >= key->GetKeySizeBytes()]
// 
// @return If the encryption fails this returns false.
// **********************************
LF_CORE_API bool RSAEncryptPublic(const RSAKey* key, const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity);
// **********************************
// Encrypts the 'inBytes' to 'outBytes' using RSA private key encryption.
// 
// @param key              -- The private key used for the encryption
// @param inBytes          -- The source data to encrypt. 
// @param inBytesLength    -- The source data length. (Must be less than (KeySize - Padding) where Padding = 42
// @param outBytes         -- The result buffer where the cipher text is written.
// @param outBytesCapacity -- Specify the capacity of the 'outBytes' buffer. [Must be >= key->GetKeySizeBytes()]
// 
// @return If the encryption fails this returns false.
// **********************************
LF_CORE_API bool RSAEncryptPrivate(const RSAKey* key, const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity);
// **********************************
// Decrypts the 'inBytes' to 'outBytes' using RSA public key decryption.
// 
// @param key              -- The public key used for the decryption
// @param inBytes          -- The source data to decrypt. 
// @param inBytesLength    -- The source data length. [Should be equal to the key->GetKeySizeBytes()]
// @param outBytes         -- The result buffer where the plain text is written.
// @param outBytesCapacity -- Specify the capacity of the 'outBytes' buffer. [Must be large enough to hold the max message size. (KeySize- Padding)]
// 
// @return If the decryption fails this returns false.
// **********************************
LF_CORE_API bool RSADecryptPublic(const RSAKey* key, const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity);
// **********************************
// Decrypts the 'inBytes' to 'outBytes' using RSA private key decryption.
// 
// @param key              -- The private key used for the decryption
// @param inBytes          -- The source data to decrypt. 
// @param inBytesLength    -- The source data length. [Should be equal to the key->GetKeySizeBytes()]
// @param outBytes         -- The result buffer where the plain text is written.
// @param outBytesCapacity -- Specify the capacity of the 'outBytes' buffer. [Must be large enough to hold the max message size. (KeySize- Padding)]
// 
// @return If the decryption fails this returns false.
// **********************************
LF_CORE_API bool RSADecryptPrivate(const RSAKey* key, const ByteT* inBytes, SizeT inBytesLength, ByteT* outBytes, SizeT& outBytesCapacity);

LF_CORE_API bool RSAEncryptPublic(const RSAKey* key, const String& input, String& output);
LF_CORE_API bool RSAEncryptPrivate(const RSAKey* key, const String& input, String& output);
LF_CORE_API bool RSADecryptPublic(const RSAKey* key, const String& input, String& output);
LF_CORE_API bool RSADecryptPrivate(const RSAKey* key, const String& input, String& output);

LF_CORE_API bool RSASignPublic(const RSAKey* key, const ByteT* data, SizeT dataLength, String& outSignature);
LF_CORE_API bool RSASignPrivate(const RSAKey* key, const ByteT* data, SizeT dataLength, String& outSignature);
LF_CORE_API bool RSAVerifyPublic(const RSAKey* key, const ByteT* data, SizeT dataLength, const String& signature);
LF_CORE_API bool RSAVerifyPrivate(const RSAKey* key, const ByteT* data, SizeT dataLength, const String& signature);

LF_CORE_API bool RSASignPublic(const RSAKey* key, const String& input, String& outSignature);
LF_CORE_API bool RSASignPrivate(const RSAKey* key, const String& input, String& outSignature);
LF_CORE_API bool RSAVerifyPublic(const RSAKey* key, const String& input, const String& signature);
LF_CORE_API bool RSAVerifyPrivate(const RSAKey* key, const String& input, const String& signature);

} // namespace Crypto

#ifdef LF_EXPERIMENTAL
namespace experimental
{
namespace Crypto
{
template<::lf::Crypto::KeySize KEY_SIZE> struct RSABuffer { };
template<> struct RSABuffer<::lf::Crypto::RSA_KEY_1024> { ByteT mData[1024 / 8]; };
template<> struct RSABuffer<::lf::Crypto::RSA_KEY_2048> { ByteT mData[2048 / 8]; };
template<> struct RSABuffer<::lf::Crypto::RSA_KEY_4096> { ByteT mData[4096 / 8]; };
using RSABuffer1024 = RSABuffer<::lf::Crypto::RSA_KEY_1024>;
using RSABuffer2048 = RSABuffer<::lf::Crypto::RSA_KEY_2048>;
using RSABuffer4096 = RSABuffer<::lf::Crypto::RSA_KEY_4096>;

struct RSA_PUBLIC_PRIVATE { };
struct RSA_PRIVATE_PUBLIC { };

// todo: I find this over complicates things at this moment.. KISS
template<typename DirectionT, ::lf::Crypto::KeySize KEY_SIZE>
class RSAEncryptor
{
public:
    using BufferType = RSABuffer<KEY_SIZE>;
    using DirectionType = DirectionT;

    RSAEncryptor(::lf::Crypto::RSAKey* key, bool plainTextMode = true)
        : mKey(key)
        , mBufferSize(plainTextMode ? MaxPlainTextSize() : MaxCipherTextSize())
        , mBuffer()
    {
        Assert(key->GetKeySize() == KEY_SIZE);
    }

    ~RSAEncryptor()
    {
        mKey = nullptr;
        mBufferSize = 0;
        memset(mBuffer.mData, 0, MaxCipherTextSize());
    }

    constexpr SizeT MaxCipherTextSize() const { return sizeof(BufferType); }
    constexpr SizeT MaxPlainTextSize() const { return MaxCipherTextSize() - 42; }

    bool Encrypt() { return Encrypt(DirectionT()); }
    bool Decrypt() { return Decrypt(DirectionT()); }

    ::lf::Crypto::RSAKey* GetKey() { return mKey; }
    const ::lf::Crypto::RSAKey* GetKey() const { return mKey; }

    // ByteT* GetBuffer() { return mBuffer.mData; }
    const ByteT* GetBuffer() const { return mBuffer.mData; }

    // How much memory of the 'buffer' is read/writeable
    SizeT Size() const { return mBufferSize; }

    ByteT& operator[](SizeT index)
    {
        Assert(index < Size());
        return mBuffer.mData[index];
    }
    const ByteT& operator[](SizeT index) const
    {
        Assert(index < Size());
        return mBuffer.mData[index];
    }
private:
    bool Encrypt(RSA_PUBLIC_PRIVATE)
    {
        BufferType output;
        SizeT outputSize = MaxCipherTextSize();
        bool result = Crypto::RSAEncryptPublic(mKey, GetBuffer(), Size(), output.mData, outputSize);
        if (result)
        {
            memset(mBuffer.mData, 0, MaxCipherTextSize());
            memcpy(mBuffer.mData, output.mData, outputSize);
            memset(output.mData, 0, MaxCipherTextSize());
            mBufferSize = outputSize;
        }
        return result;
    }

    bool Encrypt(RSA_PRIVATE_PUBLIC)
    {
        BufferType output;
        SizeT outputSize = MaxCipherTextSize();
        bool result = Crypto::RSAEncryptPrivate(mKey, GetBuffer(), Size(), output.mData, outputSize);
        if (result)
        {
            memset(mBuffer.mData, 0, MaxCipherTextSize());
            memcpy(mBuffer.mData, output.mData, outputSize);
            memset(output.mData, 0, MaxCipherTextSize());
            mBufferSize = outputSize;
        }
        return result;
    }

    bool Decrypt(RSA_PUBLIC_PRIVATE)
    {
        BufferType output;
        SizeT outputSize = MaxPlainTextSize();
        bool result = Crypto::RSADecryptPrivate(mKey, GetBuffer(), Size(), output.mData, outputSize);
        if (result)
        {
            memset(mBuffer.mData, 0, MaxCipherTextSize());
            memcpy(mBuffer.mData, output.mData, outputSize);
            memset(output.mData, 0, MaxCipherTextSize());
            mBufferSize = outputSize;
        }
        return result;
    }

    bool Decrypt(RSA_PRIVATE_PUBLIC)
    {
        BufferType output;
        SizeT outputSize = MaxPlainTextSize();
        bool result = Crypto::RSADecryptPublic(mKey, GetBuffer(), Size(), output.mData, outputSize);
        if (result)
        {
            memset(mBuffer.mData, 0, MaxCipherTextSize());
            memcpy(mBuffer.mData, output.mData, outputSize);
            memset(output.mData, 0, MaxCipherTextSize());
            mBufferSize = outputSize;
        }
        return result;
    }

    ::lf::Crypto::RSAKey* mKey;
    SizeT           mBufferSize;
    BufferType      mBuffer;
};

} // namespace Crypto
} // namespace experimental
#endif // #ifdef LF_EXPERIMENTAL

} // namespace lf

#endif // LF_CORE_RSA_H