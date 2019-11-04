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
#include "NetConnectionController.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Net/NetConnection.h"
#include "Core/Utility/Time.h"

namespace lf {

NetConnectionController::NetConnectionController() 
: mConnections()
, mIDGenerator()
{

}
NetConnectionController::~NetConnectionController()
{

}

void NetConnectionController::Reset()
{
    ScopeRWLockWrite writeLock(mConnectionLock);
    mConnections.clear();
    mIDGenerator = UniqueNumberGen();
}

NetConnection* NetConnectionController::FindConnection(ConnectionID id)
{
    ScopeRWLockRead readLock(mConnectionLock);
    auto it = mConnections.find(id);
    if (it != mConnections.end())
    {
        Assert(it->second->mID == id);
        return it->second;
    }
    return nullptr;
}

NetConnection* NetConnectionController::InsertConnection()
{
    NetConnectionAtomicPtr connection = MakeConvertableAtomicPtr<NetConnection>();
    memset(connection->mClientNonce, 0, sizeof(connection->mClientNonce));
    Crypto::SecureRandomBytes(connection->mServerNonce, sizeof(connection->mServerNonce));
    
    ScopeRWLockWrite writeLock(mConnectionLock);
    connection->mID = mIDGenerator.Allocate();

    auto iter = mConnections.insert(std::make_pair(connection->mID, connection));
    // Verify it was inserted!
    Assert(iter.second == true);
    return iter.first->second;
}

bool NetConnectionController::DeleteConnection(ConnectionID id)
{
    ScopeRWLockWrite writeLock(mConnectionLock);

    auto it = mConnections.find(id);
    if (it == mConnections.end())
    {
        return false;
    }

    Assert(it->second->mID == id);
    mIDGenerator.Free(it->second->mID);
    mConnections.erase(id);
    return true;
}

void NetConnectionController::Update(TArray<NetConnectionAtomicPtr>& disconnected)
{
    const SizeT before = disconnected.Size();

    // Calculate disconnected 'read lock only, allow looks up'
    {
        ScopeRWLockRead readLock(mConnectionLock);
        Float64 frequency = static_cast<Float64>(GetClockFrequency());
        Int64 currentTime = GetClockTime();
        for (ConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); ++it)
        {
            Float64 latency = (currentTime - it->second->mLastTick) / frequency;
            if (latency > 0.500)
            {
                disconnected.Add(it->second);
                // todo: mark Disconnected
            }
        }
    }

    // We can specify a budget time later, but we should remove them from the connection list seperately
    if (before != disconnected.Size())
    {
        ScopeRWLockWrite writeLock(mConnectionLock);
        for (NetConnection* connection : disconnected)
        {
            auto it = mConnections.find(connection->mID);
            if (it != mConnections.end())
            {
                mConnections.erase(it);
                mIDGenerator.Free(connection->mID);
            }
        }
    }
}

}