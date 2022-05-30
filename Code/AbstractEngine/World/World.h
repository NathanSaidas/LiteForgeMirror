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
#include "Runtime/Service/Service.h"
#include "AbstractEngine/World/WorldTypes.h"

namespace lf
{

class Type;
class EntityCollection;
DECLARE_ATOMIC_WPTR(Entity);
DECLARE_ASSET(EntityDefinition);
class ComponentSystem;
DECLARE_ATOMIC_PTR(WorldScene);


// What is an entity ID?
// If I have an entity ID? When will it become invalid?
// What guarantee do external services have to invalidate objects associated with an invalid entity ID?
//
class LF_ABSTRACT_ENGINE_API World : public Service
{
    DECLARE_CLASS(World, Service);
public:
    struct UpdateInfo
    {
        UpdateInfo()
        : mName()
        , mSystem(nullptr)
        , mUpdateCallback()
        , mFenceType(nullptr)
        , mUpdateType(ECSUtil::UpdateType::SERIAL)
        , mReadComponents()
        , mWriteComponents()
        {}
        // Constant Updates must be unique (eg; Systems have default 'SystemName.Update', or 'SystemName.<x>' if name is specified.
        Token mName; 
        // Constant Updates require a system
        ComponentSystem* mSystem;

        // required
        ECSUtil::UpdateCallback mUpdateCallback;
        // optional (default=Update)
        const Type* mFenceType;
        // optional (default=Serial)
        ECSUtil::UpdateType mUpdateType;

        TVector<const Type*> mReadComponents;
        TVector<const Type*> mWriteComponents;
    };

    virtual ~World() = default;

    // ********************************************************************
    // Create a entity with a data-driven type. (must be loaded)
    //
    // Entity lifetime is managed by the World and the scope of the WorldContainer
    // @see Entity Lifetimes
    // @param definition - The definition holding onto all the component types
    // @return Returns the created entity
    // ********************************************************************
    virtual EntityAtomicWPtr CreateEntity(const EntityDefinitionAsset& definition) = 0;
    // ********************************************************************
    // Create a entity with a defined set of components.
    //
    // Entity lifetime is managed by the World and the scope of the WorldContainer
    // @see Entity Lifetimes
    // @param definition - The definition holding onto all the component types
    // @return Returns the created entity
    // ********************************************************************
    virtual EntityAtomicWPtr CreateEntity(const EntityDefinition* definition) = 0;
    // ********************************************************************
    // Find all the collections that have the include types, not not the exclude types.
    //
    // The pointer to the collection remains valid until deleted. Consider registering for 'World.Rebind' callback.
    // ********************************************************************
    virtual TVector<EntityCollection*> FindCollections(const TVector<const Type*>& includeTypes, const TVector<const Type*>& excludeTypes) = 0;
    // ********************************************************************
    // Create a update fence before another fence.
    //
    // This method can only be called during world initialization.
    //
    // @param fence - The type of fence to create (only one of each type can exist, cannot be built in, must be ComponentSystemFence)
    // @param target - The target fence for reference
    // @returns true (success) or false (failed) with api result of the error
    // ********************************************************************
    virtual APIResult<bool> CreateFenceBefore(const Type* fence, const Type* target) = 0;
    // ********************************************************************
    // Create a update fence after another fence. 
    //
    // This method can only be called during world initialization.
    //
    // @param fence - The type of fence to create (only one of each type can exist, cannot be built in, must be ComponentSystemFence)
    // @param target - The target fence for reference
    // @returns true (success) or false (failed) with api result of the error
    // ********************************************************************
    virtual APIResult<bool> CreateFenceAfter(const Type* fence, const Type* target) = 0;

    // CreateUpdateToken("name"); // ObjectPtr { name, system }

    // one time update
    virtual APIResult<bool> ScheduleUpdate(const UpdateInfo& info) = 0;
    // every frame update
    virtual APIResult<bool> StartConstantUpdate(const UpdateInfo& info) = 0;
    virtual APIResult<bool> StopConstantUpdate(const Token& name) = 0;


    virtual EntityAtomicWPtr FindEntity(EntityId id) = 0;
    virtual std::pair<EntityId,EntityAtomicWPtr> FindEntitySlow(EntityId id) = 0;

    virtual void RegisterStaticEntityDefinition(const EntityDefinition* definition) = 0;

    virtual bool IsRegistering() = 0;
    virtual bool IsUnregistering() = 0;

    virtual ComponentSystem* GetSystem(const Type* type) = 0;

    template<typename T>
    T* GetSystem()
    {
        return static_cast<T*>(GetSystem(typeof(T)));
    }

    virtual void RegisterScene(const WorldSceneAtomicPtr& scene) = 0;

    virtual bool LogEntityIdChanges() const = 0;
    virtual bool LogEntityAddRemove() const = 0;
    virtual bool LogFenceUpdate() const = 0;
    virtual bool LogFenceUpdateVerbose() const = 0;

};

} // namespace lf