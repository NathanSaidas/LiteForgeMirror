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
#include "Core/Reflection/Object.h"
#include "Core/Utility/EventBus.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "Runtime/Asset/AssetTypeInfo.h"

namespace lf {
DECLARE_MANAGED_CPTR(AssetTypeInfo);
class GfxDependencyContext;
class GfxDevice;
class GfxCommandContext;

// **********************************
// REDUX:
// 
// A GfxModelRenderer (or any type of subrenderer) will exist and allocate resources
// 
// GetRequestedDescriptors()
// 
// SetupResource: 
//      x = CreateResource<Type>(); 
// 
// A resource will be created during the SetupResource function on the appropriate thread.
// It's important that the resource is created here because it cannot be guaranteed a spot on the descriptor heap otherwise.
// 
// BindResource:
//      x->Bind();
// 
// A resource must be bound to the descriptor heap (in some cases) in order to be used for rendering.
// 
// { Resource State: None, Registered,  } 
//      None,           -- Resource was created, can't utilize any graphics api
//      Registered,     -- Resource has been initialized with dependencies, can use graphics api
//      RenderReady,    -- Resource has been considered initialized such that it can partake in draw commands
// 
// { Resource Aux States }
//      DescriptorBound -- Resource has a descriptor handle
// 
//  
//  ModelRenderer()->SetData( ... ) => {
//      
//  }
// 
// 
// 
// { 
// 
// **********************************
class LF_ABSTRACT_ENGINE_API GfxResourceObject : public Object, public TAtomicWeakPointerConvertible<GfxResourceObject>
{
    DECLARE_CLASS(GfxResourceObject, Object);
public:
    using PointerConvertible = PointerConvertibleType;
    using InvalidateCallback = TCallback<void, const GfxResourceObject*>;

    GfxResourceObject();
    virtual ~GfxResourceObject();
    // ** Called once when the GfxResourceObject is created by the GfxDevice, initialize dependencies
    // ** 
    // ** Do not issue commands to command list/context. (Do that in Commit) 
    // **
    // ** Use this method only to get dependencies
    // **
    // ** threading=Any thread
    virtual bool Initialize(GfxDependencyContext& context);
    // ** Called once when the GfxResourceObject is being garbage collected, release resources.
    virtual void Release();

    // ** Called before the resource is uploaded to determine the amount of descriptors it will need.
    virtual SizeT GetRequestedDescriptors() const;

    void SetAssetType(const AssetTypeInfo* type);

    // ** Called to commit the transient properties to the underlying graphics resource object 
    virtual void Commit(GfxDevice& device, GfxCommandContext& context);


    // ** These callbacks are issued when resources have changed and might not be considered valid for rendering anymore.
    // These methods are potentially not useful.
    Int32 RegisterInvalidate(const InvalidateCallback& callback);
    void UnregisterInvalidate(Int32 id);
protected:

    GfxDevice& Graphics() { return *mService; }

    void Invalidate();
private:
    using DataInvalidatedEventBus = TEventBus<InvalidateCallback>;

    GfxDevice* mService;
    DataInvalidatedEventBus mInvalidateEventBus;
    AssetTypeInfoCPtr mAssetType; // for debugging...
    
};

using GfxInvalidateCallbasck = GfxResourceObject::InvalidateCallback;

} // namespace lf