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
#include "NetClientController.h"
#include "Core/Common/Assert.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Utility/Log.h"
#include <utility>

namespace lf {

NetClientController::NetClientController()
: mServerKey()
, mKey()
, mSharedKey()
, mUniqueKey()
, mHMACKey()
, mChallenge()
, mConnectionID(INVALID_CONNECTION)
{
    memset(mHMACKey, 0, sizeof(mHMACKey));
    memset(mChallenge, 0, sizeof(mChallenge));
}
NetClientController::NetClientController(NetClientController&& other)
: mServerKey(std::move(other.mServerKey))
, mKey(std::move(other.mKey))
, mSharedKey(std::move(other.mSharedKey))
, mUniqueKey(std::move(other.mUniqueKey))
, mHMACKey()
, mChallenge()
, mConnectionID(other.mConnectionID)
{
    memcpy(mHMACKey, other.mHMACKey, sizeof(mHMACKey));
    memcpy(mChallenge, other.mChallenge, sizeof(mChallenge));
    memset(other.mHMACKey, 0, sizeof(other.mHMACKey));
    memset(other.mChallenge, 0, sizeof(other.mChallenge));
    other.mConnectionID = INVALID_CONNECTION;
}
NetClientController::~NetClientController()
{
    CriticalAssert(!IsConnected());
    Reset();
}

NetClientController& NetClientController::operator=(NetClientController&& other)
{
    if (this != &other)
    {
        mServerKey = std::move(other.mServerKey);
        mKey = std::move(other.mKey);
        mSharedKey = std::move(other.mSharedKey);
        mUniqueKey = std::move(other.mUniqueKey);
        memcpy(mHMACKey, other.mHMACKey, sizeof(mHMACKey));
        memcpy(mChallenge, other.mChallenge, sizeof(mChallenge));
        mConnectionID = other.mConnectionID;
        memset(other.mHMACKey, 0, sizeof(other.mHMACKey));
        memset(other.mChallenge, 0, sizeof(other.mChallenge));
        other.mConnectionID = INVALID_CONNECTION;
    }
    return *this;
}

bool NetClientController::Initialize(Crypto::RSAKey&& serverKey)
{
    if (serverKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        serverKey.Clear();
        return false;
    }

    if (IsConnected())
    {
        ReportBugMsg("Failed to initialize NetClientController because they are already connected.");
        return false;
    }

    if (!Crypto::IsSecureRandom())
    {
        gSysLog.Warning(LogMessage("NetClientController::Initialize running while SecureRandom is not turned on. This can present a security risk as the secure random number generator does not generated crypto-graphically-secure random numbers."));
    }

    mServerKey = std::move(serverKey);
    mKey.GeneratePair(Crypto::RSA_KEY_2048);
    mSharedKey.Generate(Crypto::AES_KEY_256);
    Crypto::SecureRandomBytes(mHMACKey, sizeof(mHMACKey));
    Crypto::SecureRandomBytes(mChallenge, sizeof(mChallenge));
    return true;
}
void NetClientController::Reset()
{
    mServerKey.Clear();
    mKey.Clear();
    mSharedKey.Clear();
    mUniqueKey.Clear();
    memset(mHMACKey, 0, sizeof(mHMACKey));
    memset(mChallenge, 0, sizeof(mChallenge));
    mConnectionID = INVALID_CONNECTION;
}

// void NetClientController::OnConnectFailed(ConnectionFailureMsg::Value reason)
// {
//     gSysLog.Error(LogMessage("Connection failed with reason = ") << reason);
// }
// 
// void NetClientController::OnConnectSuccess(ConnectionID connectionID, Crypto::RSAKey uniqueServerKey)
// {
//     gSysLog.Info(LogMessage("Connection succeeded with id = ") << connectionID);
// 
//     mUniqueKey = std::move(uniqueServerKey);
//     mConnectionID = connectionID;
// }
// void NetClientController::OnHeartbeatReceived(ByteT clientBytes[NET_HEARTBEAT_NONCE_SIZE], ByteT serverBytes[NET_HEARTBEAT_NONCE_SIZE])
// {
//     (clientBytes);
//     (serverBytes);
// }

bool NetClientController::IsConnected() const
{
    return mConnectionID != INVALID_CONNECTION;
}

bool NetClientController::SetConnectionID(ConnectionID connectionID, Crypto::RSAKey uniqueServerKey, ByteT serverNonce[NET_HEARTBEAT_NONCE_SIZE])
{
    Assert(connectionID != INVALID_CONNECTION);
    Assert(uniqueServerKey.GetKeySize() == Crypto::RSA_KEY_2048 && uniqueServerKey.HasPublicKey());
    if (IsConnected())
    {
        return false;
    }
    mUniqueKey = std::move(uniqueServerKey);
    memcpy(mServerNonce, serverNonce, NET_HEARTBEAT_NONCE_SIZE);
    Crypto::SecureRandomBytes(mClientNonce, NET_HEARTBEAT_NONCE_SIZE);
    mConnectionID = connectionID;
    return true;
}
bool NetClientController::SetNonce(ByteT clientNonce[NET_HEARTBEAT_NONCE_SIZE], ByteT serverNonce[NET_HEARTBEAT_NONCE_SIZE])
{
    if (memcmp(clientNonce, mClientNonce, NET_HEARTBEAT_NONCE_SIZE) != 0)
    {
        return false;
    }
    Crypto::SecureRandomBytes(mClientNonce, NET_HEARTBEAT_NONCE_SIZE);
    memcpy(mServerNonce, serverNonce, NET_HEARTBEAT_NONCE_SIZE);
    return true;
}

} // namespace lf