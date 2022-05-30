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
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/String/String.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/DateTime.h"
#include "Runtime/Net/FileTransfer/FileTransferConstants.h"

namespace lf {

DECLARE_ATOMIC_WPTR(NetConnection);
DECLARE_ATOMIC_WPTR(FileTransferRequest);

struct LF_RUNTIME_API FileResourceInfo
{
    FileResourceInfo();

    String              mName;
    DateTime            mLastModifyTime;
    SizeT               mSize;
    ByteT               mHash[FILE_SERVER_HASH_SIZE];
    SizeT               mFragmentCount;
    SizeT               mChunkCount;
};

struct LF_RUNTIME_API FileResourceChunk
{
    FileResourceChunk();

    // ** Copy data at the specified fragment id to the buffer. Return the number of bytes copied.
    SizeT CopyFragment(SizeT fragmentID, ByteT* buffer, SizeT bufferCapacity) const;
    // ** Returns the number of fragments in the chunk
    SizeT GetFragmentCount() const;
    // ** Computes the SHA256 hash for the data.
    bool ComputeHash(ByteT* buffer, SizeT bufferCapacity) const;

    // ** An array of the full chunk. Fragments are every FILE_SERVER_MAX_FRAGMENT_SIZE
    TVector<ByteT> mData;
};

struct LF_RUNTIME_API FileResourceHandle : public TAtomicWeakPointerConvertible<FileResourceHandle>
{
    using Super = TAtomicWeakPointerConvertible<FileResourceHandle>;

    enum { MAX_OPS = 6 };

    enum State
    {
        // ** The handle was just created, it must begin the download with a DownloadRequest
        Initialization,
        // ** The handle has made a DownloadRequest and is awaiting confirmation.
        RequestDownload,
        // ** The client received the DownloadResponse and is now downloading the resource
        Downloading,
        // ** The client has completed the download
        Complete,
        // ** The client failed a download step (network/internal error)
        Failed
    };

    FileResourceHandle();
    void SetState(State value);
    State GetState() const;

    // ** The id of the request on the client
    UInt32                          mLocalRequstID;
    // ** The id of the request on the server
    UInt32                          mRequestID;
    // ** Information regarding the file download
    FileResourceInfo                mInfo;
    // ** The the list of active file operations. When a chunk is loaded it can then be copied to the final destination.
    FileResourceChunk               mOps[MAX_OPS];
    // ** A list of flags signally which op is free/not free
    bool                            mOpsActive[MAX_OPS];

   
    // ** A pointer to the actual connection the request belongs to
    NetConnectionAtomicWPtr         mConnection;
    // ** A pointer to the user's request object. If their request handle reaches 0 it will automatically cancel this request.
    FileTransferRequestAtomicWPtr   mUserRequest;
    // ** A buffer where the file memory is written to.
    MemoryBuffer                    mBuffer;

    DownloadResponseStatus::Value   mResponseStatus;

    volatile Atomic32               mState;

    SizeT                           mActiveNetworkCalls;
};
DECLARE_STRUCT_ATOMIC_PTR(FileResourceHandle);

} // namespace lf
