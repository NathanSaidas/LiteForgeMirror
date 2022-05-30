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

#include "AbstractEngine/World/World.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/StdVector.h"
#include "Core/Utility/UniqueNumber.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/App/AppConfig.h"
#include "AbstractEngine/World/ComponentSystem.h"
#include "AbstractEngine/World/EntityCollection.h"

namespace lf
{
DECLARE_PTR(Component);
DECLARE_ATOMIC_PTR(Entity);
DECLARE_PTR(EntityCollection);
DECLARE_ATOMIC_WPTR(Entity);
DECLARE_ATOMIC_WPTR(WorldScene);
DECLARE_WPTR(EntityCollection);
DECLARE_ASSET_TYPE(EntityDefinition);
DECLARE_ASSET(EntityDefinition);
DECLARE_PTR(ComponentSystem);
class AppService;

class LF_ENGINE_API WorldConfig : public AppConfigObject
{
    DECLARE_CLASS(WorldConfig, AppConfigObject);
public:
    WorldConfig();
    void Serialize(Stream& s);

    bool mLogEntityIdChanges;
    bool mLogEntityAddRemove;
    bool mLogFenceUpdate;
    bool mLogFenceUpdateVerbose;
};



// TODO: If we add/remove a collection we must emit 'bind tuple'
// we can optimize this a bit with Compare(mine, theirs.Hash(ComponentSequence)) && Compare(mine, theirs.ComponenSequence)
class LF_ENGINE_API WorldImpl : public World
{
    DECLARE_CLASS(WorldImpl, World);
    struct DefinitionIndex
    {
        UInt16 mId;
        UInt16 mComponentCount;
        UInt16 mMinComponentId; LF_STATIC_ASSERT(sizeof(UInt16) == sizeof(ComponentId));
        UInt16 mMaxComponentId; LF_STATIC_ASSERT(sizeof(UInt16) == sizeof(ComponentId));
    };
    using IndexedDefinitionIndex = TVector<DefinitionIndex>;               // List of definitions associated with component
    using IndexedComponentArray  = TVector<IndexedDefinitionIndex>;        // 1:1 Component Id => EntityDefinition[]
    using IndexedEntityCollectionArray = TVector<EntityCollection*>;       // 1:1 Entity Container Id => Container

    using ComponentQuery = TStackVector<ComponentId, 16>;
    using QueryHints = TStackVector<UInt16, 16>;

    using ComponentTypeMap = TMap<const Type*, ComponentPtr>;
    using ComponentSystemArray = TVector<ComponentSystemPtr>;
    using EntityTypeMap = TMap<ComponentSequence, EntityCollectionPtr>;
    using EntityMap = TMap<EntityId, EntityAtomicPtr>;

    using ComponentLockMap = TVector<Atomic32>;

    enum TaskState
    {
        TS_NONE,
        TS_RUNNING,
        TS_FINISHED
    };

    enum SchedulerUpdateType
    {
        SUT_NONE,
        SUT_UPDATE,
        SUT_CONSTANT_UPDATE
    };

    

    struct FenceUpdateData
    {};

    

    struct FenceUpdate : public FenceUpdateData
    {
        ECSUtil::UpdateCallback mUpdateCallback;
        ECSUtil::UpdateType     mUpdateType;
        TVector<ComponentId>     mReadComponents;
        TVector<ComponentId>     mWriteComponents;
        volatile Atomic32       mTaskState;
    };

    struct FenceConstantUpdate : public FenceUpdateData
    {
        Token                   mName;
        ComponentSystem*        mSystem;
        ECSUtil::UpdateCallback mUpdateCallback;
        ECSUtil::UpdateType     mUpdateType;
        TVector<ComponentId>     mReadComponents;
        TVector<ComponentId>     mWriteComponents;
        volatile Atomic32       mTaskState;
    };

    struct SchedulerUpdateData
    {
        SchedulerUpdateData()
        : mType(SUT_NONE)
        , mData(nullptr)
        {}
        SchedulerUpdateData(FenceUpdate* update)
        : mType(update ? SUT_UPDATE : SUT_NONE)
        , mData(update)
        {}
        SchedulerUpdateData(FenceConstantUpdate* update)
            : mType(update ? SUT_CONSTANT_UPDATE : SUT_NONE)
            , mData(update)
        {}
        SchedulerUpdateType mType;
        FenceUpdateData* mData;
    };

    struct FenceData
    {
        FenceData()
        : mType(nullptr)
        , mTargetBefore(nullptr)
        , mTargetAfter(nullptr)
        {}
        explicit FenceData(const Type* type)
        : mType(type)
        , mTargetBefore(nullptr)
        , mTargetAfter(nullptr)
        {}

        const Type* mType;
        const Type* mTargetBefore; // Create the fence before this target
        const Type* mTargetAfter;  // Create the fence after this target.
        TVector<FenceUpdate>         mUpdates;
        TVector<FenceConstantUpdate> mConstantUpdates;
    };

    enum State
    {
        INITIALIZE,
        INITIALIZE_COMPONENT,
        INITIALIZE_SYSTEM,
        INITIALIZE_ENTITY_DEFINITION,
        READY,
        READY_UPDATE_COLLECTIONS,
        READY_UPDATE_SYSTEMS,
        READY_UPDATE_FENCES,
        SHUTDOWN,
        INTERNAL_ERROR
    };

    enum UpdateState : Atomic32
    {
        US_NONE,
        US_REGISTER,
        US_UPDATE,
        US_UNREGISTER
    };

public:
    using Super = World;

    WorldImpl();
    virtual ~WorldImpl();
    ComponentSequence GetSequence(const EntityDefinition* definition);

    // An entity type is defined as a sequence of components (sorted by component id)
    // using EntityCachedTypeMap = TMap<ComponentSequenceHash, EntityCollection>

    // World Initialization:
    // => RegisterComponents
    // => RegisterEntityDefinitions
    //
    // DataChange_RemoveComponent, all instances referencing component must be delete'd, cache updated
    // DataChange_AddComponent, update component list, update caches
    // DataChange_RemoveEntityDefinition, all instances must be delete'd, cache updated
    // DataChange_AddEntityDefinition, update cache if definition adds new collection
    // DataChange_UpdateEntityDefinition, 'unregister','re-register' instances remain intact, cache updated.

    // CacheUpdate: Indexing caches (optimized find) must be updated, System Tuples must re-acquire their tuples

    // ********************************************************************
    // Create a entity with a data-driven type. (must be loaded)
    //
    // Entity lifetime is managed by the World and the scope of the WorldContainer
    // @see Entity Lifetimes
    // @param definition - The definition holding onto all the component types
    // @return Returns the created entity
    // ********************************************************************
    EntityAtomicWPtr CreateEntity(const EntityDefinitionAsset& definition);
    // ********************************************************************
    // Create a entity with a defined set of components.
    //
    // Entity lifetime is managed by the World and the scope of the WorldContainer
    // @see Entity Lifetimes
    // @param definition - The definition holding onto all the component types
    // @return Returns the created entity
    // ********************************************************************
    EntityAtomicWPtr CreateEntity(const EntityDefinition* definition);

    TVector<EntityCollection*> FindCollections(const TVector<const Type*>& includeTypes, const TVector<const Type*>& excludeTypes);

    APIResult<bool> CreateFenceBefore(const Type* fence, const Type* target) override;
    APIResult<bool> CreateFenceAfter(const Type* fence, const Type* target) override;

    APIResult<bool> ScheduleUpdate(const UpdateInfo& info) override;
    APIResult<bool> StartConstantUpdate(const UpdateInfo& info) override;
    APIResult<bool> StopConstantUpdate(const Token& name) override;


    bool IsRegistering() override;
    bool IsUnregistering() override;

    ComponentSystem* GetSystem(const Type* type) override;
    void RegisterScene(const WorldSceneAtomicPtr& scene) override;

protected:
    // Service Impl
    virtual APIResult<ServiceResult::Value> OnStart();
    virtual APIResult<ServiceResult::Value> OnTryInitialize();
    // virtual APIResult<ServiceResult::Value> OnPostInitialize();
    virtual APIResult<ServiceResult::Value> OnBeginFrame();
    virtual APIResult<ServiceResult::Value> OnEndFrame();
    virtual APIResult<ServiceResult::Value> OnFrameUpdate();
    // virtual APIResult<ServiceResult::Value> OnBlockingUpdate(Service* service);
    virtual APIResult<ServiceResult::Value> OnShutdown(ServiceShutdownMode mode);
    // End Service Impl

public: // private:
    void InitializeWorld();
    void ResetWorld();
    void RegisterComponents();
    void RegisterSystems();
    void PreRegisterDefinitions();
    void RegisterInternalUpdates();

    // Definition cannot be null, and must be loaded.
    // Definition cannot contain null types, duplicate types, or types that are not component types.
    // Cannot be called during frame update
    void RegisterEntityDefinition(const EntityDefinitionAsset& definition);
    void UnregisterEntityDefinition(const EntityDefinitionAsset& definition);

    // Register an entity definition for the life-time of the program
    void RegisterStaticEntityDefinition(const EntityDefinition* definition);

    void PrepareIndex();

    EntityAtomicWPtr CreateEntityInternal(const EntityDefinitionAssetType& definitionType, const EntityDefinition* definition);
    void OnUpdateId(EntityId oldId, const Entity* entity);

    // Update the entity id mapping for the world (for find functions)
    void UpdateEntityId(EntityId oldId, const Entity* entity);
    // Update the entity id mapping for the entities collection
    void UpdateEntityCollectionId(EntityId oldId, const Entity* entity);

    // Update:
    // Commit new entities to collection
    // Push unregistered to destroyed
    void UpdateCollections();
    void UpdateSystems();
    void UpdateFences();
    void UpdateFencesSerial();

    void UpdateNewEntities();
    void UpdateRegistered();
    void UpdateUnregistered();

    EntityAtomicWPtr FindEntity(EntityId id);
    EntityAtomicWPtr FindNewEntity(EntityId id);
    EntityAtomicWPtr FindRegistered(EntityId id);
    EntityAtomicWPtr FindUnregistered(EntityId id);

    std::pair<EntityId, EntityAtomicWPtr> FindEntitySlow(EntityId id);

    SizeT GetFenceIndex(const Type* target);

public:
    // Config Accessors
    bool LogEntityIdChanges() const;
    bool LogEntityAddRemove() const;
    bool LogFenceUpdate() const;
    bool LogFenceUpdateVerbose() const;
    // \ Config Accessors
private:
    bool AllowUpdateScheduling();
    bool AllowFenceCreation();
    bool IsBuiltInFence(const Type* type);
    FenceData* GetFence(const Type* type);
    FenceData* GetFence(const Token& updateName);
    ComponentQuery ToQuery(const TVector<const Type*>& types);
    TVector<EntityCollection*> FindCollections(const ComponentQuery& includeTypes, const ComponentQuery& excludeTypes);

    void ScanInclude(const ComponentQuery& query, Int16* resultBuffer, SizeT& numHits, QueryHints& hints);
    void ScanExclude(const ComponentQuery& query, Int16* resultBuffer, SizeT& numHits);

    bool AcquireLock(bool read, const TVector<ComponentId>& components);
    bool ReleaseLock(bool read, const TVector<ComponentId>& components);

    bool ExecuteNonSerialUpdate(FenceConstantUpdate* updateData);
    bool ExecuteNonSerialUpdate(FenceUpdate* updateData);

    bool                            mForceUpdateSerial;

    ComponentLockMap                mReadComponents;
    ComponentLockMap                mWriteComponents;
    ComponentTypeMap                mComponentTypes;
    ComponentSystemArray            mSystems;
    EntityTypeMap                   mCollections;
    EntityMap                       mEntities;

    TVector<EntityAtomicPtr>         mNewEntities;
    TVector<EntityAtomicPtr>         mRegisteringEntities;
    TVector<EntityAtomicPtr>         mUnregisteringEntities;

    State                           mState;
    UpdateState                     mUpdateState;

    IndexedComponentArray        mIndexedComponents;
    IndexedEntityCollectionArray mIndexedCollections;
    bool                         mIndexDirty;
    bool                         mRebindNextUpdate;

    UniqueNumber<EntityId, 64>   mEntityIDGen;

    TVector<FenceData>           mFences;
    TVector<FenceData>           mUnsortedFences;
    TVector<const Type*>         mBuiltInFences;

    AppService*                 mAppService;

    TVector<WorldSceneAtomicPtr> mScenes;
};

} // namespace lf