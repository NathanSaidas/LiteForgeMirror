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
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {
class GfxCommandContext;
class GfxFence;
class GfxDependencyContext;

class LF_ABSTRACT_ENGINE_API GfxCommandQueue : public Object, public TAtomicWeakPointerConvertible<GfxCommandQueue>
{
    DECLARE_CLASS(GfxCommandQueue, Object);
public:
    using PointerConvertible = PointerConvertibleType;

    virtual bool Initialize(GfxDependencyContext& context) = 0;
    virtual void Release() = 0;
    virtual bool Execute(GfxCommandContext* context) = 0;
    virtual bool Execute(SizeT numContexts, GfxCommandContext** contexts) = 0;
    virtual bool Signal(GfxFence* fence) = 0;
};

} // namespace lf