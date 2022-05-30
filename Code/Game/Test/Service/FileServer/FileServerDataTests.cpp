// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "Core/Crypto/SHA256.h"
#include "Core/IO/BinaryStream.h"
#include "Core/IO/TextStream.h"
#include "Core/IO/JsonStream.h"
#include "Core/Math/Random.h"
#include "Core/Utility/Log.h"
#include "Runtime/Net/FileTransfer/FileTransferTypes.h"
#include "Runtime/Net/FileTransfer/MemoryResourceLocator.h"


namespace lf {

static const SizeT TEST_MAX_DOWNLOAD_DATA_SIZE = 1200;

static TVector<ByteT> GenerateFileServerData(SizeT numBytes, Int32* outSeed = nullptr)
{
    TVector<ByteT> data;
    Int32 seed = outSeed ? *outSeed : 0xF33D7AAB;
    data.resize(numBytes);

    for (SizeT i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<ByteT>(Random::Mod(seed, 0xFF));
    }

    if (outSeed)
    {
        *outSeed = seed;
    }

    return data;
}

static DownloadHash ToDownloadHash(const TVector<ByteT>& bytes)
{
    auto sha = Crypto::SHA256Hash(bytes.data(), bytes.size());
    LF_STATIC_ASSERT(sizeof(Crypto::SHA256Hash) == sizeof(DownloadHash));
    DownloadHash hash;
    memcpy(hash.mBytes, sha.Bytes(), sha.Size());
    return hash;
}

bool operator==(const DownloadHash& a, const DownloadHash& b)
{
    return memcmp(a.mBytes, b.mBytes, sizeof(a.mBytes)) == 0;
}
bool operator==(const DownloadRequest& a, const DownloadRequest& b)
{
    return a.mResourceIdentifier == b.mResourceIdentifier
        && a.mRequestID == b.mRequestID
        && a.mVersion == b.mVersion;
}
bool operator==(const DownloadResponse& a, const DownloadResponse& b)
{
    return memcmp(&a, &b, sizeof(a)) == 0; // POD
}
bool operator==(const DownloadFetchRequest& a, const DownloadFetchRequest& b)
{
    return memcmp(&a, &b, sizeof(a)) == 0; // POD
}
bool operator==(const DownloadFetchFragmentRequest& a, const DownloadFetchFragmentRequest& b)
{
    return a.mResourceHandle == b.mResourceHandle
        && a.mChunkID == b.mChunkID
        && a.mFragmentIDs == b.mFragmentIDs
        && a.mUseRange == b.mUseRange;
}
bool operator==(const DownloadFetchStopRequest& a, const DownloadFetchStopRequest& b)
{
    return memcmp(&a, &b, sizeof(a)) == 0; // POD
}
bool operator==(const DownloadCompleteRequest& a, const DownloadCompleteRequest& b)
{
    return memcmp(&a, &b, sizeof(a)) == 0; // POD
}
bool operator==(const DownloadFetchCompleteResponse& a, const DownloadFetchCompleteResponse& b)
{
    return memcmp(&a, &b, sizeof(a)) == 0; // POD
}
bool operator==(const DownloadFetchDataResponse& a, const DownloadFetchDataResponse& b)
{
    return a.mResourceHandle == b.mResourceHandle
        && a.mChunkID == b.mChunkID
        && a.mFragmentID == b.mFragmentID
        && a.mFragmentSize == b.mFragmentSize
        && a.mData == b.mData;
}
bool operator==(const DownloadFetchStoppedResponse& a, const DownloadFetchStoppedResponse& b)
{
    return memcmp(&a, &b, sizeof(a)) == 0; // POD
}

template<typename T>
static void TestBinaryUtil(const T& input, const char* typeName)
{
    MemoryBuffer buffer;
    {
        T copy(input);
        BinaryStream s(Stream::MEMORY, &buffer, Stream::SM_WRITE);
        TEST(s.BeginObject("Object", "Object"))
        {
            (s << StreamPropertyInfo("Property")) << copy;
            s.EndObject();
        }
    }
    gTestLog.Info(LogMessage(typeName) << " binary=" << buffer.GetSize());

    {
        T copy(input);
        BinaryStream s(Stream::MEMORY, &buffer, Stream::SM_READ);
        TEST(s.BeginObject("Object", "Object"))
        {
            (s << StreamPropertyInfo("Property")) << copy;
            s.EndObject();
        }
        TEST(copy == input);
    }
}

template<typename T>
static void TestTextUtil(const T& input, const char* typeName)
{
    String buffer;
    {
        T copy(input);
        TextStream s(Stream::TEXT, &buffer, Stream::SM_WRITE);
        TEST(s.BeginObject("Object", "Object"))
        {
            (s << StreamPropertyInfo("Property")) << copy;
            s.EndObject();
        }
    }
    gTestLog.Info(LogMessage(typeName) << " text=" << buffer.Size());

    {
        T copy(input);
        TextStream s(Stream::TEXT, &buffer, Stream::SM_READ);
        TEST(s.BeginObject("Object", "Object"))
        {
            (s << StreamPropertyInfo("Property")) << copy;
            s.EndObject();
        }
        TEST(copy == input);
    }
}

template<typename T>
static void TestJsonUtil(const T& input, const char* typeName)
{
    String buffer;
    {
        T copy(input);
        JsonStream s(Stream::TEXT, &buffer, Stream::SM_WRITE);
        TEST(s.BeginObject("Object", "Object"))
        {
            s << copy;
            s.EndObject();
        }
    }
    gTestLog.Info(LogMessage(typeName) << " json=" << buffer.Size());

    {
        T copy(input);
        JsonStream s(Stream::TEXT, &buffer, Stream::SM_READ);
        TEST(s.BeginObject("Object", "Object"))
        {
            s << copy;
            s.EndObject();
        }
        TEST(copy == input);
    }
}

template<typename T>
void TestAllUtil(const T& input, const char* typeName)
{
    TestBinaryUtil(input, typeName);
    TestTextUtil(input, typeName);
    TestJsonUtil(input, typeName);
}

REGISTER_TEST(FileServerDataTest_000, "Service.FileServer")
{
    const auto INPUT_DATA = GenerateFileServerData(TEST_MAX_DOWNLOAD_DATA_SIZE);
    const auto INPUT_HASH = ToDownloadHash(INPUT_DATA);

    TestAllUtil<DownloadHash>(INPUT_HASH, "DownloadHash");


    {

        DownloadRequest o;
        o.mRequestID = 42;
        o.mResourceIdentifier = "/example/request/identifier.png";
        o.mVersion = 72;
        TestAllUtil(o, "DownloadRequest");
    }

    {
        DownloadResponse o;
        o.mResourceHandle = 9399;
        o.mResourceSize = 188 * 1024;
        o.mHash = INPUT_HASH;
        o.mChunkCount = 6;
        o.mFragmentCount = 188;
        o.mRequestID = 72;
        TestAllUtil(o, "DownloadResponse");
    }

    {
        DownloadFetchRequest o;
        o.mResourceHandle = 9399;
        o.mChunkID = 4;
        TestAllUtil(o, "DownloadFetchRequest");
    }

    {
        DownloadFetchFragmentRequest o;
        o.mResourceHandle = 9399;
        o.mChunkID = 4;
        o.mUseRange = false;
        o.mFragmentIDs.push_back(14);
        o.mFragmentIDs.push_back(16);
        o.mFragmentIDs.push_back(24);
        o.mFragmentIDs.push_back(30);
        TestAllUtil(o, "DownloadFetchFragmentRequest");
    }

    {
        DownloadFetchStopRequest o;
        o.mResourceHandle = 9399;
        o.mChunkID = 4;
        TestAllUtil(o, "DownloadFetchStopRequest");
    }

    {
        DownloadCompleteRequest o;
        o.mResourceHandle = 9399;
        TestAllUtil(o, "DownloadCompleteRequest");
    }

    {
        DownloadFetchCompleteResponse o;
        o.mResourceHandle = 9399;
        o.mChunkID = 4;
        TestAllUtil(o, "DownloadFetchCompleteResponse");
    }

    {
        DownloadFetchDataResponse o;
        o.mResourceHandle = 9399;
        o.mChunkID = 4;
        o.mFragmentID = 18;
        o.mData = INPUT_DATA;
        o.mFragmentSize = static_cast<UInt32>(o.mData.size());
        TestAllUtil(o, "DownloadFetchDataResponse");
    }

    {
        DownloadFetchStoppedResponse o;
        o.mResourceHandle = 9399;
        o.mChunkID = 4;
        o.mReason = DownloadFetchStopReason::DFSR_RESOURCE_CORRUPT;
        TestAllUtil(o, "DownloadFetchStoppedResponse");
    }

}

REGISTER_TEST(FileServerDataTest_001, "Service.FileServer")
{
    const SizeT CHUNK_SIZE = FILE_SERVER_MAX_FRAGMENT_SIZE * FILE_SERVER_MAX_FRAGMENTS_IN_CHUNK;

    Int32 seed = 0xCA557EFF;
    const auto INPUT_DATA_A = GenerateFileServerData(CHUNK_SIZE * 4, &seed);
    const auto INPUT_DATA_B= GenerateFileServerData(CHUNK_SIZE * 8, &seed);
    const auto INPUT_DATA_C = GenerateFileServerData(CHUNK_SIZE * 16, &seed);
    const auto INPUT_HASH_A = ToDownloadHash(INPUT_DATA_A);
    const auto INPUT_HASH_B = ToDownloadHash(INPUT_DATA_B);
    const auto INPUT_HASH_C = ToDownloadHash(INPUT_DATA_C);

    MemoryResourceLocator locator;
    locator.WriteResource("DataA", INPUT_DATA_A, DateTime("01/02/1994"));
    locator.WriteResource("DataB", INPUT_DATA_B, DateTime("01/02/1996"));
    locator.WriteResource("DataC", INPUT_DATA_C, DateTime("01/02/1998"));

    FileResourceLocator* l = &locator;
    FileResourceInfo info;
    TEST(l->QueryResourceInfo("DataA", info));
    TEST(info.mName == "DataA");
    TEST(info.mLastModifyTime == DateTime("01/02/1994"));
    TEST(info.mSize == INPUT_DATA_A.size());
    TEST(info.mChunkCount == 4);
    TEST(info.mFragmentCount == 4 * FILE_SERVER_MAX_FRAGMENTS_IN_CHUNK);
    TEST(memcmp(INPUT_HASH_A.mBytes, info.mHash, sizeof(info.mHash)) == 0);

    FileResourceChunk chunk;
    TEST(!l->QueryChunk(info, info.mChunkCount, chunk));
    TEST(chunk.mData.empty());

    for (SizeT i = 0; i < info.mChunkCount; ++i)
    {
        TEST(l->QueryChunk(info, i, chunk));
        TEST(!chunk.mData.empty());

        DownloadHash hash;
        TEST(chunk.ComputeHash(hash.mBytes, sizeof(hash.mBytes)));
        TEST(chunk.GetFragmentCount() == FILE_SERVER_MAX_FRAGMENTS_IN_CHUNK);

        ByteT fragmentBytes[FILE_SERVER_MAX_FRAGMENT_SIZE] = { 0 };
        for (SizeT k = 0; k < FILE_SERVER_MAX_FRAGMENTS_IN_CHUNK; ++k)
        {
            TEST(chunk.CopyFragment(k, fragmentBytes, sizeof(fragmentBytes)) == sizeof(fragmentBytes));
        }
    }
}

} // namespace lf