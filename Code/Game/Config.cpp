// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "Core/Common/Types.h"
#include "Core/Common/Assert.h"
#include "Core/Platform/Thread.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Utility/StaticCallback.h"
#include "Core/Math/Random.h"
#include "Core/String/StringUtil.h"
#include "Core/String/String.h"
#include "Core/String/WString.h"
#include "Core/String/Token.h"
#include "Core/String/TokenTable.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/Utility.h"
#include "Core/Test/Test.h"

#include "Runtime/Config.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Core/Tests/StringTest.h"
#include "Core/Tests/WStringTest.h"
#include "Core/Tests/ThreadTest.h"
#include "Core/Tests/FileTests.h"
#include "Core/Tests/PointerTest.h"
#include "Core/Tests/SStreamTest.h"

#include "Engine/App/Program.h"
#include "Engine/App/Application.h"

#include <cwchar>

namespace lf {

struct FileBuffer
{
    Int64 mPosition;        // Position in the file this buffer starts at
    Int64 mBufferLength;    // How many bytes from the file are contained within mBuffer
    char* mBuffer;          // The raw data of the file
};

struct FilePointer
{
    HANDLE mFileHandle;         // 
    Int64  mVirtualCursor;
    FileBuffer mWorkingBuffer;
    FileBuffer mCachedBuffer;
};

class IOCompletionPort
{
public:
    IOCompletionPort() :
        mHandle(NULL)
    {
    }
    ~IOCompletionPort()
    {
        Close();
    }

    bool Create(SizeT numConcurrentThreads = 0)
    {
        Assert(!mHandle);
        mHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, static_cast<DWORD>(numConcurrentThreads));
        return mHandle != NULL;
    }

    bool Close()
    {
        if (mHandle != NULL && CloseHandle(mHandle))
        {
            mHandle = NULL;
            return true;
        }
        return false;
    }

    bool AddDevice(HANDLE device, ULONG_PTR completionKey)
    {
        Assert(mHandle != NULL);
        HANDLE handle = CreateIoCompletionPort(device, mHandle, completionKey, 0);
        return handle == mHandle;
    }

    bool QueuePacket(ULONG_PTR completionKey, DWORD numBytes = 0, OVERLAPPED* po = NULL)
    {
        return PostQueuedCompletionStatus(mHandle, numBytes, completionKey, po) == TRUE;
    }

    bool DequeuePacket(ULONG_PTR* completionKey, DWORD* numBytes, OVERLAPPED** po, DWORD milliseconds = INFINITE)
    {
        return GetQueuedCompletionStatus(mHandle, numBytes, completionKey, po, milliseconds) == TRUE;
    }

private:
    HANDLE mHandle;
};

struct OVERLAPPEDEX : OVERLAPPED
{
    OVERLAPPEDEX(int nType = 0, char* pbBuffer = NULL, DWORD bufferLength = 0)
    {
        Internal = InternalHigh = 0;
        Offset = OffsetHigh = 0;
        hEvent = NULL;
        Type = nType;
        Buffer = pbBuffer;
        BufferLength = bufferLength;
    }

    int Type;
    char* Buffer;
    DWORD BufferLength;
};


SizeT gSampleFileKey = 0;
const SizeT IO_READ = 1001;
const SizeT IO_WRITE = 1002;

DWORD WINAPI IOCompletionThread(LPVOID param)
{
    IOCompletionPort* port = reinterpret_cast<IOCompletionPort*>(param);

    ULONG_PTR completionKey = 0;
    DWORD bytesTransferred = 0;
    OVERLAPPEDEX* po = NULL;

    while (port->DequeuePacket(&completionKey, &bytesTransferred, reinterpret_cast<OVERLAPPED**>(&po)))
    {
        if (bytesTransferred == 0 && completionKey == 0 && NULL == po)
        {
            break;
        }
        else if (completionKey == gSampleFileKey)
        {
            continue;
        }

        switch (po->Type)
        {
            case IO_READ:
                printf("%d bytes were read by %lld.\n", bytesTransferred,reinterpret_cast<uintptr_t>(po));
                break;
            case IO_WRITE:
                printf("%d bytes were written.\n", bytesTransferred);
                break;
        }

    }

    return 0;
}

const UInt16 BLOCK_INVALID_BLOCK_TYPE = 0xFFFF;
const UInt16 BLOCK_MAX_BLOCK_TYPE= 0xFFFF - 1;
const UInt8  BLOCK_FLAG_INDUSTRUCTIBLE = 1 << 0;
const UInt8  BLOCK_FLAG_ADMIN = 1 << 1;
const UInt8  BLOCK_FLAG_HIDDEN = 1 << 2;
const SizeT  BLOCK_CHUNK_BATCH_SIZE = 64;

struct BlockStaticData
{
    UInt16 mType;
    UInt8 mVariation;
    UInt8 mFlags;
};
struct BlockChunk
{
    BlockStaticData mBlocks[256][16][16];
};

SizeT CompareChunk(BlockChunk& a, BlockChunk& b)
{
    SizeT errors = 0;
    for (SizeT y = 0; y < 256; ++y)
    {
        for (SizeT x = 0; x < 16; ++x)
        {
            for (SizeT z = 0; z < 16; ++z)
            {
                BlockStaticData& blockA = a.mBlocks[y][x][z];
                BlockStaticData& blockB = b.mBlocks[y][x][z];

                if (blockA.mType != blockB.mType || 
                    blockA.mFlags != blockB.mFlags ||
                    blockA.mVariation != blockB.mVariation)
                {
                    ++errors;
                }
            }
        }
    }
    return errors;
}

void GenerateChunk(BlockChunk& chunk, Int32& seed)
{
    for (SizeT y = 0; y < 256; ++y)
    {
        for (SizeT x = 0; x < 16; ++x)
        {
            for (SizeT z = 0; z < 16; ++z)
            {
                BlockStaticData& block = chunk.mBlocks[y][x][z];
                block.mType = static_cast<UInt16>(Random::Mod(seed, BLOCK_MAX_BLOCK_TYPE));
                block.mVariation = static_cast<UInt8>(Random::Mod(seed, 0xFF));
                block.mFlags = 0;

                if (Random::RandF(seed) > 0.2f)
                {
                    block.mFlags |= BLOCK_FLAG_INDUSTRUCTIBLE;
                }
                if (Random::RandF(seed) > 0.08f)
                {
                    block.mFlags |= BLOCK_FLAG_HIDDEN;
                }
                if (Random::RandF(seed) > 0.01f)
                {
                    block.mFlags |= BLOCK_FLAG_ADMIN;
                }
            }
        }
    }
}

void WriteChunk(BlockChunk& chunk, HANDLE file)
{
    if (file != INVALID_HANDLE_VALUE)
    {
        void* data = reinterpret_cast<void*>(&chunk);
        SizeT dataLength = sizeof(BlockChunk);
        DWORD bytesWritten = 0;
        Assert(WriteFile(file, data, static_cast<DWORD>(dataLength), &bytesWritten, NULL) == TRUE);
    }
}

void ReadChunk(BlockChunk& chunk, HANDLE file)
{
    if (file != INVALID_HANDLE_VALUE)
    {
        void* data = reinterpret_cast<void*>(&chunk);
        SizeT dataLength = sizeof(BlockChunk);
        DWORD bytesRead = 0;
        Assert(ReadFile(file, data, static_cast<DWORD>(dataLength), &bytesRead, NULL) == TRUE);
        Assert(bytesRead == dataLength);
    }
}

void CreateChunkData(const char* filename, SizeT iterations)
{
    HANDLE file = CreateFile
                  (
                      filename,
                      GENERIC_READ | GENERIC_WRITE,
                      0, // Exclusive Access
                      NULL,
                      OPEN_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL
                  );

    if (file == INVALID_HANDLE_VALUE)
    {
        printf("Failed to create file! %s\n", filename);
        return;
    }
    printf("Opened file %s\n", filename);

    LARGE_INTEGER freq;
    LARGE_INTEGER begin;
    LARGE_INTEGER end;
    QueryPerformanceCounter(&begin);
    Int32 seed = 0x4355766F;
    BlockChunk chunk;
    for (size_t i = 0; i < iterations; ++i)
    {
        GenerateChunk(chunk, seed);
        WriteChunk(chunk, file);
    }
    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&freq);

    auto ticks = end.QuadPart - begin.QuadPart;
    double time = static_cast<double>(ticks) / freq.QuadPart;

    printf("Create Chunk Data with Iterations=%lld took %lld ticks, %f\n", iterations, ticks, time);

    Assert(CloseHandle(file) == TRUE);
}

void LoadChunkData(const char* filename, SizeT iterations)
{
    HANDLE file = CreateFile
    (
        filename,
        GENERIC_READ,
        0, // Exclusive Access
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE)
    {
        printf("Failed to open file! %s\n", filename);
        return;
    }
    printf("Opened file %s\n", filename);

    LARGE_INTEGER freq;
    LARGE_INTEGER begin;
    LARGE_INTEGER end;
    QueryPerformanceCounter(&begin);
    Int32 seed = 0x4355766F;
    BlockChunk generatedChunk;
    BlockChunk readChunk;
    for (SizeT i = 0; i < iterations; ++i)
    {
        GenerateChunk(generatedChunk, seed);
        ReadChunk(readChunk, file);
        SizeT compare = CompareChunk(generatedChunk, readChunk);
        Assert(compare == 0);
    }
    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&freq);

    auto ticks = end.QuadPart - begin.QuadPart;
    double time = static_cast<double>(ticks) / freq.QuadPart;

    printf("Load Chunk Data with Iterations=%lld took %lld ticks, %f\n", iterations, ticks, time);

    Assert(CloseHandle(file) == TRUE);
}

const long BATCH_STATE_QUEUED = 0;
const long BATCH_STATE_PROCESSING = 1;
const long BATCH_STATE_DONE = 2;
const long BATCH_STATE_EMPTY = 3;

struct ChunkBatchHandle
{
    ChunkBatchHandle() : 
    mOutputChunk(nullptr),
    mFileHandle(INVALID_HANDLE_VALUE),
    mOffset(0),
    mState(BATCH_STATE_EMPTY)
    {}

    BlockChunk* mOutputChunk;
    HANDLE      mFileHandle;
    SizeT       mOffset;
    volatile long mState;
};

struct ChunkBatch
{
    bool Create(const char* filename, IOCompletionPort* ioPort)
    {
        mPort = ioPort;

        for (SizeT i = 0; i < BLOCK_CHUNK_BATCH_SIZE; ++i)
        {
            ChunkBatchHandle& handle = mHandles[i];
            Assert(handle.mFileHandle == INVALID_HANDLE_VALUE);
            Assert(handle.mOutputChunk == nullptr);
            handle.mFileHandle = CreateFile
            (
                filename,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                NULL
            );
            if (handle.mFileHandle == INVALID_HANDLE_VALUE)
            {
                return false;
            }
        }
        return true;
    }

    ChunkBatchHandle mHandles[BLOCK_CHUNK_BATCH_SIZE];
    IOCompletionPort* mPort;
};

void LoadChunkDataParallel(const char* filename, SizeT iterations)
{
    SYSTEM_INFO sysInfo = { 0 };
    GetNativeSystemInfo(&sysInfo);

    DWORD numThreads = sysInfo.dwNumberOfProcessors;
    IOCompletionPort ioPort;
    if (ioPort.Create(numThreads))
    {
        // ChunkBatch batch;
        (filename);
        (iterations);


    }
    else
    {
        printf("Failed to create IOCompletionPort\n");
    }

    
    

}

void DisplayWorldSize()
{
    SizeT sizeOfChunk = sizeof(BlockChunk);
    SizeT numChunks = 32 * 32; // 16 * 16;
    SizeT sizeOfWorldBytes = sizeOfChunk * numChunks;
    SizeT sizeOfWorldKB = sizeOfWorldBytes / 1024;
    SizeT sizeofWorldMB = sizeOfWorldKB / 1024;

    printf("Chunk Size = %lld(B)\n", sizeOfChunk);
    printf("World Length/Width=%d\n", 32 * 16);
    printf("Size Of World %lld(B)\n", sizeOfWorldBytes);
    printf("Size Of World %lld(KB)\n", sizeOfWorldKB);
    printf("Size Of World %lld(MB)\n", sizeofWorldMB);
}

void ReserveChunkData(const char* filename, SizeT size)
{
    HANDLE file = CreateFile
    (
        filename,
        GENERIC_READ | GENERIC_WRITE,
        0, // Exclusive Access
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    bool exists = file != INVALID_HANDLE_VALUE;
    if (exists)
    {
        printf("File exists, deleting... %s\n", filename);
        Assert(CloseHandle(file) == TRUE);
        file = INVALID_HANDLE_VALUE;
        Assert(DeleteFile(filename) == TRUE);
    }

    file = CreateFile
    (
        filename,
        GENERIC_READ | GENERIC_WRITE,
        0, // Exclusive Access
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    Assert(file != INVALID_HANDLE_VALUE);
    printf("Reserving disk space file=%s, space=%lld\n", filename, size);
    LARGE_INTEGER fsize;
    fsize.QuadPart = size;
    Assert(SetFilePointerEx(file, fsize, 0, FILE_BEGIN) == TRUE);
    Assert(SetEndOfFile(file) == TRUE);
    Assert(CloseHandle(file) == TRUE);
}

void CreateChunks()
{
    // ReserveChunkData("D:\\Game Development\\Engine\\LiteForge\\Content\\Block_02.bin", 256 * 1024 * 3968);
    // CreateChunkData("D:\\Game Development\\Engine\\LiteForge\\Content\\Block_00.bin", 32);
    // CreateChunkData("D:\\Game Development\\Engine\\LiteForge\\Content\\Block_01.bin", 3968);
    // CreateChunkData("D:\\Game Development\\Engine\\LiteForge\\Content\\Block_02.bin", 3968);

    LoadChunkDataParallel("D:\\Game Development\\Engine\\LiteForge\\Content\\Block_01.bin", 3968);

}


// 

} // namespace lf


constexpr LF_FORCE_INLINE int MakeInt(const char a, const char b, const char c, const char d)
{
    // 0xAABBCCDD
    return a << 24 | b << 16 | c << 8 | d;
}


lf::TokenTable GlobalTokenTable;
STATIC_TOKEN(ArgName, "Text");

namespace lf {
    TestSuite gTests
    (
        StringTest,
        WStringTest,
        FileTest,
        ThreadTest,
        PointerTest,
        SStreamTest
    );
}



// int main(int argc, const char** argv)
// {
//     lf::Program::Execute(argc, argv);
//     /*
//     lf::SizeT bytesBefore = lf::LFGetBytesAllocated();
//     lf::SetMainThread();
//     lf::gTokenTable = &GlobalTokenTable;
//     GlobalTokenTable.Initialize();
//     lf::ExecuteStaticInit();
//     lf::CreateChunks();
// 
//     {
//         lf::MemoryBuffer buffer;
//         buffer.Allocate(200, 16);
//     }
// 
//     (argc);
//     (argv);
//     lf::gAssertCallback = GameAssert;
//     lf::TestFramework::ExecuteAllTests();
//     lf::ExecuteStaticDestroy();
//     GlobalTokenTable.Shutdown();
//     lf::SizeT bytesAfter = lf::LFGetBytesAllocated();
//     Assert(bytesBefore == bytesAfter);
//     */
//     return 0;
// }