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
#ifndef LF_CORE_NET_CONNECTION_CONTROLLER_H
#define LF_CORE_NET_CONNECTION_CONTROLLER_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Net/NetTypes.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/UniqueNumber.h"
namespace lf {

DECLARE_ATOMIC_PTR(NetConnection);
DECLARE_ATOMIC_WPTR(NetConnection);

// This class will server the purpose of simply allocating a 'connection' and unique 'connection id'
// 
// todo: Make a 'NetDriver' class that wraps all this up
// todo: Make a Client class that wraps all the connection logic up.
// todo: Make a Server class that handles all the 'manage connection' logic.
// 
class LF_CORE_API NetConnectionController
{
public:
    using ConnectionMap = TMap<ConnectionID, NetConnectionAtomicPtr>;
    using UniqueNumberGen = UniqueNumber<ConnectionID, 100>;

    NetConnection* FindConnection(ConnectionID id);
    NetConnection* InsertConnection();
    bool DeleteConnection(ConnectionID id);

private:
    ConnectionMap mConnections;
    UniqueNumberGen mIDGenerator;

    // Resolve(EndPointAny) : Nullable<Connection>
    // Create(EndPointAny) : Nullable<Connection>
    // Drop(EndPointAny): Nullable<Connection>

    // ID => Connection
    //  => Connection
    //
};

} // namespace lf

#endif // LF_CORE_CONNECTION_MGR_H