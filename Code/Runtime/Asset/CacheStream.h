#ifndef LF_RUNTIME_CACHE_STREAM_H
#define LF_RUNTIME_CACHE_STREAM_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

#include "Core/IO/Stream.h"
#include "Runtime/Asset/CacheTypes.h"

namespace lf {

class LF_RUNTIME_API CacheStream
{
public:
    using StreamMode = Stream::StreamMode;

    void Open(StreamMode mode, const String& cacheFilename, const String& manifestFilename, SizeT fileSize);
    void Close();


    bool IsOpen() const { return mMode != Stream::SM_CLOSED; }

private:
    String mCacheFilename;
    String mManifestFilename;
    StreamMode mMode;
    SizeT mFileSize;
    CacheFile* mCacheFile;

};

}

#endif // LF_RUNTIME_CACHE_STREAM_H