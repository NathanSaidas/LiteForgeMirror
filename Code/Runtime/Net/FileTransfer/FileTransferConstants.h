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
#include "Core/Common/Types.h"
#include "Core/Common/Enum.h"

namespace lf {

// The maximum size of a fragment in any network message
const SizeT FILE_SERVER_MAX_FRAGMENT_SIZE = 1200;
// The maximum number of fragments in a chunk in any network message
const SizeT FILE_SERVER_MAX_FRAGMENTS_IN_CHUNK = 32;

// SHA256 hash size
const SizeT FILE_SERVER_HASH_SIZE = 32;

namespace FileResourceUtil
{
    LF_INLINE SizeT FragmentCountToChunkCount(SizeT numFragments);
    LF_INLINE SizeT FileSizeToFragmentCount(SizeT fileSize);
    LF_INLINE SizeT FileSizeToChunkCount(SizeT fileSize);
} // namespace FileResourceUtil

DECLARE_ENUM(DownloadMessageType,
DMT_DOWNLOAD_REQUEST,
DMT_DOWNLOAD_RESPONSE,
DMT_DOWNLOAD_FETCH_REQUEST,
DMT_DOWNLOAD_FETCH_FRAGMENT_REQUEST,
DMT_DOWNLOAD_FETCH_STOP_REQUEST,
DMT_DOWNLOAD_COMPLETE_REQUEST,
DMT_DOWNLOAD_FETCH_COMPLETE_RESPONSE,
DMT_DOWNLOAD_FETCH_DATA_RESPONSE,
DMT_DOWNLOAD_FETCH_STOPPED_RESPONSE
);

DECLARE_ENUM(DownloadResponseStatus,
DRS_SUCCESS,
DRS_RESOURCE_NOT_FOUND,
DRS_ACCESS_DENIED,
DRS_INTERNAL_ERROR
);

DECLARE_ENUM(DownloadFetchStopReason,
DFSR_RESOURCE_UPDATED,
DFSR_RESOURCE_DELETED,
DFSR_RESOURCE_CORRUPT,
DFSR_RESOURCE_ACCESS_DENIED);


SizeT FileResourceUtil::FragmentCountToChunkCount(SizeT numFragments)
{
    const SizeT HASH_SIZE = 32;
    const SizeT payloadSize = (numFragments * HASH_SIZE);
    SizeT chunks = (payloadSize) / FILE_SERVER_MAX_FRAGMENT_SIZE;
    if ((chunks * FILE_SERVER_MAX_FRAGMENT_SIZE) < payloadSize)
    {
        ++chunks;
    }
    return chunks;
}
SizeT FileResourceUtil::FileSizeToFragmentCount(SizeT fileSize)
{
    SizeT fragments = fileSize / FILE_SERVER_MAX_FRAGMENT_SIZE;
    if ((fragments * FILE_SERVER_MAX_FRAGMENT_SIZE) < fileSize)
    {
        ++fragments;
    }
    return fragments;
}
SizeT FileResourceUtil::FileSizeToChunkCount(SizeT fileSize)
{
    SizeT fragments = FileSizeToFragmentCount(fileSize);
    SizeT chunks = FragmentCountToChunkCount(fragments);
    return chunks;
}

}