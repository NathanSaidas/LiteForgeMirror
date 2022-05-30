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
#include "NetSerialization.h"
#include "Core/IO/Stream.h"
#include "Core/IO/BinaryStream.h"
#include "Core/Memory/MemoryBuffer.h"

namespace lf {

Stream& operator<<(Stream& s, SessionIDSerialized& o)
{
    if (o.mItem)
    {
        s.SerializeGuid(o.mItem->Bytes(), o.mItem->Size());
    }
    return s;
}

LF_RUNTIME_API Stream& operator<<(Stream& s, NetOneTimeKeyMsg& o)
{
    SERIALIZE(s, o.mOneTimeKey, "");
    SERIALIZE(s, o.mOneTimeIV, "");
    return s;
}

Stream& operator<<(Stream& s, NetClientHelloMsg& o)
{
    SERIALIZE(s, o.mClientHandshakeKey, "");
    SERIALIZE(s, o.mClientHandshakeHmac, "");
    SERIALIZE(s, o.mClientSigningKey, "");
    return s;
}

Stream& operator<<(Stream& s, NetServerHelloRSAMsg& o)
{
    SERIALIZE(s, o.mServerHandshakeKey, "");
    SERIALIZE(s, o.mIV, "");
    return s;
}

Stream& operator<<(Stream& s, NetServerHelloMsg& o)
{
    SERIALIZE(s, o.mSessionID, "");
    SERIALIZE(s, o.mServerHandshakeHMAC, "");
    SERIALIZE(s, o.mServerSigningKey, "");
    return s;
}

template<typename MessageT>
bool ReadAllBytes(const ByteT* bytes, SizeT numBytes, MessageT& msg)
{
    MemoryBuffer buffer(const_cast<ByteT*>(bytes), numBytes);
    Assert(buffer.Allocate(numBytes, 1) && buffer.GetSize() == numBytes);
    BinaryStream bs(Stream::MEMORY, &buffer, Stream::SM_READ);
    if (bs.BeginObject("o", "o"))
    {
        bs << msg;
        bs.EndObject();
    }
    else
    {
        return false;
    }
    bs.Close();
    return true;
}

template<typename MessageT>
bool WriteAllBytes(ByteT* bytes, SizeT& numBytes, MessageT& msg)
{
    MemoryBuffer buffer(bytes, numBytes);
    BinaryStream bs(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    Assert(bs.BeginObject("o", "o"));
    bs << msg;
    bs.EndObject();
    bs.Close();
    numBytes = buffer.GetSize();
    return true;
}

bool NetSerialization::ReadAllBytes(const ByteT* bytes, SizeT numBytes, NetOneTimeKeyMsg& msg) { return ::lf::ReadAllBytes(bytes, numBytes, msg); }
bool NetSerialization::ReadAllBytes(const ByteT* bytes, SizeT numBytes, NetClientHelloMsg& msg) { return ::lf::ReadAllBytes(bytes, numBytes, msg); }
bool NetSerialization::ReadAllBytes(const ByteT* bytes, SizeT numBytes, NetServerHelloRSAMsg& msg) { return ::lf::ReadAllBytes(bytes, numBytes, msg); }
bool NetSerialization::ReadAllBytes(const ByteT* bytes, SizeT numBytes, NetServerHelloMsg& msg) { return ::lf::ReadAllBytes(bytes, numBytes, msg); }


bool NetSerialization::WriteAllBytes(ByteT* bytes, SizeT& numBytes, NetOneTimeKeyMsg& msg) { return ::lf::WriteAllBytes(bytes, numBytes, msg); }
bool NetSerialization::WriteAllBytes(ByteT* bytes, SizeT& numBytes, NetClientHelloMsg& msg) { return ::lf::WriteAllBytes(bytes, numBytes, msg); }
bool NetSerialization::WriteAllBytes(ByteT* bytes, SizeT& numBytes, NetServerHelloRSAMsg& msg) { return ::lf::WriteAllBytes(bytes, numBytes, msg); }
bool NetSerialization::WriteAllBytes(ByteT* bytes, SizeT& numBytes, NetServerHelloMsg& msg) { return ::lf::WriteAllBytes(bytes, numBytes, msg); }

} // namespace lf 