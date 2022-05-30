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
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include "AbstractEngine/World/ComponentSystem.h"
#include "AbstractEngine/World/Entity.h"
#include "AbstractEngine/World/World.h"

namespace lf
{

// ********************************************************************
// A super simple implementation of component.
//
// usage:
// 
// struct CustomComponentData : public ComponentData
// {
//      void Serialize(Stream& s); // implement this method
// };
// 
// class CustomComponent : public TComponentImpl<CustomComponent, ComponentDataT>
// {
//     DECLARE_CLASS(CustomComponent, Component);
// };
// DEFINE_CLASS(CustomComponent) { NO_REFLECTION; }
// ********************************************************************
template<typename ComponentT, typename ComponentDataT>
class TComponentImpl : public Component
{
public:
    using ComponentDataType = ComponentDataT;

    TComponentImpl() : mData(nullptr), mFactory() {}
    virtual ~TComponentImpl() {}

    void Serialize(Stream& s) override
    {
        Super::Serialize(s);
        if (mData)
        {
            mData->Serialize(s);
        }
    }

    void BeginSerialize(ComponentData* data) final { mData = static_cast<ComponentDataT*>(data); }
    void EndSerialize() final { mData = nullptr; }
    ComponentFactory* GetFactory() const final { return &mFactory; }
protected:
    typename ComponentDataT* mData;
    mutable TComponentFactory<ComponentT>   mFactory;
};

} // namespace lf