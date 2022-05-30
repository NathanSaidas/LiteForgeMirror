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
#include "CacheWriter.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"
#include "Core/String/String.h"
#include "Core/String/StringCommon.h"
#include "Core/Utility/Utility.h"
#include "Runtime/Asset/CacheBlock.h"

namespace lf {
namespace CacheWriterError {
const char* ERROR_MSG_INTERNAL_ERROR = "Internal Error.";
const char* ERROR_MSG_FAILED_TO_OPEN_FILE = "Failed to open file.";
const char* ERROR_MSG_INDEX_OUT_OF_BOUNDS = "Index out of bounds.";
} // namespace CacheWriterError
using namespace CacheWriterError;
DECLARE_ATOMIC_PTR(CacheWriter);

CacheWriter::CacheWriter() : 
mOutputBuffer(nullptr),
mOutputBufferSize(0),
mSourceMemory(nullptr),
mSourceMemorySize(0),
mObject(),
mOutputFile(),
mReserveSize(0)
{}
CacheWriter::CacheWriter(const CacheWriter& other) : 
mOutputBuffer(other.mOutputBuffer),
mOutputBufferSize(other.mOutputBufferSize),
mSourceMemory(other.mSourceMemory),
mSourceMemorySize(other.mSourceMemorySize),
mObject(other.mObject),
mOutputFile(other.mOutputFile),
mReserveSize(other.mReserveSize)
{}

CacheWriter::CacheWriter(CacheWriter&& other) :
mOutputBuffer(other.mOutputBuffer),
mOutputBufferSize(other.mOutputBufferSize),
mSourceMemory(other.mSourceMemory),
mSourceMemorySize(other.mSourceMemorySize),
mObject(other.mObject),
mOutputFile(std::forward<Token&&>(other.mOutputFile)),
mReserveSize(other.mReserveSize)
{
    other.mOutputBuffer = nullptr;
    other.mOutputBufferSize = 0;
    other.mSourceMemory = nullptr;
    other.mSourceMemorySize = 0;
    other.mObject = CacheObject();
    other.mReserveSize = 0;
}

CacheWriter::~CacheWriter()
{
    LF_DEBUG_BREAK;
}


bool CacheWriter::Write()
{
    const char* error = WriteCommon();
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

CacheWritePromise CacheWriter::WriteAsync()
{
    void* memory = LFAlloc(sizeof(CacheWriter), alignof(CacheWriter));
    CacheWriterAtomicPtr safe(new(memory)CacheWriter(*this));
    return CacheWritePromise([safe](Promise* self)
    {
        auto promise = static_cast<CacheWritePromise*>(self);
        const char* error = safe->WriteCommon();
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

bool CacheWriter::Open(const CacheBlock& block, CacheIndex index, const void* sourceMemory, SizeT sourceMemorySize)
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
        mSourceMemory = sourceMemory;
        mSourceMemorySize = sourceMemorySize;
        mReserveSize = block.GetDefaultCapacity();
        return true;
    }
    return false;
}

void CacheWriter::SetOutputBuffer(void* outputBuffer, SizeT outputBufferSize)
{
    mOutputBuffer = outputBuffer;
    mOutputBufferSize = outputBufferSize;
}

const char* CacheWriter::WriteCommon()
{
    if (mOutputBuffer && mOutputBufferSize > 0)
    {
        if (mSourceMemory && mSourceMemorySize > 0)
        {
            return WriteOutput();
        }
        else
        {
            return WriteZeroOutput();
        }
    }
    else
    {
        if (mSourceMemory && mSourceMemorySize > 0)
        {
            return WriteFile();
        }
        else
        {
            return WriteZeroFile();
        }
    }
}

const char* CacheWriter::WriteOutput()
{
    // OutputBuffer
    // OutputBufferSize

    SizeT writePos = 0;
    writePos += static_cast<SizeT>(mObject.mLocation);
    if (writePos > mOutputBufferSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    SizeT writeSize = Min(SizeT(mObject.mSize), mSourceMemorySize);
    SizeT writeEnd = writePos + writeSize;
    if (writeEnd < writePos || writeEnd > mOutputBufferSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    if (writeSize > mSourceMemorySize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    memcpy(reinterpret_cast<ByteT*>(mOutputBuffer) + writePos, mSourceMemory, writeSize);
    return nullptr;
}
const char* CacheWriter::WriteFile()
{
    String filename(mOutputFile.CStr(), COPY_ON_WRITE);

    FileSystem::PathCreate(filename);

    if (!FileSystem::FileExists(filename))
    {
        FileSystem::FileReserve(filename, mReserveSize);
    }

    File file;
    if (!file.Open(filename, FF_WRITE, FILE_OPEN_EXISTING))
    {
        return ERROR_MSG_FAILED_TO_OPEN_FILE;
    }

    SizeT writePos = 0;
    writePos += static_cast<SizeT>(mObject.mLocation);
    if (writePos > static_cast<SizeT>(file.GetSize()))
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    SizeT writeSize = Min(static_cast<SizeT>(mObject.mCapacity), mSourceMemorySize);
    SizeT writeEnd = writePos + writeSize;
    if (writeEnd < writePos || writeEnd > static_cast<SizeT>(file.GetSize()))
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    if (!file.SetCursor(static_cast<FileCursor>(writePos), FILE_CURSOR_BEGIN))
    {
        return ERROR_MSG_INTERNAL_ERROR;
    }
    file.Write(mSourceMemory, writeSize);
    return nullptr;
}

const char* CacheWriter::WriteZeroOutput()
{
    SizeT writePos = 0;
    writePos += static_cast<SizeT>(mObject.mLocation);
    if (writePos > mOutputBufferSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS; // Index out of bounds
    }

    SizeT writeSize = Min(SizeT(mObject.mSize), mSourceMemorySize);
    SizeT writeEnd = writePos + writeSize;
    if (writeEnd < writePos || writeEnd > mOutputBufferSize)
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS; // Index out of bounds
    }

    memset(reinterpret_cast<ByteT*>(mOutputBuffer) + writePos, 0, writeSize);
    return nullptr;
}
const char* CacheWriter::WriteZeroFile()
{
    String filename(mOutputFile.CStr(), COPY_ON_WRITE);

    FileSystem::PathCreate(filename);

    if (!FileSystem::FileExists(filename))
    {
        FileSystem::FileReserve(filename, mReserveSize);
    }

    File file;
    if (!file.Open(filename, FF_WRITE, FILE_OPEN_EXISTING))
    {
        return ERROR_MSG_FAILED_TO_OPEN_FILE;
    }

    SizeT writePos = 0;
    writePos += static_cast<SizeT>(mObject.mLocation);
    if (writePos > static_cast<SizeT>(file.GetSize()))
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    SizeT writeSize = Min(static_cast<SizeT>(mObject.mCapacity), mSourceMemorySize);
    SizeT writeEnd = writePos + writeSize;
    if (writeEnd < writePos || writeEnd > static_cast<SizeT>(file.GetSize()))
    {
        return ERROR_MSG_INDEX_OUT_OF_BOUNDS;
    }

    if (!file.SetCursor(static_cast<FileCursor>(writePos), FILE_CURSOR_BEGIN))
    {
        return ERROR_MSG_INTERNAL_ERROR;
    }

    ByteT buffer[1024 * 16] = { 0 };
    SizeT bytesRemaining = writeSize;

    while (bytesRemaining > 0)
    {
        SizeT written = Min(bytesRemaining, sizeof(buffer));
        file.Write(buffer, written);
        if (written > bytesRemaining)
        {
            bytesRemaining = 0;
        }
        else
        {
            bytesRemaining -= written;
        }
    }
    return nullptr;
}

}