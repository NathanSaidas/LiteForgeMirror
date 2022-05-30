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
#pragma once

#include "Core/Memory/SmartPointer.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "AbstractEngine/World/ComponentList.h"

namespace lf
{
DECLARE_PTR(ComponentList);
DECLARE_PTR(ComponentFactory);

// Instantiates component lists
class LF_ABSTRACT_ENGINE_API ComponentFactory
{
public:
    ComponentFactory();
    virtual ~ComponentFactory() = default;
    virtual ComponentListPtr Create() = 0;
    virtual const Type* GetType() const = 0;
};

template<typename ComponentT>
class TComponentFactory : public ComponentFactory
{
public:
    ComponentListPtr Create() override
    {
        ComponentListPtr list(LFNew<TComponentList<ComponentT>>());
        list->SetType(typeof(ComponentT));
        return list;
    }

    const Type* GetType() const override
    {
        return typeof(ComponentT);
    }
};

} // namespace lf