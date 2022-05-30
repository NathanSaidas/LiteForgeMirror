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
#include "Core/Common/API.h"
#include "Core/Common/Enum.h"
#include "Core/Common/Types.h"
#include "Core/Utility/Bitfield.h"
#include "Core/Utility/APIResult.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Concurrent/Task.h"
#include "Core/Reflection/DynamicCast.h"
#include "Runtime/Service/Service.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "AbstractEngine/Gfx/GfxTaskScheduler.h"

namespace lf {

DECLARE_ENUM(GfxDeviceFlags,
    GDF_DEBUG,
    GDF_HEADLESS,
    GDF_SINGLETHREADED,
    GDF_WORKERTHREADED
);
using GfxDeviceFlagsBitfield = Bitfield<GfxDeviceFlags::Value>;

DECLARE_ATOMIC_PTR(AppWindow);
DECLARE_ATOMIC_PTR(GfxResourceObject);
DECLARE_ATOMIC_PTR(GfxSwapChain);
DECLARE_ATOMIC_PTR(GfxFence);
DECLARE_ATOMIC_PTR(GfxUploadBuffer);
DECLARE_ASSET(GfxShader);
class Color;
class Type;

class LF_ABSTRACT_ENGINE_API GfxDevice : public Service
{
    DECLARE_CLASS(GfxDevice, Service);
public:
    virtual ~GfxDevice() {}
    // ** 
    virtual GfxSwapChainAtomicPtr CreateSwapChain(const AppWindowAtomicPtr& window) = 0;

    virtual GfxFenceAtomicPtr CreateFence() = 0;
    virtual GfxUploadBufferAtomicPtr CreateConstantBuffer(SizeT elementSize) = 0;
    virtual void ReleaseConstantBuffer(const GfxUploadBufferAtomicPtr& buffer) = 0;
    virtual GfxUploadBufferAtomicPtr CreateStructureBuffer(SizeT elementSize) = 0;
    virtual void ReleaseStructureBuffer(const GfxUploadBufferAtomicPtr& buffer) = 0;
    template<typename ResourceTypeT>
    TAtomicStrongPointer<ResourceTypeT> CreateResource()
    {
        LF_STATIC_IS_A(ResourceTypeT, GfxResourceObject);
        return DynamicCast<TAtomicStrongPointer<ResourceTypeT>>(CreateResourceObject(typeof(ResourceTypeT)));
    }

    virtual Gfx::FrameCountType GetCurrentFrame() const = 0;
    virtual Gfx::FrameCountType GetLastCompletedFrame() const = 0;
protected:
    virtual TAtomicStrongPointer<GfxResourceObject> CreateResourceObject(const Type* type) = 0;
    


};

} // namespace lf 