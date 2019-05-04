#ifndef LF_CORE_MEMORY_BUFFER_H
#define LF_CORE_MEMORY_BUFFER_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

class MemoryBuffer
{
public:
    MemoryBuffer(const MemoryBuffer&) = delete;
    MemoryBuffer& operator=(const MemoryBuffer&) = delete;

    MemoryBuffer();
    MemoryBuffer(MemoryBuffer&& other);
    ~MemoryBuffer();

    void Swap(MemoryBuffer& other);

    void Allocate(SizeT bytes, SizeT alignment);
    void Reallocate(SizeT bytes, SizeT alignment);
    void Free();

    void SetSize(SizeT size);
    SizeT GetSize() const { return mSize; }
    SizeT GetCapacity() const { return mCapacity; }
    void* GetData() { return mData; }
    const void* GetData() const { return mData; }
private:
    void* mData;
    SizeT mSize;
    SizeT mCapacity;
};
} // namespace lf

#endif // LF_CORE_MEMORY_BUFFER_H