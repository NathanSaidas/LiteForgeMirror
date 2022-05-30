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

#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/World/WorldTypes.h"
#include "AbstractEngine/World/ComponentList.h"

namespace lf
{

DECLARE_PTR(ComponentList);
DECLARE_ASSET_TYPE(EntityDefinition);
class Component;
struct ComponentData;

// Collection of all entity data.
// 
// NOTE: Entity sort by priority, it's a little faster to build a collection
// of sorted entities ( insert normals first, then high, then low )
// 
// USAGE:
//
// EntityCollection->Initialize( definition, components )       ; [World] At this point the collection is ready to use
// 
// CreateEntity( entityId )                                     ; [Anyone] Create a instance of an entity
// UpdateEntity( entityId, updatedFlags )                       ; [Entity] Update the flags of an entity
// CommitChanges( )                                             ; [World] Removes the deleted entities, adds the new entities
// ********************************************************************
class LF_ABSTRACT_ENGINE_API EntityCollection
{
public:
    EntityCollection();

    // Initialize the collection with the definition. (If first definition, components lists are created)
    // ********************************************************************
    // Initializes the collection with a definition and list of components.
    // 
    // @param definition - The definition. (One collection may reference many definitions, one definition refernces one collection)
    // @param sortedComponent - An array of components sorted in ComponentSequence order.
    // ********************************************************************
    bool Initialize(const EntityDefinitionAssetType& definition, const TVector<const Component*>& sortedComponents);
    // Release the definition from the collection. (Does not destroy component lists, world is expected to call ClearData)
    void Release(const EntityDefinitionAssetType& definition);
    // Check if the collection has any valid definitions
    bool Empty() const { return mTypes.empty(); }
    // Clears the entity component data from the collection.
    void ClearData();

    // Update an entities flags.
    bool UpdateEntity(EntityId entityId, EntityId updatedFlags);

    // Allocate an entity, and their components
    void CreateEntity(EntityId entityId);

    void CommitChanges();

    SizeT GetIndex(EntityId entityId) const;
    SizeT GetNewIndex(EntityId entityId) const;

    SizeT GetIndexSlow(EntityId entityId) const;
    SizeT GetNewIndexSlow(EntityId entityId) const;

    EntityId GetEntity(SizeT index) const;
    EntityId GetNewEntity(SizeT index) const;

    // ********************************************************************
    // Accessor to the 'Entity' component array (intended use for data 
    // traversal, do not manipulate the array itself!)
    // 
    // Pointer is invalidated if the collection becomes empty/reinitializes.
    // ********************************************************************
    template<typename ComponentT>
    TVector<typename ComponentT::ComponentDataType>* GetCurrentArray();
    // ********************************************************************
    // Accessor to the 'New Entity' component array (intended use for data
    // traversal, do not manipulate the array itself!)
    // 
    // Pointer is invalidated if the collection becomes empty/reinitializes.
    // ********************************************************************
    template<typename ComponentT>
    TVector<typename ComponentT::ComponentDataType>* GetNewArray();

    ComponentData* GetCurrentComponent(SizeT entityIndex, SizeT typeIndex);
    ComponentData* GetNewComponent(SizeT entityIndex, SizeT typeIndex);

    const TVector<const Type*>& GetTypes();
private:
    ComponentList* GetCurrentList(const Type* type);
    ComponentList* GetNewList(const Type* type);

    // List of 'updating' entities
    TVector<EntityIdAtomic> mEntities; // note: EntityId (flags) can change at anytime on any thread.
    TVector<ComponentListPtr> mComponents;

    // List of 'entities just created'
    SpinLock mNewEntityLock;
    TVector<EntityIdAtomic> mNewEntities;
    TVector<ComponentListPtr> mNewComponents;

    TVector<EntityDefinitionAssetType>   mDefinitions;
    TVector<const Type*>                 mTypes; // Debug Information/Utility
    bool                                mStatic;
};

template<typename ComponentT>
TVector<typename ComponentT::ComponentDataType>* EntityCollection::GetCurrentArray()
{
    LF_STATIC_IS_A(ComponentT, Component);
    ComponentList* list = GetCurrentList(typeof(ComponentT));
    if (!list)
    {
        return nullptr;
    }

    TComponentList<ComponentT>* componentList = static_cast<TComponentList<ComponentT>*>(list);
    return componentList->GetArray();
}

template<typename ComponentT>
TVector<typename ComponentT::ComponentDataType>* EntityCollection::GetNewArray()
{
    LF_STATIC_IS_A(ComponentT, Component);
    ComponentList* list = GetNewList(typeof(ComponentT));
    if (!list)
    {
        return nullptr;
    }

    TComponentList<ComponentT>* componentList = static_cast<TComponentList<ComponentT>*>(list);
    return componentList->GetArray();
}

} // namespace lf