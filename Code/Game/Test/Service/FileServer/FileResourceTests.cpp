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
#include "Runtime/Net/FileTransfer/FileTransferConstants.h"

namespace lf {

REGISTER_TEST(FileServerConstantsTest_000, "Service.FileServer")
{
    SizeT fileSize = 220 * 1024; // 220kb
    TEST(FileResourceUtil::FileSizeToFragmentCount(fileSize) == 188);
    TEST(FileResourceUtil::FileSizeToChunkCount(fileSize) == 6);
    TEST(FileResourceUtil::FragmentCountToChunkCount(188) == 6);

    fileSize = 6 * 1024 * 1024; // 6mb
    TEST(FileResourceUtil::FileSizeToFragmentCount(fileSize) == 5243);
    TEST(FileResourceUtil::FragmentCountToChunkCount(5243) == 140);
    TEST(FileResourceUtil::FileSizeToChunkCount(fileSize) == 140);

    fileSize = 104 * 1024 * 1024; // 104mb
    TEST(FileResourceUtil::FileSizeToFragmentCount(fileSize) == 90877);
    TEST(FileResourceUtil::FileSizeToChunkCount(fileSize) == 2424);
    TEST(FileResourceUtil::FragmentCountToChunkCount(90877) == 2424);
}

} // namespace lf