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
#include "FileTransferTypes.h"

namespace lf {

DownloadHash::DownloadHash()
{
    memset(mBytes, 0, sizeof(mBytes));
}
void DownloadHash::Serialize(Stream& s)
{
    s.SerializeGuid(mBytes, sizeof(mBytes));
}
DownloadRequest::DownloadRequest()
: mResourceIdentifier()
, mVersion(0)
, mRequestID(INVALID32)
{

}
void DownloadRequest::Serialize(Stream& s)
{
    SERIALIZE(s, mResourceIdentifier, "");
    SERIALIZE(s, mVersion, "");
    SERIALIZE(s, mRequestID, "");
}
DownloadResponse::DownloadResponse()
: mStatus(DownloadResponseStatus::DRS_SUCCESS)
, mResourceHandle(INVALID32)
, mResourceSize(0)
, mHash()
, mChunkCount(0)
, mFragmentCount(0)
, mRequestID(0)
{

}
void DownloadResponse::Serialize(Stream& s)
{
    SERIALIZE(s, mStatus, "");
    SERIALIZE(s, mResourceHandle, "");
    SERIALIZE(s, mResourceSize, "");
    SERIALIZE(s, mHash, "");
    SERIALIZE(s, mChunkCount, "");
    SERIALIZE(s, mFragmentCount, "");
    SERIALIZE(s, mRequestID, "");
}
DownloadFetchRequest::DownloadFetchRequest()
: mResourceHandle(INVALID32)
, mChunkID(INVALID32)
{

}
void DownloadFetchRequest::Serialize(Stream& s)
{
    SERIALIZE(s, mResourceHandle, "");
    SERIALIZE(s, mChunkID, "");
}
DownloadFetchFragmentRequest::DownloadFetchFragmentRequest()
: mResourceHandle(INVALID32)
, mChunkID(INVALID32)
, mFragmentIDs()
, mUseRange()
{

}
void DownloadFetchFragmentRequest::Serialize(Stream& s)
{
    SERIALIZE(s, mResourceHandle, "");
    SERIALIZE(s, mChunkID, "");
    SERIALIZE_ARRAY(s, mFragmentIDs, "");
    SERIALIZE(s, mUseRange, "");
}
DownloadFetchStopRequest::DownloadFetchStopRequest()
: mResourceHandle(INVALID32)
, mChunkID(INVALID32)
{

}
void DownloadFetchStopRequest::Serialize(Stream& s)
{
    SERIALIZE(s, mResourceHandle, "");
    SERIALIZE(s, mChunkID, "");
}
DownloadCompleteRequest::DownloadCompleteRequest()
: mResourceHandle(INVALID32)
{
}
void DownloadCompleteRequest::Serialize(Stream& s)
{
    SERIALIZE(s, mResourceHandle, "");
}
DownloadFetchCompleteResponse::DownloadFetchCompleteResponse()
: mResourceHandle(INVALID32)
, mChunkID(INVALID32)
{}
void DownloadFetchCompleteResponse::Serialize(Stream& s)
{
    SERIALIZE(s, mResourceHandle, "");
    SERIALIZE(s, mChunkID, "");
}
DownloadFetchDataResponse::DownloadFetchDataResponse()
: mResourceHandle(INVALID32)
, mChunkID(INVALID32)
, mFragmentID(INVALID32)
, mFragmentSize(0)
, mData()
{

}
void DownloadFetchDataResponse::Serialize(Stream& s)
{
    SERIALIZE(s, mResourceHandle, "");
    SERIALIZE(s, mChunkID, "");
    SERIALIZE(s, mFragmentID, "");
    if (!s.IsReading())
    {
        mFragmentSize = static_cast<UInt32>(mData.size());
    }
    SERIALIZE(s, mFragmentSize, "");
    if (s.IsReading())
    {
        mData.resize(static_cast<SizeT>(mFragmentSize));
    }
    static StreamPropertyInfo mDataPropertyInfo("Data");
    (s << mDataPropertyInfo).SerializeGuid(mData.data(), mData.size());

}
DownloadFetchStoppedResponse::DownloadFetchStoppedResponse()
: mResourceHandle(INVALID32)
, mChunkID(INVALID32)
, mReason(DownloadFetchStopReason::DFSR_RESOURCE_UPDATED)
{

}
void DownloadFetchStoppedResponse::Serialize(Stream& s)
{
    SERIALIZE(s, mResourceHandle, "");
    SERIALIZE(s, mChunkID, "");
    SERIALIZE(s, mReason, "");
}

} // namespace lf