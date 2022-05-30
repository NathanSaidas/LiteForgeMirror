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
#include "Core/Utility/StdVector.h"
#include "AbstractEngine/World/WorldTypes.h"

namespace lf
{

class Type;
struct ComponentData;

class LF_ABSTRACT_ENGINE_API ComponentList
{
public:
    ComponentList();
    virtual ~ComponentList() = default;

    virtual void AddDefault() = 0;
    virtual void AddCopy(ComponentList* other, SizeT index) = 0;
    virtual void Reset() = 0;
    virtual void SwapRemove(SizeT index) = 0;
    virtual void Swap(SizeT oldIndex, SizeT newIndex) = 0;
    virtual ComponentData* GetData(SizeT index) = 0;

    void SetType(const Type* value) { mComponentType = value; }
    const Type* GetType() const { return mComponentType; }
private:
    const Type*      mComponentType;
};

template<typename ComponentT>
class TComponentList : public ComponentList
{
public:
    using ComponentType = ComponentT;
    using ComponentDataType = typename ComponentT::ComponentDataType;

    void AddDefault() override 
    { 
        mComponents.push_back(ComponentDataType()); 
    }
    void AddCopy(ComponentList* other, SizeT index) override
    {
        CriticalAssert(GetType() == other->GetType());
        mComponents.push_back(static_cast<TComponentList*>(other)->mComponents[index]);
    }
    void Reset() override
    {
        mComponents.resize(0);
    }
    void SwapRemove(SizeT index) override
    {
        if (index < mComponents.size())
        {
            auto it = mComponents.begin() + index;
            mComponents.swap_erase(it);
        }
    }
    void Swap(SizeT oldIndex, SizeT newIndex) override
    {
        std::swap(mComponents[oldIndex], mComponents[newIndex]);
    }

    ComponentData* GetData(SizeT index) override
    {
        CriticalAssert(index < mComponents.size());
        return &mComponents[index];
    }


    TVector<ComponentDataType>* GetArray() { return &mComponents; }
private:
    TVector<ComponentDataType> mComponents;
};

} // namespace