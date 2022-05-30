#pragma once
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
#include "Runtime/Net/FileTransfer/FileResourceLocator.h"
#include "Runtime/Net/FileTransfer/FileTransferConstants.h"
#include "Runtime/Net/FileTransfer/FileResourceTypes.h"


#include "Core/Memory/SmartPointer.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/String/String.h"
#include "Core/Utility/Array.h" // tmp
#include "Core/Utility/DateTime.h"
#include "Core/Utility/StdMap.h"
namespace lf {

class LF_RUNTIME_API MemoryResourceLocator : public FileResourceLocator
{
public:
    MemoryResourceLocator();
    ~MemoryResourceLocator();

    bool WriteResource(const String& name, const TVector<ByteT>& resource, const DateTime& lastModified);
    bool DeleteResource(const String& name);

    bool QueryResourceInfo(const String& resourceName, FileResourceInfo& info) const override;
    bool QueryChunk(const FileResourceInfo& resourceInfo, SizeT chunkID, FileResourceChunk& chunk) const override;
private:
    struct Resource // tmp
    {
        DateTime      mLastModifyTime;
        ByteT         mHash[FILE_SERVER_HASH_SIZE];
        TVector<ByteT> mData;
    };
    using ResourcePtr = TStrongPointer<Resource>;
    using ResourceMap = TMap<String, ResourcePtr>;

    mutable RWSpinLock mResourceLock;
    ResourceMap mResources;
};

}