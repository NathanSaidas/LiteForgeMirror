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
#ifndef LF_CORE_FILE_H
#define LF_CORE_FILE_H

#include "Core/Common/Types.h"
#include "Core/Platform/PlatformTypes.h"

namespace lf {

struct FileHandle;
class  AsyncIODevice;
class  AsyncIOBuffer;
class  String;



// Primitive type used for basic file operations
class File 
{
public:
    File();
    File(const File& other);
    File(File&& other);
    ~File();

    File& operator=(const File& other);
    File& operator=(File&& other);

    // Attempts to open the file, if it failed the return result is false, otherwise true.
    bool Open(const String& filename, FileFlagsT flags, FileOpenMode openMode);
    // Attempts to open the file with async operations. If it failed the return result is false, otherwise true.
    bool OpenAsync(const String& filename, FileFlagsT flags, FileOpenMode openMode, AsyncIODevice& ioDevice);
    // Closes the file handle.
    // Note: It is preferred to wait for all pending io operations for this file to complete before closing the
    // file handle.
    void Close();

    // Block thread execution until the buffer is filled with data from the file. 
    // Returns the bytes read.
    SizeT Read(void* buffer, SizeT bufferLength);
    // Submit a async read request, a call to this function will not block thread execution
    // Returns true if the request was submitted successfully
    bool ReadAsync(AsyncIOBuffer* buffer, SizeT bufferLength);
    // Block thread execution until the buffer is written to the file.
    // Returns the bytes written
    SizeT Write(const void* buffer, SizeT bufferLength);
    // Submit a async write request, a call to this function will not block thread execution.
    // Returns true if the request was submitted succesfully
    bool WriteAsync(AsyncIOBuffer* buffer, SizeT bufferLength);
    // Blocks thread execution until the async operation is complete.
    void Wait();
    // Blocks thread execution for a period of time or if the task is complete, whichever is less.
    bool Wait(SizeT waitMilliseconds);

    FileSize GetSize() const;
    FileCursor GetCursor() const;
    bool SetCursor(FileCursor offset, FileCursorMode mode);

    bool IsReading() const;
    bool IsWriting() const;
    bool IsEof() const;
    bool IsOpen() const;
    bool IsAsync() const;
    bool HasPending() const;

    const String& GetName();
private:
    FileHandle*    mHandle;
};

}

#endif // LF_CORE_FILE_H