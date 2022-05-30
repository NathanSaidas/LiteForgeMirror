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
#include "Core/Utility/APIResult.h"
#include "Core/Utility/StdVector.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "AbstractEngine/World/WorldTypes.h"
#include "AbstractEngine/World/World.h"

namespace lf
{

class World;

class LF_ABSTRACT_ENGINE_API ComponentSystemFence : public Object
{
    DECLARE_CLASS(ComponentSystemFence, Object);
};

class LF_ABSTRACT_ENGINE_API ComponentSystem : public Object
{
    DECLARE_CLASS(ComponentSystem, Object);
public:
    using UpdateCallback = ECSUtil::UpdateCallback;

    virtual ~ComponentSystem();

    bool Initialize(World* world);
    void BindTuples();
    void ScheduleUpdates();

    virtual bool IsEnabled() const; // enable or disable systems during startup, used for testing purposes primarily.

protected:
    APIResult<bool> ScheduleUpdate(
        const String& name, 
        const ECSUtil::UpdateCallback& callback, 
        const Type* fence = nullptr,
        ECSUtil::UpdateType updateType = ECSUtil::UpdateType::SERIAL);
    APIResult<bool> StartConstantUpdate(
        const ECSUtil::UpdateCallback& callback, 
        const Type* fence = nullptr, 
        ECSUtil::UpdateType updateType = ECSUtil::UpdateType::SERIAL,
        const TVector<const Type*>& readComponents = TVector<const Type*>(),
        const TVector<const Type*>& writeComponents = TVector<const Type*>());
    APIResult<bool> StartConstantUpdate(
        const String& name, 
        const ECSUtil::UpdateCallback& callback, 
        const Type* fence = nullptr, 
        ECSUtil::UpdateType updateType = ECSUtil::UpdateType::SERIAL, 
        const TVector<const Type*>& readComponents = TVector<const Type*>(),
        const TVector<const Type*>& writeComponents = TVector<const Type*>());
    APIResult<bool> StopConstantUpdate(const String& name);

    Token CreateUpdateName(const String& name);
    // ********************************************************************
    // During game-runtime this will be called once during app initialization.
    // 
    // During editor-runtime this may be called multiple times (once during
    // app initialization) and any other time when modifications are made
    // and systems must be re-initialized.
    //
    // @return - Return false if the system failed to intiailize (this will prevent
    // the world from running updates until the error has been corrected)
    // ********************************************************************
    virtual bool OnInitialize();
    virtual void OnBindTuples();
    virtual void OnScheduleUpdates();

    // Helper to bind a tuple, see ComponentSystemTuple.h
    template<typename TupleT>
    void BindTuple(TupleT& tuple, const TVector<const Type*>& excludeTypes = TVector<const Type*>());

    // Helper to access a single component from a tuple, see ComponentSystemTuple.h
    template<typename TupleT, typename CallbackT>
    void GetEntity(TupleT& tuple, SizeT collectionID, SizeT itemID, const CallbackT& callback);

    // Helper to iterate over a tuple, see ComponentSystemTuple.h
    template<typename TupleT, typename CallbackT>
    void ForEach(TupleT& tuple, const CallbackT& callback);

    // Helper to iterator over a specific collection, see ComponentSystemTuple.h
    template<typename TupleT, typename CallbackT>
    void ForEach(TupleT& tuple, SizeT collectionID, const CallbackT& callback);

    // Helper to iterate over a tuple, see ComponentSystemTuple.h
    template<typename TupleT, typename CallbackT>
    void ForEachEntity(TupleT& tuple, const CallbackT& callback);

    World* GetWorld() { return mWorld; }
private:
    World* mWorld;
};

template<typename TupleT>
void ComponentSystem::BindTuple(TupleT& tuple, const TVector<const Type*>& excludeTypes )
{
    using TupleType = typename TupleT::TupleType;
    TupleType* typedTuple = reinterpret_cast<TupleType*>(&tuple);
    LF_STATIC_ASSERT(sizeof(TupleType) == sizeof(TupleT));
    typedTuple->Bind(GetWorld(), excludeTypes);
}

// Helper to access a single component from a tuple
template<typename TupleT, typename CallbackT>
void ComponentSystem::GetEntity(TupleT& tuple, SizeT collectionID, SizeT itemID, const CallbackT& callback)
{
    using TupleType = typename TupleT::TupleType;
    TupleType* typedTuple = reinterpret_cast<TupleType*>(&tuple);
    typedTuple->InvokeWithItems(callback, collectionID, itemID);
}

// Helper to iterate over a tuple
template<typename TupleT, typename CallbackT>
void ComponentSystem::ForEach(TupleT& tuple, const CallbackT& callback)
{
    using TupleType = typename TupleT::TupleType;
    TupleType* typedTuple = reinterpret_cast<TupleType*>(&tuple);
    SizeT collectionCount = typedTuple->CollectionCount();
    for (SizeT collection = 0; collection < collectionCount; ++collection)
    {
        for (SizeT componentIdx = 0, componentCount = typedTuple->Count(collection);
            componentIdx < componentCount;
            ++componentIdx
            )
        {
            typedTuple->InvokeWithItems(callback, collection, componentIdx);
        }
    }
}

template<typename TupleT, typename CallbackT>
void ComponentSystem::ForEach(TupleT& tuple, SizeT collectionID, const CallbackT& callback)
{
    using TupleType = typename TupleT::TupleType;
    TupleType* typedTuple = reinterpret_cast<TupleType*>(&tuple);
    for (SizeT componentIdx = 0, componentCount = typedTuple->Count(collectionID);
        componentIdx < componentCount;
        ++componentIdx
        )
    {
        typedTuple->InvokeWithItems(callback, collectionID, componentIdx);
    }
}

// Helper to iterate over a tuple, see ComponentSystemTuple.h
template<typename TupleT, typename CallbackT>
void ComponentSystem::ForEachEntity(TupleT& tuple, const CallbackT& callback)
{
    using TupleType = typename TupleT::TupleType;
    TupleType* typedTuple = reinterpret_cast<TupleType*>(&tuple);
    SizeT collectionCount = typedTuple->CollectionCount();
    for (SizeT collection = 0; collection < collectionCount; ++collection)
    {
        for (SizeT componentIdx = 0, componentCount = typedTuple->Count(collection);
            componentIdx < componentCount;
            ++componentIdx
            )
        {
            typedTuple->InvokeWithEntityItems(callback, collection, componentIdx);
        }
    }
}

// Built-in Fences

class LF_ABSTRACT_ENGINE_API ComponentSystemRegisterFence : public ComponentSystemFence
{
    DECLARE_CLASS(ComponentSystemRegisterFence, ComponentSystemFence);
};

class LF_ABSTRACT_ENGINE_API ComponentSystemUpdateFence : public ComponentSystemFence
{
    DECLARE_CLASS(ComponentSystemUpdateFence, ComponentSystemFence);
};

class LF_ABSTRACT_ENGINE_API ComponentSystemUnregisterFence : public ComponentSystemFence
{
    DECLARE_CLASS(ComponentSystemUnregisterFence, ComponentSystemFence);
};

} // namespace lf