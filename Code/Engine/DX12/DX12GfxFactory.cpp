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
#include "Engine/PCH.h"
#include "DX12GfxFactory.h"

#include "Engine/DX12/DX12GfxPipelineState.h"
#include "Engine/DX12/DX12GfxVertexBuffer.h"
#include "Engine/DX12/DX12GfxIndexBuffer.h"
#include "Engine/DX12/DX12GfxUploadBuffer.h"
#include "Engine/DX12/DX12GfxTexture.h"
#include "Engine/DX12/DX12GfxRenderTexture.h"
#include "Engine/DX12/DX12GfxSwapChain.h"
#include "Engine/DX12/DX12GfxCommandContext.h"
#include "Engine/DX12/DX12GfxCommandQueue.h"
#include "AbstractEngine/Gfx/GfxResourceObject.h"

namespace lf {

DX12GfxFactory::DX12GfxFactory()
: mMappings()
, mResourceLock()
, mResources()
, mResourceRecursiveLock(0)
{

}
DX12GfxFactory::~DX12GfxFactory()
{
    
}

void DX12GfxFactory::Initialize()
{
    CreateMapping(typeof(GfxPipelineState), typeof(DX12GfxPipelineState));
    CreateMapping(typeof(DX12GfxPipelineState), typeof(DX12GfxPipelineState));
    CreateMapping(typeof(GfxVertexBuffer), typeof(DX12GfxVertexBuffer));
    CreateMapping(typeof(DX12GfxVertexBuffer), typeof(DX12GfxVertexBuffer));
    CreateMapping(typeof(GfxIndexBuffer), typeof(DX12GfxIndexBuffer));
    CreateMapping(typeof(DX12GfxIndexBuffer), typeof(DX12GfxIndexBuffer));
    CreateMapping(typeof(GfxTexture), typeof(DX12GfxTexture));
    CreateMapping(typeof(DX12GfxTexture), typeof(DX12GfxTexture));
    CreateMapping(typeof(GfxRenderTexture), typeof(DX12GfxRenderTexture));
    CreateMapping(typeof(DX12GfxRenderTexture), typeof(DX12GfxRenderTexture));
    CreateMapping(typeof(GfxSwapChain), typeof(DX12GfxSwapChain));
    CreateMapping(typeof(DX12GfxSwapChain), typeof(DX12GfxSwapChain));
    CreateMapping(typeof(GfxCommandContext), typeof(DX12GfxCommandContext));
    CreateMapping(typeof(DX12GfxCommandContext), typeof(DX12GfxCommandContext));
    CreateMapping(typeof(GfxCommandQueue), typeof(DX12GfxCommandQueue));
    CreateMapping(typeof(DX12GfxCommandQueue), typeof(DX12GfxCommandQueue));
    CreateMapping(typeof(GfxUploadBuffer), typeof(DX12GfxUploadBuffer));
    CreateMapping(typeof(DX12GfxUploadBuffer), typeof(DX12GfxUploadBuffer));
}

const Type* DX12GfxFactory::GetType(const Type* type) const
{
    auto it = std::find_if(mMappings.begin(), mMappings.end(), 
        [type](const TypeMapping& mapping) 
        { 
            return mapping.mSource == type; 
        });

    return it != mMappings.end() ? it->mDest : nullptr;
}

void DX12GfxFactory::CollectGarbage(const GarbageCallback& garbageCallback)
{
    ScopeLock lock(mResourceLock);
    for (auto it = mResources.begin(); it != mResources.end();)
    {
        GfxResourceObjectAtomicPtr resource = *it;
        if (resource.GetStrongRefs() == 2) // 1 ref for the map, 1 ref for here.
        {
            if (garbageCallback.IsValid())
            {
                garbageCallback.Invoke(resource);
            }
            it = mResources.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void DX12GfxFactory::TrackInstance(const GfxResourceObjectAtomicPtr& resource)
{
    ScopeLock lock(mResourceLock);
    mResources.insert(resource);
}

void DX12GfxFactory::UntrackInstance(const GfxResourceObjectAtomicPtr& resource)
{
    ScopeLock lock(mResourceLock);
    mResources.erase(resource);
}

void DX12GfxFactory::ForEachInstance(const ForEachCallback& callback)
{
    if (!callback.IsValid())
    {
        ReportBugMsg("Invalid callback supplied.");
        return;
    }

    if (AtomicIncrement32(&mResourceRecursiveLock) != 1)
    {
        AssertMsg("ForEachInstance is a non-recursive callback!");
        AtomicDecrement32(&mResourceRecursiveLock);
        return;
    }

    ScopeLock lock(mResourceLock);
    for (auto& resource : mResources)
    {
        callback.Invoke(resource);
    }

    AtomicDecrement32(&mResourceRecursiveLock);
}

void DX12GfxFactory::CreateMapping(const Type* source, const Type* dest)
{
    CriticalAssert(source);
    CriticalAssert(dest);
    mMappings.push_back({ source, dest });
}

} // namespace lf