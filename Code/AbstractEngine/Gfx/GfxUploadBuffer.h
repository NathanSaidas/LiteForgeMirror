// ********************************************************************
// Copyright (c) 2022 Nathan Hanlan
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
#include "AbstractEngine/Gfx/GfxResourceObject.h"
#include "AbstractEngine/Gfx/GfxTypes.h"

namespace lf {



class LF_ABSTRACT_ENGINE_API GfxUploadBuffer : public GfxResourceObject
{
    DECLARE_CLASS(GfxUploadBuffer, GfxResourceObject);
public:
    GfxUploadBuffer();
    ~GfxUploadBuffer() override;

    void SetElementCount(SizeT value) { mElementCount = value; }
    void SetElementSize(SizeT value) { mElementSize = value; }
    void SetUploadBufferType(Gfx::UploadBufferType value) { mUploadBufferType = value; }

    SizeT GetElementCount() const { return mElementCount; }
    SizeT GetElementSize() const { return mElementSize; }
    bool IsConstantBuffer() const { return mUploadBufferType == Gfx::UploadBufferType::Constant; }
    bool IsStructuredBuffer() const { return mUploadBufferType == Gfx::UploadBufferType::Structured; }
    Gfx::UploadBufferType GetUploadBufferType() const { return mUploadBufferType; }
    bool IsMapped() const { return const_cast<GfxUploadBuffer*>(this)->GetMappedData() != nullptr; }
    virtual Gfx::FrameCountType GetLastBoundFrame() const = 0;

    void CopyData(const SizeT index, const ByteT* data, SizeT dataByteCount)
    {
        ReportBug(IsMapped());
        ReportBug(dataByteCount == GetElementSize());
        if (!IsMapped() || dataByteCount != GetElementSize())
        {
            return;
        }

        memcpy(&GetMappedData()[index * GetElementByteSize()], data, dataByteCount);
    }

    void CopyData(const ByteT* data, SizeT dataByteCount)
    {
        CopyData(0, data, dataByteCount);
    }

    template<typename T>
    void CopyStruct(SizeT index, const T& object)
    {
        ReportBug(IsMapped());
        ReportBug(sizeof(T) == GetElementSize());
        if (!IsMapped() || sizeof(T) != GetElementSize())
        {
            return;
        }

        memcpy(&GetMappedData()[index * GetElementByteSize()], &object, sizeof(T));
    }

    template<typename T>
    void CopyStruct(const T& object)
    {
        CopyStruct(0, object);
    }

    template<typename T, SizeT ObjectCount>
    void Copy(const T(&object)[ObjectCount])
    {
        ReportBug(IsMapped());
        ReportBug(sizeof(T) == GetElementSize() && ObjectCount == GetElementCount());
        if (!IsMapped() || sizeof(T) != GetElementSize() || ObjectCount != GetElementCount())
        {
            return;
        }

        memcpy(GetMappedData(), object, GetElementByteSize() * GetElementCount());
    }

protected:
    virtual ByteT* GetMappedData() = 0;
    virtual SizeT  GetElementByteSize() const = 0;
private:
    SizeT mElementCount;
    SizeT mElementSize;
    Gfx::UploadBufferType mUploadBufferType;
};

} // namespace lf
#pragma once
