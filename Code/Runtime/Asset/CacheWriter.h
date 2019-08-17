// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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

using CacheWritePromise = PromiseImpl<TCallback<void>, TCallback<void, const String&>>;

// **********************************
// A utility class that gets information to write to a 
// object contained in a cache block, writing can be performed
// asynchronously and listened on with a promise.
// **********************************
class LF_RUNTIME_API CacheWriter
{
public:
    CacheWriter();
    CacheWriter(const CacheWriter& other);
    CacheWriter(CacheWriter&& other);
    ~CacheWriter();
    // **********************************
    // Executes the write function (to write to the file)
    //
    // note: This function will only write to a file if there is no output buffer assigned.
    // **********************************
    bool Write();
    // **********************************
    // Executes the write function (to write to the file asynchronously)
    //
    // note: This function will only write to a file if there is no output buffer assigned.
    // **********************************
    CacheWritePromise WriteAsync();
    // **********************************
    // Opens the cache writer with the given arguments. Use Write or WriteAsync to actually commit the write
    // command
    //
    // note: For async operations the CacheWriter assumes the 'sourceMemory' (if not null) will remain a valid source
    //       of memory to read from.
    //
    // @param block -- The cache block the writer will write to
    // @param index -- The specific location within the cache block to write to
    // @param sourceMemory [Optional] -- The memory that will be written to the buffer 
    // @param sourceMemorySize [Optional] -- The size of the sourceMemory buffer in bytes.
    // @returns Returns true if the CacheWriter has somewhere to write
    // **********************************
    bool Open(const CacheBlock& block, CacheIndex index, const void* sourceMemory = nullptr, SizeT sourceMemorySize = 0);
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
    // ** Writes current data to output (Output buffer or File)
    const char* WriteCommon();
    // ** Writes current data to output buffer
    const char* WriteOutput();
    // ** Writes current data to file
    const char* WriteFile();
    // ** Writes zero bytes to output
    const char* WriteZeroOutput();
    // ** Writes zero bytes to file
    const char* WriteZeroFile();

    // ** Pointer to the output buffer (Optional)
    void* mOutputBuffer;
    // ** Size of the output buffer
    SizeT mOutputBufferSize;
    // ** Pointer to the input buffer (aka the source)
    const void* mSourceMemory;
    // ** Size of the input buffer (aka the source)
    SizeT mSourceMemorySize;
    // ** Cache object retrieved from the 'CacheBlock' and 'index'
    CacheObject mObject;
    // ** The output filename, determined by the 'CacheBlock' and 'index'
    Token       mOutputFile;
};

}

#endif // LF_RUNTIME_CACHE_WRITER_H