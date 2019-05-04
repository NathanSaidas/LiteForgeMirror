#include "CacheWriter.h"
#include "Core/Platform/File.h"
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
mOutputFile()
{}
CacheWriter::CacheWriter(const CacheWriter& other) : 
mOutputBuffer(other.mOutputBuffer),
mOutputBufferSize(other.mOutputBufferSize),
mSourceMemory(other.mSourceMemory),
mSourceMemorySize(other.mSourceMemorySize),
mObject(other.mObject),
mOutputFile(other.mOutputFile)
{}

CacheWriter::CacheWriter(CacheWriter&& other) :
mOutputBuffer(other.mOutputBuffer),
mOutputBufferSize(other.mOutputBufferSize),
mSourceMemory(other.mSourceMemory),
mSourceMemorySize(other.mSourceMemorySize),
mObject(other.mObject),
mOutputFile(std::forward<Token&&>(other.mOutputFile))
{
    other.mOutputBuffer = nullptr;
    other.mOutputBufferSize = 0;
    other.mSourceMemory = nullptr;
    other.mSourceMemorySize = 0;
    other.mObject = CacheObject();
}
CacheWriter::CacheWriter(const CacheBlock& block, CacheIndex index, const void* sourceMemory, SizeT sourceMemorySize) :
mOutputBuffer(nullptr),
mOutputBufferSize(0),
mSourceMemory(sourceMemory),
mSourceMemorySize(sourceMemorySize),
mObject(),
mOutputFile()
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
    }
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
        ReportBug(ERROR_MSG_FAILED_TO_OPEN_FILE, LF_ERROR_INTERNAL, ERROR_API_RUNTIME);
    }
    else if (error == ERROR_MSG_INDEX_OUT_OF_BOUNDS)
    {
        ReportBug(ERROR_MSG_INDEX_OUT_OF_BOUNDS, LF_ERROR_OUT_OF_RANGE, ERROR_API_RUNTIME);
    }
    else
    {
        ReportBug(ERROR_MSG_INTERNAL_ERROR, LF_ERROR_INTERNAL, ERROR_API_RUNTIME);
    }
    return false;
}

CacheWritePromise CacheWriter::WriteAsync()
{
    // Allocate and capture smart pointer, 'this' might go out of scope
    CacheWriterAtomicPtr safe(LFNew<CacheWriter>());
    safe->mOutputBuffer = mOutputBuffer;
    safe->mOutputBufferSize = mOutputBufferSize;
    safe->mSourceMemory = mSourceMemory;
    safe->mSourceMemorySize = mSourceMemorySize;
    safe->mObject = mObject;
    safe->mOutputFile = mOutputFile;
    return CacheWritePromise([safe](Promise* self)
    {
        const char* error = safe->WriteCommon();
        if (error == nullptr)
        {
            self->Resolve();
        }
        else
        {
            self->Reject(error);
        }
    });
}

void CacheWriter::Close()
{
    mOutputBuffer = nullptr;
    mOutputBufferSize = 0;
    mSourceMemory = nullptr;
    mSourceMemorySize = 0;
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

    memcpy(reinterpret_cast<ByteT*>(mOutputBuffer) + writePos, mSourceMemory, writeSize);
    return nullptr;
}
const char* CacheWriter::WriteFile()
{
    String filename(mOutputFile.CStr(), COPY_ON_WRITE);

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

    SizeT writeSize = Min(static_cast<SizeT>(mObject.mSize), mSourceMemorySize);
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

    SizeT writeSize = Min(static_cast<SizeT>(mObject.mSize), mSourceMemorySize);
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
        file.Write(mSourceMemory, written);
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