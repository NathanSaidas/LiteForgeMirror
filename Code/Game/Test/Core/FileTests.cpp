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
#include "Core/Test/Test.h"
#include "Core/Platform/File.h"
#include "Core/Platform/AsyncIOBuffer.h"
#include "Core/Platform/AsyncIODevice.h"
#include "Core/Platform/FileSystem.h"
#include "Core/String/String.h"
#include "Core/IO/EngineConfig.h"

namespace lf {

static void TestFileOpen(const String& FILENAME, FileFlagsT flags, bool checkRead, bool checkWrite)
{
    if (FileSystem::FileExists(FILENAME))
    {
        TEST(FileSystem::FileDelete(FILENAME));
        TEST(!FileSystem::FileExists(FILENAME));
    }

    {
        File file;
        TEST(!file.Open(FILENAME, flags, FILE_OPEN_EXISTING));
        TEST(!file.IsOpen());
    }

    {
        File file;
        TEST(file.Open(FILENAME, flags, FILE_OPEN_NEW));
        TEST(file.IsOpen());
        TEST(!file.IsAsync());
        TEST(file.IsReading() == checkRead);
        TEST(file.IsWriting() == checkWrite);
        TEST(!FileSystem::FileDelete(FILENAME));
        file.Close();
        TEST(FileSystem::FileDelete(FILENAME));
        TEST(!FileSystem::FileExists(FILENAME));
    }

    {
        File file;
        TEST(file.Open(FILENAME, flags, FILE_OPEN_ALWAYS));
        TEST(file.IsOpen());
        TEST(!file.IsAsync());
        TEST(file.IsReading() == checkRead);
        TEST(file.IsWriting() == checkWrite);
    }

    {
        File file;
        TEST(file.Open(FILENAME, flags, FILE_OPEN_ALWAYS));
        TEST(file.IsOpen());
        TEST(!file.IsAsync());
        TEST(file.IsReading() == checkRead);
        TEST(file.IsWriting() == checkWrite);
    }

    {
        File file;
        TEST(file.Open(FILENAME, flags, FILE_OPEN_EXISTING));
        TEST(file.IsOpen());
        TEST(!file.IsAsync());
        TEST(file.IsReading() == checkRead);
        TEST(file.IsWriting() == checkWrite);

    }

    {
        File file;
        TEST(!file.Open(FILENAME, flags, FILE_OPEN_NEW));
        TEST(!file.IsOpen());
        TEST(FileSystem::FileExists(FILENAME));
    }
}

static void TestFileOpenAsync(const String& FILENAME, FileFlagsT flags, bool checkRead, bool checkWrite)
{
    AsyncIODevice ioDevice;
    TEST(ioDevice.Create());

    if (FileSystem::FileExists(FILENAME))
    {
        TEST(FileSystem::FileDelete(FILENAME));
        TEST(!FileSystem::FileExists(FILENAME));
    }

    {
        File file;
        TEST(!file.OpenAsync(FILENAME, flags, FILE_OPEN_EXISTING, ioDevice));
        TEST(!file.IsOpen());
    }

    {
        File file;
        TEST(file.OpenAsync(FILENAME, flags, FILE_OPEN_NEW, ioDevice));
        TEST(file.IsOpen());
        TEST(file.IsAsync());
        TEST(file.IsReading() == checkRead);
        TEST(file.IsWriting() == checkWrite);
        TEST(!FileSystem::FileDelete(FILENAME));
        file.Close();
        TEST(FileSystem::FileDelete(FILENAME));
        TEST(!FileSystem::FileExists(FILENAME));
    }

    {
        File file;
        TEST(file.OpenAsync(FILENAME, flags, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.IsOpen());
        TEST(file.IsAsync());
        TEST(file.IsReading() == checkRead);
        TEST(file.IsWriting() == checkWrite);
    }

    {
        File file;
        TEST(file.OpenAsync(FILENAME, flags, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.IsOpen());
        TEST(file.IsAsync());
        TEST(file.IsReading() == checkRead);
        TEST(file.IsWriting() == checkWrite);
    }

    {
        File file;
        TEST(file.OpenAsync(FILENAME, flags, FILE_OPEN_EXISTING, ioDevice));
        TEST(file.IsOpen());
        TEST(file.IsAsync());
        TEST(file.IsReading() == checkRead);
        TEST(file.IsWriting() == checkWrite);

    }

    {
        File file;
        TEST(!file.OpenAsync(FILENAME, flags, FILE_OPEN_NEW, ioDevice));
        TEST(!file.IsOpen());
        TEST(FileSystem::FileExists(FILENAME));
    }
}

static void TestFileShareOpen(const String& FILENAME)
{
    TEST(FileSystem::FileExists(FILENAME) || FileSystem::FileCreate(FILENAME));

    {
        File fileA;
        File fileB;

        TEST(fileA.Open(FILENAME, FF_READ, FILE_OPEN_EXISTING));
        TEST(fileA.IsOpen());
        TEST(!fileA.IsAsync());
        
        TEST(!fileB.Open(FILENAME, FF_READ, FILE_OPEN_EXISTING));
        TEST(!fileB.IsOpen());
        TEST(!fileB.IsAsync());
    }

    {
        File fileA;
        File fileB;

        TEST(fileA.Open(FILENAME, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING));
        TEST(fileA.IsOpen());
        TEST(!fileA.IsAsync());

        TEST(!fileB.Open(FILENAME, FF_READ, FILE_OPEN_EXISTING));
        TEST(!fileB.IsOpen());
        TEST(!fileB.IsAsync());
    }

    // Multiple readers are allowed
    {
        File fileA;
        File fileB;

        TEST(fileA.Open(FILENAME, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING));
        TEST(fileA.IsOpen());
        TEST(!fileA.IsAsync());

        TEST(fileB.Open(FILENAME, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING));
        TEST(fileB.IsOpen());
        TEST(!fileB.IsAsync());
    }

    // Multiple writers are allowed
    {
        File fileA;
        File fileB;

        TEST(fileA.Open(FILENAME, FF_WRITE | FF_SHARE_WRITE, FILE_OPEN_EXISTING));
        TEST(fileA.IsOpen());
        TEST(!fileA.IsAsync());

        TEST(fileB.Open(FILENAME, FF_WRITE | FF_SHARE_WRITE, FILE_OPEN_EXISTING));
        TEST(fileB.IsOpen());
        TEST(!fileB.IsAsync());
    }


    {
        File fileA;
        File fileB;

        TEST(fileA.Open(FILENAME, FF_WRITE | FF_SHARE_WRITE, FILE_OPEN_EXISTING));
        TEST(fileA.IsOpen());
        TEST(!fileA.IsAsync());

        TEST(fileB.Open(FILENAME, FF_WRITE | FF_SHARE_WRITE, FILE_OPEN_EXISTING));
        TEST(fileB.IsOpen());
        TEST(!fileB.IsAsync());
    }
}

static void SimpleSynchronousTests()
{
    const String TEST_DIR(TestFramework::GetConfig().mEngineConfig->GetTempDirectory());

    const String TEST_FILENAME = TEST_DIR + "TestSyncFile.txt";
    TestFileOpen(TEST_FILENAME, FF_READ, true, false);
    TestFileOpen(TEST_FILENAME, FF_WRITE, false, true);
    TestFileOpen(TEST_FILENAME, FF_READ | FF_WRITE, true, true);
    TestFileShareOpen(TEST_FILENAME);

}

static void SimpleAsyncTests()
{
    const String TEST_DIR(TestFramework::GetConfig().mEngineConfig->GetTempDirectory());

    const String TEST_FILENAME = TEST_DIR + "TestASyncFile.txt";
    TestFileOpenAsync(TEST_FILENAME, FF_READ, true, false);
    TestFileOpenAsync(TEST_FILENAME, FF_WRITE, false, true);
    TestFileOpenAsync(TEST_FILENAME, FF_READ | FF_WRITE, true, true);
}

static void TestReadWrite()
{
    const String TEST_DIR(TestFramework::GetConfig().mEngineConfig->GetTempDirectory());
    const String TEST_FILENAME = TEST_DIR + "TestASyncFileWrite.txt";
    const String TEST_TEXT = "Hello Test Text.\r\nThis is a new line.\r\n";

    {
        File file;
        TEST(file.Open(TEST_FILENAME, FF_WRITE, FILE_OPEN_ALWAYS));
        SizeT bytesWritten = file.Write(TEST_TEXT.CStr(), TEST_TEXT.Size());
        TEST(bytesWritten == TEST_TEXT.Size());
    }

    {
        File file;
        TEST(file.Open(TEST_FILENAME, FF_READ, FILE_OPEN_ALWAYS));
        SizeT bytesWritten = file.Write(TEST_TEXT.CStr(), TEST_TEXT.Size());
        TEST(bytesWritten == 0);
    }

    {
        File file;
        TEST(file.Open(TEST_FILENAME, FF_WRITE, FILE_OPEN_ALWAYS));
        SizeT bytesRead = file.Read(const_cast<char*>(TEST_TEXT.CStr()), TEST_TEXT.Size());
        TEST(bytesRead == 0);
    }

    {
        File file;
        TEST(file.Open(TEST_FILENAME, FF_READ, FILE_OPEN_ALWAYS));
        String buffer;
        buffer.Resize(TEST_TEXT.Size());
        SizeT bytesRead = file.Read(const_cast<char*>(buffer.CStr()), buffer.Size());
        TEST(bytesRead == TEST_TEXT.Size());
        TEST(buffer == TEST_TEXT);

        char c[1] = { 0 };
        bytesRead = file.Read(c, 1);
        TEST(bytesRead == 0);
        TEST(file.IsEof());
    }
}

static void TestReadWriteAsync()
{
    const String TEST_DIR(TestFramework::GetConfig().mEngineConfig->GetTempDirectory());
    const String TEST_FILENAME = TEST_DIR + "TestASyncFileWrite.txt";
    const String TEST_TEXT = "Hello Test Text.\r\nThis is a new line.\r\n";

    AsyncIODevice ioDevice;
    TEST(ioDevice.Create());

    {
        File file;
        TEST(file.OpenAsync(TEST_FILENAME, FF_WRITE, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.IsAsync());
        SizeT bytesWritten = file.Write(TEST_TEXT.CStr(), TEST_TEXT.Size());
        TEST(bytesWritten == TEST_TEXT.Size());
    }

    {
        File file;
        TEST(file.OpenAsync(TEST_FILENAME, FF_READ, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.IsAsync());
        SizeT bytesWritten = file.Write(TEST_TEXT.CStr(), TEST_TEXT.Size());
        TEST(bytesWritten == 0);
    }

    {
        File file;
        TEST(file.OpenAsync(TEST_FILENAME, FF_WRITE, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.IsAsync());
        SizeT bytesRead = file.Read(const_cast<char*>(TEST_TEXT.CStr()), TEST_TEXT.Size());
        TEST(bytesRead == 0);
    }

    {
        File file;
        TEST(file.OpenAsync(TEST_FILENAME, FF_READ, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.IsAsync());
        String buffer;
        buffer.Resize(TEST_TEXT.Size());
        SizeT bytesRead = file.Read(const_cast<char*>(buffer.CStr()), buffer.Size());
        TEST(bytesRead == TEST_TEXT.Size());
        TEST(buffer == TEST_TEXT);

        char c[1] = { 0 };
        bytesRead = file.Read(c, 1);
        TEST(bytesRead == 0);
        TEST(file.IsEof());
    }
}

static void TestFileCursor()
{
    const String TEST_DIR(TestFramework::GetConfig().mEngineConfig->GetTempDirectory());
    const String TEST_FILENAME = TEST_DIR + "TestASyncFileWrite.txt";
    const String TEST_MSG = "0123456789";


    if (FileSystem::FileExists(TEST_FILENAME))
    {
        TEST(FileSystem::FileDelete(TEST_FILENAME));
        TEST(FileSystem::FileReserve(TEST_FILENAME, 10));
    }

    {

        File file;
        TEST(file.Open(TEST_FILENAME, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS));
        TEST(file.GetCursor() == 0);
        TEST(file.SetCursor(0, FILE_CURSOR_END));
        FileCursor cursor = file.GetCursor();
        TEST(file.Write(TEST_MSG.CStr(), TEST_MSG.Size()) == TEST_MSG.Size());
        TEST(file.GetCursor() - cursor == TEST_MSG.Size());
        file.Close();
        TEST(!file.IsOpen());

        TEST(file.Open(TEST_FILENAME, FF_READ, FILE_OPEN_EXISTING));
        TEST(file.GetCursor() == 0);
        TEST(file.SetCursor(-10, FILE_CURSOR_END));

        char buffer[10];
        cursor = file.GetCursor();
        TEST(file.Read(buffer, sizeof(buffer)) == sizeof(buffer));
        TEST(file.GetCursor() - cursor == sizeof(buffer));
        String text(sizeof(buffer), buffer);
        TEST(text == TEST_MSG);
    }

    {
        File file;
        TEST(file.Open(TEST_FILENAME, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS));
        TEST(file.GetCursor() == 0);
        TEST(file.SetCursor(10, FILE_CURSOR_CURRENT));

        char empty[10];
        memset(empty, '-', sizeof(empty));
        FileCursor cursor = file.GetCursor();
        TEST(file.Write(empty, sizeof(empty)) == sizeof(empty));
        TEST(file.GetCursor() - cursor == sizeof(empty));
        char buffer[10];
        TEST(file.SetCursor(- FileCursor(sizeof(empty)), FILE_CURSOR_CURRENT));
        cursor = file.GetCursor();
        TEST(file.Read(buffer, sizeof(buffer)) == sizeof(buffer));
        TEST(file.GetCursor() - cursor == sizeof(buffer));
        TEST(memcmp(empty, buffer, sizeof(buffer)) == 0);
    }
}

static void TestFileCursorAsync()
{
    const String TEST_DIR(TestFramework::GetConfig().mEngineConfig->GetTempDirectory());
    const String TEST_FILENAME = TEST_DIR + "TestASyncFileWrite.txt";
    const String TEST_MSG = "0123456789";

    AsyncIODevice ioDevice;
    TEST(ioDevice.Create());


    if (FileSystem::FileExists(TEST_FILENAME))
    {
        TEST(FileSystem::FileDelete(TEST_FILENAME));
        TEST(FileSystem::FileReserve(TEST_FILENAME, 10));
    }

    {

        File file;
        TEST(file.OpenAsync(TEST_FILENAME, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.GetCursor() == 0);
        TEST(file.SetCursor(0, FILE_CURSOR_END));
        FileCursor cursor = file.GetCursor();
        TEST(file.Write(TEST_MSG.CStr(), TEST_MSG.Size()) == TEST_MSG.Size());
        TEST(file.GetCursor() - cursor == TEST_MSG.Size());
        file.Close();
        TEST(!file.IsOpen());

        TEST(file.OpenAsync(TEST_FILENAME, FF_READ, FILE_OPEN_EXISTING, ioDevice));
        TEST(file.GetCursor() == 0);
        TEST(file.SetCursor(-10, FILE_CURSOR_END));

        char buffer[10];
        cursor = file.GetCursor();
        TEST(file.Read(buffer, sizeof(buffer)) == sizeof(buffer));
        TEST(file.GetCursor() - cursor == sizeof(buffer));
        String text(sizeof(buffer), buffer);
        TEST(text == TEST_MSG);
    }

    {
        File file;
        TEST(file.OpenAsync(TEST_FILENAME, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.GetCursor() == 0);
        TEST(file.SetCursor(10, FILE_CURSOR_CURRENT));

        char empty[10];
        memset(empty, '-', sizeof(empty));
        FileCursor cursor = file.GetCursor();
        TEST(file.Write(empty, sizeof(empty)) == sizeof(empty));
        TEST(file.GetCursor() - cursor == sizeof(empty));
        char buffer[10];
        TEST(file.SetCursor(-FileCursor(sizeof(empty)), FILE_CURSOR_CURRENT));
        cursor = file.GetCursor();
        TEST(file.Read(buffer, sizeof(buffer)) == sizeof(buffer));
        TEST(file.GetCursor() - cursor == sizeof(buffer));
        TEST(memcmp(empty, buffer, sizeof(buffer)) == 0);
    }

    {
        File file;
        TEST(file.OpenAsync(TEST_FILENAME, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS, ioDevice));
        TEST(file.GetCursor() == 0);

        char buffer[10];
        FileCursor cursor = file.GetCursor();
        AsyncIOBuffer ioBuffer(buffer);
        TEST(file.ReadAsync(&ioBuffer, sizeof(buffer)));
        TEST(!ioBuffer.IsDone());
        file.Wait();
        TEST(ioBuffer.IsDone());
        TEST(ioBuffer.GetBytesTransferred() == sizeof(buffer));
        TEST(file.GetCursor() - cursor == sizeof(buffer));

        file.SetCursor(0, FILE_CURSOR_BEGIN);
        ioBuffer.SetBuffer(const_cast<char*>(TEST_MSG.CStr()));
        ioBuffer.SetState(ASYNC_IO_IDLE);
        cursor = file.GetCursor();
        TEST(file.WriteAsync(&ioBuffer, TEST_MSG.Size()));
        TEST(!ioBuffer.IsDone());
        file.Wait();
        TEST(ioBuffer.IsDone());
        TEST(ioBuffer.GetBytesTransferred() == TEST_MSG.Size());
        TEST(file.GetCursor() - cursor == sizeof(buffer));
    }
}

void TestPath()
{
    TEST(FileSystem::PathJoin("D:\\House\\Food\\", "Dinner") == "D:\\House\\Food\\Dinner\\");
    TEST(FileSystem::PathJoin("D:\\House\\Food", "Dinner") == "D:\\House\\Food\\Dinner\\");
    TEST(FileSystem::PathJoin("D:\\House\\Food", "\\Dinner") == "D:\\House\\Food\\Dinner\\");
    TEST(FileSystem::PathJoin("D:\\House\\Food\\", "\\Dinner") == "D:\\House\\Food\\Dinner\\");
    TEST(FileSystem::PathJoin("D:\\House\\Food\\Dinner\\", "Chili.txt") == "D:\\House\\Food\\Dinner\\Chili.txt");
    TEST(FileSystem::PathJoin("House\\Food", "Dinner") == "House\\Food\\Dinner\\");
    TEST(FileSystem::PathJoin("House", "") == "House\\");

    TEST(FileSystem::PathCorrectPath("D:/House/Food") == "D:\\House\\Food\\");
    TEST(FileSystem::PathGetParent("D:\\House\\Food\\Dinner") == "D:\\House\\Food\\");

    TEST(FileSystem::PathCreate(FileSystem::PathResolve("../Temp/Logs")));
}

REGISTER_TEST(FileTest)
{
    TestPath();
    SimpleSynchronousTests();
    SimpleAsyncTests();
    if (TestFramework::HasFailed())
    {
        return;
    }
    TestReadWrite();
    TestReadWriteAsync();
    TestFileCursor();
    TestFileCursorAsync();
}

} // namespace lf