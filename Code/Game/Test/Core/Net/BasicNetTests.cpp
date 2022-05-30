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
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Crypto/SHA256.h"

#include "Core/Math/Random.h"

#include "Core/Net/NetTypes.h"
#include "Core/Net/NetFramework.h"

#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"

#include "Core/Utility/ByteOrder.h"
#include "Core/Utility/Log.h"

#include "Game/Test/Core/Net/NetTestUtils.h"

namespace lf {

const char* RSA_KEY_SERVER = "\\Core\\Net\\rsa_server.key";
const char* RSA_KEY_CLIENT = "\\Core\\Net\\rsa_client.key";
const char* AES_KEY_SHARED = "\\Core\\Net\\aes_shared.key";
const char* RSA_KEY_UNIQUE = "\\Core\\Net\\rsa_unique.key";

bool NetTestUtil::LoadPrivateKey(const char* filename, Crypto::RSAKey& key)
{
    String fullpath = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), filename));
    String text;
    File file;
    if (!file.Open(fullpath, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        // Create:
        if (!file.Open(fullpath, FF_WRITE, FILE_OPEN_ALWAYS))
        {
            gTestLog.Error(LogMessage("Failed to create private key ") << fullpath << " file could not be created.");
            return false;
        }

        if (!key.GeneratePair(Crypto::RSA_KEY_2048))
        {
            return false;
        }

        text = key.GetPrivateKey();
        if (file.Write(text.CStr(), text.Size()) != text.Size())
        {
            return false;
        }
        return key.HasPrivateKey();
    }
    text.Resize(static_cast<SizeT>(file.GetSize()));
    if (file.Read(const_cast<char*>(text.CStr()), text.Size()) != text.Size())
    {
        return false;
    }

    if (!key.LoadPrivateKey(text))
    {
        return false;
    }
    return key.HasPrivateKey();
}

bool NetTestUtil::LoadPublicKey(const char* filename, Crypto::RSAKey& key)
{
    String fullpath = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), filename));
    String text;
    File file;
    if (!file.Open(fullpath, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        // Create:
        if (!file.Open(fullpath, FF_WRITE, FILE_OPEN_ALWAYS))
        {
            gTestLog.Error(LogMessage("Failed to create public key ") << fullpath << " file could not be created.");
            return false;
        }

        if (!key.GeneratePair(Crypto::RSA_KEY_2048))
        {
            return false;
        }

        text = key.GetPublicKey();
        if (file.Write(text.CStr(), text.Size()) != text.Size())
        {
            return false;
        }
        if (!key.LoadPublicKey(text))
        {
            return false;
        }

        return key.HasPublicKey() && !key.HasPrivateKey() && key.GetKeySize() == Crypto::RSA_KEY_2048;
    }
    text.Resize(static_cast<SizeT>(file.GetSize()));
    if (file.Read(const_cast<char*>(text.CStr()), text.Size()) != text.Size())
    {
        return false;
    }

    if (!key.LoadPublicKey(text))
    {
        return false;
    }
    return key.HasPublicKey() && !key.HasPrivateKey() && key.GetKeySize() == Crypto::RSA_KEY_2048;
}

bool NetTestUtil::LoadAsPublicKey(const char* filename, Crypto::RSAKey& key)
{
    if (!LoadPrivateKey(filename, key))
    {
        return false;
    }

    return key.LoadPublicKey(key.GetPublicKey()) && !key.HasPrivateKey();
}

bool NetTestUtil::LoadSharedKey(const char* filename, Crypto::AESKey& key)
{
    String fullpath = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), filename));

    UInt32 keySize = 0;
    ByteT  keyBytes[32];

    File file;
    if (!file.Open(fullpath, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        if (!file.Open(fullpath, FF_WRITE, FILE_OPEN_ALWAYS))
        {
            gTestLog.Error(LogMessage("Failed to create shared key ") << fullpath << " file could not be created.");
            return false;
        }

        if (!key.Generate(Crypto::AES_KEY_256))
        {
            return false;
        }

        if (LF_ARRAY_SIZE(keyBytes) < key.GetKeySizeBytes())
        {
            return false;
        }

        keySize = Crypto::AES_KEY_256;
        memcpy(keyBytes, key.GetKey(), key.GetKeySizeBytes());

        if (file.Write(&keySize, sizeof(keySize)) != sizeof(keySize))
        {
            return false;
        }
        if (file.Write(&keyBytes, key.GetKeySizeBytes()) != key.GetKeySizeBytes())
        {
            return false;
        }
        return true;
    }

    file.Read(&keySize, sizeof(keySize));
    switch (keySize)
    {
    case Crypto::AES_KEY_128:
        return false;
    case Crypto::AES_KEY_256:
        if (file.Read(keyBytes, 32) != 32)
        {
            return false;
        }
        break;
    default:
        return false;
    }

    if (!key.Load(static_cast<Crypto::AESKeySize>(keySize), keyBytes))
    {
        return false;
    }
    return true;
}

using namespace NetTestUtil;

// Test to make sure we can generate and maintain 'stable' keys for deterministic testing.
REGISTER_TEST(SetupNetKeys, "Core.Net", TestFlags::TF_SETUP)
{
    String tempDir = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), "\\Core\\Net\\"));
    TEST_CRITICAL(FileSystem::PathExists(tempDir) || FileSystem::PathCreate(tempDir));

    Crypto::RSAKey serverKey;
    TEST(LoadPrivateKey(RSA_KEY_SERVER, serverKey));

    Crypto::RSAKey clientKey;
    TEST(LoadPublicKey(RSA_KEY_CLIENT, clientKey));

    Crypto::AESKey sharedKey;
    TEST(LoadSharedKey(AES_KEY_SHARED, sharedKey));

    Crypto::RSAKey uniqueKey;
    TEST(LoadPrivateKey(RSA_KEY_UNIQUE, uniqueKey));
}

// Test to make sure our SwapBytes functions work as expected and that we run in LittleEndian mode.
REGISTER_TEST(ByteOrderSwapTest, "Core.Utility")
{
    TEST_CRITICAL(IsLittleEndian()); // Test assumes little endian mode.
    TEST(SwapBytes(UInt64(0xAABBCCDD11223344)) == UInt64(0x44332211DDCCBBAA));
    TEST(SwapBytes(Int64(0xAABBCCDD11223344)) == Int64(0x44332211DDCCBBAA));
    TEST(SwapBytes(UInt32(0xAABBCCDD)) == UInt32(0xDDCCBBAA));
    TEST(SwapBytes(Int32(0xAABBCCDD)) == Int32(0xDDCCBBAA));
    TEST(SwapBytes(UInt16(0xAABB)) == UInt16(0xBBAA));
    TEST(SwapBytes(Int16(0x1122)) == Int16(0x2211));
}

REGISTER_TEST(IPV4EndPointTest, "Core.Net")
{
    IPv4EndPoint ipv4;
    TEST(InvalidEnum(static_cast<NetAddressFamily::Value>(ipv4.mAddressFamily)));
    TEST(ipv4.mPort == 0);
    TEST(ipv4.mAddress.mWord == 0);

    TEST_CRITICAL(IPV4(reinterpret_cast<IPEndPointAny&>(ipv4), "127.0.0.1", 27015));
    TEST(ValidEnum(static_cast<NetAddressFamily::Value>(ipv4.mAddressFamily)));
    TEST(ipv4.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4);
    TEST(ipv4.mPort == SwapBytes(UInt16(27015)));
    TEST(ipv4.mAddress.mBytes[0] == 127);
    TEST(ipv4.mAddress.mBytes[1] == 0);
    TEST(ipv4.mAddress.mBytes[2] == 0);
    TEST(ipv4.mAddress.mBytes[3] == 1);

    {
        IPv4EndPoint copied(ipv4);
        TEST(copied == ipv4);
        TEST(copied != IPv4EndPoint());
    }

    {
        IPv4EndPoint copied;
        TEST(copied != ipv4);
        copied = ipv4;
        TEST(copied == ipv4);
    }

    {
        IPv4EndPoint copied(ipv4);
        IPv4EndPoint moved(std::move(copied));
        TEST(copied == IPv4EndPoint());
        TEST(moved == ipv4);
    }

    {
        IPv4EndPoint copied(ipv4);
        IPv4EndPoint moved;
        moved = std::move(copied);
        TEST(copied == IPv4EndPoint());
        TEST(moved == ipv4);
    }
}

REGISTER_TEST(IPV6EndPointTest, "Core.Net")
{
    IPv6EndPoint ipv6;
    TEST(InvalidEnum(static_cast<NetAddressFamily::Value>(ipv6.mAddressFamily)));
    TEST(ipv6.mPort == 0);
    for (SizeT i = 0; i < LF_ARRAY_SIZE(ipv6.mAddress.mWord); ++i)
    {
        TEST(ipv6.mAddress.mWord[i] == 0);
    }

    TEST_CRITICAL(IPV6(reinterpret_cast<IPEndPointAny&>(ipv6), "::1", 27015));
    TEST(ValidEnum(static_cast<NetAddressFamily::Value>(ipv6.mAddressFamily)));
    TEST(ipv6.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV6);
    TEST(ipv6.mPort == SwapBytes(UInt16(27015)));
    TEST(ipv6.mAddress.mWord[0] == 0);
    TEST(ipv6.mAddress.mWord[1] == 0);
    TEST(ipv6.mAddress.mWord[2] == 0);
    TEST(ipv6.mAddress.mWord[3] == 0);
    TEST(ipv6.mAddress.mWord[4] == 0);
    TEST(ipv6.mAddress.mWord[5] == 0);
    TEST(ipv6.mAddress.mWord[6] == 0);
    TEST(ipv6.mAddress.mWord[7] == SwapBytes(UInt16(1)));

    {
        IPv6EndPoint copied(ipv6);
        TEST(copied == ipv6);
        TEST(copied != IPv6EndPoint());
    }

    {
        IPv6EndPoint copied;
        TEST(copied != ipv6);
        copied = ipv6;
        TEST(copied == ipv6);
    }

    {
        IPv6EndPoint copied(ipv6);
        IPv6EndPoint moved(std::move(copied));
        TEST(copied == IPv6EndPoint());
        TEST(moved == ipv6);
    }

    {
        IPv6EndPoint copied(ipv6);
        IPv6EndPoint moved;
        moved = std::move(copied);
        TEST(copied == IPv6EndPoint());
        TEST(moved == ipv6);
    }
}

} // namespace lf