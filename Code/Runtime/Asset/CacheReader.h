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
#ifndef LF_RUNTIME_CACHE_READER_H
#define LF_RUNTIME_CACHE_READER_H

#include "Core/Common/API.h"
#include "Core/String/Token.h"
#include "Runtime/Async/PromiseImpl.h"
#include "Runtime/Asset/CacheTypes.h"

namespace lf {
namespace CacheReaderError {
LF_RUNTIME_API extern const char* ERROR_MSG_INTERNAL_ERROR;
LF_RUNTIME_API extern const char* ERROR_MSG_FAILED_TO_OPEN_FILE;
LF_RUNTIME_API extern const char* ERROR_MSG_INDEX_OUT_OF_BOUNDS;
} // namespace CacheReaderError

using CacheReadPromise = PromiseImpl<TCallback<void>, TCallback<void, const String&>>;

// **********************************
// A utility class that retrieves information on how to read
// from a cache block, the reads can be performed asynchronously
// and listened on with a promise.
// **********************************
class LF_RUNTIME_API CacheReader
{
public:
    CacheReader();
    CacheReader(const CacheReader& other);
    CacheReader(CacheReader&& other);

    // **********************************
    // Executes the read function (to read from a file)
    //
    // note: This function will only read from a file if there is no input buffer assigned.
    // **********************************
    bool Read();
    // **********************************
    // Executes the read function (to read from a file asynchronously)
    //
    // note: This function will only read from a file if there is no input buffer assigned.
    // **********************************
    CacheReadPromise ReadAsync();
    // **********************************
    // Opens the cache reader with the given arguments. Use Read or ReadAsync to actually 
    // read the data from the cache block/input buffer.
    //
    // note: For async operations the CacheReader assumes the 'outputBuffer' memory will remain
    //       a valid source of memory to write to.
    //
    // @param block -- The cache block the reader will read from
    // @param index -- The specific location within the cache block to read from
    // @param outputBuffer -- Where the read data will be copied to
    // @param outputBufferSize -- How much data the output buffer can contain. (If read cannot copy
    //                            all data to the output buffer it fails.)
    // @returns Returns true if the CacheWriter has somewhere to write 
    // **********************************
    bool Open(const CacheBlock& block, CacheIndex index, void* outputBuffer, SizeT outputBufferSize);
    // **********************************
    // For cases where you might be reading from a network stream or some other type of input
    // other than file, you can specify an inputBuffer buffer. (Assumes same format as file)
    // **********************************
    void SetInputBuffer(const void* inputBuffer, SizeT inputBufferSize);
    // **********************************
    // @returns Returns the name of the file that would be read from when the read function is called.
    // **********************************
    const Token& GetOutputFilename() const { return mOutputFile; }
private:
    // ** Writes current data to output
    const char* ReadCommon();
    // ** Reads the current data from the input buffer
    const char* ReadInput();
    // ** Reads the current data from the file
    const char* ReadFile();

    // ** Pointer to the buffer the reader will copy the 'read' data to
    void*       mOutputBuffer;
    // ** Size of the output buffer
    SizeT       mOutputBufferSize;
    // ** Pointer to the input buffer where data can be read from
    const void* mInputBuffer;
    // ** Size of the input buffer
    SizeT       mInputBufferSize;
    // ** Cache object retrieved from the 'CacheBlock' and 'index' when opened.
    CacheObject mObject;
    // ** The name of the output filename, determined by the 'CacheBlock' and 'index'
    Token       mOutputFile;
};

} // namespace lf

#endif // LF_RUNTIME_CACHE_READER_H