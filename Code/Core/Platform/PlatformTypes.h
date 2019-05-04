#ifndef LF_CORE_PLATFORM_TYPES_H
#define LF_CORE_PLATFORM_TYPES_H

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
    FF_OUT_OF_MEMORY    = 1 << 5
};

enum FileOpenMode
{
    FILE_OPEN_EXISTING, // Open file only if it exists
    FILE_OPEN_NEW,      // Open file only if it does not exist
    FILE_OPEN_ALWAYS    // Open file regardless of its existence.
};

enum FileCursorMode
{
    FILE_CURSOR_BEGIN,
    FILE_CURSOR_END,
    FILE_CURSOR_CURRENT
};

}

#endif // LF_CORE_PLATFORM_TYPES_H