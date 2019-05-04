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
#include "File.h"
#include "Core/Common/Assert.h"
#include "Core/String/String.h"
#include "Core/Memory/Memory.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Platform/AsyncIODevice.h"
#include "Core/Platform/AsyncIOBuffer.h"

#if defined(LF_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace lf {



struct FileHandle
{
#if defined(LF_OS_WINDOWS)
    HANDLE          mFileHandle;
    AsyncIODevice*  mIODevice;
    AsyncIOUserData mUserData;
    String          mFilename;
    FileFlagsT      mFlags;
    FileOpenMode    mOpenMode;
#endif
};



File::File() : 
mHandle(nullptr)
{
}
File::File(const File& other) :
mHandle(nullptr)
{
    if (other.IsOpen())
    {
        const FileHandle* otherHandle = other.mHandle;
        if (other.IsAsync())
        {
            OpenAsync(otherHandle->mFilename, otherHandle->mFlags, otherHandle->mOpenMode, *otherHandle->mIODevice);
        }
        else
        {
            Open(otherHandle->mFilename, otherHandle->mFlags, otherHandle->mOpenMode);
        }
    }
}
File::File(File&& other) :
mHandle(other.mHandle)
{
    other.mHandle = nullptr;
}

File::~File()
{
    Close();
}

File& File::operator=(const File& other)
{
    if (this == &other)
    {
        return *this;
    }
    Close();
    if (other.IsOpen())
    {
        const FileHandle* otherHandle = other.mHandle;
        if (other.IsAsync())
        {
            OpenAsync(otherHandle->mFilename, otherHandle->mFlags, otherHandle->mOpenMode, *otherHandle->mIODevice);
        }
        else
        {
            Open(otherHandle->mFilename, otherHandle->mFlags, otherHandle->mOpenMode);
        }
    }
    return *this;
}
File& File::operator=(File&& other)
{
    Close();
    mHandle = other.mHandle;
    other.mHandle = nullptr;
    return *this;
}

bool File::Open(const String& filename, FileFlagsT flags, FileOpenMode openMode)
{
    if (IsOpen())
    {
        return false; // File is already open!
    }

    bool read = (flags & FF_READ) > 0;
    bool write = (flags & FF_WRITE) > 0;
    if (!read && !write)
    {
        return false;
    }

    if ((flags & FF_EOF) > 0)
    {
        flags = flags & ~FF_EOF;
    }
    if ((flags & FF_EOF) > 0)
    {
        flags = flags & ~FF_OUT_OF_MEMORY;
    }

    mHandle = LFNew<FileHandle>();
    if (mHandle == nullptr)
    {
        return false;
    }

#if defined(LF_OS_WINDOWS)
    DWORD access = 0;
    if (read)
    {
        access |= GENERIC_READ;
    }
    if (write)
    {
        access |= GENERIC_WRITE;
    }

    DWORD share = 0;
    if ((flags &FF_SHARE_READ) > 0)
    {
        share = FILE_SHARE_READ;
    }
    else if ((flags & FF_SHARE_WRITE) > 1)
    {
        share = FILE_SHARE_WRITE;
    }

    DWORD creationDisposition = 0;
    switch (openMode)
    {
        case FILE_OPEN_EXISTING:
            creationDisposition = OPEN_EXISTING;
            break;
        case FILE_OPEN_NEW:
            creationDisposition = CREATE_NEW;
            break;
        case FILE_OPEN_ALWAYS:
            creationDisposition = OPEN_ALWAYS;
            break;
        default:
            Crash("File::Open invalid argument", LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
            break;
    }

    mHandle->mFileHandle = CreateFile
    (
        filename.CStr(),
        access,
        share,
        NULL,
        creationDisposition,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (mHandle->mFileHandle == INVALID_HANDLE_VALUE)
    {
        LFDelete(mHandle);
        mHandle = nullptr;
        return false; // File does not exist:
    }
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif

    mHandle->mIODevice = nullptr;
    mHandle->mFilename = filename;
    mHandle->mFlags = flags;
    mHandle->mOpenMode = openMode;
    return true;
}

bool File::OpenAsync(const String& filename, FileFlagsT flags, FileOpenMode openMode, AsyncIODevice& ioDevice)
{
    if (IsOpen())
    {
        return false; // File is already open!
    }

    bool read = (flags & FF_READ) > 0;
    bool write = (flags & FF_WRITE) > 0;
    if (!read && !write)
    {
        return false;
    }

    if ((flags & FF_EOF) > 0)
    {
        flags = flags & ~FF_EOF;
    }
    if ((flags & FF_EOF) > 0)
    {
        flags = flags & ~FF_OUT_OF_MEMORY;
    }

    mHandle = LFNew<FileHandle>();
    if (mHandle == nullptr)
    {
        return false;
    }

#if defined(LF_OS_WINDOWS)

    DWORD access = 0;
    if (read)
    {
        access |= GENERIC_READ;
    }
    if (write)
    {
        access |= GENERIC_WRITE;
    }

    DWORD share = 0;
    if ((flags &FF_SHARE_READ) > 0)
    {
        share = FILE_SHARE_READ;
    }
    else if ((flags & FF_SHARE_WRITE) > 1)
    {
        share = FILE_SHARE_WRITE;
    }

    DWORD creationDisposition = 0;
    switch (openMode)
    {
        case FILE_OPEN_EXISTING:
            creationDisposition = OPEN_EXISTING;
            break;
        case FILE_OPEN_NEW:
            creationDisposition = CREATE_NEW;
            break;
        case FILE_OPEN_ALWAYS:
            creationDisposition = OPEN_ALWAYS;
            break;
        default:
            Crash("File::Open invalid argument", LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
            break;
    }

    mHandle->mFileHandle = CreateFile
    (
        filename.CStr(),
        access,
        share,
        NULL,
        creationDisposition,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (mHandle->mFileHandle == INVALID_HANDLE_VALUE)
    {
        LFDelete(mHandle);
        mHandle = nullptr;
        return false; // File does not exist:
    }
    if (!ioDevice.AssociateDevice(mHandle->mFileHandle, nullptr))
    {
        AssertError(CloseHandle(mHandle->mFileHandle), LF_ERROR_INTERNAL, ERROR_API_CORE);
        LFDelete(mHandle);
        mHandle = nullptr;
        return false;
    }
    mHandle->mUserData.mFileHandle = mHandle;
    mHandle->mUserData.mHandle = mHandle->mFileHandle;
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif

    mHandle->mIODevice = &ioDevice;
    mHandle->mFilename = filename;
    mHandle->mFlags = flags;
    mHandle->mOpenMode = openMode;
    return true;
}

void File::Close()
{
    if (!mHandle)
    {
        return;
    }

#if defined(LF_OS_WINDOWS)
    // Wait for pending IO operations to complete.
    while (mHandle->mUserData.mPendingBuffer)
    {
        SleepEx(1000, TRUE);
        _ReadWriteBarrier();
    }

    AssertError(mHandle->mFileHandle != INVALID_HANDLE_VALUE, LF_ERROR_BAD_STATE, ERROR_API_CORE);
    AssertError(CloseHandle(mHandle->mFileHandle), LF_ERROR_INTERNAL, ERROR_API_CORE);
    LFDelete(mHandle);
    mHandle = nullptr;
#endif
}

SizeT File::Read(void* buffer, SizeT bufferLength)
{
    AssertError(buffer != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(bufferLength != 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    if (!IsOpen())
    {
        return 0;
    }
    if (!IsReading())
    {
        return 0;
    }

#if defined(LF_OS_WINDOWS)

    if (IsAsync())
    {
        if (HasPending())
        {
            return 0; // Cannot mix async/sync reads.
        }
        // todo: Setup overlapped data file pointers:
        AsyncIOBuffer ioBuffer(buffer);
        ioBuffer.SetState(ASYNC_IO_WAITING);
        mHandle->mUserData.mPendingBuffer = &ioBuffer;
        mHandle->mUserData.Offset = GetCursor();
        SetLastError(ERROR_SUCCESS);
        ReadFile(mHandle->mFileHandle, buffer, static_cast<DWORD>(bufferLength), NULL, &mHandle->mUserData);
        DWORD error = GetLastError();
        if (error == ERROR_HANDLE_EOF)
        {
            mHandle->mFlags |= FF_EOF;
            mHandle->mUserData.mLastBytesRead = 0;
            mHandle->mUserData.mPendingBuffer = nullptr;
            SetCursor(0, FileCursorMode::FILE_CURSOR_END);
            return 0;
        }
        else if (error == ERROR_IO_PENDING)
        {
            DWORD bytesRead = 0;
            SetLastError(ERROR_SUCCESS);
            BOOL result = GetOverlappedResultEx(mHandle->mFileHandle, &mHandle->mUserData, &bytesRead, INFINITE, TRUE);
            error = GetLastError();
            if (error == ERROR_HANDLE_EOF)
            {
                mHandle->mFlags |= FF_EOF;
                mHandle->mUserData.mLastBytesRead = 0;
                mHandle->mUserData.mPendingBuffer = nullptr;
                SetCursor(0, FileCursorMode::FILE_CURSOR_END);
                return 0;
            }
            else if (error == ERROR_IO_INCOMPLETE)
            {
                Crash("Failed to wait for IO", LF_ERROR_INTERNAL, ERROR_API_CORE);
            }
            else
            {
                AssertError(result == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
                while (HasPending()) {} // Spin until completion we should be just about done
            }
        }
        AssertError(!HasPending(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        AssertError(ioBuffer.IsDone(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return ioBuffer.GetBytesTransferred();
    }
    else
    {
        SetLastError(ERROR_SUCCESS);
        DWORD bytesRead = 0;
        BOOL done = ReadFile(mHandle->mFileHandle, buffer, static_cast<DWORD>(bufferLength), &bytesRead, NULL);
        AssertError(done == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
        if (bytesRead == 0)
        {
            mHandle->mFlags |= FF_EOF;
        }
        mHandle->mUserData.mLastBytesRead = bytesRead;
        return static_cast<SizeT>(bytesRead);
    }
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}
bool File::ReadAsync(AsyncIOBuffer* buffer, SizeT bufferLength)
{
    AssertError(buffer != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(bufferLength != 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(buffer->IsDone(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(buffer->GetBuffer() != nullptr, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    if (!IsOpen())
    {
        return false;
    }
    if (!IsAsync() && !IsReading())
    {
        return false;
    }
    if (HasPending())
    {
        return false;
    }
    mHandle->mUserData.mPendingBuffer = buffer;
    mHandle->mUserData.Offset = GetCursor();
    buffer->SetState(ASYNC_IO_WAITING);
#if defined(LF_OS_WINDOWS)
    ReadFile(mHandle->mFileHandle, buffer->GetBuffer(), static_cast<DWORD>(bufferLength), NULL, &mHandle->mUserData);
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
    return true;
}
SizeT File::Write(const void* buffer, SizeT bufferLength)
{
    AssertError(buffer != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(bufferLength != 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    if (!IsOpen())
    {
        return 0;
    }
    if (!IsWriting())
    {
        return 0;
    }

#if defined(LF_OS_WINDOWS)
    if (IsAsync())
    {
        if (HasPending())
        {
            return 0; // Cannot mix async/sync reads.
        }

        // todo: Setup overlapped data file pointers:
        AsyncIOBuffer ioBuffer(const_cast<void*>(buffer));
        ioBuffer.SetState(ASYNC_IO_WAITING);
        mHandle->mUserData.mPendingBuffer = &ioBuffer;
        mHandle->mUserData.Offset = GetCursor();
        SetLastError(ERROR_SUCCESS);
        WriteFile(mHandle->mFileHandle, buffer, static_cast<DWORD>(bufferLength), NULL, &mHandle->mUserData);
        DWORD error = GetLastError();
        if (error == ERROR_NOT_ENOUGH_MEMORY)
        {
            mHandle->mFlags |= FF_OUT_OF_MEMORY;
            mHandle->mUserData.mPendingBuffer = nullptr;
            return 0;
        }
        else if (error == ERROR_IO_PENDING)
        {
            DWORD bytesWritten = 0;
            SetLastError(ERROR_SUCCESS);
            BOOL result = GetOverlappedResultEx(mHandle->mFileHandle, &mHandle->mUserData, &bytesWritten, INFINITE, TRUE);
            error = GetLastError();
            if (error == ERROR_NOT_ENOUGH_MEMORY)
            {
                mHandle->mFlags |= FF_OUT_OF_MEMORY;
                mHandle->mUserData.mPendingBuffer = nullptr;
                return 0;
            }
            else if (error == ERROR_IO_INCOMPLETE)
            {
                Crash("Failed to wait for IO", LF_ERROR_INTERNAL, ERROR_API_CORE);
            }
            else
            {
                AssertError(result == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
                while (HasPending()) {} // Spin until completion, we should be just about done.
            }
        }
        AssertError(!HasPending(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        AssertError(ioBuffer.IsDone(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return ioBuffer.GetBytesTransferred();
    }
    else
    {
        SetLastError(ERROR_SUCCESS);
        DWORD bytesWritten = 0;
        BOOL done = WriteFile(mHandle->mFileHandle, buffer, static_cast<DWORD>(bufferLength), &bytesWritten, NULL);
        AssertError(done == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
        return static_cast<SizeT>(bytesWritten);
    }
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}
bool File::WriteAsync(AsyncIOBuffer* buffer, SizeT bufferLength)
{
    AssertError(buffer != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(bufferLength != 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(buffer->IsDone(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(buffer->GetBuffer() != nullptr, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    if (!IsOpen())
    {
        return false;
    }
    if (!IsAsync() && !IsWriting())
    {
        return false;
    }
    if (HasPending())
    {
        return false;
    }
    
#if defined(LF_OS_WINDOWS)
    mHandle->mUserData.mPendingBuffer = buffer;
    mHandle->mUserData.Offset = GetCursor();
    buffer->SetState(ASYNC_IO_WAITING);
    WriteFile(mHandle->mFileHandle, buffer->GetBuffer(), static_cast<DWORD>(bufferLength), NULL, &mHandle->mUserData);
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
    return true;
}
void File::Wait()
{
    if (!IsOpen())
    {
        return;
    }

    if (HasPending())
    {
#if defined(LF_OS_WINDOWS)
        DWORD dummy = 0;
        AssertError(GetOverlappedResultEx(mHandle->mFileHandle, &mHandle->mUserData, &dummy, INFINITE, TRUE) == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
        while (HasPending()) {}
#endif
    }
}

bool File::Wait(SizeT waitMilliseconds)
{
    if (!IsOpen())
    {
        return false;
    }

    if (HasPending())
    {
#if defined(LF_OS_WINDOWS)
        DWORD dummy = 0;
        AssertError(GetOverlappedResultEx(mHandle->mFileHandle, &mHandle->mUserData, &dummy, static_cast<DWORD>(waitMilliseconds), TRUE) == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
#endif
    }
    return HasPending();
}

FileSize File::GetSize() const
{
    if (!IsOpen())
    {
        return 0;
    }
#if defined(LF_OS_WINDOWS)
    LARGE_INTEGER fSize;
    if (GetFileSizeEx(mHandle->mFileHandle, &fSize) == TRUE)
    {
        return static_cast<FileSize>(fSize.QuadPart);
    }
    return 0;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}
FileCursor File::GetCursor() const
{
    if (!IsOpen())
    {
        return 0;
    }
#if defined(LF_OS_WINDOWS)
    LARGE_INTEGER dummy = { 0 };
    LARGE_INTEGER cursor = { 0 };
    if (SetFilePointerEx(mHandle->mFileHandle, dummy, &cursor, FILE_CURRENT) == TRUE)
    {
        return static_cast<FileCursor>(cursor.QuadPart);
    }
    return 0;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}
bool File::SetCursor(FileCursor offset, FileCursorMode mode)
{
    if (!IsOpen())
    {
        return false;
    }
#if defined(LF_OS_WINDOWS)
    LARGE_INTEGER cursor = { 0 };
    LARGE_INTEGER dummy = { 0 };
    DWORD cursorMode = 0;
    switch (mode)
    {
        case FILE_CURSOR_BEGIN:
            cursorMode = FILE_BEGIN;
            break;
        case FILE_CURSOR_END:
            cursorMode = FILE_END;
            break;
        case FILE_CURSOR_CURRENT:
            cursorMode = FILE_CURRENT;
            break;
    }
    cursor.QuadPart = offset;
    if (SetFilePointerEx(mHandle->mFileHandle, cursor, NULL, cursorMode) == TRUE)
    {
        return true;
    }
    return false;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}

bool File::IsReading() const
{
    return IsOpen() && (mHandle->mFlags & FF_READ) > 0;
}
bool File::IsWriting() const
{
    return IsOpen() && (mHandle->mFlags & FF_WRITE) > 0;
}
bool File::IsEof() const
{
    bool result = IsReading() && ((mHandle->mFlags & FF_EOF) > 0 || (mHandle->mUserData.mLastBytesRead == 0));
    _ReadWriteBarrier();
    return result;
}
bool File::IsOpen() const
{
    return mHandle != nullptr;
}
bool File::IsAsync() const
{
    return IsOpen() && mHandle->mUserData.mFileHandle;
}

bool File::HasPending() const
{
    bool result = IsOpen() && mHandle->mUserData.mPendingBuffer;
    _ReadWriteBarrier();
    return result;
}

const String& File::GetName()
{
    return IsOpen() ? mHandle->mFilename : EMPTY_STRING;
}

} // namespace lf