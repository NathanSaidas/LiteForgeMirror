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
#include "AbstractEngine/PCH.h"
#include "GfxIndexBuffer.h"

namespace lf {
DEFINE_ABSTRACT_CLASS(lf::GfxIndexBuffer) { NO_REFLECTION; }
GfxIndexBuffer::GfxIndexBuffer()
: Super()
, mGPUReady(0)
, mUsage(Gfx::BufferUsage::INVALID_ENUM)
, mStride(0)
, mNumElements(0)
{}
GfxIndexBuffer::~GfxIndexBuffer()
{}

void GfxIndexBuffer::SetUsage(Gfx::BufferUsage value)
{
    if (ValidEnum(GetUsage()))
    {
        ReportBugMsg("GfxIndexBuffer::SetUsage failed, cannot set a buffer usage once initialized.");
        return;
    }
    mUsage = value;
}

} // namespace lf