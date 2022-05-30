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
#include "Runtime/PCH.h"
#include "PacketSerializer.h"
#include "Core/Common/Assert.h"
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Utility/Crc32.h"

namespace lf {

struct BasePacketHeader
{
    ByteT mAppId[2];
    ByteT mAppVersion[2];
    ByteT mCrc32[4];
    ByteT mFlags[2];
    ByteT mType[1];
    ByteT mPacketUID[4];
    ByteT mSessionID[16];
};
struct SecurityPacketHeader
{
    ByteT mIV[Crypto::AES_IV_SIZE];
    ByteT mEncryptedHMAC[sizeof(Crypto::HMACBuffer)];
};

struct FullHeader
{
    BasePacketHeader     mBaseHeader;
    SecurityPacketHeader mSecurityHeader;
};

SizeT PacketSerializer::GetFullHeaderSize() { return sizeof(FullHeader); }

PacketSerializer::PacketSerializer()
: mBuffer(nullptr)
, mBufferCapacity(0)
, mDataSize(0)
, mEncryptedSecurity(false)
, mReadOnly(false)
{}

PacketSerializer::~PacketSerializer()
{

}

bool PacketSerializer::SetBuffer(ByteT* buffer, SizeT size)
{
    if (size < sizeof(FullHeader))
    {
        return false;
    }

    mBuffer = buffer;
    mDataSize = 0; // todo:
    mBufferCapacity = size;
    mReadOnly = false;
    return true;
}
bool PacketSerializer::SetBuffer(const ByteT* buffer, SizeT size)
{
    if (size < sizeof(FullHeader))
    {
        return false;
    }

    mBuffer = const_cast<ByteT*>(buffer);
    mDataSize = size - sizeof(FullHeader);
    mBufferCapacity = size;
    if(HasFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED))
    {
        const SizeT RSA_KEY_2048_BYTE_SIZE = 256;
        mDataSize -= RSA_KEY_2048_BYTE_SIZE;
    }
    if (HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC))
    {
        mDataSize -= sizeof(Crypto::HMACBuffer);
    }

    mReadOnly = true;
    return true;
}


void PacketSerializer::SetAppId(UInt16 appId)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mBaseHeader.mAppId, &appId, sizeof(appId));
}

UInt16 PacketSerializer::GetAppId() const
{
    CriticalAssert(mBuffer != nullptr);
    UInt16 appId;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(&appId, header->mBaseHeader.mAppId, sizeof(appId));
    return appId;
}

void PacketSerializer::SetAppVersion(UInt16 appVersion)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mBaseHeader.mAppVersion, &appVersion, sizeof(appVersion));
}

UInt16 PacketSerializer::GetAppVersion() const
{
    CriticalAssert(mBuffer != nullptr);
    UInt16 appVersion;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(&appVersion, header->mBaseHeader.mAppVersion, sizeof(appVersion));
    return appVersion;
}

void PacketSerializer::SetCrc32(UInt32 crc32)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mBaseHeader.mCrc32, &crc32, sizeof(crc32));
}

UInt32 PacketSerializer::GetCrc32() const
{
    CriticalAssert(mBuffer != nullptr);
    UInt32 crc32;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(&crc32, header->mBaseHeader.mCrc32, sizeof(crc32));
    return crc32;
}

void PacketSerializer::SetFlags(UInt16 flags)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mBaseHeader.mFlags, &flags, sizeof(flags));
}

UInt16 PacketSerializer::GetFlags() const
{
    CriticalAssert(mBuffer != nullptr);
    UInt16 flags;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(&flags, header->mBaseHeader.mFlags, sizeof(flags));
    return flags;
}

void PacketSerializer::SetFlag(const NetPacketFlag::Value flag)
{
    NetPacketFlag::Bitfield16Type bf(GetFlags());
    bf.Set(flag);
    SetFlags(bf.value);
}

void PacketSerializer::UnsetFlag(const NetPacketFlag::Value flag)
{
    NetPacketFlag::Bitfield16Type bf(GetFlags());
    bf.Unset(flag);
    SetFlags(bf.value);
}

bool PacketSerializer::HasFlag(const NetPacketFlag::Value flag) const
{
    NetPacketFlag::Bitfield16Type bf(GetFlags());
    return bf.Has(flag);
}

void PacketSerializer::SetType(UInt8 type)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mBaseHeader.mType, &type, sizeof(type));
}

UInt8 PacketSerializer::GetType() const
{
    CriticalAssert(mBuffer != nullptr);
    UInt8 type;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(&type, header->mBaseHeader.mType, sizeof(type));
    return type;
}
void PacketSerializer::SetPacketUID(UInt32 packetUID)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mBaseHeader.mPacketUID, &packetUID, sizeof(packetUID));
}
UInt32 PacketSerializer::GetPacketUID() const
{
    CriticalAssert(mBuffer != nullptr);
    UInt32 packetUid;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(&packetUid, header->mBaseHeader.mPacketUID, sizeof(packetUid));
    return packetUid;
}
void PacketSerializer::SetSessionID(const SessionID& sessionID)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mBaseHeader.mSessionID, sessionID.Bytes(), sessionID.Size());
}
SessionID PacketSerializer::GetSessionID() const
{
    CriticalAssert(mBuffer != nullptr);
    SessionID sessionId;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(sessionId.Bytes(), header->mBaseHeader.mSessionID, sessionId.Size());
    return sessionId;
}
void PacketSerializer::SetIV(const Crypto::AESIV& iv)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mSecurityHeader.mIV, iv.mBytes, sizeof(iv.mBytes));
}
Crypto::AESIV PacketSerializer::GetIV() const
{
    CriticalAssert(mBuffer != nullptr);
    Crypto::AESIV iv;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(iv.mBytes, header->mSecurityHeader.mIV, sizeof(iv.mBytes));
    return iv;
}
void PacketSerializer::SetEncryptedHMAC(const Crypto::HMACBuffer& hmac)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return;
    }
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(header->mSecurityHeader.mEncryptedHMAC, hmac.Bytes(), hmac.Size());
    mEncryptedSecurity = false;
}
Crypto::HMACBuffer PacketSerializer::GetEncryptedHMAC() const 
{
    CriticalAssert(mBuffer != nullptr);
    Crypto::HMACBuffer hmac;
    FullHeader* header = reinterpret_cast<FullHeader*>(mBuffer);
    memcpy(hmac.Bytes(), header->mSecurityHeader.mEncryptedHMAC, hmac.Size());
    return hmac;
}

bool PacketSerializer::SetData(const ByteT* data, SizeT size)
{
    CriticalAssert(mBuffer != nullptr);
    if (mReadOnly)
    {
        ReportBugMsg("Invalid operation, the serializer is set to read only.");
        return false;
    }

    SizeT available = mBufferCapacity - sizeof(FullHeader);
    if (available < size)
    {
        return false;
    }
    mDataSize = size;
    memcpy(GetDataPointer() , data, size);
    return true;
}

bool PacketSerializer::GetData(ByteT* outData, SizeT& inOutSize) const
{
    CriticalAssert(mBuffer != nullptr);
    if (mDataSize> inOutSize)
    {
        return false;
    }

    const ByteT* dataPointer = GetDataPointer();
    if (!dataPointer)
    {
        inOutSize = 0;
        return false;
    }

    memcpy(outData, dataPointer, mDataSize);
    inOutSize = mDataSize;
    return true;
}

SizeT PacketSerializer::GetPacketSize() const
{
    SizeT size = sizeof(FullHeader) + mDataSize;
    if (HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC))
    {
        size += sizeof(Crypto::HMACBuffer);
    }
    if (HasFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED))
    {
        const SizeT RSA_KEY_2048_BYTE_SIZE = 256;
        size += RSA_KEY_2048_BYTE_SIZE;
    }
    return size;
}

UInt32 PacketSerializer::CalcCrc32() const
{
    SizeT offset = offsetof(BasePacketHeader, mCrc32) + sizeof(BasePacketHeader::mCrc32);
    SizeT length = GetPacketSize() - offset;
    return Crc32(&mBuffer[offset], length);
}

bool PacketSerializer::ComputeHeaderHmac(const Crypto::HMACKey* key, Crypto::HMACBuffer& hmac)
{
    CriticalAssert(mBuffer != nullptr);
    if (key->Empty())
    {
        return false;
    }

    const FullHeader* header = reinterpret_cast<const FullHeader*>(mBuffer);

    const ByteT* begin = &header->mBaseHeader.mFlags[0];
    const ByteT* end = &header->mSecurityHeader.mEncryptedHMAC[0];

    if (!key->Compute(begin, end - begin, hmac))
    {
        return false;
    }
    return true;
}

bool PacketSerializer::SetDataHMAC(const Crypto::HMACBuffer& hmac)
{
    CriticalAssert(mBuffer != nullptr);
    if (mDataSize == 0)
    {
        ReportBugMsg("Invalid operation, cannot set the data hmac or a packet that has no data.");
        return false;
    }

    if (HasFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED))
    {
        return false;
    }

    SizeT remaining = sizeof(FullHeader) + mDataSize;
    if (remaining < hmac.Size())
    {
        return false; // not enough room
    }
    SetFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC);
    ByteT* hmacPointer = GetHmacPointer();
    memcpy(hmacPointer, hmac.Bytes(), hmac.Size());
    return true;
}
bool PacketSerializer::GetDataHMAC(Crypto::HMACBuffer& hmac) const
{
    CriticalAssert(mBuffer != nullptr);
    if (!HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC))
    {
        return false;
    }
    memcpy(hmac.Bytes(), GetHmacPointer(), hmac.Size());
    return true;
}

bool PacketSerializer::Sign(const Crypto::RSAKey* key)
{
    CriticalAssert(mBuffer != nullptr);
    if (!key || !key->HasPrivateKey() || key->GetKeySize() != Crypto::RSA_KEY_2048)
    {
        ReportBugMsg("Invalid argument 'key'.");
        return false;
    }

    if (mDataSize == 0)
    {
        ReportBugMsg("Invalid operation, cannot sign a packet which has no data.");
        return false;
    }

    // Calc remaining bytes
    SizeT remaining = sizeof(FullHeader) + mDataSize;
    if (HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC))
    {
        remaining += sizeof(Crypto::HMACBuffer);
    }
    CriticalAssert(remaining < mBufferCapacity);
    remaining = mBufferCapacity - remaining;
    if (remaining < key->GetKeySizeBytes())
    {
        return false; // Not enough room to sign
    }

    SetFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED);
    const ByteT* dataPointer = GetDataPointer();
    ByteT* signaturePointer = GetSignaturePointer();

    Crypto::SHA256Hash hash(dataPointer, mDataSize);
    if (!Crypto::RSAEncryptPrivate(key, hash.Bytes(), hash.Size(), signaturePointer, remaining))
    {
        UnsetFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED);
        return false;
    }

    return true;
}

bool PacketSerializer::Verify(const Crypto::RSAKey* key) const
{
    CriticalAssert(mBuffer != nullptr);

    if (!key || !key->HasPublicKey() || key->GetKeySize() != Crypto::RSA_KEY_2048)
    {
        ReportBugMsg("Invalid argument 'key'.");
        return false;
    }

    if (!HasFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED))
    {
        ReportBugMsg("Invalid operation the packet does not have the signature flag.");
        return false;
    }

    if (mDataSize == 0)
    {
        ReportBugMsg("Invalid operation the packet does not have data to sign.");
        return false;
    }

    Crypto::SHA256Hash hash(&mBuffer[sizeof(FullHeader)], mDataSize);
    const ByteT* signature = GetSignaturePointer();

    ByteT plainText[256] = { 0 };
    SizeT plainTextSize = sizeof(plainText);
    if (!Crypto::RSADecryptPublic(key, signature, key->GetKeySizeBytes(), plainText, plainTextSize))
    {
        return false;
    }

    if (plainTextSize != hash.Size() || memcmp(hash.Bytes(), plainText, plainTextSize) != 0)
    {
        return false;
    }

    return true;
}

ByteT* PacketSerializer::GetHeaderPointer()
{
    return mBuffer;
}
const ByteT* PacketSerializer::GetHeaderPointer() const
{
    return mBuffer;
}

ByteT* PacketSerializer::GetDataPointer()
{
    CriticalAssert(mBuffer || mDataSize == 0);
    return mDataSize > 0 ? &mBuffer[sizeof(FullHeader)] : nullptr;
}
const ByteT* PacketSerializer::GetDataPointer() const
{
    CriticalAssert(mBuffer || mDataSize == 0);
    return mDataSize > 0 ? &mBuffer[sizeof(FullHeader)] : nullptr;
}

ByteT* PacketSerializer::GetHmacPointer()
{
    CriticalAssert(mBuffer || mDataSize == 0);
    return mDataSize > 0 && HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC) ? &mBuffer[sizeof(FullHeader) + mDataSize] : nullptr;
}
const ByteT* PacketSerializer::GetHmacPointer() const
{
    CriticalAssert(mBuffer || mDataSize == 0);
    return mDataSize > 0 && HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC) ? &mBuffer[sizeof(FullHeader) + mDataSize] : nullptr;
}

ByteT* PacketSerializer::GetSignaturePointer()
{
    CriticalAssert(mBuffer || mDataSize == 0);
    if (mDataSize > 0 && HasFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED))
    {
        return HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC) ? 
              &mBuffer[sizeof(FullHeader) + mDataSize + sizeof(Crypto::HMACBuffer)]
            : &mBuffer[sizeof(FullHeader) + mDataSize];
    }
    return nullptr;
}
const ByteT* PacketSerializer::GetSignaturePointer() const
{
    CriticalAssert(mBuffer || mDataSize == 0);
    if (mDataSize > 0 && HasFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED))
    {
        return HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC) ?
            &mBuffer[sizeof(FullHeader) + mDataSize + sizeof(Crypto::HMACBuffer)]
            : &mBuffer[sizeof(FullHeader) + mDataSize];
    }
    return nullptr;
}

} // namespace lf