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
#include "Core/Test/Test.h"
#include "Core/Test/TestUtils.h"

#include "Core/Crypto/SHA256.h"
#include "Core/String/StringCommon.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Math/Random.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"
#include <new>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace lf {

const SizeT GB = 1024 * 1024 * 1024;

struct MemoryMappedTestResult;

class MemoryMappedFile
{
public:
    MemoryMappedFile();
    bool Open(const String& filename);
    void Close();

    HANDLE mFileHandle;
    HANDLE mMapHandle;
    void*  mMemory;
    String mFilename;
};

struct SimpleClock
{
    void Start() { mBegin = GetClockTime(); }
    void Stop() { mEnd = GetClockTime(); }
    Float64 Delta() { return (mEnd - mBegin) / static_cast<Float64>(GetClockFrequency()); }

    Int64 mBegin;
    Int64 mEnd;
};

struct MemoryMappedTestResult
{
    MemoryMappedFile mFile;

    String mFilename;
    Int32  mSeed;

    String mMemoryHash;

    String mMapWriteFileHash;
    String mMapReadFileHash;

    String mWriteFileHash;
    String mReadFileHash;

    SimpleClock mReserveTime;

    SimpleClock mRandomGenerationTime;
    SimpleClock mHashMemoryTime;

    SimpleClock mHashWriteMapFileTime;
    SimpleClock mHashReadMapFileTime;

    SimpleClock mHashWriteFileTime;
    SimpleClock mHashReadFileTime;
};

// ByteToHex

MemoryMappedFile::MemoryMappedFile() :
mFileHandle(INVALID_HANDLE_VALUE),
mMapHandle(NULL),
mMemory(nullptr),
mFilename()
{

}

bool MemoryMappedFile::Open(const String& filename)
{
    // Try and open the file read/write (assume the file is already 1GB in size)
    SetLastError(NO_ERROR);
    HANDLE file = CreateFile(
        filename.CStr(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        gTestLog.Error(LogMessage("Failed to open file ") << filename << " with error code " << static_cast<SizeT>(GetLastError()));
        return false;
    }

    // Try and create the file mapping
    SetLastError(NO_ERROR);
    HANDLE mapFile = CreateFileMappingA(
        file,
        NULL,
        PAGE_READWRITE,
        0,
        0,
        NULL);
    if (mapFile == NULL)
    {
        gTestLog.Error(LogMessage("Failed to map file ") << filename << " with error code " << static_cast<SizeT>(GetLastError()));
        CloseHandle(mapFile);
        return false;
    }

    // Try to map the file to memory.
    SetLastError(NO_ERROR);
    void* address = MapViewOfFile(
        mapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        GB);

    if (address == nullptr)
    {
        gTestLog.Error(LogMessage("Failed to map view of file ") << filename << " with error code " << static_cast<SizeT>(GetLastError()));
        CloseHandle(mapFile);
        CloseHandle(file);
        return false;
    }

    
    mFileHandle = file;
    mMapHandle = mapFile;
    mMemory = address;
    mFilename = filename;
    return true;
}

void MemoryMappedFile::Close()
{
    if (mMemory)
    {
        if (UnmapViewOfFile(mMemory) == FALSE)
        {
            gTestLog.Error(LogMessage("Failed to UnmapViewOfFile File=") << mFilename);
        }
        mMemory = nullptr;
    }

    if (mMapHandle != NULL)
    {
        if (CloseHandle(mMapHandle) == FALSE)
        {
            gTestLog.Error(LogMessage("Failed to CloseHandle(mMapHandle) File=") << mFilename);
        }
        mMapHandle = nullptr;
    }

    if (mFileHandle != INVALID_HANDLE_VALUE)
    {
        if (CloseHandle(mFileHandle) == FALSE)
        {
            gTestLog.Error(LogMessage("Failed to CloseHandle(mFileHandle) File=") << mFilename);
        }
        mFileHandle = nullptr;
    }

    mFilename.Clear();
}

void FileIOMMapStressTest_Reserve(TArray<MemoryMappedTestResult>& results)
{
    for (MemoryMappedTestResult& result : results)
    {
        result.mReserveTime.Start();
        TEST_CRITICAL(FileSystem::FileReserve(result.mFilename, GB));
        result.mReserveTime.Stop();
        gTestLog.Info(LogMessage("Create File: ") << result.mFilename << " in " << result.mReserveTime.Delta() * 1000.0 << " ms");
    }
    
}

void FileIOMMapStressTest_GenerateMemoryHash(TArray<MemoryMappedTestResult>& results)
{
    gTestLog.Info(LogMessage("Generating Memory Hashes..."));

    ByteT* buffer = reinterpret_cast<ByteT*>(LFAlloc(GB, 16));
    TEST_CRITICAL(buffer != nullptr);
    for (MemoryMappedTestResult& result : results)
    {
        Int32 seed = result.mSeed;

        result.mRandomGenerationTime.Start();
        for (SizeT i = 0; i < GB; ++i)
        {
            buffer[i] = static_cast<ByteT>(Random::Rand(seed) & 0xFF);
        }
        result.mRandomGenerationTime.Stop();
        
        result.mHashMemoryTime.Start();
        Crypto::SHA256HashType hash = Crypto::SHA256Hash(buffer, GB);
        result.mHashMemoryTime.Stop();
        result.mMemoryHash = BytesToHex(hash.mData, Crypto::SHA256_BLOCK_SIZE);
    }
    LFFree(buffer);
}

void FileIOMMapStressTest_WriteMapMemoryHash(TArray<MemoryMappedTestResult>& results)
{
    gTestLog.Info(LogMessage("Writing mapped memory hashes..."));

    for (MemoryMappedTestResult& result : results)
    {
        Int32 seed = result.mSeed;

        result.mHashWriteMapFileTime.Start();
        if (result.mFile.Open(result.mFilename))
        {
            ByteT* buffer = reinterpret_cast<ByteT*>(result.mFile.mMemory);
            for (SizeT i = 0; i < GB; ++i)
            {
                buffer[i] = static_cast<ByteT>(Random::Rand(seed) & 0xFF);
            }
            result.mHashWriteMapFileTime.Stop();
            result.mMapWriteFileHash = BytesToHex(Crypto::SHA256Hash(buffer, GB).mData, Crypto::SHA256_BLOCK_SIZE);
            result.mFile.Close();
        }
    }
}

void FileIOMMapStressTest_WriteMemoryHash(TArray<MemoryMappedTestResult>& results)
{
    gTestLog.Info(LogMessage("Writing memory hashes..."));

    ByteT* buffer = reinterpret_cast<ByteT*>(LFAlloc(GB, 16));
    for (MemoryMappedTestResult& result : results)
    {
        Int32 seed = result.mSeed;

        result.mHashWriteFileTime.Start();
        File file;
        if (file.Open(result.mFilename, FF_READ | FF_WRITE, FILE_OPEN_EXISTING))
        {
            for (SizeT i = 0; i < GB; ++i)
            {
                buffer[i] = static_cast<ByteT>(Random::Rand(seed) & 0xFF);
            }
            file.Write(buffer, GB);
            result.mHashWriteFileTime.Stop();
            result.mWriteFileHash = BytesToHex(Crypto::SHA256Hash(buffer, GB).mData, Crypto::SHA256_BLOCK_SIZE);
            file.Close();
        }
    }

    LFFree(buffer);
}

void FileIOMMapStressTest_ReadMapMemoryHash(TArray<MemoryMappedTestResult>& results)
{
    gTestLog.Info(LogMessage("Reading mapped memory hashes..."));
    
    for (MemoryMappedTestResult& result : results)
    {
        result.mHashReadMapFileTime.Start();
        if (result.mFile.Open(result.mFilename))
        {
            ByteT* buffer = reinterpret_cast<ByteT*>(result.mFile.mMemory);
            result.mHashReadMapFileTime.Stop();
            result.mMapReadFileHash = BytesToHex(Crypto::SHA256Hash(buffer, GB).mData, Crypto::SHA256_BLOCK_SIZE);
            result.mFile.Close();
        }
    }
}

void FileIOMMapStressTest_ReadMemoryHash(TArray<MemoryMappedTestResult>& results)
{
    gTestLog.Info(LogMessage("Reading memory hashes..."));
    ByteT* buffer = reinterpret_cast<ByteT*>(LFAlloc(GB, 16));

    for (MemoryMappedTestResult& result : results)
    {
        result.mHashReadFileTime.Start();
        File file;
        if (file.Open(result.mFilename, FF_READ, FILE_OPEN_EXISTING))
        {
            SizeT bytesRead = file.Read(buffer, GB);
            TEST(bytesRead == GB);
            result.mHashReadFileTime.Stop();
            result.mReadFileHash = BytesToHex(Crypto::SHA256Hash(buffer, GB).mData, Crypto::SHA256_BLOCK_SIZE);
            file.Close();
        }
    }

    LFFree(buffer);
}

void FileIOMMapStressTest_Free(const String& testPath)
{
    TArray<String> files = FileSystem::GetFiles(testPath);
    for (const String& file : files)
    {
        if (Valid(file.Find("TestFile_")))
        {
            FileSystem::FileDelete(file);
        }
    }
}

void FileIOMMapStressTest(const String& testPath)
{
    const SizeT NUM_FILES = 100;
    const Int32 SEED = 0xDEFBAC;
    Int32 seed = SEED;
    TArray<MemoryMappedTestResult> testResults;
    testResults.Reserve(NUM_FILES);
    testResults.Resize(NUM_FILES);


    // Setup:
    for (SizeT i = 0; i < NUM_FILES; ++i)
    {
        Int32 scratch[100];
        for (SizeT k = 0; k < LF_ARRAY_SIZE(scratch); ++k)
        {
            scratch[k] = Random::Rand(seed);
        }
        Crypto::SHA256HashType hash = Crypto::SHA256Hash(reinterpret_cast<ByteT*>(scratch), sizeof(scratch));
        testResults[i].mFilename = FileSystem::PathJoin(testPath, "TestFile_" + ToHexString(i) + ".txt");
        testResults[i].mSeed = *reinterpret_cast<Int32*>(&hash.mData[i % Crypto::SHA256_BLOCK_SIZE]);
    }

    // Reserve memory...
    FileIOMMapStressTest_Reserve(testResults);
    TEST_CRITICAL(!TestFramework::HasFailed());
    

    //
    // Create Seed (constant)
    // Create Temp Seed
    // 
    // Create Data                  -- Profile_GenerateRandom
    // Generate a Hash MD5/Sha256   -- Profile_HashMemory
    // Reset Seed                   
    FileIOMMapStressTest_GenerateMemoryHash(testResults);

    // MapFile                      -- Profile_MapFile
    // Create Data                  -- Profile_MapWriteFile
    // Generate a Hash MD5/Sha256   -- Profile_HashWriteFile
    // Close/Unmap File
    FileIOMMapStressTest_WriteMapMemoryHash(testResults);

    // MapFile ReadOnly             -- Profile_MapFileForRead
    // Generate a Hash MD5/Sha256   -- Profile_HashReadFile
    FileIOMMapStressTest_ReadMapMemoryHash(testResults);

    // VerifyHash
    //
    // DeleteFiles
    FileIOMMapStressTest_Free(testPath);

    // -- // Reserve memory...
    // -- FileIOMMapStressTest_Reserve(testResults);
    // -- TEST_CRITICAL(!TestFramework::HasFailed());
    // -- 
    // -- FileIOMMapStressTest_WriteMemoryHash(testResults);
    // -- // FileIOMMapStressTest_ReadMemoryHash(testResults);
    // -- 
    // -- // FileIOMMapStressTest_WriteMapMemoryHash(testResults);
    // -- FileIOMMapStressTest_ReadMapMemoryHash(testResults);
    // -- 
    // -- FileIOMMapStressTest_Free(testPath);

    for (SizeT i = 0; i < NUM_FILES; ++i)
    {
        gTestLog.Info(LogMessage("TestResults for ") << testResults[i].mFilename
            << "\n  Reserve Time " << testResults[i].mReserveTime.Delta() * 1000.0 << " (ms)"

            << "\n  Generation Time " << testResults[i].mRandomGenerationTime.Delta() * 1000.0 << " (ms)"
            << "\n  HashMemory Time " << testResults[i].mHashMemoryTime.Delta() * 1000.0 << " (ms)"
            << "\n      Memory Hash " << testResults[i].mMemoryHash

            << "\n  -- Mapped File --"
            << "\n  Write Map File Time " << testResults[i].mHashWriteMapFileTime.Delta() * 1000.0 << " (ms)"
            << "\n  Write Map File Hash " << testResults[i].mMapWriteFileHash
        
            << "\n  Read Map File Time " << testResults[i].mHashReadMapFileTime.Delta() * 1000.0 << " (ms)"
            << "\n  Read Map File Hash " << testResults[i].mMapReadFileHash

            // << "\n -- Normal File --"
            // << "\n  Write File Time " << testResults[i].mHashWriteFileTime.Delta() * 1000.0 << " (ms)"
            // << "\n  Write File Hash " << testResults[i].mWriteFileHash
            // 
            // << "\n  Read File Time " << testResults[i].mHashReadFileTime.Delta() * 1000.0 << " (ms)"
            // << "\n  Read File Hash " << testResults[i].mReadFileHash
        );


    }
}

void FileIOCreateFiles(const String& path, Int32& randSeed)
{
    // auto x = TypeConstructionTraits<TaskItem>::TypeT();

    // Allocate 15GB of files...
    const SizeT NUM_FILES = 15;

    for (SizeT i = 0; i < NUM_FILES; ++i)
    {
        String filename = FileSystem::PathJoin(path, "TestFile_" + ToHexString(i) + ".txt");
        gTestLog.Info(LogMessage("Create File: ") << filename);
        TEST_CRITICAL(FileSystem::FileReserve(filename, GB));
    }

    // TArray<ByteT> buffer;
    // buffer.Reserve(GB);
    // buffer.Resize(GB);
    ByteT* buffer = nullptr;

    // Fill with "random" data
    for (SizeT i = 0; i < NUM_FILES; ++i)
    {
        String filename = FileSystem::PathJoin(path, "TestFile_" + ToHexString(i) + ".txt");

        // File file;
        // TEST_CRITICAL(file.Open(filename, FF_READ | FF_WRITE | FF_BULK_WRITE, FILE_OPEN_EXISTING));
        
        SetLastError(NO_ERROR);
        HANDLE file = CreateFile(
                            filename.CStr(),
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            gTestLog.Error(LogMessage("Failed to open file ") << filename << " with error code " << static_cast<SizeT>(GetLastError()));
            continue;
        }
        SetLastError(NO_ERROR);
        HANDLE mapFile = CreateFileMappingA(
                            file,
                            NULL,
                            PAGE_READWRITE,
                            0,
                            0,
                            NULL);
        if (mapFile == NULL)
        {
            gTestLog.Error(LogMessage("Failed to map file ") << filename << " with error code " << static_cast<SizeT>(GetLastError()));
            CloseHandle(mapFile);
            continue;
        }

        SetLastError(NO_ERROR);
        void* address = MapViewOfFile(
                            mapFile,
                            FILE_MAP_ALL_ACCESS,
                            0,
                            0,
                            GB / 2);
        
        if (address == nullptr)
        {
            gTestLog.Error(LogMessage("Failed to map view of file ") << filename << " with error code " << static_cast<SizeT>(GetLastError()));
            CloseHandle(mapFile);
            CloseHandle(file);
            continue;
        }

        buffer = reinterpret_cast<ByteT*>(address);
        Int64 clockFreq = GetClockFrequency();
        Int64 clockBegin = GetClockTime();
        for (SizeT k = 0; k < GB / 2; ++k)
        {
            ByteT byte = static_cast<ByteT>(Random::Rand(randSeed) & 0xFF);
            buffer[k] = byte;
        }
        Int64 clockEnd = GetClockTime();
        Float64 seconds = (clockEnd - clockBegin) / static_cast<Float64>(clockFreq);
        buffer = nullptr;
        gTestLog.Info(LogMessage("Generated 1GB of data in ") << seconds << " seconds or " << (seconds * 1000.0) << " ms");

        UnmapViewOfFile(address);
        CloseHandle(mapFile);
        CloseHandle(file);


        // My findings....
        // WriteFile is fucking slow...
        // Writing to a memory mapped file is fast.. but it's not streamed...
        // 

        // clockBegin = GetClockTime();
        // // TEST(file.Write(buffer.GetData(), GB) == GB);
        // 
        // 
        // clockEnd = GetClockTime();
        // seconds = (clockEnd - clockBegin) / static_cast<Float64>(clockFreq);
        // gTestLog.Info(LogMessage("Wrote 1GB of data in ") << seconds << " seconds or " << (seconds * 1000.0) << " ms");

        // file.Close();
    }
}

void FileIODeleteFiles(const String& path)
{
    TArray<String> files = FileSystem::GetFiles(path);
    for (const String& file : files)
    {
        if (Valid(file.Find("TestFile_")))
        {
            FileSystem::FileDelete(file);
        }
    }
}

REGISTER_TEST(FileIOBenchmarkTest)
{
    String testPath = FileSystem::PathJoin(GetTestDirectory(), "FileIOBenchmarkTest");
    TEST_CRITICAL(FileSystem::PathCreate(testPath));

    // What stats am I looking for?
    // Open 1GB file...
    // Read 1000 different sectors...

    FileIOMMapStressTest(testPath);

    // Int32 seed = 0xDEFBAC;
    // 
    // FileIOCreateFiles(testPath, seed);
    // FileIODeleteFiles(testPath);



    gTestLog.Info(LogMessage("Working Path=") << testPath);
}

} // namespace lf