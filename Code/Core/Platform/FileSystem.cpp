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
#include "FileSystem.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/String/String.h"

#if defined(LF_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include <algorithm>

namespace lf {

const char DIR_CHAR = '\\';
const char TYPE_CHAR = '/';
const SizeT LF_MAX_PATH = 2048;

static bool EndsWithDirChar(const String& str)
{
    return str.Empty() ? false : str.Last() == DIR_CHAR || str.Last() == TYPE_CHAR;
}
static bool BeginsWithDirChar(const String& str)
{
    return str.Empty() ? false : str.First() == DIR_CHAR || str.First() == TYPE_CHAR;
}
static bool IsLikelyFilePath(const String& str)
{
    for (SizeT i = str.Size() - 1; Valid(i); --i)
    {
        if (str[i] == DIR_CHAR || str[i] == TYPE_CHAR)
        {
            return false;
        }
        if (str[i] == '.')
        {
            return true;
        }
    }
    return false;
}
static bool PathRecursiveCreate(const String& path)
{
#if defined(LF_OS_WINDOWS)
    if (!FileSystem::PathExists(path))
    {
        bool result = PathRecursiveCreate(FileSystem::PathGetParent(path));
        return result && CreateDirectory(path.CStr(), NULL) == TRUE;
    }
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
    return true;
}


bool FileSystem::FileCreate(const String& filename)
{
#if defined(LF_OS_WINDOWS)
    HANDLE file = CreateFile
    (
        filename.CStr(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    bool created = file != INVALID_HANDLE_VALUE;
    AssertEx(CloseHandle(file), LF_ERROR_INTERNAL, ERROR_API_CORE);
    return created;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}


bool FileSystem::FileDelete(const String& filename)
{
#if defined(LF_OS_WINDOWS)
    SetLastError(ERROR_SUCCESS);
    DWORD error = 0;
    DWORD attribs = 0;
    
    attribs = GetFileAttributes(filename.CStr());
    error = GetLastError();

    // Failed to get file attributes:
    if (error != ERROR_SUCCESS)
    {
        return false;
    }

    // If files are marked for 'Read Only' make remove the flag so we can delete.
    if ((attribs & FILE_ATTRIBUTE_READONLY) > 0)
    {
        attribs = attribs & ~(FILE_ATTRIBUTE_READONLY);
        if (SetFileAttributes(filename.CStr(), attribs) == FALSE)
        {
            return false;
        }
    }
    return DeleteFile(filename.CStr()) == TRUE;
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

bool FileSystem::FileExists(const String& filename)
{
#if defined(LF_OS_WINDOWS)
    DWORD fileAttribs = GetFileAttributes(filename.CStr());
    return fileAttribs != INVALID_FILE_ATTRIBUTES && (fileAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

bool FileSystem::FileReserve(const String& filename, FileSize size)
{
#if defined(LF_OS_WINDOWS)
    HANDLE file = CreateFile
    (
        filename.CStr(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    LARGE_INTEGER cursor;
    cursor.QuadPart = size;
    bool result = SetFilePointerEx(file, cursor, NULL, FILE_BEGIN) == TRUE && SetEndOfFile(file) == TRUE;
    AssertEx(CloseHandle(file), LF_ERROR_INTERNAL, ERROR_API_CORE);
    return result;
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

bool FileSystem::PathCreate(const String& path)
{
    return PathRecursiveCreate(path);
}

bool FileSystem::PathDelete(const String& path)
{
#if defined(LF_OS_WINDOWS)
    return RemoveDirectory(path.CStr()) == TRUE;
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

bool FileSystem::PathDeleteRecursive(const String& path)
{
#if defined(LF_OS_WINDOWS)
    if (!FileSystem::PathExists(path))
    {
        return true;
    }

    TArray<String> paths;
    FileSystem::GetAllFiles(path, paths);

    for (const String& file : paths)
    {
        if (!FileSystem::FileDelete(file))
        {
            return false;
        }
    }

    paths.Resize(0);
    FileSystem::GetAllDirectories(path, paths);
    std::stable_sort(paths.begin(), paths.end(), [](const String& a, const String& b)
    {
        return a.Size() > b.Size();
    });

    for (const String& directory : paths)
    {
        if (!FileSystem::PathDelete(directory))
        {
            return false;
        }
    }
    return FileSystem::PathDelete(path);
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

bool FileSystem::PathExists(const String& path)
{
#if defined(LF_OS_WINDOWS)
    DWORD fileAttribs = GetFileAttributes(path.CStr());
    return fileAttribs != INVALID_FILE_ATTRIBUTES && (fileAttribs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

String FileSystem::PathJoin(const String& path, const String& other)
{
    String result;
    if (!EndsWithDirChar(path))
    {
        result = path + DIR_CHAR;
    }
    else
    {
        result = path;
    }

    if (IsLikelyFilePath(other))
    {
        if (BeginsWithDirChar(other))
        {
            result += other.SubString(1);
        }
        else
        {
            result += other;
        }
    }
    else
    {
        if (BeginsWithDirChar(other))
        {
            result += other.SubString(1);
        }
        else
        {
            result += other;
        }
        if (EndsWithDirChar(other))
        {
            result += DIR_CHAR;
        }
    }

    // Correct Path:
    result.Replace(TYPE_CHAR, DIR_CHAR);
    if (!IsLikelyFilePath(result))
    {
        if (!EndsWithDirChar(result))
        {
            result += DIR_CHAR;
        }
    }
    return result;
}

String FileSystem::PathGetParent(const String& path)
{
    String result = path;
    for (SizeT i = result.Size() - 1; i >= 0; --i)
    {
        if (result[i] == DIR_CHAR)
        {
            if (i == result.Size() - 1)
            {
                continue;
            }
            result = path.SubString(0, i + 1);
            return result;
        }
    }
    return result;
}

String FileSystem::PathGetExtension(const String& path)
{
    SizeT ext = path.FindLast('.');
    if (Invalid(ext))
    {
        return String();
    }
    return path.SubString(ext + 1);
}

String FileSystem::PathResolve(const String& path)
{
#if defined(LF_OS_WINDOWS)
    CHAR buffer[LF_MAX_PATH];
    DWORD result = GetFullPathName(path.CStr(), LF_MAX_PATH, buffer, NULL);
    if (result)
    {
        return PathCorrectPath(String(buffer));
    }
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
    return String();
}

String FileSystem::PathCorrectPath(const String& path)
{
    String result(path);
    result.Replace(TYPE_CHAR, DIR_CHAR);
    if (!IsLikelyFilePath(result))
    {
        if (!EndsWithDirChar(result))
        {
            result += DIR_CHAR;
        }
    }
    return result;
}

String FileSystem::GetWorkingPath()
{
#if defined(LF_OS_WINDOWS)
    CHAR buffer[LF_MAX_PATH + 1];
    DWORD result = GetCurrentDirectory(LF_MAX_PATH, buffer);
    if (result == 0 || result > LF_MAX_PATH)
    {
        return String();
    }
    if (buffer[result] != DIR_CHAR)
    {
        buffer[result] = DIR_CHAR;
        buffer[result + 1] = '\0';
    }
    return String(buffer);
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}


void FileSystem::GetFiles(const String& path, TArray<String>& outFiles)
{
#if defined(LF_OS_WINDOWS)
    if (!FileSystem::PathExists(path))
    {
        return;
    }
    String filter = path + "\\*.*";
    HANDLE handle;
    WIN32_FIND_DATA data;
    handle = FindFirstFile(filter.CStr(), &data);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (data.dwFileAttributes != INVALID_FILE_ATTRIBUTES &&
                (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY &&
                (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != FILE_ATTRIBUTE_HIDDEN)
            {
                String filename = data.cFileName;
                if (filename != "." && filename != "..")
                {
                    outFiles.Add(filename);
                }
            }
        } while (FindNextFile(handle, &data));
        FindClose(handle);
    }
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}
void FileSystem::GetDirectories(const String& path, TArray<String>& outDirectories)
{
#if defined(LF_OS_WINDOWS)
    if (!FileSystem::PathExists(path))
    {
        return;
    }
    String filter = path + "\\*.*";
    HANDLE handle;
    WIN32_FIND_DATA data;
    handle = FindFirstFile(filter.CStr(), &data);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (data.dwFileAttributes != INVALID_FILE_ATTRIBUTES &&
                (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY &&
                (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != FILE_ATTRIBUTE_HIDDEN)
            {
                String filename = data.cFileName;
                if (filename != "." && filename != "..")
                {
                    outDirectories.Add(filename);
                }
            }
        } while (FindNextFile(handle, &data));
        FindClose(handle);
    }
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}
void FileSystem::GetAllFiles(const String& path, TArray<String>& outFiles)
{
#if defined(LF_OS_WINDOWS)
    TArray<String> paths;
    FileSystem::GetFiles(path, paths);
    outFiles.Reserve(paths.Size());
    for (const String& filePath : paths)
    {
        outFiles.Add(FileSystem::PathJoin(path, filePath));
    }

    paths.Resize(0);
    GetDirectories(path, paths);
    for (const String& directory : paths)
    {
        GetAllFiles(FileSystem::PathJoin(path, directory), outFiles);
    }
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

void FileSystem::GetAllDirectories(const String& path, TArray<String>& outFiles)
{
#if defined(LF_OS_WINDOWS)
    TArray<String> paths;
    FileSystem::GetDirectories(path, paths);
    outFiles.Reserve(paths.Size());
    for (const String& filePath : paths)
    {
        outFiles.Add(FileSystem::PathJoin(path, filePath));
    }

    for (const String& directory : paths)
    {
        GetAllDirectories(FileSystem::PathJoin(path, directory), outFiles);
    }
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

}