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
#include "Entity.h"
#include "AbstractEngine/World/World.h"

namespace lf
{
DEFINE_CLASS(lf::EntityDefinition) { NO_REFLECTION; }
DEFINE_CLASS(lf::Entity) { NO_REFLECTION; }

EntityDefinition::EntityDefinition()
: Super()
, mComponentTypes()
{

}
EntityDefinition::~EntityDefinition()
{

}

void EntityDefinition::Serialize(Stream& s)
{
    Super::Serialize(s);
    SERIALIZE_ARRAY(s, mComponentTypes, "");
}

Entity::Entity()
: Super()
, mDefinition()
, mCollection()
, mLock()
, mID(INVALID_ENTITY_ID)
, mWorld(nullptr)
{}
Entity::~Entity()
{
    Destroy();
}

void Entity::PreInit(EntityInitializeData& initData)
{
    // required:
    Assert(Valid(initData.mID));
    AtomicStore(&mID, initData.mID);
    // required:
    Assert(initData.mWorld != nullptr);
    mWorld = initData.mWorld;
    // required:
    Assert(initData.mCollection != NULL_PTR);
    mCollection = initData.mCollection;
    // required:
    Assert(initData.mUpdateIDCallback.IsValid());
    UpdateId = initData.mUpdateIDCallback;
    // optional:
    mDefinition = initData.mDefinition;
}

void Entity::Serialize(Stream& s)
{
    Super::Serialize(s);
    SERIALIZE(s, mDefinition, "");
    // TODO: We must be able to serialize all our component types (which we can get from the definition)

}

void Entity::Destroy()
{
    Assert(GetWorld());
    ScopeLock lock(mLock);
    EntityId id = GetId();

    // Alive entities should go through unregister flow.
    if (ECSUtil::IsAlive(id))
    {
        SetIdInternal(ECSUtil::SetUnregister(id));
        return;
    }

    // If we've been inserted into the frame entities we should go through unregister flow
    const bool inFrame = GetWorld()->FindEntity(id) == this;
    if (inFrame)
    {
        SetIdInternal(ECSUtil::SetUnregister(id));
        return;
    }

    // If we're not in frame then we can just destroy
    if (ECSUtil::IsRegister(id))
    {
        SetIdInternal(ECSUtil::SetDestroyed(id));
        return;
    }
    
    // Destroy [ Unregisted] => SetId(Unregistered) == nop
    // Destroy [ Destroyed ] => SetId(Unregistered) == invalid
}
void Entity::SetPriority(ECSUtil::EntityPriority priority)
{
    ScopeLock lock(mLock);
    EntityId id = GetId();
    if (ECSUtil::GetPriority(id) == priority)
    {
        return;
    }

    EntityId newId = id;
    switch (priority)
    {
    case ECSUtil::EntityPriority::HIGH:
        newId = ECSUtil::SetHighPriority(id);
        break;
    case ECSUtil::EntityPriority::NORMAL:
        newId = ECSUtil::SetNormalPriority(id);
        break;
    case ECSUtil::EntityPriority::LOW:
        newId = ECSUtil::SetLowPriority(id);
        break;
    default:
        CriticalAssertMsg("Invalid priority");
        break;
    }
    SetIdInternal(newId);
}

void Entity::SetIdInternal(EntityId value)
{
    EntityId id = GetId();

    // Case for when first initializing
    if (Invalid(id))
    {
        AtomicStore(&mID, value);
        return;
    }

    EntityId rawId = mID & ECSUtil::ENTITY_ID_BITMASK;
    EntityId newId = value & ECSUtil::ENTITY_ID_BITMASK;
    if (rawId != newId) // Can only call this to change flags
    {
        return;
    }

    AtomicStore(&mID, value);
    UpdateId(id, this);

    // // Notify the collection we've updated
    // EntityCollection* collection = mCollection;
    // if (collection)
    // {
    //     collection->UpdateEntity(id, value);
    // }
    // // Notify the world we've updated.
    // if (mWorld)
    // {
    //     mWorld->UpdateEntityMapping(id, this);
    // }
    // Anything else that monitors ids should also update (consider an event)
    // Although users should really hold onto the pointer as a handle rather then id, id is used for optimized finds.
}

} // namespace lf 