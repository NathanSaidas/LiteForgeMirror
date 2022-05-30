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
#include "AbstractEngine/PCH.h"
#include "EntityCollection.h"
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentList.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include <algorithm>

#include "Core/Utility/Log.h"

namespace lf
{

LF_INLINE bool IsValidComponentType(const Type* type)
{
    if (!type || type == typeof(Component) || !type->IsA(typeof(Component)))
    {
        return false;
    }
    return true;
}

EntityCollection::EntityCollection()
: mEntities()
, mComponents()
, mNewEntities()
, mNewComponents()
, mDefinitions()
, mTypes()
, mStatic(false)
{

}

bool EntityCollection::Initialize(const EntityDefinitionAssetType& definition, const TVector<const Component*>& sortedComponents)
{
    bool staticDef = !definition;

    if (sortedComponents.empty())
    {
        gSysLog.Error(LogMessage("Cannot initialize collection with no components."));
        return false;
    }

    // Under the assumption sortedComponents is validated.
    if (!staticDef)
    {
        if (mDefinitions.end() != std::find(mDefinitions.begin(), mDefinitions.end(), definition))
        {
            gSysLog.Warning(LogMessage("Entity definition already exists in collection ") << definition.GetPath().AsToken());
            return true;
        }
        mDefinitions.push_back(definition);
    }

    bool createLists = mTypes.empty();
    if (createLists)
    {
        mComponents.resize(sortedComponents.size());
        mNewComponents.resize(sortedComponents.size());
        for (SizeT i = 0; i < sortedComponents.size(); ++i)
        {
            mComponents[i] = sortedComponents[i]->GetFactory()->Create();
            mNewComponents[i] = sortedComponents[i]->GetFactory()->Create();
        }

        for (const Component* component : sortedComponents)
        {
            mTypes.push_back(component->GetType());
        }
    }

    mStatic = mStatic || staticDef;

    return true;
}
void EntityCollection::Release(const EntityDefinitionAssetType& definition)
{
    if (!definition)
    {
        gSysLog.Error(LogMessage("Cannot release collection with null definition."));
        return;
    }

    auto it = std::find(mDefinitions.begin(), mDefinitions.end(), definition);
    if (it == mDefinitions.end())
    {
        gSysLog.Error(LogMessage("Cannot release collection, definition does not exist. ") << definition.GetPath().AsToken());
        return;
    }

    mDefinitions.swap_erase(it);
}

void EntityCollection::ClearData()
{
    Assert(mDefinitions.empty());
    mEntities.clear();
    mComponents.clear();
    mNewComponents.clear();
    mTypes.clear();
}

bool EntityCollection::UpdateEntity(EntityId entityId, EntityId updatedFlags)
{
    EntityId id = ECSUtil::GetId(entityId);
    EntityId updateId = ECSUtil::GetId(updatedFlags);
    if (id != updateId)
    {
        gSysLog.Error(LogMessage("Mismatch Entity Ids=") << id << ", " << updateId << ". Original=" << entityId << ", UpdatedFlags" << updatedFlags);
        ReportBugMsg("UpdateEntity has mismatched id arguments.");
        return false;
    }

    if (!ECSUtil::IsLifeChanged(entityId, updatedFlags))
    {
        if (ECSUtil::IsDestroyed(entityId) || ECSUtil::IsDestroyed(updatedFlags))
        {
            ReportBugMsg("Cannot update entity who is destroyed!");
            return false;
        }
    }

    auto entity = std::find_if(mEntities.begin(), mEntities.end(), [entityId](const EntityIdAtomic& id) { return AtomicLoad(&id) == entityId; });
    if (entity != mEntities.end())
    {
        AtomicStore(&(*entity), updatedFlags);
        return true;
    }

    ScopeLock lock(mNewEntityLock);
    entity = std::find(mNewEntities.begin(), mNewEntities.end(), entityId);
    if (entity == mNewEntities.end())
    {
        gSysLog.Error(LogMessage("Unable to update entity with id ") << id << ", they don't exist.");
        return false;
    }
    AtomicStore(&(*entity), updatedFlags);
    return true;
}

void EntityCollection::CreateEntity(EntityId entityId)
{
    Assert(!Empty());
    ScopeLock lock(mNewEntityLock);
    mNewEntities.push_back(entityId);
    for (ComponentList* list : mNewComponents)
    {
        list->AddDefault();
    }
}


void EntityCollection::CommitChanges()
{
    // TODO: We can traverse 'deleted' entities exclusively.

    // So we change the ids here.. but the world and the entity itself need to know...
    // a) We keep a list of...
    // ....i) Entities Added ( World->Create() )
    // ....i) Entities Unregistered ( Entity->Destroy() )
    // ....i) Entities Destroyed ( World->FlushDestroyed() )

    // Remove destroyed entities
    for (auto it = mEntities.begin(); it != mEntities.end();)
    {
        EntityId id = AtomicLoad(&(*it));
        ECSUtil::EntityLifeState state = ECSUtil::GetLifeState(id);
        switch (state)
        {
        case ECSUtil::EntityLifeState::REGISTER:
        case ECSUtil::EntityLifeState::ALIVE:
        case ECSUtil::EntityLifeState::UNREGISTER:
        {
            ++it;
        } break;
        case ECSUtil::EntityLifeState::DESTROYED:
        {
            SizeT index = static_cast<SizeT>(it - mEntities.begin());
            for (auto list = mComponents.begin(); list != mComponents.end(); ++list)
            {
                (*list)->SwapRemove(index);
            }
            // Remove the entity
            it = mEntities.swap_erase(it);
        } break;
        default:
            break;

        }
    }

    ScopeLock lock(mNewEntityLock);
    // Add new entities (unless they are destroyed)
    for (auto it = mNewEntities.begin(); it != mNewEntities.end(); ++it)
    {
        EntityId id = AtomicLoad(&(*it));
        if (!ECSUtil::IsDestroyed(id))
        {
            SizeT index = static_cast<SizeT>(it - mNewEntities.begin());
            for(SizeT i = 0; i < mComponents.size(); ++i)
            {
                mComponents[i]->AddCopy(mNewComponents[i], index);
            }
            mEntities.push_back(id);
        }
    }

    // Reset the lists
    mNewEntities.resize(0);
    for (ComponentList* list : mNewComponents)
    {
        list->Reset();
    }
}

SizeT EntityCollection::GetIndex(EntityId entityId) const
{
    for (SizeT i = 0; i < mEntities.size(); ++i)
    {
        if (AtomicLoad(&mEntities[i]) == entityId)
        {
            return i;
        }
    }
    return INVALID;
}
SizeT EntityCollection::GetNewIndex(EntityId entityId) const
{
    for (SizeT i = 0; i < mNewEntities.size(); ++i)
    {
        if (AtomicLoad(&mNewEntities[i]) == entityId)
        {
            return i;
        }
    }
    return INVALID;
}

SizeT EntityCollection::GetIndexSlow(EntityId entityId) const
{
    EntityId rawId = ECSUtil::GetId(entityId);
    for (SizeT i = 0; i < mEntities.size(); ++i)
    {
        if (ECSUtil::GetId(AtomicLoad(&mEntities[i])) == rawId)
        {
            return i;
        }
    }
    return INVALID;
}
SizeT EntityCollection::GetNewIndexSlow(EntityId entityId) const
{
    EntityId rawId = ECSUtil::GetId(entityId);
    for (SizeT i = 0; i < mNewEntities.size(); ++i)
    {
        if (ECSUtil::GetId(AtomicLoad(&mNewEntities[i])) == rawId)
        {
            return i;
        }
    }
    return INVALID;
}

EntityId EntityCollection::GetEntity(SizeT index) const
{
    if (index < mEntities.size())
    {
        return AtomicLoad(&mEntities[index]);
    }
    return INVALID_ENTITY_ID;
}
EntityId EntityCollection::GetNewEntity(SizeT index) const
{
    if (index < mNewEntities.size())
    {
        return AtomicLoad(&mNewEntities[index]);
    }
    return INVALID_ENTITY_ID;
}

ComponentData* EntityCollection::GetCurrentComponent(SizeT entityIndex, SizeT typeIndex)
{
    return mComponents[typeIndex]->GetData(entityIndex);
}

ComponentData* EntityCollection::GetNewComponent(SizeT entityIndex, SizeT typeIndex)
{
    return mNewComponents[typeIndex]->GetData(entityIndex);
}

ComponentList* EntityCollection::GetCurrentList(const Type* type)
{
    if (!IsValidComponentType(type))
    {
        return nullptr;
    }
    for (ComponentList* list : mComponents)
    {
        if (list->GetType() == type)
        {
            return list;
        }
    }
    return nullptr;
}
ComponentList* EntityCollection::GetNewList(const Type* type)
{
    if (!IsValidComponentType(type))
    {
        return nullptr;
    }
    for (ComponentList* list : mNewComponents)
    {
        if (list->GetType() == type)
        {
            return list;
        }
    }
    return nullptr;
}

} // namespace lf