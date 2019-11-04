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
#include "NetServerController.h"
#include <utility>

namespace lf {

NetServerController::NetServerController()
: mServerKey()
{
}
NetServerController::NetServerController(NetServerController&& other)
: mServerKey(std::move(other.mServerKey))
{

}
NetServerController::~NetServerController()
{
    Reset();
}

NetServerController& NetServerController::operator=(NetServerController&& other)
{
    if (this != &other)
    {
        mServerKey = std::move(other.mServerKey);
    }
    return *this;
}

bool NetServerController::Initialize(Crypto::RSAKey&& serverKey)
{
    if (serverKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        serverKey.Clear();
        return false;
    }

    mServerKey = std::move(serverKey);
    return true;
}
void NetServerController::Reset()
{
    mServerKey.Clear();
}

}