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
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/World/WorldTypes.h"
#include "AbstractEngine/World/EntityCollection.h"

namespace lf {

DECLARE_ASSET_TYPE(EntityDefinition);
DECLARE_WPTR(EntityCollection);
class World;

// Asset Serializeable Entity Definition (data driven entity types)
class LF_ABSTRACT_ENGINE_API EntityDefinition : public AssetObject
{
    DECLARE_CLASS(EntityDefinition, AssetObject);
public:
    EntityDefinition();
    virtual ~EntityDefinition();

    void Serialize(Stream& s);

    void SetComponentTypes(const TVector<const Type*>& value) { mComponentTypes = value; }
    const TVector<const Type*>& GetComponentTypes() const { return mComponentTypes; }
private:
    TVector<const Type*> mComponentTypes;
};

class Entity;

struct EntityInitializeData
{
    using UpdateIdCallback = TCallback<void, EntityId, const Entity*>;


    EntityId                  mID;               // Required:
    EntityCollectionWPtr      mCollection;       // Required:
    World*                    mWorld;            // Required:
    UpdateIdCallback          mUpdateIDCallback; // Required:
    EntityDefinitionAssetType mDefinition;       // Optional:
};

// Asset Serializeable Entity Data (data driven entities)
class LF_ABSTRACT_ENGINE_API Entity : public AssetObject
{
    DECLARE_CLASS(Entity, AssetObject);
public:
    using UpdateIdCallback = EntityInitializeData::UpdateIdCallback;

    Entity();
    virtual ~Entity();

    void PreInit(EntityInitializeData& initData);

    void Serialize(Stream& s);

    // Update entity flags
    void Destroy();
    // Update entity flags
    void SetPriority(ECSUtil::EntityPriority priority);

    // Accessor to the entity Id (only the world should change it)
    void SetId(EntityId value) { ScopeLock lock(mLock); SetIdInternal(value); }
    EntityId GetId() const { return AtomicLoad(&mID); }

    // Accessor to the definition (if any, entities don't have to be created with explicit data-driven type)
    const EntityDefinitionAssetType& GetDefinition() const { return mDefinition; }
    // Accessor to the collection the entity belongs in (only the world should need access to this)
    const EntityCollection* GetCollection() const { return mCollection; }
    // Accessor to the world (service)
    World* GetWorld() const { return mWorld; }

    template<typename T>
    typename T::ComponentDataType* GetComponent()
    {
        // CreateEntity( ... )->GetComponent<...>() will probably be a used pattern to initialize data..
        // prefer indexing 'new' entities over 'existing' entities.

        SizeT componentIndex = mCollection->GetNewIndex(GetId());
        if (Valid(componentIndex))
        {
            return &(*mCollection->GetNewArray<T>())[componentIndex];
        }

        componentIndex = mCollection->GetIndex(GetId());
        if (Valid(componentIndex))
        {
            return &(*mCollection->GetCurrentArray<T>())[componentIndex];
        }

        return nullptr;
    }
private:
    void SetIdInternal(EntityId value);
    EntityDefinitionAssetType mDefinition;
    EntityCollectionWPtr mCollection;
    SpinLock mLock;
    EntityIdAtomic mID; // Unique ID to the World
    mutable World* mWorld; // ServicePtr

    UpdateIdCallback UpdateId; // Function
};

} // namespace lf