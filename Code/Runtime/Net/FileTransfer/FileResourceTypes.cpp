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
#include "FileResourceTypes.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Utility/Utility.h"

namespace lf {

FileResourceInfo::FileResourceInfo()
: mName()
, mLastModifyTime()
, mSize()
, mHash()
, mFragmentCount()
, mChunkCount()
{
}

FileResourceChunk::FileResourceChunk()
: mData()
{

}

// ** Copy data at the specified fragment id to the buffer. Return the number of bytes copied.
SizeT FileResourceChunk::CopyFragment(SizeT fragmentID, ByteT* buffer, SizeT bufferCapacity) const
{
    SizeT byteOffset = fragmentID * FILE_SERVER_MAX_FRAGMENT_SIZE;
    if (byteOffset >= mData.size())
    {
        return 0;
    }

    SizeT length = Min(FILE_SERVER_MAX_FRAGMENT_SIZE, mData.size() - byteOffset);
    if (bufferCapacity < length)
    {
        return 0;
    }
    memcpy(buffer, &mData[byteOffset], length);
    return length;
}
// ** Returns the number of fragments in the chunk
SizeT FileResourceChunk::GetFragmentCount() const
{
    return FileResourceUtil::FileSizeToFragmentCount(mData.size());
}
// ** Computes the SHA256 hash for the data.
bool FileResourceChunk::ComputeHash(ByteT* buffer, SizeT bufferCapacity) const
{
    if (mData.empty() || bufferCapacity != sizeof(Crypto::SHA256Hash))
    {
        return false;
    }
    Crypto::SHA256Hash hash(mData.data(), mData.size());
    memcpy(buffer, hash.Bytes(), hash.Size());
    return true;
}

FileResourceHandle::FileResourceHandle()
    : Super()
    , mLocalRequstID(INVALID32)
    , mRequestID(INVALID32)
    , mInfo()
    , mOps()
    , mOpsActive()
    , mConnection()
    , mUserRequest()
    , mBuffer()
    , mResponseStatus()
    , mState(Initialization)
    , mActiveNetworkCalls(0)
{
    for (SizeT i = 0; i < MAX_OPS; ++i)
    {
        mOpsActive[i] = false;
    }
}
void FileResourceHandle::SetState(State value)
{
    AtomicStore(&mState, value);
}

FileResourceHandle::State FileResourceHandle::GetState() const
{
    return static_cast<State>(AtomicLoad(&mState));
}

} // namespace lf