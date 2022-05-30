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
#include "Core/PCH.h"
#include "Core/Common/Types.h"
#if defined(LF_OS_WINDOWS)
#include "MappedFile.h"
#include "Core/Memory/Memory.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace lf {


struct MappedFileHandle
{
    MappedFileHandle()
    : mFile(INVALID_HANDLE_VALUE)
    , mMapping(NULL)
    , mView(nullptr)
    , mFileSize(0)
    {}
    HANDLE mFile;
    HANDLE mMapping;
    void*  mView;
    SizeT  mFileSize;
};

MappedFile::MappedFile()
: mHandle(LFNew<MappedFileHandle>())
{}
MappedFile::~MappedFile()
{
    LFDelete(mHandle);
}

// https://github.com/fenbf/WinFileTests/blob/master/FileTransformers.cpp 
// Maybe...
bool MappedFile::Open(const char* filename)
{
    if (mHandle->mFile != INVALID_HANDLE_VALUE
    || mHandle->mMapping != NULL)
    {
        return false;
    }

    mHandle->mFile = CreateFile(
        filename,
        GENERIC_READ | GENERIC_WRITE, 
        FILE_SHARE_READ, 
        NULL, 
        OPEN_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL, 
        NULL);
    if (mHandle->mFile == INVALID_HANDLE_VALUE)
    {
        Close();
        return false;
    }

    LARGE_INTEGER filesize;
    if (GetFileSizeEx(mHandle->mFile, &filesize) == FALSE)
    {
        Close();
        return false;
    }
    mHandle->mFileSize = static_cast<SizeT>(filesize.QuadPart);

    mHandle->mMapping = CreateFileMappingA(
        mHandle->mFile, 
        NULL, 
        PAGE_READWRITE,
        0, 
        0, 
        NULL);
    if (mHandle->mMapping == NULL)
    {
        Close();
        return false;
    }

    mHandle->mView = MapViewOfFile(mHandle->mMapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        mHandle->mFileSize);

    return true;
}
void MappedFile::Close()
{
    if (mHandle->mView != nullptr)
    {
        UnmapViewOfFile(mHandle->mView);
        mHandle->mView = nullptr;
    }

    if (mHandle->mFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(mHandle->mFile);
        mHandle->mFile = INVALID_HANDLE_VALUE;
    }

    if (mHandle->mMapping != NULL)
    {
        CloseHandle(mHandle->mMapping);
        mHandle->mMapping = NULL;
    }

    mHandle->mFileSize = 0;
}

bool MappedFile::Write(SizeT filePosition, const void* bytes, SizeT numBytes)
{
    if (!mHandle->mView)
    {
        return false;
    }

    if (filePosition > mHandle->mFileSize || (filePosition + numBytes) > mHandle->mFileSize)
    {
        return false;
    }

    memcpy(mHandle->mView, bytes, numBytes);
    return true;
}

bool MappedFile::Flush()
{
    return FlushViewOfFile(mHandle->mView, 0) == TRUE;
}

} // namespace lf
#endif // LS_OS_WINDOWS