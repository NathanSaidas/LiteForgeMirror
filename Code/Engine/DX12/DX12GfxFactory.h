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
#include "AbstractEngine/Gfx/GfxTypeFactory.h"

#include "Core/Utility/StdUnorderedSet.h"

namespace lf {
DECLARE_ATOMIC_PTR(GfxResourceObject);



class LF_ENGINE_API DX12GfxFactory
{
public:
    using GarbageCallback = TCallback<void, GfxResourceObjectAtomicPtr&>;
    using ForEachCallback = TCallback<void, const GfxResourceObject*>;

    DX12GfxFactory();
    ~DX12GfxFactory();

    void Initialize();

    const Type* GetType(const Type* type) const;

    void CollectGarbage(const GarbageCallback& garbageCallback);

    void TrackInstance(const GfxResourceObjectAtomicPtr& resource);
    void UntrackInstance(const GfxResourceObjectAtomicPtr& resource);

    // Non Recursive
    void ForEachInstance(const ForEachCallback& callback);
    template<typename LambdaT>
    void ForEachInstance(const LambdaT& callback)
    {
        ForEachInstance(ForEachCallback::Make(callback));
    }

private:
    void CreateMapping(const Type* source, const Type* dest);

    struct TypeMapping
    {
        const Type* mSource;
        const Type* mDest;
    };

    TVector<TypeMapping> mMappings;
    SpinLock                                  mResourceLock;
    TUnorderedSet<GfxResourceObjectAtomicPtr, std::hash<GfxResourceObject*>> mResources;
    volatile Atomic32 mResourceRecursiveLock;
    
};

} // namespace lf