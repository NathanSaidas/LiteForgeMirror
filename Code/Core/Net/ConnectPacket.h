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
#ifndef LF_CORE_CONNECT_PACKET_H
#define LF_CORE_CONNECT_PACKET_H
#include "Core/Net/NetTypes.h"

namespace lf {

namespace Crypto {
class AESKey;
class RSAKey;
} // namespace Crypto

// **********************************
// Clients can establish a connection with a server by first sending
// them a request to connect.
//
// The client needs to verify the authenticity of the 'server'
// as well as a way to have 'secure' communication (when required).
// 
// The client will first send off their 'client-public' key encrypted
// with AES-256. They will also send the generated 'AES-256' key to
// the server in the signature as well as hash w/ salt to verify data
// has not been tampered with. The signature is encrypted with the 
// server's public key.
//
// A server can then decrypt the signature.
//
// 
// **********************************
class LF_CORE_API ConnectPacket
{
public:
    struct Signature
    {
        ByteT mIV[16];
        ByteT mKey[32];
        ByteT mSalt[32];
        ByteT mHash[32];
    };

    using SignatureType = Signature;
    using HeaderType = PacketHeader;

    // Static class cannot be instantiated.
    ConnectPacket() = delete;
    ConnectPacket(const ConnectPacket&) = delete;
    ConnectPacket(ConnectPacket&& other) = delete;
    ~ConnectPacket() = delete;

    // Packet Structure:
    //   Request -> Ack -> Response -> Ack
    //   Message -> Ack
    //
    //
    // todo: This could become a static class.
    //
    // void Client:Connect()
    //      generate ClientKey;
    //      generate SharedKey;
    //      load     ServerKey;
    //      Construct(...)
    //      Send(bytes, byteLength)
    //      
    // void Server:OnReceive()
    //      Verify(AppID,AppVersion).OnFail( Drop() )
    //      VerifyCrc32().OnFail( Ack(Corrupt) )
    //      VerifyFlagTypeLogic().OnFail( Drop() )
    //      VerifyHandler().OnFail( Drop() )
    //      Ack(Success)
    //      Handler[Connect].OnPacket()
    //      Deconstruct(...).OnFail( Drop() )
    //      CreateConnection().OnMaxConnection( SecureSend(MaxConnection) )
    //      Connection.CreateKeyPair()
    //      ConnectResponse().Construct(...)
    //      Send(bytes, byteLength)
    //      Free()
    //
    // void Client:OnReceive()
    //      Verify(AppID,AppVersion).OnFail( Drop() )
    //      VerifyCrc32().OnFail( Ack(Corrupt) )
    //      VerifyFlagTypeLogic().OnFail( Drop() )
    //      VerifyHandler().OnFail( Drop() )
    //      Ack(Success)
    //      Handler[Connect].OnPacket()
    //      Deconstruct(...).OnFail( Drop() )
    //      SaveConnection()
    //      Free()
    //
    // void Server:OnReceive()
    static SizeT RequestSize(const Crypto::RSAKey& clientKey, const Crypto::AESKey& sharedKey);

    static bool ConstructRequest(
        ByteT* packetBytes,
        SizeT& packetBytesLength,
        const Crypto::RSAKey& clientKey,
        const Crypto::RSAKey& serverKey,
        const Crypto::AESKey& sharedKey);

    static bool DeconstructRequest(
        const ByteT* packetBytes, 
        SizeT packetBytesLength,
        const Crypto::RSAKey& serverKey,
        Crypto::RSAKey& clientKey,
        Crypto::AESKey& sharedKey,
        HeaderType& header);
};
} // namespace lf


#endif // LF_CORE_CONNECT_PACKET_H