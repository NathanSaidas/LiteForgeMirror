// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#include "CacheReader.h"
#include "Core/String/String.h"
#include "Core/String/StringCommon.h"
#include "Core/Platform/File.h"
#include "Core/Utility/Utility.h"
#include "Runtime/Asset/CacheBlock.h"

#include <utility>

namespace lf {
namespace CacheReaderError {
const char* ERROR_MSG_INTERNAL_ERROR = "Internal Error.";
const char* ERROR_MSG_FAILED_TO_OPEN_FILE = "Failed to open file.";
const char* ERROR_MSG_INDEX_OUT_OF_BOUNDS = "Index out of bounds.";
} // namespace CacheReaderError
using namespace CacheReaderError;
DECLARE_ATOMIC_PTR(CacheReader);

CacheReader::CacheReader()
{
}
CacheReader::CacheReader(const CacheReader& other) :
mOutputBuffer(other.mOutputBuffer),
mOutputBufferSize(other.mOutputBufferSize),
mInputBuffer(other.mInputBuffer),
mInputBufferSize(other.mInputBufferSize),
mObject(other.mObject),
mOutputFile(other.mOutputFile)
{

}
CacheReader::CacheReader(CacheReader&& other) :
mOutputBuffer(other.mOutputBuffer),
mOutputBufferSize(other.mOutputBufferSize),
mInputBuffer(other.mInputBuffer),
mInputBufferSize(other.mInputBufferSize),
mObject(other.mObject),
mOutputFile(std::forward<Token&&>(other.mOutputFile))
{

}

bool CacheReader::Read()
{
    const char* error = ReadCommon();
    if (error == nullptr)
    {
        return true;
    }
    else if (error == ERROR_MSG_FAILED_TO_OPEN_FILE)
    {
        ReportBugMsgEx(ERROR_MSG_FAILED_TO_OPEN_FILE, LF_ERROR_INTERNAL, ERROR_API_RUNTIME);
    }
    else if (error == ERROR_MSG_INDEX_OUT_OF_BOUNDS)
    {
        ReportBugMsgEx(ERROR_MSG_INDEX_OUT_OF_BOUNDS, LF_ERROR_OUT_OF_RANGE, ERROR_API_RUNTIME);
    }
    else
    {
        ReportBugMsgEx(ERROR_MSG_INTERNAL_ERROR, LF_ERROR_INTERNAL, ERROR_API_RUNTIME);
    }
    return false;
}

CacheReadPromise CacheReader::ReadAsync()
{
    void* memory = LFAlloc(sizeof(CacheReader), alignof(CacheReader));
    CacheReaderAtomicPtr safe(new(memory)CacheReader(*this));
    return CacheReadPromise([safe](Promise* self)
    {
        auto promise = static_cast<CacheReadPromise*>(self);
        const char* error = safe->ReadCommon();
        if (error == nullptr)
        {
            promise->Resolve();
        }
        else
        {
            promise->Reject(error);
        }
    });
}


bool CacheReader::Open(const CacheBlock& block, CacheIndex index, void* outputBuffer, SizeT outputBufferSize)
{
    if (block.GetObject(index, mObject))
    {
        String blockTitle(block.GetFilename().CStr(), COPY_ON_WRITE);
        String blockExtension("_");
        ByteT blobID = static_cast<ByteT>(index.mBlobID);
        blockExtension.Append(ByteToHex(blobID & 0xF0));
        blockExtension.Append(ByteToHex(blobID & 0x0F));
        blockExtension.Append(".lfcache");
        mOutputFile = Token(blockTitle + blockExtension);
        mOutputBuffer = outputBuffer;
        mOutputBufferSize = outputBufferSize;
        return true;
    }
    return false;
}
void CacheReader::SetInputBuffer(const void* inputBuffer, SizeT inputBufferSize)
{
    mInputBuffer = inputBuffer;
    mInputBufferSize = inputBufferSize;
}

const char* CacheReader::ReadCommon()
{
    if (mInputBuffer && mInputBufferSize > 0)
    {
        return ReadInput();
    }
    else
    {
        return ReadFile();
    }
}

const char* CacheReader::ReadInput()
{
    SizeT readPos = 0;
    readPos += static_cast<SizeT>(mObject.mLocation);
    if (readPos > mInputBufferSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    SizeT readSize = static_cast<SizeT>(mObject.mSize);
    SizeT readEnd = readPos + readSize;
    if (readEnd < readPos || readEnd > mInputBufferSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    if (readSize > mOutputBufferSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    memcpy(mOutputBuffer, reinterpret_cast<const ByteT*>(mInputBuffer) + readPos, readSize);
    return nullptr;
}
const char* CacheReader::ReadFile()
{
    String filename(mOutputFile.CStr(), COPY_ON_WRITE);
    
    File file;
    if (!file.Open(filename, FF_READ | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_EXISTING))
    {
        return ERROR_MSG_FAILED_TO_OPEN_FILE;
    }

    SizeT fileSize = static_cast<SizeT>(file.GetSize());
    SizeT readPos = 0;
    readPos += static_cast<SizeT>(mObject.mLocation);
    if (readPos > fileSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    SizeT readSize = static_cast<SizeT>(mObject.mSize);
    SizeT readEnd = readPos + readSize;
    if (readEnd < readPos || readEnd > fileSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    if (readSize > mOutputBufferSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    if (!file.SetCursor(static_cast<FileCursor>(readPos), FILE_CURSOR_BEGIN))
    {
        return ERROR_MSG_INTERNAL_ERROR;
    }

    file.Read(mOutputBuffer, readSize);
    return nullptr;
}

} // namespace lf