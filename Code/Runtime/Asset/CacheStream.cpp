#include "CacheStream.h"

#if defined(LF_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace lf {

struct CacheFile
{
    HANDLE mCacheFile;
    HANDLE mManifestFile;
};

void CacheStream::Open(StreamMode mode, const String& cacheFilename, const String& manifestFilename, SizeT fileSize)
{
    mMode = mode;
    mCacheFilename = cacheFilename;
    mManifestFilename = manifestFilename;
    mFileSize = fileSize;
    
    // ManifestFile:
    // Object ID: (2 bytes) 
    // Object Location (4 bytes)
    // Object Size     (4 bytes)
    // Object Capacity (4 bytes)
}
void CacheStream::Close()
{

}


} // namespace lf