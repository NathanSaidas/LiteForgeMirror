#ifndef LF_RUNTIME_CACHE_WRITER_H
#define LF_RUNTIME_CACHE_WRITER_H

#include "Core/Common/API.h"
#include "Core/String/Token.h"
#include "Runtime/Async/PromiseImpl.h"
#include "Runtime/Asset/CacheTypes.h"

namespace lf {
namespace CacheWriterError {
LF_RUNTIME_API extern const char* ERROR_MSG_INTERNAL_ERROR;
LF_RUNTIME_API extern const char* ERROR_MSG_FAILED_TO_OPEN_FILE;
LF_RUNTIME_API extern const char* ERROR_MSG_INDEX_OUT_OF_BOUNDS;
} // namespace CacheWriterError

struct AssetType;

using CacheWritePromise = PromiseImpl<TCallback<void>, TCallback<void, const String&>>;

// **********************************
// A utility class that gets information to write to a 
// object contained in a cache block
// **********************************
class LF_RUNTIME_API CacheWriter
{
public:
    CacheWriter();
    CacheWriter(const CacheWriter& other);
    CacheWriter(CacheWriter&& other);
    CacheWriter(const CacheBlock& block, CacheIndex index, const void* sourceMemory = nullptr, SizeT sourceMemorySize = 0);
    ~CacheWriter();

    // **********************************
    // Executes the write function (to write to the file)
    // **********************************
    bool Write();

    CacheWritePromise WriteAsync();
    // **********************************
    // Clears all data stored in the cache writer
    // **********************************
    void Close();

    // **********************************
    // For cases where you might be writing to a network stream or some other type of output
    // other than file, you can specify an output buffer. (Assumes same format as file)
    // **********************************
    void SetOutputBuffer(void* outputBuffer, SizeT outputBufferSize);

    // **********************************
    // @returns Returns the name of the file that would be written to when the write function is called.
    // **********************************
    const Token& GetOutputFilename() const { return mOutputFile; }
private:
    const char* WriteCommon();
    const char* WriteOutput();
    const char* WriteFile();
    const char* WriteZeroOutput();
    const char* WriteZeroFile();

    void* mOutputBuffer;
    SizeT mOutputBufferSize;
    const void* mSourceMemory;
    SizeT mSourceMemorySize;

    CacheObject mObject;
    Token       mOutputFile;
};

}

#endif // LF_RUNTIME_CACHE_WRITER_H