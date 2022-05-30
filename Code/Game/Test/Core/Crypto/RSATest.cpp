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
#include "Core/Crypto/RSA.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Math/Random.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"
#include "Core/String/StringCommon.h"
#include "Core/String/StringUtil.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/CmdLine.h"

namespace lf {

static const char* RSA_PUBLIC_1024 = "\\Core\\Crypto\\rsa_public_1024.key";
static const char* RSA_PRIVATE_1024 = "\\Core\\Crypto\\rsa_private_1024.key";
static const char* RSA_PUBLIC_2048 = "\\Core\\Crypto\\rsa_public_2048.key";
static const char* RSA_PRIVATE_2048 = "\\Core\\Crypto\\rsa_private_2048.key";
static const char* RSA_PUBLIC_4096 = "\\Core\\Crypto\\rsa_public_4096.key";
static const char* RSA_PRIVATE_4096 = "\\Core\\Crypto\\rsa_private_4096.key";

static bool LoadKey(const String& fileName, Crypto::RSAKeySize keySize, Crypto::RSAKey& key, bool isPublic)
{
    String text;
    File file;
    if (!file.Open(fileName, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        if (!file.Open(fileName, FF_WRITE, FILE_OPEN_ALWAYS))
        {
            gTestLog.Error(LogMessage("Failed to create rsa key ") << fileName << " file could not be created.");
            return false;
        }

        if (key.GetKeySize() != keySize)
        {
            return false;
        }

        if ((isPublic && !key.HasPublicKey()) || (!isPublic && !key.HasPrivateKey()))
        {
            return false;
        }

        text = isPublic ? key.GetPublicKey() : key.GetPrivateKey();
        if (file.Write(text.CStr(), text.Size()) != text.Size())
        {
            return false;
        }

        if (!(isPublic ? key.LoadPublicKey(text) : key.LoadPrivateKey(text)))
        {
            return false;
        }

        return true;
    }
    text.Resize(static_cast<SizeT>(file.GetSize()));
    if (file.Read(const_cast<char*>(text.CStr()), text.Size()) != text.Size())
    {
        return false;
    }

    if (!(isPublic ? key.LoadPublicKey(text) : key.LoadPrivateKey(text)))
    {
        return false;
    }
    return (isPublic ? key.HasPublicKey() && !key.HasPrivateKey() : key.HasPublicKey() && key.HasPrivateKey()) && key.GetKeySize() == keySize;
}

static bool LoadKey(Crypto::RSAKeySize keySize, Crypto::RSAKey& key)
{
    const char* publicKey = nullptr;
    const char* privateKey = nullptr;

    switch (keySize)
    {
        case Crypto::RSA_KEY_1024:
            publicKey = RSA_PUBLIC_1024;
            privateKey = RSA_PRIVATE_1024;
            break;
        case Crypto::RSA_KEY_2048:
            publicKey = RSA_PUBLIC_2048;
            privateKey = RSA_PRIVATE_2048;
            break;
        case Crypto::RSA_KEY_4096:
            publicKey = RSA_PUBLIC_4096;
            privateKey = RSA_PRIVATE_4096;
            break;
        default:
            return false;
    }

    String publicKeyFileName = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), publicKey));
    String privateKeyFileName = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), privateKey));

    if (!LoadKey(privateKeyFileName, keySize, key, false))
    {
        return false;
    }

    if (!LoadKey(publicKeyFileName, keySize, key, true))
    {
        return false;
    }

    return true;
}

static bool LoadKey(Crypto::RSAKeySize keySize)
{
    Crypto::RSAKey key;
    if (!key.GeneratePair(keySize))
    {
        return false;
    }
    return LoadKey(keySize, key);
}

static bool LoadPrivateKey(Crypto::RSAKeySize keySize, Crypto::RSAKey& key)
{
    const char* keyName = nullptr;

    switch (keySize)
    {
    case Crypto::RSA_KEY_1024:
        keyName = RSA_PRIVATE_1024;
        break;
    case Crypto::RSA_KEY_2048:
        keyName = RSA_PRIVATE_2048;
        break;
    case Crypto::RSA_KEY_4096:
        keyName = RSA_PRIVATE_4096;
        break;
    default:
        return false;
    }

    String filename = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), keyName));
    return LoadKey(filename, keySize, key, false);
}

static bool LoadPublicKey(Crypto::RSAKeySize keySize, Crypto::RSAKey& key)
{
    const char* keyName = nullptr;

    switch (keySize)
    {
    case Crypto::RSA_KEY_1024:
        keyName = RSA_PUBLIC_1024;
        break;
    case Crypto::RSA_KEY_2048:
        keyName = RSA_PUBLIC_2048;
        break;
    case Crypto::RSA_KEY_4096:
        keyName = RSA_PUBLIC_4096;
        break;
    default:
        return false;
    }

    String filename = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), keyName));
    return LoadKey(filename, keySize, key, true);
}

static bool LoadKeyText(Crypto::RSAKeySize keySize, String& publicText, String& privateText)
{
    const char* publicKey = nullptr;
    const char* privateKey = nullptr;

    switch (keySize)
    {
    case Crypto::RSA_KEY_1024:
        publicKey = RSA_PUBLIC_1024;
        privateKey = RSA_PRIVATE_1024;
        break;
    case Crypto::RSA_KEY_2048:
        publicKey = RSA_PUBLIC_2048;
        privateKey = RSA_PRIVATE_2048;
        break;
    case Crypto::RSA_KEY_4096:
        publicKey = RSA_PUBLIC_4096;
        privateKey = RSA_PRIVATE_4096;
        break;
    default:
        return false;
    }

    String publicKeyFileName = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), publicKey));
    String privateKeyFileName = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), privateKey));

    File file;
    if (!file.Open(publicKeyFileName, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        return false;
    }
    publicText.Resize(static_cast<SizeT>(file.GetSize()));
    if (file.Read(const_cast<char*>(publicText.CStr()), publicText.Size()) != publicText.Size())
    {
        return false;
    }
    file.Close();

    if (!file.Open(privateKeyFileName, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        return false;
    }
    privateText.Resize(static_cast<SizeT>(file.GetSize()));
    if (file.Read(const_cast<char*>(privateText.CStr()), privateText.Size()) != privateText.Size())
    {
        return false;
    }
    return true;
}

REGISTER_TEST(RSATestSetup, "Core.Crypto", TestFlags::TF_SETUP)
{
    String tempDir = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), "\\Core\\Crypto\\"));
    TEST_CRITICAL(FileSystem::PathExists(tempDir) || FileSystem::PathCreate(tempDir));

    TEST(LoadKey(Crypto::RSA_KEY_1024));
    TEST(LoadKey(Crypto::RSA_KEY_2048));
    TEST(LoadKey(Crypto::RSA_KEY_4096));
}

REGISTER_TEST(RSATest_GenerateKey, "Core.Crypto")
{
    Crypto::RSAKey key;
    Crypto::RSAKey publicKey;
    Crypto::RSAKey masterKey;

    // Key has correct default values
    TEST(key.HasPublicKey() == false);
    TEST(key.HasPrivateKey() == false);
    TEST(key.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(key.GetKeySizeBytes() == 0);
    TEST(key.GetPublicKey().Empty());
    TEST(key.GetPrivateKey().Empty());

    // 1024 bit Key can be generated
    TEST_CRITICAL(key.GeneratePair(Crypto::RSA_KEY_1024));
    TEST(key.HasPublicKey());
    TEST(key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_1024);
    TEST(key.GetKeySizeBytes() == (1024 / 8));
    TEST(key.GetPublicKey().Empty() == false);
    TEST(key.GetPrivateKey().Empty() == false);
    TEST(key.GetPublicKey() != key.GetPrivateKey());

    // 1024 bit Key can be loaded from generated key
    TEST_CRITICAL(publicKey.LoadPublicKey(key.GetPublicKey()));
    TEST_CRITICAL(masterKey.LoadPrivateKey(key.GetPrivateKey()));

    // 1024 bit public key contains 0 information on private key
    TEST(publicKey.HasPublicKey());
    TEST(publicKey.HasPrivateKey() == false);
    TEST(publicKey.GetKeySize() == Crypto::RSA_KEY_1024);
    TEST(publicKey.GetKeySizeBytes() == (1024 / 8));
    TEST(publicKey.GetPublicKey().Empty() == false);
    TEST(publicKey.GetPrivateKey().Empty() == true);
    TEST(publicKey.GetPublicKey() != publicKey.GetPrivateKey());
    TEST(publicKey.GetPublicKey() == key.GetPublicKey());

    // 1024 bit private key contains information on public and private key
    TEST(masterKey.HasPublicKey());
    TEST(masterKey.HasPrivateKey());
    TEST(masterKey.GetKeySize() == Crypto::RSA_KEY_1024);
    TEST(masterKey.GetKeySizeBytes() == (1024 / 8));
    TEST(masterKey.GetPublicKey().Empty() == false);
    TEST(masterKey.GetPrivateKey().Empty() == false);
    TEST(masterKey.GetPublicKey() != masterKey.GetPrivateKey());
    TEST(masterKey.GetPublicKey() == key.GetPublicKey());
    TEST(masterKey.GetPrivateKey() == key.GetPrivateKey());

    key.Clear();
    publicKey.Clear();
    masterKey.Clear();

    // Key has correct default values
    TEST(key.HasPublicKey() == false); TEST(publicKey.HasPublicKey() == false); TEST(masterKey.HasPublicKey() == false);
    TEST(key.HasPrivateKey() == false); TEST(publicKey.HasPrivateKey() == false); TEST(masterKey.HasPrivateKey() == false);
    TEST(key.GetKeySize() == Crypto::RSA_KEY_Unknown); TEST(publicKey.GetKeySize() == Crypto::RSA_KEY_Unknown); TEST(masterKey.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(key.GetKeySizeBytes() == 0); TEST(publicKey.GetKeySizeBytes() == 0); TEST(masterKey.GetKeySizeBytes() == 0);
    TEST(key.GetPublicKey().Empty()); TEST(publicKey.GetPublicKey().Empty()); TEST(masterKey.GetPublicKey().Empty());
    TEST(key.GetPrivateKey().Empty()); TEST(publicKey.GetPrivateKey().Empty()); TEST(masterKey.GetPrivateKey().Empty());

    // 2048 bit Key can be generated
    TEST_CRITICAL(key.GeneratePair(Crypto::RSA_KEY_2048));
    TEST(key.HasPublicKey());
    TEST(key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(key.GetKeySizeBytes() == (2048 / 8));
    TEST(key.GetPublicKey().Empty() == false);
    TEST(key.GetPrivateKey().Empty() == false);
    TEST(key.GetPublicKey() != key.GetPrivateKey());

    // 2048 bit Key can be loaded from generated key
    TEST_CRITICAL(publicKey.LoadPublicKey(key.GetPublicKey()));
    TEST_CRITICAL(masterKey.LoadPrivateKey(key.GetPrivateKey()));

    // 2048 bit public key contains 0 information on private key
    TEST(publicKey.HasPublicKey());
    TEST(publicKey.HasPrivateKey() == false);
    TEST(publicKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(publicKey.GetKeySizeBytes() == (2048 / 8));
    TEST(publicKey.GetPublicKey().Empty() == false);
    TEST(publicKey.GetPrivateKey().Empty() == true);
    TEST(publicKey.GetPublicKey() != publicKey.GetPrivateKey());
    TEST(publicKey.GetPublicKey() == key.GetPublicKey());

    // 2048 bit private key contains information on public and private key
    TEST(masterKey.HasPublicKey());
    TEST(masterKey.HasPrivateKey());
    TEST(masterKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(masterKey.GetKeySizeBytes() == (2048 / 8));
    TEST(masterKey.GetPublicKey().Empty() == false);
    TEST(masterKey.GetPrivateKey().Empty() == false);
    TEST(masterKey.GetPublicKey() != masterKey.GetPrivateKey());
    TEST(masterKey.GetPublicKey() == key.GetPublicKey());
    TEST(masterKey.GetPrivateKey() == key.GetPrivateKey());

    key.Clear();
    publicKey.Clear();
    masterKey.Clear();

    // Key has correct default values
    TEST(key.HasPublicKey() == false); TEST(publicKey.HasPublicKey() == false); TEST(masterKey.HasPublicKey() == false);
    TEST(key.HasPrivateKey() == false); TEST(publicKey.HasPrivateKey() == false); TEST(masterKey.HasPrivateKey() == false);
    TEST(key.GetKeySize() == Crypto::RSA_KEY_Unknown); TEST(publicKey.GetKeySize() == Crypto::RSA_KEY_Unknown); TEST(masterKey.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(key.GetKeySizeBytes() == 0); TEST(publicKey.GetKeySizeBytes() == 0); TEST(masterKey.GetKeySizeBytes() == 0);
    TEST(key.GetPublicKey().Empty()); TEST(publicKey.GetPublicKey().Empty()); TEST(masterKey.GetPublicKey().Empty());
    TEST(key.GetPrivateKey().Empty()); TEST(publicKey.GetPrivateKey().Empty()); TEST(masterKey.GetPrivateKey().Empty());

    // 4096 bit Key can be generated
    TEST_CRITICAL(key.GeneratePair(Crypto::RSA_KEY_4096));
    TEST(key.HasPublicKey());
    TEST(key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_4096);
    TEST(key.GetKeySizeBytes() == (4096 / 8));
    TEST(key.GetPublicKey().Empty() == false);
    TEST(key.GetPrivateKey().Empty() == false);
    TEST(key.GetPublicKey() != key.GetPrivateKey());

    // 4096 bit Key can be loaded from generated key
    TEST_CRITICAL(publicKey.LoadPublicKey(key.GetPublicKey()));
    TEST_CRITICAL(masterKey.LoadPrivateKey(key.GetPrivateKey()));

    // 4096 bit public key contains 0 information on private key
    TEST(publicKey.HasPublicKey());
    TEST(publicKey.HasPrivateKey() == false);
    TEST(publicKey.GetKeySize() == Crypto::RSA_KEY_4096);
    TEST(publicKey.GetKeySizeBytes() == (4096 / 8));
    TEST(publicKey.GetPublicKey().Empty() == false);
    TEST(publicKey.GetPrivateKey().Empty() == true);
    TEST(publicKey.GetPublicKey() != publicKey.GetPrivateKey());
    TEST(publicKey.GetPublicKey() == key.GetPublicKey());

    // 4096 bit private key contains information on public and private key
    TEST(masterKey.HasPublicKey());
    TEST(masterKey.HasPrivateKey());
    TEST(masterKey.GetKeySize() == Crypto::RSA_KEY_4096);
    TEST(masterKey.GetKeySizeBytes() == (4096 / 8));
    TEST(masterKey.GetPublicKey().Empty() == false);
    TEST(masterKey.GetPrivateKey().Empty() == false);
    TEST(masterKey.GetPublicKey() != masterKey.GetPrivateKey());
    TEST(masterKey.GetPublicKey() == key.GetPublicKey());
    TEST(masterKey.GetPrivateKey() == key.GetPrivateKey());

    key.Clear();
    publicKey.Clear();
    masterKey.Clear();

    // Key has correct default values
    TEST(key.HasPublicKey() == false); TEST(publicKey.HasPublicKey() == false); TEST(masterKey.HasPublicKey() == false);
    TEST(key.HasPrivateKey() == false); TEST(publicKey.HasPrivateKey() == false); TEST(masterKey.HasPrivateKey() == false);
    TEST(key.GetKeySize() == Crypto::RSA_KEY_Unknown); TEST(publicKey.GetKeySize() == Crypto::RSA_KEY_Unknown); TEST(masterKey.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(key.GetKeySizeBytes() == 0); TEST(publicKey.GetKeySizeBytes() == 0); TEST(masterKey.GetKeySizeBytes() == 0);
    TEST(key.GetPublicKey().Empty()); TEST(publicKey.GetPublicKey().Empty()); TEST(masterKey.GetPublicKey().Empty());
    TEST(key.GetPrivateKey().Empty()); TEST(publicKey.GetPrivateKey().Empty()); TEST(masterKey.GetPrivateKey().Empty());
}

REGISTER_TEST(RSATest_SaveLoadKey, "Core.Crypto")
{
    String PUBLIC_1024;
    String PRIVATE_1024;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_1024, PUBLIC_1024, PRIVATE_1024));
    String PUBLIC_2048;
    String PRIVATE_2048;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_2048, PUBLIC_2048, PRIVATE_2048));
    String PUBLIC_4096;
    String PRIVATE_4096;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_4096, PUBLIC_4096, PRIVATE_4096));

    Crypto::RSAKey key;
    TEST(key.HasPublicKey() == false);
    TEST(key.HasPrivateKey() == false);
    TEST(key.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(key.GetKeySizeBytes() == 0);
    TEST(key.GetPublicKey().Empty());
    TEST(key.GetPrivateKey().Empty());
    // 1024
    TEST_CRITICAL(LoadPrivateKey(Crypto::RSA_KEY_1024, key));
    TEST(key.HasPublicKey());
    TEST(key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_1024);
    TEST(key.GetKeySizeBytes() == (1024 / 8));
    TEST(key.GetPublicKey() == PUBLIC_1024);
    TEST(key.GetPrivateKey() == PRIVATE_1024);

    key.Clear();
    TEST(key.HasPublicKey() == false);
    TEST(key.HasPrivateKey() == false);
    TEST(key.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(key.GetKeySizeBytes() == 0);
    TEST(key.GetPublicKey().Empty());
    TEST(key.GetPrivateKey().Empty());

    TEST_CRITICAL(LoadPublicKey(Crypto::RSA_KEY_1024, key));
    TEST(key.HasPublicKey());
    TEST(!key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_1024);
    TEST(key.GetKeySizeBytes() == (1024 / 8));
    TEST(key.GetPublicKey() == PUBLIC_1024);
    TEST(key.GetPrivateKey().Empty());

    // 2048
    key.Clear();
    TEST_CRITICAL(LoadPrivateKey(Crypto::RSA_KEY_2048, key));
    TEST(key.HasPublicKey());
    TEST(key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(key.GetKeySizeBytes() == (2048 / 8));
    TEST(key.GetPublicKey() == PUBLIC_2048);
    TEST(key.GetPrivateKey() == PRIVATE_2048);

    key.Clear();
    TEST(key.HasPublicKey() == false);
    TEST(key.HasPrivateKey() == false);
    TEST(key.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(key.GetKeySizeBytes() == 0);
    TEST(key.GetPublicKey().Empty());
    TEST(key.GetPrivateKey().Empty());

    TEST_CRITICAL(LoadPublicKey(Crypto::RSA_KEY_2048, key));
    TEST(key.HasPublicKey());
    TEST(!key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(key.GetKeySizeBytes() == (2048 / 8));
    TEST(key.GetPublicKey() == PUBLIC_2048);
    TEST(key.GetPrivateKey().Empty());

    // 4096
    key.Clear();
    TEST_CRITICAL(LoadPrivateKey(Crypto::RSA_KEY_4096, key));
    TEST(key.HasPublicKey());
    TEST(key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_4096);
    TEST(key.GetKeySizeBytes() == (4096 / 8));
    TEST(key.GetPublicKey() == PUBLIC_4096);
    TEST(key.GetPrivateKey() == PRIVATE_4096);

    key.Clear();
    TEST(key.HasPublicKey() == false);
    TEST(key.HasPrivateKey() == false);
    TEST(key.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(key.GetKeySizeBytes() == 0);
    TEST(key.GetPublicKey().Empty());
    TEST(key.GetPrivateKey().Empty());

    TEST_CRITICAL(LoadPublicKey(Crypto::RSA_KEY_4096, key));
    TEST(key.HasPublicKey());
    TEST(!key.HasPrivateKey());
    TEST(key.GetKeySize() == Crypto::RSA_KEY_4096);
    TEST(key.GetKeySizeBytes() == (4096 / 8));
    TEST(key.GetPublicKey() == PUBLIC_4096);
    TEST(key.GetPrivateKey().Empty());

}



static void TestEncryptDecrypt(Crypto::RSAKey& publicKey, Crypto::RSAKey& privateKey, const SizeT MAX_PLAIN_TEXT, const SizeT MAX_CIPHER_TEXT)
{
    gTestLog.Info(LogMessage("TestEncryptDecrypt -- KeySize=") << (publicKey.GetKeySizeBytes() * 8));

    ByteT originalBytes[512] = { 0 };
    ByteT encryptedBytes[512] = { 0 };
    ByteT decryptedBytes[512] = { 0 };
    SizeT capacity = 0;
    Int32 seed = 0xDEFCAB;

    TEST(Crypto::RSAEncryptPrivate(&privateKey, originalBytes, MAX_PLAIN_TEXT + 1, decryptedBytes, capacity) == false);
    TEST(Crypto::RSAEncryptPublic(&publicKey, originalBytes, MAX_PLAIN_TEXT + 1, decryptedBytes, capacity) == false);


    // Test 0s N to Max Plain Text
    // Test random N to Max Plain Text
    for (SizeT N = 0; N <= MAX_PLAIN_TEXT; ++N)
    {
        memset(originalBytes, 0, sizeof(originalBytes));
        memset(encryptedBytes, 0, sizeof(encryptedBytes));
        memset(decryptedBytes, 0, sizeof(decryptedBytes));


        // fill: originalBytes with N zeros
        memset(originalBytes, 0, N);

        // { public -> private }
        capacity = MAX_CIPHER_TEXT;
        TEST(Crypto::RSAEncryptPublic(&publicKey, originalBytes, N, encryptedBytes, capacity));
        TEST(capacity == MAX_CIPHER_TEXT);

        capacity = MAX_PLAIN_TEXT;
        TEST(Crypto::RSADecryptPrivate(&privateKey, encryptedBytes, MAX_CIPHER_TEXT, decryptedBytes, capacity));
        TEST(capacity == N);

        if (N != 0)
        {
            TEST(memcmp(originalBytes, decryptedBytes, sizeof(originalBytes)) == 0);
            TEST(memcmp(originalBytes, encryptedBytes, sizeof(originalBytes)) != 0);
        }

        // { private -> public }
        capacity = MAX_CIPHER_TEXT;
        TEST(Crypto::RSAEncryptPrivate(&privateKey, originalBytes, N, encryptedBytes, capacity));
        TEST(capacity == MAX_CIPHER_TEXT);

        capacity = MAX_PLAIN_TEXT;
        TEST(Crypto::RSADecryptPublic(&publicKey, encryptedBytes, MAX_CIPHER_TEXT, decryptedBytes, capacity));
        TEST(capacity == N);

        if (N != 0)
        {
            TEST(memcmp(originalBytes, decryptedBytes, sizeof(originalBytes)) == 0);
            TEST(memcmp(originalBytes, encryptedBytes, sizeof(originalBytes)) != 0);
        }

        memset(originalBytes, 0, sizeof(originalBytes));
        memset(encryptedBytes, 0, sizeof(encryptedBytes));
        memset(decryptedBytes, 0, sizeof(decryptedBytes));


        // fill: originalBytes with N 'random' bytes
        for (SizeT k = 0; k < N; ++k)
        {
            originalBytes[k] = static_cast<ByteT>(Random::Mod(seed, 0xFF));
        }

        // { public -> private }
        capacity = MAX_CIPHER_TEXT;
        TEST(Crypto::RSAEncryptPublic(&publicKey, originalBytes, N, encryptedBytes, capacity));
        TEST(capacity == MAX_CIPHER_TEXT);

        capacity = MAX_PLAIN_TEXT;
        TEST(Crypto::RSADecryptPrivate(&privateKey, encryptedBytes, MAX_CIPHER_TEXT, decryptedBytes, capacity));
        TEST(capacity == N);

        if (N != 0)
        {
            TEST(memcmp(originalBytes, decryptedBytes, sizeof(originalBytes)) == 0);
            TEST(memcmp(originalBytes, encryptedBytes, sizeof(originalBytes)) != 0);
        }

        // { private -> public }
        capacity = MAX_CIPHER_TEXT;
        TEST(Crypto::RSAEncryptPrivate(&privateKey, originalBytes, N, encryptedBytes, capacity));
        TEST(capacity == MAX_CIPHER_TEXT);

        capacity = MAX_PLAIN_TEXT;
        TEST(Crypto::RSADecryptPublic(&publicKey, encryptedBytes, MAX_CIPHER_TEXT, decryptedBytes, capacity));
        TEST(capacity == N);

        if (N != 0)
        {
            TEST(memcmp(originalBytes, decryptedBytes, sizeof(originalBytes)) == 0);
            TEST(memcmp(originalBytes, encryptedBytes, sizeof(originalBytes)) != 0);
        }
    }
}

REGISTER_TEST(RSATest_EncryptDecrypt, "Core.Crypto", TestFlags::TF_STRESS)
{
    String PUBLIC_1024;
    String PRIVATE_1024;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_1024, PUBLIC_1024, PRIVATE_1024));
    String PUBLIC_2048;
    String PRIVATE_2048;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_2048, PUBLIC_2048, PRIVATE_2048));
    String PUBLIC_4096;
    String PRIVATE_4096;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_4096, PUBLIC_4096, PRIVATE_4096));

    // The amount of padding required for encryption
    const SizeT PADDING = 42;
    // The maximum number of bytes that can be contained within the 'plain' text
    const SizeT MAX_PLAIN_TEXT_1024 = (1024 / 8) - PADDING;
    const SizeT MAX_PLAIN_TEXT_2048 = (2048 / 8) - PADDING;
    const SizeT MAX_PLAIN_TEXT_4096 = (4096 / 8) - PADDING;

    const SizeT MAX_CIPHER_TEXT_1024 = (1024 / 8);
    const SizeT MAX_CIPHER_TEXT_2048 = (2048 / 8);
    const SizeT MAX_CIPHER_TEXT_4096 = (4096 / 8);

    Crypto::RSAKey privateKey;
    Crypto::RSAKey publicKey;

    // 1024:
    TEST_CRITICAL(privateKey.LoadPrivateKey(PRIVATE_1024));
    TEST_CRITICAL(publicKey.LoadPublicKey(PUBLIC_1024));
    TEST(privateKey.GetPrivateKey() == PRIVATE_1024);
    TEST(publicKey.GetPublicKey() == PUBLIC_1024);
    TestEncryptDecrypt(publicKey, privateKey, MAX_PLAIN_TEXT_1024, MAX_CIPHER_TEXT_1024);

    // 2048:
    TEST_CRITICAL(privateKey.LoadPrivateKey(PRIVATE_2048));
    TEST_CRITICAL(publicKey.LoadPublicKey(PUBLIC_2048));
    TEST(privateKey.GetPrivateKey() == PRIVATE_2048);
    TEST(publicKey.GetPublicKey() == PUBLIC_2048);
    TestEncryptDecrypt(publicKey, privateKey, MAX_PLAIN_TEXT_2048, MAX_CIPHER_TEXT_2048);

    // 4096:
    TEST_CRITICAL(privateKey.LoadPrivateKey(PRIVATE_4096));
    TEST_CRITICAL(publicKey.LoadPublicKey(PUBLIC_4096));
    TEST(privateKey.GetPrivateKey() == PRIVATE_4096);
    TEST(publicKey.GetPublicKey() == PUBLIC_4096);
    TestEncryptDecrypt(publicKey, privateKey, MAX_PLAIN_TEXT_4096, MAX_CIPHER_TEXT_4096);
}

static void TestSignVerify(Crypto::RSAKey& publicKey, Crypto::RSAKey& privateKey, const String& MESSAGE, const ByteT* MESSAGE_BYTES)
{
    gTestLog.Info(LogMessage("TestSignVerify -- KeySize=") << (publicKey.GetKeySizeBytes() * 8));

    String signatureA;
    String signatureB;
    String signatureC;
    String signatureD;

    TEST(MESSAGE.Size() > privateKey.GetKeySizeBytes());
    TEST(MESSAGE.Size() > publicKey.GetKeySizeBytes());
    TEST(Crypto::RSASignPublic(&publicKey, MESSAGE_BYTES, MESSAGE.Size(), signatureA));
    TEST(Crypto::RSASignPrivate(&privateKey, MESSAGE_BYTES, MESSAGE.Size(), signatureB));
    TEST(Crypto::RSASignPublic(&publicKey, MESSAGE_BYTES, MESSAGE.Size(), signatureC));
    TEST(Crypto::RSASignPrivate(&privateKey, MESSAGE_BYTES, MESSAGE.Size(), signatureD));
    TEST(signatureA != signatureB);
    TEST(signatureA != signatureC);
    TEST(signatureA != signatureD);
    TEST(signatureB != signatureC);
    TEST(signatureB != signatureD);
    TEST(signatureC != signatureD);
    TEST(Crypto::RSAVerifyPrivate(&privateKey, MESSAGE_BYTES, MESSAGE.Size(), signatureA));
    TEST(Crypto::RSAVerifyPublic(&publicKey, MESSAGE_BYTES, MESSAGE.Size(), signatureB));
    TEST(Crypto::RSAVerifyPrivate(&privateKey, MESSAGE_BYTES, MESSAGE.Size(), signatureC));
    TEST(Crypto::RSAVerifyPublic(&publicKey, MESSAGE_BYTES, MESSAGE.Size(), signatureD));

    signatureA.Clear();
    signatureB.Clear();
    signatureC.Clear();
    signatureD.Clear();

    TEST(Crypto::RSASignPublic(&publicKey, MESSAGE, signatureA));
    TEST(Crypto::RSASignPrivate(&privateKey, MESSAGE, signatureB));
    TEST(Crypto::RSASignPublic(&publicKey, MESSAGE, signatureC));
    TEST(Crypto::RSASignPrivate(&privateKey, MESSAGE, signatureD));
    TEST(signatureA != signatureB);
    TEST(signatureA != signatureC);
    TEST(signatureA != signatureD);
    TEST(signatureB != signatureC);
    TEST(signatureB != signatureD);
    TEST(signatureC != signatureD);
    TEST(Crypto::RSAVerifyPrivate(&privateKey, MESSAGE, signatureA));
    TEST(Crypto::RSAVerifyPublic(&publicKey, MESSAGE, signatureB));
    TEST(Crypto::RSAVerifyPrivate(&privateKey, MESSAGE, signatureC));
    TEST(Crypto::RSAVerifyPublic(&publicKey, MESSAGE, signatureD));
}

REGISTER_TEST(RSATest_SignVerify, "Core.Crypto")
{
    String PUBLIC_1024;
    String PRIVATE_1024;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_1024, PUBLIC_1024, PRIVATE_1024));
    String PUBLIC_2048;
    String PRIVATE_2048;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_2048, PUBLIC_2048, PRIVATE_2048));
    String PUBLIC_4096;
    String PRIVATE_4096;
    TEST_CRITICAL(LoadKeyText(Crypto::RSA_KEY_4096, PUBLIC_4096, PRIVATE_4096));

    SStream ss;
    ss << "This is message that we want to verify hasn't been tampered with, notice how large the text buffer is.";
    ss << "This message can actually exceed the size of the key because the sign/verify is not going to encrypt/decrypt";
    ss << "the message itself. We're just going to compute a hash with salt and encrypt the hash. That way only the one";
    ss << "with the oppossite key can decrypt the message and verify the authenticity of the data.";
    ss << "\n------------------------------------------------------------------------------------------------------------------------";
    const String& MESSAGE = ss.Str();
    const ByteT* MESSAGE_BYTES = reinterpret_cast<const ByteT*>(MESSAGE.CStr());

    // RSA Sign/Verify guarantees that no signature will be the same for the same content but we can verify the signature by decrypting the hash/salt and comparing the rehash w/ salt.
    // We can sign/verify in both direction { public -> private} and { private <- public }


    Crypto::RSAKey privateKey;
    Crypto::RSAKey publicKey;

    TEST_CRITICAL(privateKey.LoadPrivateKey(PRIVATE_1024));
    TEST_CRITICAL(publicKey.LoadPublicKey(PUBLIC_1024));
    TEST(privateKey.GetPrivateKey() == PRIVATE_1024);
    TEST(publicKey.GetPublicKey() == PUBLIC_1024);
    TestSignVerify(publicKey, privateKey, MESSAGE, MESSAGE_BYTES);

    TEST_CRITICAL(privateKey.LoadPrivateKey(PRIVATE_2048));
    TEST_CRITICAL(publicKey.LoadPublicKey(PUBLIC_2048));
    TEST(privateKey.GetPrivateKey() == PRIVATE_2048);
    TEST(publicKey.GetPublicKey() == PUBLIC_2048);
    TestSignVerify(publicKey, privateKey, MESSAGE, MESSAGE_BYTES);

    TEST_CRITICAL(privateKey.LoadPrivateKey(PRIVATE_4096));
    TEST_CRITICAL(publicKey.LoadPublicKey(PUBLIC_4096));
    TEST(privateKey.GetPrivateKey() == PRIVATE_4096);
    TEST(publicKey.GetPublicKey() == PUBLIC_4096);
    TestSignVerify(publicKey, privateKey, MESSAGE, MESSAGE_BYTES);
}

// REGISTER_TEST(RSATest, "Core.Crypto")
// {
//     if (TestFramework::TestAll())
//     {
//         return;
//     }
// 
//     auto config = TestFramework::GetConfig();
//     TestFramework::ExecuteTest("RSATest_GenerateKey", config);
//     TestFramework::ExecuteTest("RSATest_SaveLoadKey", config);
//     TestFramework::ExecuteTest("RSATest_EncryptDecrypt", config);
//     TestFramework::ExecuteTest("RSATest_SignVerify", config);
//     TestFramework::TestReset();
// }

}
