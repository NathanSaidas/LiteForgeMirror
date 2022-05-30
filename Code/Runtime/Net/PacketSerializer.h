#pragma once
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

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Net/NetTypes.h"

namespace lf {
namespace Crypto {
class RSAKey;
} // namespace Crypto

// Helper class eases the ability of setting certain attributes of a packet.
// 
// Order of operation reference:
//   * Any operation in the same group can be done in any order of that group
//   *
//
// -------------
// SetAppId
// SetAppVersion
// SetFlags
// SetType
// SetPacketUID
// SetSessionID
// SetIV
// -------------
// SetData
// -------------
// SetDataHMAC    *Hmac must know how much data there is in order to place it at the right spot
// ------------- 
// Sign           *Signature needs to know how much data there is
// ------------- 
// SetHeaderHmac  *Previous operations may modify the header at anytime
// -------------
// CalcCrc32      *Crc32 is calculated on the entire packet after AppId/AppVersion/Crc32 memory.
class LF_RUNTIME_API PacketSerializer
{
public:
    static SizeT GetFullHeaderSize();

    PacketSerializer();
    ~PacketSerializer();

    bool SetBuffer(ByteT* buffer, SizeT size);
    bool SetBuffer(const ByteT* buffer, SizeT size);

    void SetAppId(UInt16 appId);
    UInt16 GetAppId() const;
    void SetAppVersion(UInt16 appVersion);
    UInt16 GetAppVersion() const;
    void SetCrc32(UInt32 crc32);
    UInt32 GetCrc32() const;
    void SetFlags(UInt16 flags);
    UInt16 GetFlags() const;
    void SetFlag(const NetPacketFlag::Value flag);
    void UnsetFlag(const NetPacketFlag::Value flag);
    bool HasFlag(const NetPacketFlag::Value flag) const;
    void SetType(UInt8 type);
    UInt8 GetType() const;
    void SetPacketUID(UInt32 packetUID);
    UInt32 GetPacketUID() const;
    void SetSessionID(const SessionID& sessionID);
    SessionID GetSessionID() const;
    void SetIV(const Crypto::AESIV& iv);
    Crypto::AESIV GetIV() const;
    void SetEncryptedHMAC(const Crypto::HMACBuffer& hmac);
    Crypto::HMACBuffer GetEncryptedHMAC() const;

    bool SetData(const ByteT* data, SizeT size);
    bool GetData(ByteT* outData, SizeT& inOutSize) const;

    SizeT GetPacketSize() const;

    UInt32 CalcCrc32() const;
    bool ComputeHeaderHmac(const Crypto::HMACKey* key, Crypto::HMACBuffer& hmac);

    bool SetDataHMAC(const Crypto::HMACBuffer& hmac);
    bool GetDataHMAC(Crypto::HMACBuffer& hmac) const;
    bool Sign(const Crypto::RSAKey* key);
    bool Verify(const Crypto::RSAKey* key) const;
private:
    

    ByteT* GetHeaderPointer();
    const ByteT* GetHeaderPointer() const;

    ByteT* GetDataPointer();
    const ByteT* GetDataPointer() const;

    ByteT* GetHmacPointer();
    const ByteT* GetHmacPointer() const;

    ByteT* GetSignaturePointer();
    const ByteT* GetSignaturePointer() const;

    ByteT* mBuffer;
    SizeT  mBufferCapacity;
    SizeT  mDataSize;

    bool   mEncryptedSecurity;
    bool   mReadOnly;
};

} // namespace lf