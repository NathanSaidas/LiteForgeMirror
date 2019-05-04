// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
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

namespace lf {

const char DIR_CHAR = '\\';
const char TYPE_CHAR = '/';
const SizeT LF_MAX_PATH = 2048;

static bool EndsWithDirChar(const String& str)
{
    return str.Empty() ? false : str.Last() == DIR_CHAR || str.First() == TYPE_CHAR;
}
static bool BeginsWithDirChar(const String& str)
{
    return str.Empty() ? false : str.First() == DIR_CHAR || str.First() == TYPE_CHAR;
}
static bool IsLikelyFilePath(const String& str)
{
    for (SizeT i = str.Size() - 1; i >= 0; --i)
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
    AssertError(CloseHandle(file), LF_ERROR_INTERNAL, ERROR_API_CORE);
    return created;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}


bool FileSystem::FileDelete(const String& filename)
{
#if defined(LF_OS_WINDOWS)
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
    AssertError(CloseHandle(file), LF_ERROR_INTERNAL, ERROR_API_CORE);
    return result;
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

bool FileSystem::PathCreate(const String& path)
{
    return PathRecursiveCreate(path);
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
    CHAR buffer[LF_MAX_PATH];
    DWORD result = GetCurrentDirectory(LF_MAX_PATH, buffer);
    if (result == 0 || result > LF_MAX_PATH)
    {
        return String();
    }
    return String(buffer);
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

}