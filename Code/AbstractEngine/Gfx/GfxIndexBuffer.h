// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Utility/APIResult.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "AbstractEngine/Gfx/GfxResourceObject.h"

namespace lf {
// Usage:
//  GfxIndexBuffer buffer;
//  buffer.SetUsage(...)
//  buffer.SetIndices(...)
//  buffer.Commit()
class LF_ABSTRACT_ENGINE_API GfxIndexBuffer : public GfxResourceObject
{
    DECLARE_CLASS(GfxIndexBuffer, GfxResourceObject);
public:
    GfxIndexBuffer();
    ~GfxIndexBuffer() override;

    void SetUsage(Gfx::BufferUsage value);
    Gfx::BufferUsage GetUsage() const { return mUsage; }
    SizeT GetStride() const { return mStride; }
    SizeT GetNumElements() const { return mNumElements; }

    virtual APIResult<bool> SetIndices(const MemoryBuffer& indices, SizeT stride, SizeT numElements) = 0;
    virtual APIResult<bool> SetIndices(MemoryBuffer&& indices, SizeT stride, SizeT numElements) = 0;

    LF_INLINE APIResult<bool> SetIndices(const TVector<UInt16>& indices)
    {
        const SizeT stride = sizeof(UInt16);
        MemoryBuffer buffer(indices.data(), indices.size() * stride, MemoryBuffer::STATIC);
        buffer.SetSize(indices.size() * stride);
        return SetIndices(buffer, stride, indices.size());
    }

    LF_INLINE APIResult<bool> SetIndices(const TVector<UInt32>& indices)
    {
        const SizeT stride = sizeof(UInt32);
        MemoryBuffer buffer(indices.data(), indices.size() * stride, MemoryBuffer::STATIC);
        buffer.SetSize(indices.size() * stride);
        return SetIndices(buffer, stride, indices.size());
    }

    bool IsGPUReady() const { return AtomicLoad(&mGPUReady) != 0; }
protected:
    void SetStride(SizeT value) { mStride = value; }
    void SetNumElements(SizeT value) { mNumElements = value; }
    void SetGPUReady(bool value) { AtomicStore(&mGPUReady, value ? 1 : 0); }

private:
    volatile Atomic32 mGPUReady;
    Gfx::BufferUsage mUsage;
    SizeT mStride;
    SizeT mNumElements;
};

} // namespace lf
