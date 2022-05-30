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
#include "Core/Common/API.h"
#include "Core/Common/Enum.h"
#include "Core/Common/Types.h"
#include "Core/String/String.h"
#include "Runtime/Net/FileTransfer/FileTransferConstants.h"

namespace lf {

class Stream;



// ********************************** 
// 
// **********************************
struct LF_RUNTIME_API DownloadHash
{
public:
    DownloadHash();
    void Serialize(Stream& s);
    ByteT mBytes[32];
};
LF_INLINE Stream& operator<<(Stream& s, DownloadHash& o)
{
    o.Serialize(s);
    return s;
}


// ********************************** 
// This is the initial request sent by the 'client' to initiate a download
// the client should await the DownloadResponse before issuing further
// requests since they will need the RequestHandle;
// 
// **********************************
struct LF_RUNTIME_API DownloadRequest
{
    DownloadRequest();
    void Serialize(Stream& s);
    String mResourceIdentifier;
    Int32  mVersion;
    UInt32 mRequestID;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadRequest& o)
{
    o.Serialize(s);
    return s;
}

// ********************************** 
// This is the response sent back from the 'server' after the server
// receives and processes a request.
// **********************************
struct LF_RUNTIME_API DownloadResponse
{
    DownloadResponse();
    void Serialize(Stream& s);
    TDownloadResponseStatus mStatus;
    UInt32       mResourceHandle;
    UInt32       mResourceSize;
    DownloadHash mHash;
    UInt32       mChunkCount;
    UInt32       mFragmentCount;
    UInt32       mRequestID;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadResponse& o)
{
    o.Serialize(s);
    return s;
}

// ********************************** 
// This is a request a 'client' sends to the 'server' to have the server
// begin a 'send' operation on a specific chunk id, the server will send
// ALL fragments in that chunk.
// **********************************
struct LF_RUNTIME_API DownloadFetchRequest
{
    DownloadFetchRequest();
    void Serialize(Stream& s);
    UInt32 mResourceHandle;
    UInt32 mChunkID;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadFetchRequest& o)
{
    o.Serialize(s);
    return s;
}

// ********************************** 
// This is a request a 'client' sends to the 'server' should they need a specific
// set of fragments.
// **********************************
struct LF_RUNTIME_API DownloadFetchFragmentRequest
{
    DownloadFetchFragmentRequest();
    void Serialize(Stream& s);

    UInt32 mResourceHandle;
    UInt32 mChunkID;
    TVector<UInt32> mFragmentIDs;
    bool mUseRange;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadFetchFragmentRequest& o)
{
    o.Serialize(s);
    return s;
}

// ********************************** 
// This is a request a 'client' sends to the 'server' to stop the server from sending
// them anymore data for the specific chunk id.
// **********************************
struct LF_RUNTIME_API DownloadFetchStopRequest
{
    DownloadFetchStopRequest();
    void Serialize(Stream& s);

    UInt32 mResourceHandle;
    UInt32 mChunkID;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadFetchStopRequest& o)
{
    o.Serialize(s);
    return s;
}

struct LF_RUNTIME_API DownloadCompleteRequest
{
public:
    DownloadCompleteRequest();
    void Serialize(Stream& s);

    UInt32 mResourceHandle;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadCompleteRequest& o)
{
    o.Serialize(s);
    return s;
}

struct LF_RUNTIME_API DownloadFetchCompleteResponse
{
    DownloadFetchCompleteResponse();
    void Serialize(Stream& s);

    UInt32 mResourceHandle;
    UInt32 mChunkID;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadFetchCompleteResponse& o)
{
    o.Serialize(s);
    return s;
}

struct LF_RUNTIME_API DownloadFetchDataResponse
{
    DownloadFetchDataResponse();
    void Serialize(Stream& s);

    UInt32 mResourceHandle;
    UInt32 mChunkID;
    UInt32 mFragmentID;
    UInt32 mFragmentSize;
    TVector<ByteT> mData;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadFetchDataResponse& o)
{
    o.Serialize(s);
    return s;
}

struct LF_RUNTIME_API DownloadFetchStoppedResponse
{
    DownloadFetchStoppedResponse();
    void Serialize(Stream& s);

    UInt32 mResourceHandle;
    UInt32 mChunkID;
    TDownloadFetchStopReason mReason;
};
LF_INLINE Stream& operator<<(Stream& s, DownloadFetchStoppedResponse& o)
{
    o.Serialize(s);
    return s;
}

}