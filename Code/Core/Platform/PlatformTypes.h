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
#pragma once

#include "Core/Common/Types.h"

namespace lf {

using FileSize = long long;
using FileCursor = long;
using FileFlagsT = Int16;

enum FileFlags
{
    FF_READ             = 1 << 0,
    FF_WRITE            = 1 << 1,
    FF_SHARE_READ       = 1 << 2,      // Multiple users can read
    FF_SHARE_WRITE      = 1 << 3,      // Multiple writers
    FF_EOF              = 1 << 4,
    FF_OUT_OF_MEMORY    = 1 << 5,
    FF_NO_BUFFERING     = 1 << 6,
    FF_RANDOM_ACCESS    = 1 << 7
};

enum FileOpenMode
{
    FILE_OPEN_EXISTING,   // Open file only if it exists
    FILE_OPEN_NEW,        // Open file only if it does not exist
    FILE_OPEN_CREATE_NEW, // Open new file overwritting any file that was there
    FILE_OPEN_ALWAYS      // Open file regardless of its existence.
};

enum FileCursorMode
{
    FILE_CURSOR_BEGIN,
    FILE_CURSOR_END,
    FILE_CURSOR_CURRENT
};

}