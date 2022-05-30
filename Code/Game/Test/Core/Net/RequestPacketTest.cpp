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
#if 0
#include "Core/IO/Stream.h"
#include "Core/IO/TextStream.h"
#include "Core/Math/Random.h"
#include "Core/Math/MathFunctions.h"

#include "Core/String/String.h"
#include "Core/String/Token.h"
#include "Core/Utility/Time.h"
#include "Core/Utility/DateTime.h"
#include "Core/Utility/Log.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/SecureRandom.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "Runtime/Net/Packets/RequestPacket.h"
#include "Runtime/Net/NetRequestArgs.h"
#include "Runtime/Net/NetServerDriver.h"
#include "Runtime/Net/NetClientDriver.h"
#include "Runtime/Net/NetRequest.h"
#include "Runtime/Net/Controllers/NetRequestController.h"


#include "Game/Test/Core/Net/NetTestUtils.h"

namespace lf {

// We check the memory usage of our packet functions
// to verify we don't write before or after the allocated
// space.
const Int32 BYTE_CHECK = 0xF8;
const SizeT PACKET_BUFFER_SIZE = 4096;
const SizeT PACKET_USABLE_SIZE = 2048;


class TextRequest : public NetRequest
{
    DECLARE_CLASS(TextRequest, NetRequest);
public:
    TextRequest() {}
    virtual void Serialize(Stream& s) { (s); };

    bool Write() override
    {
        CriticalAssert(GetType());

        // Serialize to text
        String text;
        TextStream ts;
        ts.Open(Stream::TEXT, &text, Stream::SM_WRITE);
        if (ts.BeginObject(GetType()->GetFullName().CStr(), GetType()->GetSuper()->GetFullName().CStr()))
        {
            Serialize(ts);
            ts.EndObject();
            ts.Close();
        }
        else
        {
            return false;
        }

        SizeT bufferSize = text.Size() + 1;
        if (!PrepareBuffer(bufferSize))
        {
            return false;
        }

        memcpy(mBuffer, text.CStr(), text.Size());
        mBuffer[text.Size()] = 0;
        return true;
    }

    bool Read(const ByteT* sourceBytes, SizeT sourceLength) override
    {
        CriticalAssert(GetType());

        if (!sourceBytes)
        {
            return false;
        }

        SizeT actualLength = strnlen_s(reinterpret_cast<const char*>(sourceBytes), sourceLength);
        String text(actualLength, reinterpret_cast<const char*>(sourceBytes), COPY_ON_WRITE);
        TextStream ts(Stream::TEXT, &text, Stream::SM_READ);
        ts.BeginObject(GetType()->GetFullName().CStr(), GetType()->GetSuper()->GetFullName().CStr());
        Serialize(ts);
        ts.EndObject();
        ts.Close();
        return true;
    }

    const String& GetRoute() const { return mRoute; }
protected:
    TextRequest(const String& route)
    : Super()
    {
        mRoute = route;
    }

private:

    bool PrepareBuffer(SizeT size)
    {
        if (GetBufferLength() >= size)
        {
            return true;
        }

        // Cannot allocate enough memory
        if (!AllocatedMemory())
        {
            return false;
        }

        if (mBuffer)
        {
            Free(mBuffer);
            mBuffer = mBufferEnd = mBufferLast = nullptr;
        }

        mBuffer = Allocate(size);
        if (!mBuffer)
        {
            return false;
        }

        mBufferEnd = mBufferLast = (mBuffer + size);
        return true;
    }
};
DEFINE_CLASS(lf::TextRequest) { NO_REFLECTION; }

// RequestArg generators
namespace RequestGenerator
{
struct RequestLoginBody
{
    void Serialize(Stream& s)
    {
        SERIALIZE(s, mUserName, "");
        SERIALIZE(s, mSecret, "");
    }
    String mUserName;
    String mSecret;

    static char BODY[2048];
};
char RequestLoginBody::BODY[2048];
LF_INLINE Stream& operator<<(Stream& s, RequestLoginBody& d) { d.Serialize(s); }

void PrepareLoginBody()
{
    ByteT randomBytes[16];
    Crypto::SecureRandomBytes(randomBytes, sizeof(randomBytes));
    RequestGenerator::RequestLoginBody loginBody;
    loginBody.mUserName = "banana.man@yahoo.com";
    loginBody.mSecret = BytesToHex(randomBytes, sizeof(randomBytes));
    String requestText;
    TextStream ts;
    ts.Open(Stream::TEXT, &requestText, Stream::SM_WRITE);
    ts.BeginObject("RequestLoginBody", "Request");
    loginBody.Serialize(ts);
    ts.EndObject();
    ts.Close();
    strcpy_s(RequestGenerator::RequestLoginBody::BODY, sizeof(RequestGenerator::RequestLoginBody::BODY), requestText.CStr());
}

void GetFriends(NetRequestArgs& args)
{
    TEST(args.Set("GetFriends", "-guid /v=ffff:ffff:ffff:ffff", NetRequestBodyFormat::RBF_TEXT, nullptr, 0));
}
void GetFriendsIndex(NetRequestArgs& args)
{
    TEST(args.Set(22, "-guid /v=ffff:ffff:ffff:ffff", NetRequestBodyFormat::RBF_TEXT, nullptr, 0));
}
void Authenticate(NetRequestArgs& args)
{
    TEST(args.Set("Authenticate", "-guid /v=ffff:ffff:ffff:ffff", NetRequestBodyFormat::RBF_TEXT, RequestLoginBody::BODY, strlen(RequestLoginBody::BODY)));
}
void AuthenticateIndex(NetRequestArgs& args)
{
    TEST(args.Set(3, "-guid /v=ffff:ffff:ffff:ffff", NetRequestBodyFormat::RBF_TEXT, RequestLoginBody::BODY, strlen(RequestLoginBody::BODY)));
}
}


using NetRequestGenerator = void(*)(NetRequestArgs& args);
struct TestPacketInfo
{
    TestPacketInfo(Crypto::RSAKey& rsa, Crypto::AESKey& aes, Crypto::HMACKey& hmac)
    : mRSAKey(rsa)
    , mAESKey(aes)
    , mHMACKey(hmac)
    , mConnectionID(0)
    , mTransmitID(INVALID8)
    , mIV()
    {}

    Crypto::RSAKey& mRSAKey;
    Crypto::AESKey& mAESKey;
    Crypto::HMACKey& mHMACKey;

    ConnectionID mConnectionID;
    UInt32       mPacketUID;
    UInt8        mTransmitID;
    ByteT*       mIV;
};



// Mirror the string decoding function we use so that we can test it.
// fwd declare with 'TESTABLE == static' define might be good to
static bool TestableDecodeString(const char* base, const SizeT maxBufferLength, char* outBuffer, const SizeT maxOutBufferLength, SizeT& writtenBytes)
{
    if (maxBufferLength == 0)
    {
        return false;
    }

    writtenBytes = 0;
    // find null byte
    const char* nullByte = base;
    while (*nullByte != '\0')
    {
        ++nullByte;
        if (static_cast<SizeT>(nullByte - base) >= maxBufferLength)
        {
            return false;
        }
        if (static_cast<SizeT>(nullByte - base) >= maxOutBufferLength)
        {
            return false;
        }
    }

    writtenBytes = (nullByte - base);
    memcpy(outBuffer, base, writtenBytes);
    return true;
}

REGISTER_TEST(DecodeStringTest, "Core.Net")
{
    char writeable[10];
    char input[10];
    input[0] = 'a';
    input[1] = 'a';
    input[2] = 'a';
    input[3] = 'a';
    input[4] = 'a';
    input[5] = '\0';

    SizeT writtenBytes = 0;
    // Everything is as expected
    TEST(TestableDecodeString(input, 6, writeable, 10, writtenBytes) && writtenBytes == 5);
    // Oops the 'input' is not null terminated
    TEST(!TestableDecodeString(input, 5, writeable, 10, writtenBytes) && writtenBytes == 0);
    // Oops the 'input' is larger than the 'writeable' buffer
    TEST(!TestableDecodeString(input, 6, writeable, 5, writtenBytes) && writtenBytes == 0);
}

REGISTER_TEST(NetRequestArgsTest, "Core.Net")
{
    RequestGenerator::PrepareLoginBody();

    const String     TEST_ROUTE_STR = "TestRoute";
    const RouteIndex TEST_ROUTE_INDEX = 0x4F;
    const String     TEST_ROUTE_ARGS = "-guid /v=ffff:ffff:ffff:ffff";
    const String     TEST_BODY = "[a,b]\r\n{\r\n    Foo=5\r\n}\r\n";

    ByteT TEST_BODY_BYTES[256];
    strcpy_s(reinterpret_cast<char*>(TEST_BODY_BYTES), sizeof(TEST_BODY_BYTES), TEST_BODY.CStr());

    NetRequestArgs a;
    NetRequestArgs b;

    // todo: Larger buffer and bounds checking...
    ByteT bytes[256];
    SizeT bytesWritten = 0;
    ByteT readBuffer[256];

    TEST(a.Set(TEST_ROUTE_STR, TEST_ROUTE_ARGS, NetRequestBodyFormat::RBF_TEXT, nullptr, 0));
    TEST(a.GetRouteName() == TEST_ROUTE_STR);
    TEST(a.GetRouteArgs() == TEST_ROUTE_ARGS);
    TEST(Invalid(a.GetRouteIndex()));
    TEST(!a.UseRouteIndex());
    bytesWritten = a.Write(bytes, sizeof(bytes));
    TEST_CRITICAL(bytesWritten > 0);
    TEST_CRITICAL(b.Read(bytes, bytesWritten));

    TEST(a.UseRouteIndex() == b.UseRouteIndex());
    TEST(a.GetRouteName() == b.GetRouteName());
    TEST(a.GetRouteIndex() == b.GetRouteIndex());
    TEST(a.GetRouteArgs() == b.GetRouteArgs());
    TEST(b.GetBody() == nullptr);

    b.Clear();

    memset(bytes, 0, 40);
    bytes[0] = 0; // use route name
    TEST(!b.Read(bytes, 0)); // buffer not large enough
    TEST(!b.Read(bytes, 1)); // buffer not large enough
    TEST(!b.Read(bytes, 2)); // buffer not large enough
    TEST(b.Read(bytes, 3) && !b.Valid()); // buffer not large enough

    bytes[1] = 'A'; // RouteName==A
    bytes[3] = 'B'; // RouteArgs==B

    b.Clear();
    TEST(!b.Read(bytes, 0));
    TEST(!b.Read(bytes, 1));
    TEST(!b.Read(bytes, 2));
    TEST(!b.Read(bytes, 3));
    TEST(!b.Read(bytes, 4));
    TEST(b.Read(bytes, 5) && b.Valid());
    TEST(b.GetRouteName() == "A");
    TEST(b.GetRouteArgs() == "B");
    TEST(b.GetBody() == nullptr);

    bytes[0] = 1;
    bytes[1] = 72;
    bytes[2] = 'B';
    bytes[3] = 0;
    b.Clear();
    TEST(!b.Read(bytes, 0));
    TEST(!b.Read(bytes, 1));
    TEST(!b.Read(bytes, 2));
    TEST(!b.Read(bytes, 3));
    TEST(b.Read(bytes, 4) && b.Valid());

    TEST(a.Set(TEST_ROUTE_STR, TEST_ROUTE_ARGS, NetRequestBodyFormat::RBF_TEXT, RequestGenerator::RequestLoginBody::BODY, strlen(RequestGenerator::RequestLoginBody::BODY)));
    TEST(a.GetRouteName() == TEST_ROUTE_STR);
    TEST(a.GetRouteArgs() == TEST_ROUTE_ARGS);
    TEST(Invalid(a.GetRouteIndex()));
    TEST(!a.UseRouteIndex());
    TEST(a.GetBody() != nullptr);
    TEST(a.GetBodyFormat() == NetRequestBodyFormat::RBF_TEXT);
    TEST(a.GetBodyLength() == strlen(RequestGenerator::RequestLoginBody::BODY));
    TEST(memcmp(a.GetBody(), RequestGenerator::RequestLoginBody::BODY, strlen(RequestGenerator::RequestLoginBody::BODY)) == 0);

    bytesWritten = a.Write(bytes, sizeof(bytes));
    TEST(bytesWritten > 0);
    // Quick test to ensure we use a buffer
    b.Clear();
    TEST(!b.Read(bytes, bytesWritten));
    b.ReserveBody(readBuffer, sizeof(readBuffer));
    TEST(b.Read(bytes, bytesWritten) && b.Valid());
    TEST(b.GetRouteName() == TEST_ROUTE_STR);
    TEST(b.GetRouteArgs() == TEST_ROUTE_ARGS);
    TEST(Invalid(b.GetRouteIndex()));
    TEST(!b.UseRouteIndex());
    TEST(b.GetBody() != nullptr);
    TEST(b.GetBodyFormat() == NetRequestBodyFormat::RBF_TEXT);
    TEST(b.GetBodyLength() == strlen(RequestGenerator::RequestLoginBody::BODY));
    TEST(memcmp(b.GetBody(), RequestGenerator::RequestLoginBody::BODY, strlen(RequestGenerator::RequestLoginBody::BODY)) == 0);


}

class CustomRequest : public NetRequest
{
    DECLARE_CLASS(CustomRequest, NetRequest);
public:
    CustomRequest() : Super() { }
    ~CustomRequest() { }

    void Serialize(Stream& s)
    {
        SERIALIZE(s, mUsername, "");
        SERIALIZE(s, mPassword, "");
    }

    bool Write() override
    {
        CriticalAssert(GetType());

        String text;
        TextStream ts;
        ts.Open(Stream::TEXT, &text, Stream::SM_WRITE);
        ts.BeginObject(GetType()->GetFullName().CStr(), GetType()->GetSuper()->GetFullName().CStr());
        Serialize(ts);
        ts.EndObject();
        ts.Close();

        if (!mBuffer)
        {
            SizeT bufferSize = text.Size() + 1;
            mBuffer = Allocate(bufferSize);
            mBufferEnd = mBufferLast = (mBuffer + bufferSize);
            memcpy(mBuffer, text.CStr(), text.Size());
            mBuffer[text.Size()] = 0;
        }

        return true;
    }
    bool Read(const ByteT*, SizeT) override
    {
        CriticalAssert(GetType());

        if (!mBuffer)
        {
            return false;
        }

        SizeT actualLength = strnlen_s(reinterpret_cast<const char*>(mBuffer), GetBufferCapacity());
        String text(actualLength, reinterpret_cast<const char*>(mBuffer), COPY_ON_WRITE);
        TextStream ts(Stream::TEXT, &text, Stream::SM_READ);
        ts.BeginObject(GetType()->GetFullName().CStr(), GetType()->GetSuper()->GetFullName().CStr());
        Serialize(ts);
        ts.EndObject();
        ts.Close();
        return true;
    }

    void SetUsername(const String& username) { mUsername = username; }
    void SetPassword(const String& password) { mPassword = password; }
    const String& GetUsername() const { return mUsername; }
    const String& GetPassword() const { return mPassword; }
private:
    String mUsername;
    String mPassword;
};
DEFINE_CLASS(lf::CustomRequest) { NO_REFLECTION; }
DECLARE_ATOMIC_PTR(CustomRequest);

REGISTER_TEST(BasicNetRequestTest, "Core.Net")
{
    NetTestInitializer netInit;

    const String USERNAME = "nana.lan@gmail.com";
    const String PASSWORD = "xXY$33742069Xx";

    CustomRequestAtomicPtr request = GetReflectionMgr().CreateAtomic<CustomRequest>();

    // Exercise setting up a request.
    //
    // This can be done in 'user code'
    request->SetUsername(USERNAME);
    request->SetPassword(PASSWORD);
    // This can be done on a background thread.
    TEST(request->Write());

    // Exercise receiving a request
    request->SetUsername(String());
    request->SetPassword(String());
    // We somehow get the bytes?
    // request->WriteBuffer(pointer_from_packet, buffer_length);

    // Before we send the request off to 'user code' make the data 'readable'
    TEST(request->Read(nullptr, 0));

    TEST(request->GetUsername() == USERNAME);
    TEST(request->GetPassword() == PASSWORD);

    
    // IN: (client)
    // -- [      User] Set data in user format
    // -- [Background] Serialize data to buffer
    // -- [Background] Copy data to packet for transport
    // -- [Background] On ack received, clear buffer
    
    // OUT: (server)
    // -- [Background] Pass ptr and buffer length. (Copy if next step is async)
    // -- [Background] Serialize buffer to user data.
    // -- [      User] Process request

}

REGISTER_TEST(NetRequestControllerTest, "Core.Net")
{
    NetTestInitializer netInit;

    Crypto::RSAKey serverKey;
    TEST(serverKey.GeneratePair(Crypto::RSA_KEY_2048));

    NetServerDriver server;
    TEST(server.Initialize(serverKey, 27015, NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION));

    

    IPEndPointAny endPoint;
    IPCast(IPV4("127.0.0.1", 27015), endPoint);
    NetClientDriver client;
    TEST(client.Initialize(serverKey, endPoint, NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION));

    Int64 frequency = GetClockFrequency();
    Int64 beginTick = GetClockTime();

    while (true)
    {
        Float64 duration = (GetClockTime() - beginTick) / static_cast<Float64>(frequency);
        if (duration > 5.0)
        {
            break;
        }

        if (client.IsConnected())
        {
            break;
        }

        server.Update();

    }

    SizeT frame = 0;
    while (true)
    {
        Float64 duration = (GetClockTime() - beginTick) / static_cast<Float64>(frequency);
        if (duration > 30.0)
        {
            break;
        }


        if (!client.IsConnected())
        {
            break;
        }

        client.EmitHeartbeat();
        client.Update();
        server.Update();

        Thread::Sleep(32);
        ++frame;
    }

    gTestLog.Info(LogMessage("Completed ") << frame << " frames.");

    // NetRequestController controller(transport, clientKey, uniqueKey, hmacKey, sharedKey, connectionID);
    // Thread::Sleep(250);
    // client.GetRequestController().Send(request, requestData, requestDataSize, PacketSecurity::PS_NONE, INVALID8, false);
    // Thread::Sleep(2250);

    client.Shutdown();
    server.Shutdown();

}

class MailRequest : public TextRequest
{
    DECLARE_CLASS(MailRequest, TextRequest);
public:
    MailRequest() : Super("services/ingame/mail") {}

    void Serialize(Stream& s) override
    {
        SERIALIZE(s, mSenderId, "");
        SERIALIZE(s, mReceiverId, "");
        SERIALIZE(s, mTitle, "");
        SERIALIZE(s, mDescription, "");
        SERIALIZE(s, mServiceCost, "");
        SERIALIZE_ARRAY(s, mItemIds, "");
        SERIALIZE(s, mGoldSent, "");
    }

    void SetItemIds(const TVector<String>& value) { mItemIds = value; }
    void SetSenderId(const String& value) { mSenderId = value; }
    void SetReceiverId(const String& value) { mReceiverId = value; }
    void SetTitle(const String& value) { mTitle = value; }
    void SetDescription(const String& value) { mDescription = value; }
    void SetGoldSent(Int32 value) { mGoldSent = value; }
    void SetServiceCost(Int32 value) { mServiceCost = value; }
    const TVector<String>& GetItemIds() const { return mItemIds; }
    const String& GetSenderId() const { return mSenderId; }
    const String& GetReceiverId() const { return mReceiverId; }
    const String& GetTitle() const { return mTitle; }
    const String& GetDescription() const { return mDescription; }
    Int32 GetGoldSent() const { return mGoldSent; }
    Int32 GetServiceCost() const { return mServiceCost; }

private:
    TVector<String> mItemIds;
    String mSenderId;
    String mReceiverId;
    String mTitle;
    String mDescription;
    Int32 mGoldSent;
    Int32 mServiceCost;
};
DECLARE_ATOMIC_PTR(MailRequest);
DECLARE_ATOMIC_WPTR(MailRequest);
DEFINE_CLASS(lf::MailRequest) { NO_REFLECTION; }

// An example request to apply transmogrification to an item for a character.
class TransmogrifyRequest : public TextRequest
{
    DECLARE_CLASS(TransmogrifyRequest, TextRequest);
public:
    TransmogrifyRequest() : Super("skills/vendor/transmogrify") {}

    void Serialize(Stream& s) override
    {
        SERIALIZE(s, mCharacterId, "");
        SERIALIZE(s, mItemAppearanceId, "");
        SERIALIZE(s, mItemId, "");
        SERIALIZE(s, mGoldCost, "");
    }

    void SetCharacterId(const String& value) { mCharacterId = value; }
    void SetItemAppearanceId(const String& value) { mItemAppearanceId = value; }
    void SetItemId(Int32 value) { mItemId = value; }
    void SetGoldCost(Int32 value) { mGoldCost = value; }
    const String& GetCharacterId() const { return mCharacterId; }
    const String& GetItemAppearanceId() const { return mItemAppearanceId; }
    Int32 GetItemId() const { return mItemId; }
    Int32 GetGoldCost() const { return mGoldCost; }
private:
    String mCharacterId;
    String mItemAppearanceId;
    Int32  mItemId;
    Int32  mGoldCost;
};
DECLARE_ATOMIC_PTR(TransmogrifyRequest);
DECLARE_ATOMIC_WPTR(TransmogrifyRequest);
DEFINE_CLASS(lf::TransmogrifyRequest) { NO_REFLECTION; }

// for now we'll work with connection unordered packets
// 

static String RandomId()
{
    ByteT buffer[24];
    Crypto::SecureRandomBytes(buffer, sizeof(buffer));
    return BytesToHex(buffer, sizeof(buffer));
}

static Int32 RandomItemId()
{
    Int32 id;
    Crypto::SecureRandomBytes(reinterpret_cast<ByteT*>(&id), 4);
    return Abs(id);
}

static Int32 RandomEntry(Int32 maxEntry)
{
    return RandomItemId() % maxEntry;
}

void SendTransmogrify(NetRequestController& controller)
{
    ByteT buffer[24];

    TransmogrifyRequestAtomicPtr request = GetReflectionMgr().CreateAtomic<TransmogrifyRequest>();
    request->SetCharacterId(RandomId());
    request->SetItemAppearanceId(RandomId());
    Crypto::SecureRandomBytes(buffer, 4);
    request->SetItemId(*reinterpret_cast<Int32*>(buffer));
    request->SetGoldCost(250);

    controller.Send(request, PacketSecurity::PS_ENCRYPTED, INVALID8, false);
}

void SendMail(NetRequestController& controller)
{
    String titles[] = { "Hey Chelsea", "Hey Kris", "Hey Amanda" };
    String descriptions[] = { "Here are the jewels", "Where are my trade goods?!", ":)" };

    MailRequestAtomicPtr request = GetReflectionMgr().CreateAtomic<MailRequest>();
    request->SetSenderId(RandomId());
    request->SetReceiverId(RandomId());
    request->SetTitle(titles[RandomEntry(LF_ARRAY_SIZE(titles))]);
    request->SetDescription(descriptions[RandomEntry(LF_ARRAY_SIZE(titles))]);
    request->SetGoldSent(RandomEntry(5000));
    request->SetServiceCost(30);

    TVector<String> ids;
    for (SizeT i = 0, k = RandomEntry(4); i < k; ++i)
    {
        ids.Add(RandomId());
    }
    request->SetItemIds(ids);

    controller.Send(request, PacketSecurity::PS_ENCRYPTED, INVALID8, false);
}

REGISTER_TEST(CurrentTest, "Core.Net")
{
    Crypto::RSAKey client;
    Crypto::RSAKey server;
    Crypto::HMACKey hmac;
    Crypto::AESKey shared;
    client.GeneratePair(Crypto::RSA_KEY_2048);
    server.GeneratePair(Crypto::RSA_KEY_2048);
    Crypto::SecureRandomBytes(hmac.Bytes(), Crypto::HMAC_KEY_SIZE);
    shared.Generate(Crypto::AES_KEY_256);

    NetRequestController controller;
    controller.Init(nullptr, &client, &server, &hmac, &shared, 17);
    controller.Update();

    Int32 seed = 4012020;

    for (SizeT i = 0; i < 175; ++i)
    {
        if (Random::Range(seed, 0 , 100) < 50)
        {
            SendTransmogrify(controller);
        }
        else
        {
            SendMail(controller);
        }
    }

    controller.Update();

}

void SendPacketFrom(NetDriver* driver)
{
    PacketData2048 packet;
    PacketData::SetZero(packet);

    MailRequestAtomicPtr request = GetReflectionMgr().CreateAtomic<MailRequest>();
    request->SetSenderId(RandomId());
    request->SetReceiverId(RandomId());
    request->SetTitle("New Mail!");
    request->SetDescription("Wow new mail description, amazing!");
    request->SetGoldSent(RandomEntry(5000));
    request->SetServiceCost(30);

    TVector<String> ids;
    for (SizeT i = 0; i < 4; ++i)
    {
        ids.Add(RandomId());
    }
    request->SetItemIds(ids);
    TEST(request->Write());

    RequestPacketWriter writer(packet.mBytes, packet.mSize, sizeof(packet.mBytes));
    writer.WriteConnectedHeader(NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION, 0, 0, PacketSecurity::PS_NONE);
    writer.WriteHeader(1, request->GetBufferLength(), PacketSecurity::PS_NONE, request->GetRoute());
    writer.WriteBody(request->GetBuffer(), request->GetBufferLength());
    writer.WriteFooter(nullptr, nullptr, nullptr, nullptr);

    driver->SendRawPacket(packet.mBytes, static_cast<SizeT>(packet.mSize), nullptr);
}

REGISTER_TEST(RequestPacketAckTest, "Core.Net")
{
    NetTestInitializer netInit;

    enum State
    {
        None,
        SendPacket,
        Wait
    };

    State state = None;
    auto host = LocalClientServer::Create("127.0.0.1", 27015);
    host->OnUpdate([&](NetDriver* server, NetDriver* client)
    {
        (server);
        switch (state)
        {
            case None:
                state = SendPacket;
                break;
            case SendPacket:
                SendPacketFrom(client);
                state = Wait;
                break;
            case Wait:
                break;
        }
    });
    host->ClientOnConnect([]()
    {
        gSysLog.Info(LogMessage("On client connected..."));
    });
    host->ClientOnDisconnect([]()
    {
        gSysLog.Info(LogMessage("On client disconnected..."));
    });
    host->Run();
}

REGISTER_TEST(RequestPacketAckEncodeDecodeTest, "Core.Net")
{
    PacketData2048 packet;
    PacketData::SetZero(packet);
    packet.mSize = sizeof(packet.mBytes);
    SizeT packetSize = static_cast<SizeT>(packet.mSize);

    Crypto::RSAKey key;
    TEST_CRITICAL(key.GeneratePair(Crypto::RSA_KEY_2048));

    Crypto::HMACBuffer inHmac;
    Crypto::SecureRandomBytes(inHmac.Bytes(), Crypto::HMAC_HASH_SIZE);

    TVector<NetAckStatus::Value> ackStatusTestCases;
    TVector<PacketSecurity::Value> packetSecurityTestCases;
    TVector<PacketUID> packetUIDTestCases;
    TVector<ConnectionID> connectionIDTestCases;

    for (SizeT i = 0; i < NetAckStatus::MAX_VALUE; ++i)
    {
        ackStatusTestCases.Add(static_cast<NetAckStatus::Value>(i));
    }

    for (SizeT i = 0; i < PacketSecurity::MAX_VALUE; ++i)
    {
        packetSecurityTestCases.Add(static_cast<PacketSecurity::Value>(i));
    }

    packetUIDTestCases.Add(348);
    packetUIDTestCases.Add(666);
    packetUIDTestCases.Add(0);
    packetUIDTestCases.Add(1);
    packetUIDTestCases.Add(2);

    connectionIDTestCases.Add(0);
    connectionIDTestCases.Add(488973);
    connectionIDTestCases.Add(INVALID_CONNECTION);
    connectionIDTestCases.Add(498992);

    auto ExecuteTestCase = [&]
    (
        NetAckStatus::Value inAckStatus,
        PacketSecurity::Value inSecurity,
        PacketUID inPacketUID,
        ConnectionID inConnectionID
    )
    {
        PacketData::SetZero(packet);
        packet.mSize = sizeof(packet.mBytes);
        packetSize = static_cast<SizeT>(packet.mSize);

        const bool testHMAC = inSecurity >= PacketSecurity::PS_SIGNED;
        const bool testConnection = Valid(inConnectionID);

        bool requestPacketEncoded = RequestPacket::EncodeAck(
            packet.mBytes,
            packetSize,
            inSecurity,
            inAckStatus,
            &inHmac,
            inConnectionID,
            inPacketUID,
            &key);

        TEST(requestPacketEncoded);

        PacketSecurity::Value outSecurity;
        NetAckStatus::Value   outAckStatus;
        PacketUID             outPacketUID;
        Crypto::HMACBuffer    outHmac;
        ConnectionID          outConnectionID = INVALID_CONNECTION;

        bool requestPacketDecoded = RequestPacket::DecodeAck(
            packet.mBytes,
            packetSize,
            outSecurity,
            outAckStatus,
            outHmac,
            testConnection ? &outConnectionID : nullptr,
            outPacketUID,
            &key);

        TEST(requestPacketDecoded);
        TEST(inSecurity == outSecurity);
        TEST(inAckStatus == outAckStatus);
        TEST(inPacketUID == outPacketUID);

        if (testHMAC)
        {
            TEST(inHmac == outHmac);
        }

        if (testConnection)
        {
            TEST(inConnectionID == outConnectionID);
        }
    };

    for (auto ackStatus : ackStatusTestCases)
    {
        for (auto security : packetSecurityTestCases)
        {
            for (auto uid : packetUIDTestCases)
            {
                for (auto connectionId : connectionIDTestCases)
                {
                    ExecuteTestCase(ackStatus, security, uid, connectionId);
                }
            }
        }
    }

}

} // namespace lf
#endif