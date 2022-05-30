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

#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "AbstractEngine/World/WorldTypes.h"

// Use this for components instead of DECLARE_CLASS handles all the boiler-plate code
#define DECLARE_COMPONENT(ComponentT)                                                                       \
    DECLARE_CLASS(ComponentT, Component);                                                                   \
    void BeginSerialize(ComponentData* dataPtr) override { mData = static_cast<ComponentT##Data*>(dataPtr); }     \
    void EndSerialize() override { mData = nullptr; }                                                       \
    ComponentFactory* GetFactory() const override { return &mFactory; }                                     \
    ComponentT##Data* mData;                                                                                      \
    mutable ::lf::TComponentFactory<ComponentT> mFactory;                                                         \
    public:                                                                                                 \
        using ComponentDataType = ComponentT##Data;


namespace lf
{

class World;
class ComponentFactory;

// Base type of component data. (Non-Polymorphic, data only)
struct ComponentData
{

};

// Interface for interacting with data in polymorphic way.
class LF_ABSTRACT_ENGINE_API Component : public Object
{
    DECLARE_CLASS(Component, Object);
public:
    // 
    // using ComponentDataType = ComponentData;

    virtual void BeginSerialize(ComponentData* dataPtr) = 0;
    virtual void EndSerialize() = 0;


    virtual ComponentFactory* GetFactory() const = 0;

    void SetId(ComponentId value) { mID = value; }
    ComponentId GetId() const { return mID; }
private:
    ComponentId mID;
};

} // namespace lf