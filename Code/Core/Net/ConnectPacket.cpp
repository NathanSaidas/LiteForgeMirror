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
#include "ConnectPacket.h"
#include "Core/Common/Assert.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Memory/Memory.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Utility/Crc32.h"

#include <utility>

namespace lf {

SizeT ConnectPacket::RequestSize(const Crypto::RSAKey& clientKey, const Crypto::AESKey& sharedKey)
{
    if (!clientKey.HasPublicKey() || !clientKey.HasPrivateKey() || clientKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        return INVALID;
    }

    if (!sharedKey.GetKey() || sharedKey.GetKeySize() != Crypto::AES_KEY_256)
    {
        return INVALID;
    }

    String message = clientKey.GetPublicKey();
    return Crypto::AESCipherTextLength(&sharedKey, message.Size());
}

bool ConnectPacket::ConstructRequest(
    ByteT* packetBytes,
    SizeT& packetBytesLength,
    const Crypto::RSAKey& clientKey,
    const Crypto::RSAKey& serverKey,
    const Crypto::AESKey& sharedKey)
{
    LF_STATIC_ASSERT(sizeof(Signature::mHash) == sizeof(Crypto::SHA256HashType::mData));
    if (!Crypto::IsSecureRandom())
    {
        return false;
    }

    if (!clientKey.HasPublicKey() || !clientKey.HasPrivateKey() || clientKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        // If you got the client key on the server and try calling this, it's expected to fail
        // as you SHOULD NOT have their private key.
        return false; // Bad client key.
    }
    if (!serverKey.HasPublicKey() || serverKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        return false; // Bad server key
    }
    if (!sharedKey.GetKey() || sharedKey.GetKeySize() != Crypto::AES_KEY_256)
    {
        return false; // Bad shared key
    }

    // Encrypt the message
    ByteT iv[16];
    Crypto::SecureRandomBytes(iv, 16);
    String message = clientKey.GetPublicKey();
    String encryptedMessage;
    if (!Crypto::AESEncrypt(&sharedKey, iv, message, encryptedMessage))
    {
        return false;
    }

    // Must be able to fit the entire packet.
    const SizeT requiredSize = HeaderType::ACTUAL_SIZE + encryptedMessage.Size() + 256;
    if (packetBytesLength < requiredSize)
    {
        packetBytesLength = requiredSize;
        return false;
    }

    // Generate a signature
    SignatureType sig;
    memcpy(sig.mIV, iv, sizeof(sig.mIV));
    memcpy(sig.mKey, sharedKey.GetKey(), sizeof(sig.mKey));
    Crypto::SecureRandomBytes(sig.mSalt, sizeof(sig.mSalt));
    Crypto::SHA256HashType hash = Crypto::SHA256Hash(reinterpret_cast<const ByteT*>(encryptedMessage.CStr()), encryptedMessage.Size(), sig.mSalt, sizeof(sig.mSalt));
    memcpy(sig.mHash, hash.mData, sizeof(sig.mHash));

    // Encrypt signature
    const ByteT* sigBytes = reinterpret_cast<const ByteT*>(&sig);
    ByteT sigEncrypted[256];
    SizeT sigEncryptedLength = sizeof(sigEncrypted);
    if (!Crypto::RSAEncryptPublic(&serverKey, sigBytes, sizeof(sig), sigEncrypted, sigEncryptedLength))
    {
        return false;
    }

    if (sigEncryptedLength != sizeof(sigEncrypted))
    {
        return false;
    }

    packetBytesLength = requiredSize;
    ByteT* offsetBase = packetBytes;
    ByteT* offsetMessage = packetBytes + HeaderType::ACTUAL_SIZE;
    ByteT* offsetSignature = (packetBytes + packetBytesLength) - 256;

    memcpy(offsetMessage, encryptedMessage.CStr(), encryptedMessage.Size());
    memcpy(offsetSignature, sigEncrypted, sigEncryptedLength);
    HeaderType * header = reinterpret_cast<HeaderType*>(offsetBase);
    header->mAppID = NetConfig::NET_APP_ID;
    header->mAppVersion = NetConfig::NET_APP_VERSION;
    header->mType = static_cast<UInt8>(NetPacketType::NET_PACKET_TYPE_CONNECT);
    
    NetPacketFlag::BitfieldType flags;
    flags.Set(NetPacketFlag::NET_PACKET_FLAG_RELIABILITY); // reliable
    header->mFlags = flags.value;
    header->mCrc32 = PacketUtility::CalcCrc32(packetBytes, packetBytesLength);
    return true;
}

bool ConnectPacket::DeconstructRequest(
    const ByteT* packetBytes,
    SizeT packetBytesLength,
    const Crypto::RSAKey& serverKey,
    Crypto::RSAKey& clientKey,
    Crypto::AESKey& sharedKey,
    HeaderType& header)
{
    if (!packetBytes || packetBytesLength < (256 + HeaderType::ACTUAL_SIZE))
    {
        return false;
    }

    if (!serverKey.HasPrivateKey())
    {
        return false;
    }

    const ByteT* offsetBase = packetBytes;
    const ByteT* offsetMessage = packetBytes + HeaderType::ACTUAL_SIZE;
    const ByteT* offsetSignature = (packetBytes + packetBytesLength) - 256;
    const SizeT encryptedMessageLength = (packetBytesLength - HeaderType::ACTUAL_SIZE) - 256;
    
    ByteT overflowProtection[256] = { 0 };
    SizeT sigSize = sizeof(overflowProtection);
    SignatureType* sig = reinterpret_cast<SignatureType*>(&overflowProtection[0]);
    
    if (!Crypto::RSADecryptPrivate(&serverKey, offsetSignature, 256, reinterpret_cast<ByteT*>(&overflowProtection), sigSize))
    {
        return false;
    }

    if (sigSize != sizeof(SignatureType))
    {
        return false;
    }

    Crypto::SHA256HashType hash = Crypto::SHA256Hash(offsetMessage, encryptedMessageLength, sig->mSalt, sizeof(sig->mSalt));
    if (memcmp(hash.mData, sig->mHash, sizeof(sig->mHash)) != 0)
    {
        return false;
    }

    if (!sharedKey.Load(Crypto::AES_KEY_256, sig->mKey))
    {
        return false;
    }
    
    String publicKey;
    String encryptedMessage(encryptedMessageLength, reinterpret_cast<const char*>(offsetMessage), COPY_ON_WRITE);
    if (!Crypto::AESDecrypt(&sharedKey, sig->mIV, encryptedMessage, publicKey))
    {
        return false;
    }

    if (!clientKey.LoadPublicKey(publicKey))
    {
        return false;
    }

    const HeaderType* packetHeader = reinterpret_cast<const HeaderType*>(offsetBase);
    header.mAppID = packetHeader->mAppID;
    header.mAppVersion = packetHeader->mAppVersion;
    header.mCrc32 = packetHeader->mCrc32;
    header.mFlags = packetHeader->mFlags;
    header.mType = packetHeader->mType;
    return true;
}

} // namespace lf