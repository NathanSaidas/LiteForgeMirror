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

#include "Core/Test/Test.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/ECDH.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Crypto/CryptoSerialization.h"
#include "Core/Crypto/SHA256.h"
#include "Core/IO/BinaryStream.h"
#include "Core/IO/TextStream.h"
#include "Core/Math/Random.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/String/SStream.h"
#include "Core/String/StringCommon.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"
#include <utility>
#include <cmath>

namespace lf {

REGISTER_TEST(SHA256_Test, "Core.Crypto")
{
    const String content = "g=small prime. g^a mod n | g ^ b mod n";
    const String expected = StrToUpper("67aba555c12712860283253aef6be9f5cc7109a85389f9770e054982db79bfe0");

    Crypto::SHA256Hash hash(reinterpret_cast<const ByteT*>(content.CStr()), content.Size());
    const String result = BytesToHex(hash.Bytes(), hash.Size());
    TEST(result == expected);
}

REGISTER_TEST(HMAC_Test, "Core.Crypto")
{
    String shortMessage = "This is a short message";
    String longMessage = "This is a very long message we are going to use to test the behavior of the HMAC. Will it work? Will it fail? Find out next time on dbz.";

    Crypto::HMACKey randomKey;
    Crypto::SecureRandomBytes(randomKey.Bytes(), randomKey.Size());

    Crypto::HMACBuffer shortHMAC[2];
    Crypto::HMACBuffer longHMAC[2];

    TEST(randomKey.Compute(reinterpret_cast<const ByteT*>(shortMessage.CStr()), shortMessage.Size(), shortHMAC[0]));
    TEST(randomKey.Compute(reinterpret_cast<const ByteT*>(shortMessage.CStr()), shortMessage.Size(), shortHMAC[1]));

    TEST(randomKey.Compute(reinterpret_cast<const ByteT*>(longMessage.CStr()), longMessage.Size(), longHMAC[0]));
    TEST(randomKey.Compute(reinterpret_cast<const ByteT*>(longMessage.CStr()), longMessage.Size(), longHMAC[1]));

    // Key should generate same HMAC all the time
    TEST(shortHMAC[0] == shortHMAC[1]);
    TEST(!(shortHMAC[0] != shortHMAC[1]));
    TEST(longHMAC[0] == longHMAC[1]);
    TEST(!(longHMAC[0] != longHMAC[1]));

    // HMAC should be different between content
    TEST(shortHMAC[0] != longHMAC[0]);

    // Different keys generate different hmac
    Crypto::SecureRandomBytes(randomKey.Bytes(), randomKey.Size());
    TEST(randomKey.Compute(reinterpret_cast<const ByteT*>(shortMessage.CStr()), shortMessage.Size(), shortHMAC[1]));
    TEST(randomKey.Compute(reinterpret_cast<const ByteT*>(longMessage.CStr()), longMessage.Size(), longHMAC[1]));
    TEST(shortHMAC[0] != shortHMAC[1]);
    TEST(longHMAC[0] != longHMAC[1]);
}

struct HMACTestOutput
{
    Crypto::HMACKey mKey;
    Crypto::HMACBuffer mHMAC;
    String mContent;
    volatile Atomic32 mRunning;
};

static void HMACThread(void* data)
{
    HMACTestOutput* testOutput = reinterpret_cast<HMACTestOutput*>(data);

    while (AtomicLoad(&testOutput->mRunning) == 0) {}

    const ByteT* bytes = reinterpret_cast<const ByteT*>(testOutput->mContent.CStr());
    const SizeT bytesLength = testOutput->mContent.Size();

    for (SizeT i = 0; i < 10000; ++i)
    {
        Crypto::HMACBuffer hmac;
        TEST(testOutput->mKey.Compute(bytes, bytesLength, hmac));
        TEST(hmac == testOutput->mHMAC);
    }
}

REGISTER_TEST(HMACThreadSafety_Test, "Core.Crypto")
{
    HMACTestOutput testOutput;
    testOutput.mContent = "This is a very long message we are going to use to test the behavior of the HMAC. Will it work? Will it fail? Find out next time on dbz.";
    Crypto::SecureRandomBytes(testOutput.mKey.Bytes(), testOutput.mKey.Size());

    TEST(testOutput.mKey.Compute(reinterpret_cast<const ByteT*>(testOutput.mContent.CStr()), testOutput.mContent.Size(), testOutput.mHMAC));

    AtomicStore(&testOutput.mRunning, 0);
    Thread threads[16];
    for (SizeT i = 0; i < LF_ARRAY_SIZE(threads); ++i)
    {
        threads[i].Fork(HMACThread, &testOutput);
    }

    AtomicStore(&testOutput.mRunning, 1);
    for (SizeT i = 0; i < LF_ARRAY_SIZE(threads); ++i)
    {
        threads[i].Join();
    }
}

static UInt64 CMod(UInt64 base, UInt64 exponent, UInt64 modulus)
{
    if ((base < 1) || (exponent < 0) || (modulus < 1))
    {
        return 0;
    }
    UInt64 result = 1;
    while (exponent > 0)
    {
        if ((exponent % 2) == 1)
        {
            result = (result * base) % modulus;
        }
        base = (base * base) % modulus;
        exponent = static_cast<UInt64>(floor(exponent / 2.0));
    }
    return result;
}

REGISTER_TEST(CryptoTheory, "Core.Crypto", TestFlags::TF_STRESS)
{

    UInt64 e = 17;
    UInt64 n = 3233;
    UInt64 d = 2753;
    UInt64 k = 0;

    SStream ss;

    ByteT mask[256];
    Int32 seed = 0xDACE;
    for (int i = 0; i < LF_ARRAY_SIZE(mask); ++i)
    {
        mask[i] = static_cast<ByteT>(Random::Mod(seed, 0xFF));
    }

    ss << "\n";
    do
    {
        // c = k^e % n
        // k = c^d % n

        UInt64 encrypted = CMod(k, e, n);
        UInt64 decrypted = CMod(encrypted, d, n);
        TEST(decrypted == k);
        
        ss << StreamFillLeft(5) << encrypted << StreamFillLeft() << " : " << StreamFillLeft(5) << decrypted << StreamFillLeft();
        if (decrypted != encrypted)
        {
            ss << "====";
        }
        else
        {
            ss << "=BAD";
        }
        ss << " ==|== ";

        encrypted = CMod(k ^ mask[k], e, n);
        decrypted = CMod(encrypted, d, n) ^ mask[k];
        TEST(decrypted == k);
        ss << StreamFillLeft(5) << encrypted << StreamFillLeft() << " : " << StreamFillLeft(5) << decrypted << StreamFillLeft();
        if (decrypted == encrypted)
        {
            ss << "=BAD";
        }
        ss << "\n";


        ++k;
    } while (k < 256);

    gTestLog.Info(LogMessage(ss.Str()));

}

REGISTER_TEST(ECDHTest, "Core.Crypto")
{
    // Client and Server generate their own keys.
    Crypto::ECDHKey clientKey;
    Crypto::ECDHKey serverKey;

    TEST(clientKey.Generate());
    TEST(serverKey.Generate());

    // Verify derived bytes will remain the same
    TVector<ByteT> bytes;
    SizeT numBytes = 0;
    numBytes = Crypto::ECDHDerive(&clientKey, &serverKey, nullptr, numBytes);
    TEST(numBytes > 0);
    bytes.resize(numBytes);
    TEST(numBytes == Crypto::ECDHDerive(&clientKey, &serverKey, bytes.data(), numBytes));

    // note: Client will share their public key with server
    // note: Server will share their public key with client
    String clientPublicText = clientKey.GetPublicKey();
    String clientPrivateText = clientKey.GetPrivateKey();
    TEST(!clientPublicText.Empty());
    TEST(!clientPrivateText.Empty());

    String serverPublicText = serverKey.GetPublicKey();
    String serverPrivateText = serverKey.GetPrivateKey();
    TEST(!serverPublicText.Empty());
    TEST(!clientPrivateText.Empty());

    // Verify we can load the keys back from text.
    clientKey.Clear();
    serverKey.Clear();

    TEST(clientKey.LoadPublicKey(clientPublicText));
    TEST(clientKey.LoadPrivateKey(clientPrivateText));

    TEST(serverKey.LoadPublicKey(serverPublicText));
    TEST(serverKey.LoadPrivateKey(serverPrivateText));

    TVector<ByteT> afterBytes;
    SizeT afterNumBytes = 0;

    // The derived bytes should act as a 'shared key' for aes.
    afterNumBytes = Crypto::ECDHDerive(&clientKey, &serverKey, nullptr, afterNumBytes);
    TEST(afterNumBytes == numBytes);
    afterBytes.resize(afterNumBytes);
    TEST(afterNumBytes == Crypto::ECDHDerive(&clientKey, &serverKey, afterBytes.data(), numBytes));
    TEST(afterBytes == bytes);

    clientKey.Clear();
    serverKey.Clear();

    clientKey.LoadPublicKey(clientPublicText);
    clientKey.LoadPrivateKey(clientPrivateText);
    serverKey.LoadPublicKey(serverPublicText);

    TVector<ByteT> b0;
    SizeT b0Size = Crypto::ECDHDerive(&clientKey, &serverKey, nullptr, 0);
    b0.resize(b0Size);
    TEST(Crypto::ECDHDerive(&clientKey, &serverKey, b0.data(), b0Size) == b0Size);
    TEST(b0 == afterBytes);

    clientKey.Clear();
    serverKey.Clear();

    clientKey.LoadPublicKey(clientPublicText);
    serverKey.LoadPublicKey(serverPublicText);
    serverKey.LoadPrivateKey(serverPrivateText);

    TVector<ByteT> b1;
    SizeT b1Size = Crypto::ECDHDerive(&serverKey, &clientKey, nullptr, 0);
    b1.resize(b1Size);
    TEST(Crypto::ECDHDerive(&serverKey, &clientKey, b1.data(), b1Size) == b1Size);
    TEST(b1 == afterBytes);
    TEST(b0 == b1);
}

REGISTER_TEST(ECDHInterceptTest, "Core.Crypto")
{
    Crypto::ECDHKey clientKey;
    Crypto::ECDHKey serverKey;
    Crypto::ECDHKey interceptor;

    TEST(clientKey.Generate());
    TEST(serverKey.Generate());
    TEST(interceptor.Generate());

    TVector<ByteT> b0;
    SizeT b0Size = Crypto::ECDHDerive(&clientKey, &serverKey, nullptr, 0);
    b0.resize(b0Size);
    TEST(b0Size > 0);
    TEST(Crypto::ECDHDerive(&clientKey, &serverKey, b0.data(), b0Size) == b0Size);
    
    TVector<ByteT> b1;
    SizeT b1Size = Crypto::ECDHDerive(&clientKey, &interceptor, nullptr, 0);
    b1.resize(b1Size);
    TEST(b1Size > 0);
    TEST(Crypto::ECDHDerive(&clientKey, &interceptor, b1.data(), b1Size) == b1Size);

    TEST(b0 != b1);
}

// AESKey :: Serialize
// HMACKey :: Serialize
// RSAKey :: SerializePublic
// RSAKey :: SerializePrivate
// ECDHKey :: SerializePublic
// ECDHKey :: SerializePrivate


struct ExampleStructure
{
    ExampleStructure() {}

    Crypto::AES256KeySerialized         mAES;
    Crypto::HMACKeySerialized           mHMAC;
    Crypto::RSA2048PublicKeySerialized  mRSAPublic;
    Crypto::RSA2048PrivateKeySerialized mRSAPrivate;
    Crypto::ECDHPublicKeySerialized     mECDHPublic;
    Crypto::ECDHPrivateKeySerialized    mECDHPrivate;

    void Serialize(Stream& s)
    {
        SERIALIZE(s, mAES, "");
        SERIALIZE(s, mHMAC, "");
        SERIALIZE(s, mRSAPublic, "");
        SERIALIZE(s, mRSAPrivate, "");
        SERIALIZE(s, mECDHPublic, "");
        SERIALIZE(s, mECDHPrivate, "");
    }
};

static bool KeyCompare(Crypto::AESKey& a, Crypto::AESKey& b)
{
    if (a.GetKeySize() != b.GetKeySize())
    {
        return false;
    }

    return memcmp(a.Bytes(), b.Bytes(), a.Size()) == 0;
}

static bool KeyCompare(Crypto::HMACKey& a, Crypto::HMACKey& b)
{
    return memcmp(a.Bytes(), b.Bytes(), a.Size()) == 0;
}

static bool KeyCompare(Crypto::RSAKey& a, Crypto::RSAKey& b)
{
    if (a.GetKeySize() != b.GetKeySize())
    {
        return false;
    }

    if (a.HasPrivateKey() != b.HasPrivateKey())
    {
        return false;
    }

    if (a.HasPublicKey() != b.HasPublicKey())
    {
        return false;
    }

    if (a.HasPrivateKey())
    {
        return a.GetPrivateKey() == b.GetPrivateKey();
    }
    return a.GetPublicKey() == b.GetPublicKey();
}

static bool KeyCompare(Crypto::ECDHKey& a, Crypto::ECDHKey& b)
{
    const String aPriv = a.GetPrivateKey();
    const String bPriv = b.GetPrivateKey();
    if (aPriv != bPriv)
    {
        return false;
    }

    const String aPub = a.GetPublicKey();
    const String bPub = b.GetPublicKey();
    if (aPub != bPub)
    {
        return false;
    }
    return true;
}

REGISTER_TEST(CryptoSerialization_Test, "Core.Crypto")
{
    Crypto::AESKey aes;
    aes.Generate(Crypto::AES_KEY_256);

    Crypto::HMACKey hmac;
    hmac.Generate();

    Crypto::RSAKey rsa;
    rsa.GeneratePair(Crypto::RSA_KEY_2048);

    Crypto::RSAKey rsaPublic;
    rsaPublic.LoadPublicKey(rsa.GetPublicKey());
    
    Crypto::ECDHKey ecdh;
    ecdh.Generate();

    Crypto::ECDHKey ecdhPublic;
    ecdhPublic.LoadPublicKey(ecdh.GetPublicKey());

    ExampleStructure es;
    es.mAES.mItem = &aes;
    es.mHMAC.mItem = &hmac;
    es.mRSAPublic.mItem = &rsaPublic;
    es.mRSAPrivate.mItem = &rsa;
    es.mECDHPublic.mItem = &ecdhPublic;
    es.mECDHPrivate.mItem = &ecdh;

    MemoryBuffer buffer;
    BinaryStream ts;
    ts.Open(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    if (ts.BeginObject("o", "o"))
    {
        es.Serialize(ts);
        ts.EndObject();
    }

    ts.Close();

    const void* ss = buffer.GetData(); (ss);

    // PacketStream:
    Crypto::AESKey aesOut;
    Crypto::HMACKey hmacOut;
    Crypto::RSAKey rsaPublicOut;
    Crypto::RSAKey rsaPrivateOut;
    Crypto::ECDHKey ecdhPublicOut;
    Crypto::ECDHKey ecdhPrivateOut;

    es.mAES.mItem = &aesOut;
    es.mHMAC.mItem = &hmacOut;
    es.mRSAPublic.mItem = &rsaPublicOut;
    es.mRSAPrivate.mItem = &rsaPrivateOut;
    es.mECDHPublic.mItem = &ecdhPublicOut;
    es.mECDHPrivate.mItem = &ecdhPrivateOut;

    TEST(!KeyCompare(aesOut, aes));
    TEST(!KeyCompare(hmacOut, hmac));
    TEST(!KeyCompare(rsaPublicOut, rsaPublic));
    TEST(!KeyCompare(rsaPrivateOut, rsa));
    TEST(!KeyCompare(ecdhPublicOut, ecdhPublic));
    TEST(!KeyCompare(ecdhPrivateOut, ecdh));

    ts.Open(Stream::MEMORY, &buffer, Stream::SM_READ);
    if (ts.BeginObject("o", "o"))
    {
        es.Serialize(ts);
        ts.EndObject();
    }
    ts.Close();

    TEST(KeyCompare(aesOut, aes));
    TEST(KeyCompare(hmacOut, hmac));
    TEST(KeyCompare(rsaPublicOut, rsaPublic));
    TEST(KeyCompare(rsaPrivateOut, rsa));
    TEST(KeyCompare(ecdhPublicOut, ecdhPublic));
    TEST(KeyCompare(ecdhPrivateOut, ecdh));

    gTestLog.Info(LogMessage("Done"));

}

SizeT FindLowest(const Float64* values, SizeT size)
{
    Float64 lowest = DBL_MAX;
    SizeT lowestIndex = INVALID;
    for (SizeT i = 0; i < size; ++i)
    {
        if (values[i] < lowest)
        {
            lowest = values[i];
            lowestIndex = i;
        }
    }
    return lowestIndex;
}

SizeT FindHighest(const Float64* values, SizeT size)
{
    Float64 lowest = DBL_MIN;
    SizeT lowestIndex = INVALID;
    for (SizeT i = 0; i < size; ++i)
    {
        if (values[i] > lowest)
        {
            lowest = values[i];
            lowestIndex = i;
        }
    }
    return lowestIndex;
}

static void ClearCache(ByteT* cache, SizeT size)
{
    for (SizeT i = 0; i < size; ++i)
    {
        cache[i] = static_cast<ByteT>((cache[i] * 6) % 255);
    }
}

REGISTER_TEST(AESBreak_Test, "Core.Crypto", TestFlags::TF_DISABLED)
{
    SizeT  CACHE_SIZE = 1024 * 1024;
    ByteT* CACHE = static_cast<ByteT*>(LFAlloc(CACHE_SIZE, 1));

    Int32 seed = 0xCABBAB;
    ByteT KEY_BYTES[32] = { 0 };
    for (SizeT i = 0; i < 32; ++i)
    {
        KEY_BYTES[i] = static_cast<ByteT>(Random::Mod(seed, 255));
    }

    Crypto::AESKey key;
    TEST(key.Load(Crypto::AES_KEY_256, KEY_BYTES));

    Crypto::AESIV salt;
    Crypto::SecureRandomBytes(salt.mBytes, sizeof(salt.mBytes));

    const SizeT actualByteLength = 1200;
    ByteT source[1500] = { 0 };
    Crypto::SecureRandomBytes(source, actualByteLength);

    // These 2 variables are known
    ByteT PLAIN_TEXT[1500] = { 0 };
    SizeT PLAIN_TEXT_LENGTH = sizeof(PLAIN_TEXT);
    ByteT CIPHER_TEXT[1500] = { 0 };
    SizeT CIPHER_TEXT_LENGTH = sizeof(CIPHER_TEXT);
    TEST(Crypto::AESEncrypt(&key, salt.mBytes, source, actualByteLength, CIPHER_TEXT, CIPHER_TEXT_LENGTH));
    TEST(Crypto::AESDecrypt(&key, salt.mBytes, CIPHER_TEXT, CIPHER_TEXT_LENGTH, PLAIN_TEXT, PLAIN_TEXT_LENGTH));


    ByteT plainText[1500] = { 0 };
    SizeT plainTextLength = sizeof(plainText);
    ByteT cipherText[1500] = { 0 };
    SizeT cipherTextLength = sizeof(cipherText);

    ByteT keyBytes[32] = { 0 };

    for (SizeT keyIndex = 0; keyIndex < sizeof(keyBytes); ++keyIndex)
    {
        Float64 encrypt_times[255] = { 0.0 };
        Float64 decrypt_times[255] = { 0.0 };
        Timer timer;
        for (SizeT byteIndex = 0; byteIndex < 255; ++byteIndex)
        {
            // load our testing key
            Crypto::AESKey sampleKey;
            sampleKey.Load(Crypto::AES_KEY_256, keyBytes);

            cipherTextLength = sizeof(cipherText);
            plainTextLength = sizeof(plainText);

            ClearCache(CACHE, CACHE_SIZE);
            timer.Start();
            TEST(Crypto::AESEncrypt(&sampleKey, salt.mBytes, PLAIN_TEXT, PLAIN_TEXT_LENGTH, cipherText, cipherTextLength));
            timer.Stop();
            encrypt_times[byteIndex] = timer.GetDelta();

            ClearCache(CACHE, CACHE_SIZE);
            timer.Start();
            TEST(Crypto::AESDecrypt(&sampleKey, salt.mBytes, cipherText, cipherTextLength, plainText, plainTextLength));
            timer.Stop();
            decrypt_times[byteIndex] = timer.GetDelta();
        }

        SizeT lowestEncrypt = FindLowest(encrypt_times, 255);
        SizeT lowestDecrypt = FindLowest(decrypt_times, 255);
        SizeT highestEncrypt = FindHighest(encrypt_times, 255);
        SizeT highestDecrypt = FindHighest(decrypt_times, 255);

        gTestLog.Info(LogMessage("Select [") << KEY_BYTES[keyIndex] << "][" << keyIndex << "] @ " 
                                << lowestEncrypt << ", " << lowestDecrypt << " vs "
                                << highestEncrypt << ", " << highestDecrypt);
    }

    LFFree(CACHE);
}

}