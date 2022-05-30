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
#include "Engine/PCH.h"
#include "WorldImpl.h"

#include "Core/Utility/Error.h"
#include "Runtime/Async/Async.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "AbstractEngine/App/AppService.h"
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/Entity.h"
#include "AbstractEngine/World/EntityCollection.h"
#include "AbstractEngine/World/WorldScene.h"

#include <algorithm>


#include "Core/Utility/Log.h"

namespace lf
{

constexpr SizeT MAX_COMPONENT = std::numeric_limits<ComponentId>::max();
constexpr SizeT MAX_COLLECTION = std::numeric_limits<UInt16>::max();
DEFINE_CLASS(lf::WorldImpl) { NO_REFLECTION; }
DEFINE_CLASS(lf::WorldConfig) { NO_REFLECTION; }

WorldConfig::WorldConfig()
: Super()
, mLogEntityIdChanges(false)
, mLogEntityAddRemove(false)
, mLogFenceUpdate(false)
, mLogFenceUpdateVerbose(false)
{

}
void WorldConfig::Serialize(Stream& s)
{
    SERIALIZE(s, mLogEntityIdChanges, "");
    SERIALIZE(s, mLogEntityAddRemove, "");
    SERIALIZE(s, mLogFenceUpdate, "");
    SERIALIZE(s, mLogFenceUpdateVerbose, "");
}

static bool IsSerialUpdate(ECSUtil::UpdateType updateType)
{
    return updateType == ECSUtil::UpdateType::SERIAL || updateType == ECSUtil::UpdateType::SERIAL_DISTRIBUTED;
}

static bool CheckDupes(const TVector<const Type*>& components)
{
    for(SizeT i = 0; i < components.size(); ++i)
    {
        for (SizeT k = 0; k < components.size(); ++k)
        {
            if (i == k)
            {
                continue;
            }

            if (components[i] == components[k])
            {
                return false;
            }
        }
    }
    return true;
}

static bool VerifyReadWriteComponents(const TVector<const Type*>& readComponents, const TVector<const Type*>& writeComponents)
{
    if (!CheckDupes(readComponents) || !CheckDupes(writeComponents))
    {
        return false;
    }
    // Exclusive access

    for (const Type* type : readComponents)
    {
        if (writeComponents.end() != std::find(writeComponents.begin(), writeComponents.end(), type))
        {
            gSysLog.Info(LogMessage("Duplicate type=") << type->GetFullName());
            return false;
        }
    }
    return true;

}

WorldImpl::WorldImpl()
: Super()
, mForceUpdateSerial(false)
, mReadComponents()
, mWriteComponents()
, mComponentTypes()
, mSystems()
, mCollections()
, mEntities()
, mRegisteringEntities()
, mUnregisteringEntities()
, mState(INITIALIZE)
, mUpdateState(US_NONE)
, mIndexedComponents()
, mIndexedCollections()
, mIndexDirty(true)
, mRebindNextUpdate(true)
, mEntityIDGen()
, mFences()
, mUnsortedFences()
, mBuiltInFences()
, mAppService(nullptr)
{

}
WorldImpl::~WorldImpl()
{
    ResetWorld(); // Destruction Order is kinda important
}

ComponentSequence WorldImpl::GetSequence(const EntityDefinition* definition)
{
    ComponentSequence sequence;
    if (!definition || definition->GetComponentTypes().empty())
    {
        return ComponentSequence();
    }
    for (const Type* type : definition->GetComponentTypes())
    {
        if (!type)
        {
            return ComponentSequence();
        }

        auto it = mComponentTypes.find(type);
        if (it == mComponentTypes.end())
        {
            return ComponentSequence();
        }
        sequence.push_back(it->second->GetId());
    }
    std::sort(sequence.begin(), sequence.end());
    return sequence;
}

EntityAtomicWPtr WorldImpl::CreateEntity(const EntityDefinitionAsset& definition)
{
    if (!definition || !definition.IsLoaded())
    {
        return EntityAtomicWPtr();
    }
    return CreateEntityInternal(definition, definition.GetPrototype());
}

EntityAtomicWPtr WorldImpl::CreateEntity(const EntityDefinition* definition)
{
    return CreateEntityInternal(EntityDefinitionAssetType(), definition);
}

TVector<EntityCollection*> WorldImpl::FindCollections(const TVector<const Type*>& includeTypes, const TVector<const Type*>& excludeTypes)
{
    ComponentQuery includeQuery = ToQuery(includeTypes);
    ComponentQuery excludeQuery = ToQuery(excludeTypes);
    if (includeQuery.size() != includeTypes.size())
    {
        return TVector<EntityCollection*>();
    }

    if (excludeQuery.size() != excludeTypes.size())
    {
        return TVector<EntityCollection*>();
    }
    return FindCollections(includeQuery, excludeQuery);
}

APIResult<bool> WorldImpl::CreateFenceBefore(const Type* fence, const Type* target)
{
    if (!AllowFenceCreation())
    {
        return ReportError(false, OperationFailureError, "Fences cannot be created at this time, create them during ComponentSystem::OnInitialize", "<NONE>");
    }

    if (!fence)
    {
        return ReportError(false, ArgumentNullError, "fence");
    }
    if (!target)
    {
        return ReportError(false, ArgumentNullError, "target");
    }
    if (!fence->IsA(typeof(ComponentSystemFence)))
    {
        return ReportError(false, InvalidArgumentError, "fence", "Must be of type ComponentSystemFence");
    }
    if (!target->IsA(typeof(ComponentSystemFence)))
    {
        return ReportError(false, InvalidArgumentError, "target", "Must be of type ComponentSystemFence");
    }
    if (IsBuiltInFence(fence))
    {
        return ReportError(false, InvalidArgumentError, "fence", "Cannot register built-in ComponentSystemRegisterFence");
    }
    if (mUnsortedFences.end() != std::find_if(mUnsortedFences.begin(), mUnsortedFences.end(), [fence](const FenceData& data) { return data.mType == fence; }))
    {
        return ReportError(false, OperationFailureError, "Fence already exists.", fence->GetFullName().CStr());
    }

    FenceData fenceData(fence);
    fenceData.mTargetBefore = target;
    mUnsortedFences.push_back(fenceData);

    return APIResult<bool>(true);
}
APIResult<bool> WorldImpl::CreateFenceAfter(const Type* fence, const Type* target)
{
    if (!AllowFenceCreation())
    {
        return ReportError(false, OperationFailureError, "Fences cannot be created at this time, create them during ComponentSystem::OnInitialize", "<NONE>");
    }
    if (!fence)
    {
        return ReportError(false, ArgumentNullError, "fence");
    }
    if (!target)
    {
        return ReportError(false, ArgumentNullError, "target");
    }
    if (!fence->IsA(typeof(ComponentSystemFence)))
    {
        return ReportError(false, InvalidArgumentError, "fence", "Must be of type ComponentSystemFence");
    }
    if (!target->IsA(typeof(ComponentSystemFence)))
    {
        return ReportError(false, InvalidArgumentError, "target", "Must be of type ComponentSystemFence");
    }
    if (IsBuiltInFence(fence))
    {
        return ReportError(false, InvalidArgumentError, "fence", "Cannot register built-in fence.");
    }
    if (mUnsortedFences.end() != std::find_if(mUnsortedFences.begin(), mUnsortedFences.end(), [fence](const FenceData& data) { return data.mType == fence; }))
    {
        return ReportError(false, OperationFailureError, "Fence already exists.", fence->GetFullName().CStr());
    }

    FenceData fenceData(fence);
    fenceData.mTargetAfter = target;
    mUnsortedFences.push_back(fenceData);

    return APIResult<bool>(true);
}

APIResult<bool> WorldImpl::ScheduleUpdate(const UpdateInfo& info)
{
    if (!AllowUpdateScheduling())
    {
        return ReportError(false, OperationFailureError, "Failed to schedule update, operation can only be completed while update scheduling is permitted.", info.mName.CStr());
    }
    if (!info.mUpdateCallback.IsValid())
    {
        return ReportError(false, InvalidArgumentError, "info.mUpdateCallback", "The update callback must be a valid callback.");
    }
    if (info.mFenceType == nullptr)
    {
        return ReportError(false, ArgumentNullError, "info.mFenceType");
    }
    if (InvalidEnum(info.mUpdateType))
    {
        return ReportError(false, InvalidArgumentError, "info.mUpdateType", "Invalid enum");
    }

    FenceData* fence = GetFence(info.mFenceType);
    if (!fence)
    {
        return ReportError(false, OperationFailureError, "Failed to find the fence for scheduled update. (Is the fence not registered? See CreateFenceBefore/CreateFenceAfter)", info.mName.CStr());
    }

    if (!VerifyReadWriteComponents(info.mReadComponents, info.mWriteComponents))
    {
        return ReportError(false, InvalidArgumentError, "info.mReadComponents,info.mWriteComponents", "There can be no duplicates. CheckDupes(Read+Write) == False");
    }

    FenceUpdate update;
    update.mUpdateCallback = info.mUpdateCallback;
    update.mUpdateType = info.mUpdateType;
    for (const Type* type : info.mReadComponents)
    {
        auto it = mComponentTypes.find(type);
        if (it == mComponentTypes.end())
        {
            return ReportError(false, InvalidArgumentError, "info.mReadComponents", "Types are expected to be a component type.");
        }
        update.mReadComponents.push_back(it->second->GetId());
    }
    for (const Type* type : info.mWriteComponents)
    {
        auto it = mComponentTypes.find(type);
        if (it == mComponentTypes.end())
        {
            return ReportError(false, InvalidArgumentError, "info.mWriteComponents", "Types are expected to be a component type.");
        }
        update.mWriteComponents.push_back(it->second->GetId());
    }

    fence->mUpdates.push_back(update);
    return APIResult<bool>(true);
}

APIResult<bool> WorldImpl::StartConstantUpdate(const UpdateInfo& info)
{
    if (!AllowUpdateScheduling())
    {
        return ReportError(false, OperationFailureError, "Failed to schedule constant update, operation can only be completed while update scheduling is permitted.", info.mName.CStr());
    }
    if (info.mName.Empty())
    {
        return ReportError(false, InvalidArgumentError, "info.mName", "Constant updates require a unique update name.");
    }
    if (!info.mSystem)
    {
        return ReportError(false, ArgumentNullError, "info.mSystem");
    }
    if (!info.mUpdateCallback.IsValid())
    {
        return ReportError(false, InvalidArgumentError, "info.mUpdateCallback", "The update callback must be a valid callback.");
    }
    if (info.mFenceType == nullptr)
    {
        return ReportError(false, ArgumentNullError, "info.mFenceType");
    }
    if (InvalidEnum(info.mUpdateType))
    {
        return ReportError(false, InvalidArgumentError, "info.mUpdateType", "Invalid enum");
    }

    FenceData* fence = GetFence(info.mFenceType);
    if (!fence)
    {
        return ReportError(false, OperationFailureError, "Failed to find the fence for constant update. (Is the fence not registered? See CreateFenceBefore/CreateFenceAfter)", info.mName.CStr());
    }

    if (!VerifyReadWriteComponents(info.mReadComponents, info.mWriteComponents))
    {
        return ReportError(false, InvalidArgumentError, "info.mReadComponents,info.mWriteComponents", "Nonsensical types provided in the Read/Write component type lists. Ensure a type is referenced once between both lists. ");
    }

    FenceConstantUpdate update;
    update.mName = info.mName;
    update.mSystem = info.mSystem;
    update.mUpdateCallback = info.mUpdateCallback;
    update.mUpdateType = info.mUpdateType;
    for (const Type* type : info.mReadComponents)
    {
        auto it = mComponentTypes.find(type);
        if (it == mComponentTypes.end())
        {
            return ReportError(false, InvalidArgumentError, "info.mReadComponents", "Types are expected to be a component type.");
        }
        update.mReadComponents.push_back(it->second->GetId());
    }
    for (const Type* type : info.mWriteComponents)
    {
        auto it = mComponentTypes.find(type);
        if (it == mComponentTypes.end())
        {
            return ReportError(false, InvalidArgumentError, "info.mWriteComponents", "Types are expected to be a component type.");
        }
        update.mWriteComponents.push_back(it->second->GetId());
    }
    update.mTaskState = TS_NONE;
    
    fence->mConstantUpdates.push_back(update);
    // update.
    return APIResult<bool>(true);
}

APIResult<bool> WorldImpl::StopConstantUpdate(const Token& name)
{
    if (!AllowUpdateScheduling())
    {
        return ReportError(false, OperationFailureError, "Failed to stop a constant update, operation can only be completed while update scheduling is permitted.", name.CStr());
    }

    if (name.Empty())
    {
        return ReportError(false, InvalidArgumentError, "name", "Stopping a constant update requires a name", name.CStr());
    }

    FenceData* fence = GetFence(name);
    if (!fence)
    {
        return ReportError(false, OperationFailureError, "Failed to find the fence to stop the constant update. (Is it not registered?)", name.CStr());
    }
    
    for (auto it = fence->mConstantUpdates.begin(); it != fence->mConstantUpdates.end(); ++it)
    {
        if (it->mName == name)
        {
            fence->mConstantUpdates.swap_erase(it);
            return APIResult<bool>(true);
        }
    }
    return APIResult<bool>(false);
}

bool WorldImpl::IsRegistering()
{
    return TAtomicLoad(&mUpdateState) == US_REGISTER;
}

bool WorldImpl::IsUnregistering()
{
    return TAtomicLoad(&mUpdateState) == US_UNREGISTER;
}

ComponentSystem* WorldImpl::GetSystem(const Type* type)
{
    for (ComponentSystem* system : mSystems)
    {
        if (system->GetType() == type)
        {
            return system;
        }
    }

    return nullptr;
}

void WorldImpl::RegisterScene(const WorldSceneAtomicPtr& scene)
{
    mScenes.push_back(scene);
}

APIResult<ServiceResult::Value> WorldImpl::OnStart()
{
    APIResult<ServiceResult::Value> super = Super::OnStart();
    if (super.GetItem() == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return super;
    }

    mAppService = GetServices()->GetService<AppService>();

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> WorldImpl::OnTryInitialize()
{
    APIResult<ServiceResult::Value> super = Super::OnTryInitialize();
    if (super.GetItem() == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return super;
    }
    InitializeWorld();
    
    // TODO: If we're running this as a standalone game, we should 'shutdown' as 
    // a user will not be able to correct/fix problems. But if we're in editor mode
    // then allow it to pass.

    return APIResult<ServiceResult::Value>(ServiceResult::Combine(super, ServiceResult::SERVICE_RESULT_SUCCESS));
}

APIResult<ServiceResult::Value> WorldImpl::OnBeginFrame()
{
    APIResult<ServiceResult::Value> super = Super::OnBeginFrame();
    if (super.GetItem() != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return super;
    }

    if (mState == INITIALIZE)
    {
        InitializeWorld();
    }

    if (mState == READY)
    {
        UpdateNewEntities();
        UpdateSystems();
    }


    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> WorldImpl::OnEndFrame()
{
    APIResult<ServiceResult::Value> super = Super::OnEndFrame();
    if (super.GetItem() != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return super;
    }
    UpdateUnregistered();
    UpdateCollections();

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> WorldImpl::OnFrameUpdate()
{
    APIResult<ServiceResult::Value> super = Super::OnFrameUpdate();
    if (super.GetItem() != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return super;
    }

    TAtomicStore(&mUpdateState, US_REGISTER);
    UpdateFences();
    TAtomicStore(&mUpdateState, US_NONE);

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> WorldImpl::OnShutdown(ServiceShutdownMode mode)
{
    APIResult<ServiceResult::Value> super = Super::OnShutdown(mode);
    if (super.GetItem() != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return super;
    }

    ResetWorld();

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

void WorldImpl::InitializeWorld()
{
    Assert(mState == INITIALIZE);
    ResetWorld();

    mState = INITIALIZE_COMPONENT;
    RegisterComponents();
    if (mState == INTERNAL_ERROR)
    {
        ResetWorld();
        return;
    }

    mState = INITIALIZE_SYSTEM;
    RegisterSystems();
    if (mState == INTERNAL_ERROR)
    {
        ResetWorld();
        return;
    }

    mState = INITIALIZE_ENTITY_DEFINITION;
    PreRegisterDefinitions();
    if (mState == INTERNAL_ERROR)
    {
        ResetWorld();
        return;
    }
    mState = READY_UPDATE_SYSTEMS;
    RegisterInternalUpdates();
    if (mState == INTERNAL_ERROR)
    {
        ResetWorld();
        return;
    }
    mState = READY;
}

void WorldImpl::ResetWorld()
{
    // Assert(IsShutdown())
    mState = SHUTDOWN;

    mEntities.clear();
    mNewEntities.clear();
    mRegisteringEntities.clear();
    mUnregisteringEntities.clear();
    
    mSystems.clear();

    mReadComponents.clear();
    mWriteComponents.clear();
    mComponentTypes.clear();

    mFences.clear();
    mBuiltInFences.clear();
    mUnsortedFences.clear();

    mCollections.clear();

    mRebindNextUpdate = true;
    mIndexDirty = true;
    mIndexedCollections.clear();
    mIndexedComponents.clear();

    mState = INITIALIZE;
}

void WorldImpl::RegisterComponents()
{
    mComponentTypes.clear();

    TVector<const Type*> componentTypes = GetReflectionMgr().FindAll(typeof(Component));
    gSysLog.Info(LogMessage("World targeting ") << componentTypes.size() << " component types...");
    Assert(componentTypes.size() < MAX_COMPONENT);

    for (const Type* type : componentTypes)
    {
        if (type == typeof(Component) || type->IsAbstract())
        {
            continue;
        }

        ComponentPtr component = GetReflectionMgr().Create<Component>(type);
        if (component)
        {
            mComponentTypes.insert(std::make_pair(type, component));
            component->SetId(static_cast<ComponentId>(mComponentTypes.size()));
            gSysLog.Info(LogMessage("Register component ") << type->GetFullName() << " with ID " << component->GetId());
        }
    }

    // Create mapping for 'locks'
    if (!mComponentTypes.empty())
    {
        mReadComponents.resize(mComponentTypes.size() + 1);
        memset(mReadComponents.data(), 0, sizeof(Atomic32) * mReadComponents.size());
        mWriteComponents.resize(mComponentTypes.size() + 1);
        memset(mWriteComponents.data(), 0, sizeof(Atomic32) * mWriteComponents.size());
    }
}

void WorldImpl::RegisterSystems()
{
    // Setup the builtin fences.
    mFences.clear();
    mFences.push_back(FenceData(typeof(ComponentSystemRegisterFence)));
    mFences.push_back(FenceData(typeof(ComponentSystemUpdateFence)));
    mFences.push_back(FenceData(typeof(ComponentSystemUnregisterFence)));

    for (const FenceData& fence : mFences)
    {
        mBuiltInFences.push_back(fence.mType);
    }

    // Create and register systems.
    mSystems.clear();

    TVector<const Type*> systemTypes = GetReflectionMgr().FindAll(typeof(ComponentSystem));
    gSysLog.Info(LogMessage("Targeting ") << systemTypes.size() << " system types.");

    for (const Type* type : systemTypes)
    {
        if (type == typeof(ComponentSystem) || type->IsAbstract())
        {
            continue;
        }

        ComponentSystemPtr system = GetReflectionMgr().Create<ComponentSystem>(type);
        if (system && system->IsEnabled())
        {
            mSystems.push_back(system);
        }
    }

    bool systemError = false;

    // Initialize the systems.
    for (ComponentSystem* system : mSystems)
    {
        if (!system->Initialize(this))
        {
            systemError = true;
        }
    }

    if (systemError)
    {
        mState = INTERNAL_ERROR;
        return;
    }

    bool fenceError = false;
    // Ensure all fences exist.
    for (auto fence = mUnsortedFences.begin(); fence != mUnsortedFences.end(); ++fence)
    {
        if (!fence->mType)
        {
            gSysLog.Error(LogMessage("Invalid fence was registered! Missing type!"));
            continue;
        }

        if (!fence->mTargetAfter && !fence->mTargetBefore)
        {
            gSysLog.Error(LogMessage("Invalid fence was registered! Missing target!") << fence->mType->GetFullName());
            fenceError = true;
            continue;
        }

        if (IsBuiltInFence(fence->mTargetAfter) || IsBuiltInFence(fence->mTargetBefore))
        {
            continue;
        }

        const Type* targetType = fence->mTargetAfter ? fence->mTargetAfter : fence->mTargetBefore;
        auto findBy = [targetType](const FenceData& current)
        {
            return current.mType == targetType;
        };

        if (mUnsortedFences.end() == std::find_if(mUnsortedFences.begin(), mUnsortedFences.end(), findBy))
        {
            gSysLog.Error(LogMessage("Missing target fence. Fence=") << fence->mType->GetFullName() << ", Target=" << targetType->GetFullName());
            fenceError = true;
            continue;
        }

    }

    if (fenceError)
    {
        mState = INTERNAL_ERROR;
        return;
    }

    // Sort/Insert the fences.
    while (!mUnsortedFences.empty())
    {
        for (auto fence = mUnsortedFences.begin(); fence != mUnsortedFences.end();)
        {
            const Type* targetType = fence->mTargetAfter ? fence->mTargetAfter : fence->mTargetBefore;
            auto findBy = [targetType](const FenceData& current)
            {
                return current.mType == targetType;
            };

            auto target = std::find_if(mFences.begin(), mFences.end(), findBy);
            if (target != mFences.end())
            {
                if (fence->mTargetAfter)
                {
                    ++target;
                }
                mFences.insert(target, *fence);
                fence = mUnsortedFences.swap_erase(fence);
            }
            else
            {
                ++fence;
            }
        }
    }

    // Very correctness
    for (auto fence = mFences.begin(); fence != mFences.end(); ++fence)
    {
        if (IsBuiltInFence(fence->mType))
        {
            continue;
        }

        const Type* target = fence->mTargetAfter ? fence->mTargetAfter : fence->mTargetBefore;
        auto targetIt = std::find_if(mFences.begin(), mFences.end(), [target](const FenceData& fence) { return fence.mType == target; });
        Assert(targetIt != mFences.end());
        Assert(fence != targetIt);

        if (fence->mTargetAfter)
        {
            if (fence < targetIt)
            {
                gSysLog.Error(LogMessage("Fence out of order, possible conflict? Fence=") << fence->mType->GetFullName());
                fenceError = true;
            }
        }
        else
        {
            if (fence > targetIt)
            {
                gSysLog.Error(LogMessage("Fence out of order, possible conflict? Fence=") << fence->mType->GetFullName());
                fenceError = true;
            }
        }
    }

    if (fenceError) 
    {
        mState = INTERNAL_ERROR;
    }

}

void WorldImpl::PreRegisterDefinitions()
{
    TVector<AssetTypeInfoCPtr> types = GetAssetMgr().GetTypes(typeof(EntityDefinition));
    gSysLog.Info(LogMessage("World targeting ") << types.size() << "  entity definition types...");

    for (const AssetTypeInfo* type : types)
    {
        if (type->IsConcrete())
        {
            continue;
        }

        EntityDefinitionAsset definition(type, AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
        if (definition && definition.IsLoaded())
        {
            if (!definition->GetComponentTypes().empty())
            {
                RegisterEntityDefinition(definition);
            }
            else
            {
                gSysLog.Warning(LogMessage("Skipping empty EntityDefinition ") << type->GetPath().AsToken());
            }
        }
        else
        {
            gSysLog.Warning(LogMessage("Failed to load EntityDefinition ") << type->GetPath().AsToken());
        }
    }
}

void WorldImpl::RegisterInternalUpdates()
{
    FenceConstantUpdate update;
    update.mSystem = nullptr;
    update.mTaskState = TS_NONE;
    update.mUpdateType = ECSUtil::UpdateType::SERIAL;

    FenceData* fence = GetFence(typeof(ComponentSystemRegisterFence));
    Assert(fence != nullptr);
    update.mName = Token("WorldImpl.RegisterEntities");
    update.mUpdateCallback = ECSUtil::UpdateCallback::Make(this, &WorldImpl::UpdateRegistered);
    fence->mConstantUpdates.push_back(update);

    fence = GetFence(typeof(ComponentSystemUnregisterFence));
    Assert(fence != nullptr);
    update.mName = Token("WorldImpl.UnregisterEntities");
    update.mUpdateCallback = ECSUtil::UpdateCallback::Make([this]() { TAtomicStore(&mUpdateState, US_UNREGISTER); });
    fence->mConstantUpdates.push_back(update);
}

void WorldImpl::RegisterEntityDefinition(const EntityDefinitionAsset& definition)
{
    if (!definition && !definition.IsLoaded())
    {
        return;
    }

    const TVector<const Type*>& types = definition->GetComponentTypes();
    TVector<const Component*> components;
    ComponentSequence sequence;
    // Validate
    for (const Type* type : types)
    {
        if (type == nullptr)
        {
            gSysLog.Error(LogMessage("Failed to RegisterEntityDefinition for type ") << definition.GetPath().AsToken() << ", because it contains a null component type.");
            return;
        }
        auto it = mComponentTypes.find(type);
        if (it == mComponentTypes.end())
        {
            gSysLog.Error(LogMessage("Failed to RegisterEntityDefinition for type ") << definition.GetPath().AsToken() << ", because it contains a invalid component type " << type->GetFullName());
            return;
        }
        if (sequence.end() != std::find(sequence.begin(), sequence.end(), it->second->GetId()))
        {
            gSysLog.Error(LogMessage("Failed to RegisterEntityDefinition for type ") << definition.GetPath().AsToken() << ", because it contains a duplicate component type " << type->GetFullName());
            return;
        }

        components.push_back(it->second);
        sequence.push_back(it->second->GetId());
    }
    // GenerateSequence( types )
    std::sort(sequence.begin(), sequence.end());
    EntityCollectionPtr& collection = mCollections[sequence];
    Assert(mCollections.size() <= MAX_COLLECTION); // If this trips, consider changing the data types...
    if (!collection)
    {
        collection = EntityCollectionPtr(LFNew<EntityCollection>());
    }
    collection->Initialize(definition, components);

    mIndexDirty = true;
    mRebindNextUpdate = true;

}

void WorldImpl::UnregisterEntityDefinition(const EntityDefinitionAsset& definition)
{
    if (!definition && !definition.IsLoaded())
    {
        return;
    }

    ComponentSequence sequence = GetSequence(definition.GetPrototype());
    auto collection = mCollections.find(sequence);
    if (collection == mCollections.end())
    {
        return;
    }

    collection->second->Release(definition);
    if (collection->second->Empty())
    {
        // TODO: Mark Garbage
    }
}

void WorldImpl::RegisterStaticEntityDefinition(const EntityDefinition* definition)
{
    if (!definition || definition->GetComponentTypes().empty())
    {
        return;
    }

    const TVector<const Type*>& types = definition->GetComponentTypes();
    TVector<const Component*> components;
    ComponentSequence sequence;
    // Validate
    for (const Type* type : types)
    {
        if (type == nullptr)
        {
            gSysLog.Error(LogMessage("Failed RegisterStaticEntityDefinition, because it contains a null component type."));
            return;
        }
        auto it = mComponentTypes.find(type);
        if (it == mComponentTypes.end())
        {
            gSysLog.Error(LogMessage("Failed RegisterStaticEntityDefinition because it contains a invalid component type ") << type->GetFullName());
            return;
        }
        if (sequence.end() != std::find(sequence.begin(), sequence.end(), it->second->GetId()))
        {
            gSysLog.Error(LogMessage("Failed RegisterStaticEntityDefinition because it contains a duplicate component type ") << type->GetFullName());
            return;
        }

        components.push_back(it->second);
        sequence.push_back(it->second->GetId());
    }
    // GenerateSequence( types )
    std::sort(sequence.begin(), sequence.end());
    EntityCollectionPtr& collection = mCollections[sequence];
    Assert(mCollections.size() <= MAX_COLLECTION); // If this trips, consider changing the data types...
    if (!collection)
    {
        collection = EntityCollectionPtr(LFNew<EntityCollection>());
    }
    collection->Initialize(EntityDefinitionAsset(), components);

    mIndexDirty = true;
    mRebindNextUpdate = true;
}

void WorldImpl::PrepareIndex()
{
    mIndexedCollections.clear();
    mIndexedComponents.clear();

    UInt16 id = 0;

    mIndexedComponents.resize(mComponentTypes.size() + 1); // +1 for max component id (0 is reserved)
    mIndexedCollections.resize(mCollections.size());
    for (const auto& pair : mCollections)
    {
        mIndexedCollections[id] = pair.second;
        
        UInt16 componentCount = static_cast<UInt16>(pair.first.size());
        UInt16 minComponent = pair.first.front();
        UInt16 maxComponent = pair.first.back();

        for (UInt16 componentId : pair.first)
        {
            mIndexedComponents[componentId].push_back({ id, componentCount, minComponent, maxComponent });
        }
        ++id;
    }
    mIndexDirty = false;
}

EntityAtomicWPtr WorldImpl::CreateEntityInternal(const EntityDefinitionAssetType& definitionType, const EntityDefinition* definition)
{
    if (!definition || definition->GetComponentTypes().empty())
    {
        return EntityAtomicWPtr(); // Invalid definition
    }
    ComponentSequence sequence = GetSequence(definition);
    auto collection = mCollections.find(sequence);
    if (collection == mCollections.end())
    {
        return EntityAtomicWPtr(); // Definition not loaded, call RegisterEntityDefinition!
    }
    EntityId id = mEntityIDGen.Allocate();
    EntityId maskedId = (id & ECSUtil::ENTITY_ID_BITMASK);
    if (maskedId != id)
    {
        mEntityIDGen.Free(id);
        return EntityAtomicWPtr(); // Reached max entity!
    }

    // no flags should be set at this point!
    Assert((id & ECSUtil::ENTITY_FLAG_BITMASK) == 0);

    ECSUtil::SetNormalPriority(id); // noop
    ECSUtil::SetRegister(id); // noop

    // Allocate the entity and their components!
    collection->second->CreateEntity(id);

    EntityAtomicPtr entity = MakeConvertibleAtomicPtr<Entity>();
    entity->SetType(typeof(Entity));

    EntityInitializeData initData;
    initData.mID = id;
    initData.mCollection = collection->second;
    initData.mWorld = this;
    initData.mDefinition = definitionType;
    initData.mUpdateIDCallback = Entity::UpdateIdCallback::Make(this, &WorldImpl::OnUpdateId);

    entity->PreInit(initData);

    // New entities are added next frame
    // New entities are registered.
    mNewEntities.push_back(entity);
    return entity;
}

void WorldImpl::OnUpdateId(EntityId oldId, const Entity* entity)
{
    if (mState == SHUTDOWN)
    {
        return;
    }
    UpdateEntityCollectionId(oldId, entity);
    UpdateEntityId(oldId, entity);
}

void WorldImpl::UpdateEntityId(EntityId oldId, const Entity* entity)
{
    if (Invalid(oldId))
    {
        ReportError(0, InvalidArgumentError, "oldId", "Must have a valid entity id.");
        return;
    }
    if (!entity)
    {
        ReportError(0, ArgumentNullError, "entity");
        return;
    }
    if (ECSUtil::GetId(oldId) != ECSUtil::GetId(entity->GetId()))
    {
        ReportError(0, InvalidArgumentError, "oldId", "oldId does not match id of entity.");
        return;
    }

    auto it = mEntities.end();
    if (ECSUtil::IsLifeChanged(oldId, entity->GetId()))
    {
        ECSUtil::EntityLifeState oldLifeState = ECSUtil::GetLifeState(oldId);
        ECSUtil::EntityLifeState newLifeState = ECSUtil::GetLifeState(entity->GetId());

        if (LogEntityIdChanges())
        {
            gSysLog.Info(LogMessage("Update Life State: ID=") << ECSUtil::GetId(oldId)
                << ", Old=" << ECSUtil::TEntityLifeState::GetString(oldLifeState)
                << ", New=" << ECSUtil::TEntityLifeState::GetString(newLifeState));
        }

        switch (oldLifeState)
        {
        case ECSUtil::EntityLifeState::REGISTER:
        {
            switch (newLifeState)
            {
            case ECSUtil::EntityLifeState::ALIVE: // no-op
                it = mEntities.find(oldId);
                Assert(it != mEntities.end());
                mEntities.insert(std::make_pair(entity->GetId(), GetAtomicPointer(entity)));
                mEntities.erase(it);
                return;
            case ECSUtil::EntityLifeState::DESTROYED: // no-op
                return;
            case ECSUtil::EntityLifeState::UNREGISTER:
                it = mEntities.find(oldId);
                Assert(it != mEntities.end());
                mEntities.insert(std::make_pair(entity->GetId(), GetAtomicPointer(entity)));
                mEntities.erase(it);
                mUnregisteringEntities.push_back(GetAtomicPointer(entity));
                return;
            case ECSUtil::EntityLifeState::REGISTER:
                AssertMsg("Invalid state transition!");
                return;
            default:
                CriticalAssertMsg("Invalid life state!");
                return;
            }
        } break;
        case ECSUtil::EntityLifeState::ALIVE:
        {
            switch (newLifeState)
            {
            case ECSUtil::EntityLifeState::UNREGISTER:
                it = mEntities.find(oldId);
                Assert(it != mEntities.end());
                mEntities.insert(std::make_pair(entity->GetId(), GetAtomicPointer(entity)));
                mEntities.erase(it);
                mUnregisteringEntities.push_back(GetAtomicPointer(entity));
                return;
            case ECSUtil::EntityLifeState::REGISTER:
            case ECSUtil::EntityLifeState::ALIVE:
            case ECSUtil::EntityLifeState::DESTROYED:
                AssertMsg("Invalid state transition!");
                return;
            default:
                CriticalAssertMsg("Invalid life state!");
                return;
            }
        } break;
        case ECSUtil::EntityLifeState::UNREGISTER:
        {
            switch (newLifeState)
            {
            case ECSUtil::EntityLifeState::DESTROYED: // no-op
                it = mEntities.find(oldId);
                Assert(it != mEntities.end()); // Entity should be registered!
                mEntities.erase(it);
                return;
            case ECSUtil::EntityLifeState::REGISTER:
            case ECSUtil::EntityLifeState::ALIVE:
            case ECSUtil::EntityLifeState::UNREGISTER:
                AssertMsg("Invalid state transition!");
                return;
            default:
                CriticalAssertMsg("Invalid life state!");
                return;
            }
        } break;
        case ECSUtil::EntityLifeState::DESTROYED:
        {
            switch (newLifeState)
            {
            case ECSUtil::EntityLifeState::REGISTER:
            case ECSUtil::EntityLifeState::ALIVE:
            case ECSUtil::EntityLifeState::UNREGISTER:
            case ECSUtil::EntityLifeState::DESTROYED:
                AssertMsg("Invalid state transition!");
                return;
            default:
                CriticalAssertMsg("Invalid life state!");
                return;
            }
        } break;
        default:
            CriticalAssertMsg("Invalid life state!");
            return;

        }
        return;
    }

    it = mEntities.find(oldId);
    if (it != mEntities.end())
    {
        if (it->second != entity)
        {
            gSysLog.Info(LogMessage("Entity id mismatch! Id=") << oldId);
            return;
        }
        Assert(mEntities.find(entity->GetId()) == mEntities.end());
        mEntities.insert(std::make_pair(entity->GetId(), it->second));
        mEntities.erase(it);
    }
    else
    {
        ECSUtil::EntityLifeState lifeState = ECSUtil::GetLifeState(entity->GetId());
        gSysLog.Error(LogMessage("Failed to update entity mapping, id could not be found for entity. Id=") << oldId << ", LifeState=" << ECSUtil::TEntityLifeState::GetString(lifeState));
    }
}
void WorldImpl::UpdateEntityCollectionId(EntityId oldId, const Entity* entity)
{
    const_cast<EntityCollection*>(entity->GetCollection())->UpdateEntity(oldId, entity->GetId());
}

void WorldImpl::UpdateCollections()
{
    if (mState != READY)
    {
        return;
    }
    mState = READY_UPDATE_COLLECTIONS;
   
    for (EntityAtomicPtr& entity : mNewEntities)
    {
        if (ECSUtil::IsRegister(entity->GetId()))
        {
            mEntities.insert(std::make_pair(entity->GetId(), entity));
            mRegisteringEntities.push_back(entity);
            continue;
        }
        Assert(ECSUtil::IsDestroyed(entity->GetId()));
    }
    mNewEntities.resize(0);

    for (auto& pair : mCollections)
    {
        pair.second->CommitChanges();
    }
    mState = READY;
}

void WorldImpl::UpdateSystems()
{
    if (mState != READY)
    {
        return;
    }
    mState = READY_UPDATE_SYSTEMS;
    if (mRebindNextUpdate)
    {
        for (ComponentSystem* system : mSystems)
        {
            system->BindTuples();
        }
        mRebindNextUpdate = false;
    }
    for (ComponentSystem* system : mSystems)
    {
        system->ScheduleUpdates();
    }
    mState = READY;
}

void WorldImpl::UpdateFences()
{
    if (mState != READY)
    {
        return;
    }
    mState = READY_UPDATE_FENCES;
    if (mForceUpdateSerial)
    {
        UpdateFencesSerial();
        mState = READY;
        return;
    }
    // Serial Update:
    // TODO: Apply update type logic

    // Algo:
    //  Foreach Fence
    //      arr.push {Serial, NonSerial } GetUpdates
    //      arr.push {Serial, NonSerial } GetConstantUpdates
    //     
    //      Execute arr { Serial }
    //      while !arr { NonSerial }.Empty
    //          if Execute(arr.Item)
    //              arr.Remove(arr.Item)
    //  
    
    TVector<SchedulerUpdateData> serialUpdates;
    TVector<SchedulerUpdateData> nonSerialUpdates;
    for (FenceData& fence : mFences)
    {
        if (LogFenceUpdate() || LogFenceUpdateVerbose())
        {
            gSysLog.Info(LogMessage("Updating fence ") << fence.mType->GetFullName());
        }

        // mem
        serialUpdates.resize(0);
        nonSerialUpdates.resize(0);
        serialUpdates.reserve(fence.mUpdates.size() + fence.mConstantUpdates.size());
        nonSerialUpdates.reserve(fence.mUpdates.size() + fence.mConstantUpdates.size());

        // Gather:
        for (FenceUpdate& update : fence.mUpdates)
        {
            if (IsSerialUpdate(update.mUpdateType))
            {
                serialUpdates.push_back(SchedulerUpdateData(&update));
            }
            else
            {
                nonSerialUpdates.push_back(SchedulerUpdateData(&update));
            }
        }
        for (FenceConstantUpdate& update : fence.mConstantUpdates)
        {
            if (IsSerialUpdate(update.mUpdateType))
            {
                serialUpdates.push_back(SchedulerUpdateData(&update));
            }
            else
            {
                nonSerialUpdates.push_back(SchedulerUpdateData(&update));
            }
        }

        // Serial:
        for (SchedulerUpdateData& update : serialUpdates)
        {
            switch (update.mType)
            {
                case SUT_CONSTANT_UPDATE:
                {
                    FenceConstantUpdate* updateData = static_cast<FenceConstantUpdate*>(update.mData);
                    if (LogFenceUpdateVerbose())
                    {
                        gSysLog.Info(LogMessage("ConstantUpdate.Invoke for ") << updateData->mName.CStr());
                    }
                    updateData->mUpdateCallback.Invoke(); // TODO: Profile & Add times for AI
                } break;
                case SUT_UPDATE:
                {
                    FenceUpdate* updateData = static_cast<FenceUpdate*>(update.mData);
                    if (LogFenceUpdateVerbose())
                    {
                        gSysLog.Info(LogMessage("Update.Invoke"));
                    }
                    updateData->mUpdateCallback.Invoke(); // TODO: Profile & Add times for AI
                } break;
                default:
                    CriticalAssertMsg("Unknown update type!");
                    break;
            }
        }
        // Repurpose as 'pending updates'
        serialUpdates.resize(0);

        // NonSerial:
        while (!nonSerialUpdates.empty())
        {
            for(auto update = nonSerialUpdates.begin(); update != nonSerialUpdates.end();)
            {
                switch (update->mType)
                {
                    case SUT_CONSTANT_UPDATE:
                    {
                        FenceConstantUpdate* updateData = static_cast<FenceConstantUpdate*>(update->mData);
                        if (ExecuteNonSerialUpdate(updateData))
                        {
                            serialUpdates.push_back(*update);
                            update = nonSerialUpdates.swap_erase(update);
                        }
                        else
                        {
                            ++update;
                        }
                    } break;
                    case SUT_UPDATE:
                    {
                        FenceUpdate* updateData = static_cast<FenceUpdate*>(update->mData);
                        if (ExecuteNonSerialUpdate(updateData))
                        {
                            serialUpdates.push_back(*update);
                            update = nonSerialUpdates.swap_erase(update);
                        }
                        else
                        {
                            ++update;
                        }
                    } break;
                    default:
                        CriticalAssertMsg("Unknown update type!");
                        break;
                }
            }
        }

        // Wait for non-serial
        while (!serialUpdates.empty())
        {
            for (auto update = serialUpdates.begin(); update != serialUpdates.end();)
            {
                switch (update->mType)
                {
                case SUT_CONSTANT_UPDATE:
                {
                    FenceConstantUpdate* updateData = static_cast<FenceConstantUpdate*>(update->mData);
                    if (AtomicLoad(&updateData->mTaskState) == TS_FINISHED)
                    {
                        update = serialUpdates.swap_erase(update);
                    }
                    else
                    {
                        ++update;
                    }
                } break;
                case SUT_UPDATE:
                {
                    FenceUpdate* updateData = static_cast<FenceUpdate*>(update->mData);
                    if (AtomicLoad(&updateData->mTaskState) == TS_FINISHED)
                    {
                        update = serialUpdates.swap_erase(update);
                    }
                    else
                    {
                        ++update;
                    }
                } break;
                default:
                    CriticalAssertMsg("Unknown update type!");
                    break;
                }
            }
        }
    }
    mState = READY;
}

void WorldImpl::UpdateFencesSerial()
{
    for (FenceData& fence : mFences)
    {
        for (FenceUpdate& update : fence.mUpdates)
        {
            CriticalAssert(update.mUpdateCallback.IsValid());
            update.mUpdateCallback();
        }
        fence.mUpdates.resize(0);

        for (FenceConstantUpdate& update : fence.mConstantUpdates)
        {
            CriticalAssert(update.mUpdateCallback.IsValid());
            update.mUpdateCallback();
        }
    }
}

void WorldImpl::UpdateNewEntities()
{
    for (EntityAtomicPtr& entity : mNewEntities)
    {
        if (ECSUtil::IsRegister(entity->GetId()))
        {
            mEntities.insert(std::make_pair(entity->GetId(), entity));
            mRegisteringEntities.push_back(entity);
        }
    }

    mNewEntities.clear();
}

void WorldImpl::UpdateRegistered()
{
    for (EntityAtomicPtr& entity : mRegisteringEntities)
    {
        ECSUtil::EntityLifeState state = ECSUtil::GetLifeState(entity->GetId());
        switch (state)
        {
        case ECSUtil::EntityLifeState::REGISTER:
        {
            // Update Entity Collection to 'alive'
            // Update Entities list id
            entity->SetId(ECSUtil::SetAlive(entity->GetId()));
        } break;
        case ECSUtil::EntityLifeState::UNREGISTER: // This is ok, this means the entity did not make the register
        case ECSUtil::EntityLifeState::ALIVE:      // This is semi-ok, this would mean the entity skipped the register phase (sort of)
        case ECSUtil::EntityLifeState::DESTROYED:  // Bad state, this would mean the entity got into register phase but should unregister!
        {
        } break;
        default:
            CriticalAssertMsg("Invalid entity state.");
            break;
        }
    }

    mRegisteringEntities.resize(0);

    TAtomicStore(&mUpdateState, US_UPDATE);
}

void WorldImpl::UpdateUnregistered()
{
    for (EntityAtomicPtr& entity : mUnregisteringEntities)
    {
        ECSUtil::EntityLifeState state = ECSUtil::GetLifeState(entity->GetId());
        switch (state)
        {
        case ECSUtil::EntityLifeState::UNREGISTER:
        {
            // Update Entity Collection to 'Alive'
            // Update Entities list to remove this entity.
            entity->SetId(ECSUtil::SetDestroyed(entity->GetId()));
        } break;
        case ECSUtil::EntityLifeState::REGISTER:
        case ECSUtil::EntityLifeState::ALIVE:
        case ECSUtil::EntityLifeState::DESTROYED:

        default:
            CriticalAssertMsg("Invalid entity state.");
            break;
        }
    }
    mUnregisteringEntities.resize(0);
}

EntityAtomicWPtr WorldImpl::FindEntity(EntityId id)
{
    auto it = mEntities.find(id);
    return it == mEntities.end() ? EntityAtomicWPtr() : it->second;
}
EntityAtomicWPtr WorldImpl::FindNewEntity(EntityId id)
{
    auto it = std::find_if(mNewEntities.begin(), mNewEntities.end(), [id](const Entity* entity)
        {
            return entity->GetId() == id;
        });
    return it == mNewEntities.end() ? EntityAtomicWPtr() : *it;
}
EntityAtomicWPtr WorldImpl::FindRegistered(EntityId id)
{
    auto it = std::find_if(mRegisteringEntities.begin(), mRegisteringEntities.end(), [id](const Entity* entity)
        {
            return entity->GetId() == id;
        });
    return it == mRegisteringEntities.end() ? EntityAtomicWPtr() : *it;
}
EntityAtomicWPtr WorldImpl::FindUnregistered(EntityId id)
{
    auto it = std::find_if(mUnregisteringEntities.begin(), mUnregisteringEntities.end(), [id](const Entity* entity)
        {
            return entity->GetId() == id;
        });
    return it == mUnregisteringEntities.end() ? EntityAtomicWPtr() : *it;
}

std::pair<EntityId, EntityAtomicWPtr> WorldImpl::FindEntitySlow(EntityId id)
{
    EntityId rawId = ECSUtil::GetId(id);
    for (auto& pair : mEntities)
    {
        if (rawId == ECSUtil::GetId(pair.first))
        {
            return std::make_pair(pair.first, pair.second);
        }
    }
    return std::make_pair(INVALID_ENTITY_ID, EntityAtomicWPtr());
}

SizeT WorldImpl::GetFenceIndex(const Type* target)
{
    auto it = std::find_if(mFences.begin(), mFences.end(), [target](const FenceData& fence) { return fence.mType == target; });
    if (it == mFences.end())
    {
        return INVALID;
    }
    return static_cast<SizeT>(it - mFences.begin());
}

bool WorldImpl::LogEntityIdChanges() const
{
    return mAppService ? mAppService->GetConfigObject<WorldConfig>()->mLogEntityIdChanges : false;
}
bool WorldImpl::LogEntityAddRemove() const
{
    return mAppService ? mAppService->GetConfigObject<WorldConfig>()->mLogEntityAddRemove : false;
}
bool WorldImpl::LogFenceUpdate() const
{
    return mAppService ? mAppService->GetConfigObject<WorldConfig>()->mLogFenceUpdate : false;
}
bool WorldImpl::LogFenceUpdateVerbose() const
{
    return mAppService ? mAppService->GetConfigObject<WorldConfig>()->mLogFenceUpdateVerbose : false;
}

bool WorldImpl::AllowUpdateScheduling()
{
    return Async::GetAppThreadId() == APP_THREAD_ID_MAIN && mState == READY_UPDATE_SYSTEMS;
}

bool WorldImpl::AllowFenceCreation()
{
    return Async::GetAppThreadId() == APP_THREAD_ID_MAIN && mState == INITIALIZE_SYSTEM;
}

bool WorldImpl::IsBuiltInFence(const Type* type)
{
    return mBuiltInFences.end() != std::find(mBuiltInFences.begin(), mBuiltInFences.end(), type);
}
WorldImpl::FenceData* WorldImpl::GetFence(const Type* type)
{
    auto fence = std::find_if(mFences.begin(), mFences.end(), [type](const FenceData& fence) { return fence.mType == type; });
    return fence == mFences.end() ? nullptr : &(*fence);
}
WorldImpl::FenceData* WorldImpl::GetFence(const Token& updateName)
{
    for (FenceData& fence : mFences)
    {
        for(const FenceConstantUpdate& update : fence.mConstantUpdates)
        {
            if (update.mName == updateName)
            {
                return &fence;
            }
        }
    }
    return nullptr;
}


WorldImpl::ComponentQuery WorldImpl::ToQuery(const TVector<const Type*>& types)
{
    ComponentQuery query;
    for (const Type* type : types)
    {
        if (!type)
        {
            return ComponentQuery();
        }

        auto it = mComponentTypes.find(type);
        if (it == mComponentTypes.end())
        {
            return ComponentQuery();
        }
        query.push_back(it->second->GetId());
    }
    std::sort(query.begin()._Unwrapped(), query.end()._Unwrapped());
    return query;
}

TVector<EntityCollection*> WorldImpl::FindCollections(const ComponentQuery& includeQuery, const ComponentQuery& excludeQuery)
{
    if (includeQuery.empty())
    {
        return TVector<EntityCollection*>();
    }

    if (mIndexDirty)
    {
        PrepareIndex();
    }

    TVector<Int16> includeHeap; includeHeap.resize(mIndexedCollections.size());
    Int16* include = includeHeap.data(); memset(include, 0, includeHeap.size() * sizeof(Int16));

    SizeT includeHits = 0;
    SizeT excludeHits = 0;

    QueryHints hints;

    ScanInclude(includeQuery, include, includeHits, hints);
    if (!excludeQuery.empty())
    {
        ScanExclude(excludeQuery, include, excludeHits);
    }
    
    SizeT expectSize = includeQuery.size();
    TVector<EntityCollection*> result;
    result.resize(includeHits);
    SizeT i = 0;
    // Iterate through 'touched'
    for (UInt16 hint : hints)
    {
        if (include[hint] == expectSize)
        {
            result[i++] = mIndexedCollections[hint];
        }
    }
    // Iterate through all
    // for (SizeT k = 0, size = includeHeap.Size(); k < size; ++k)
    // {
    //     if (include[k] == expectSize)
    //     {
    //         result[i++] = mIndexedCollections[k];
    //     }
    // }
    result.resize(i);
    return result;
}

void WorldImpl::ScanInclude(const ComponentQuery& query, Int16* resultBuffer, SizeT& numHits, QueryHints& hints)
{
    const UInt16 queryMin = query.front();
    const UInt16 queryMax = query.back();
    const SizeT querySize = query.size();

    for (UInt16 typeId : query)
    {
        const IndexedDefinitionIndex& indices = mIndexedComponents[typeId];
        for (const DefinitionIndex& index : indices)
        {
            if (querySize <= index.mComponentCount && queryMin >= index.mMinComponentId && queryMax <= index.mMaxComponentId)
            {
                if (resultBuffer[index.mId] == 0)
                {
                    ++numHits;
                    hints.push_back(index.mId);
                }
                ++resultBuffer[index.mId];
            }
        }
    }
}

void WorldImpl::ScanExclude(const ComponentQuery& query, Int16* resultBuffer, SizeT& numHits)
{
    const UInt16 queryMin = query.front();
    const UInt16 queryMax = query.back();
    const SizeT querySize = query.size();

    for (UInt16 typeId : query)
    {
        const IndexedDefinitionIndex& indices = mIndexedComponents[typeId];
        for (const DefinitionIndex& index : indices)
        {
            if (querySize <= index.mComponentCount && queryMin >= index.mMinComponentId && queryMax <= index.mMaxComponentId)
            {
                if (resultBuffer[index.mId] == 0)
                {
                    ++numHits;
                }
                --resultBuffer[index.mId];
            }
        }
    }
}

bool WorldImpl::AcquireLock(bool read, const TVector<ComponentId>& components)
{
    bool success = true;
    SizeT progress = 0;
    // multi-thread, single-writer
    if (read)
    {
        for (; progress < components.size(); ++progress)
        {
            ComponentId component = components[progress];
            if (AtomicLoad(&mWriteComponents[component]) > 0)
            {
                success = false;
                --progress;
                break;
            }
            AtomicIncrement32(&mReadComponents[component]);
        }
        // Undo failures
        if (progress != components.size())
        {
            for (; Valid(progress); --progress)
            {
                ComponentId component = components[progress];
                AtomicDecrement32(&mReadComponents[component]);
            }
        }
        return success;
    }
    else 
    {
        for (; progress < components.size(); ++progress)
        {
            ComponentId component = components[progress];
            if (AtomicLoad(&mReadComponents[component]) > 0 || AtomicLoad(&mWriteComponents[component]) > 0)
            {
                success = false;
                --progress;
                break;
            }
            AtomicIncrement32(&mWriteComponents[component]);
        }
        // Undo failures
        if (progress != components.size())
        {
            for (; Valid(progress); --progress)
            {
                ComponentId component = components[progress];
                AtomicDecrement32(&mWriteComponents[component]);
            }
        }
        return success;
    }
}
bool WorldImpl::ReleaseLock(bool read, const TVector<ComponentId>& components)
{
    bool success = true;
    SizeT progress = 0;
    if (read)
    {
        for (; progress < components.size(); ++progress)
        {
            ComponentId component = components[progress];
            if (AtomicLoad(&mWriteComponents[component]) > 0 || AtomicLoad(&mReadComponents[component]) == 0)
            {
                AssertMsg("Failed to release component lock, invalid permissions.");
                success = false;
                --progress;
                break;
            }
            AtomicDecrement32(&mReadComponents[component]);
        }
        // Undo failures
        if (progress != components.size())
        {
            for (; Valid(progress); --progress)
            {
                ComponentId component = components[progress];
                AtomicIncrement32(&mReadComponents[component]);
            }
        }
        return success;
    }
    else
    {
        for (; progress < components.size(); ++progress)
        {
            ComponentId component = components[progress];
            if (AtomicLoad(&mReadComponents[component]) != 0 || AtomicLoad(&mWriteComponents[component]) != 1)
            {
                AssertMsg("Failed to release component lock, invalid permissions.");
                success = false;
                --progress;
                break;
            }
            AtomicDecrement32(&mWriteComponents[component]);
        }
        // Undo failures
        if (progress != components.size())
        {
            for (; Valid(progress); --progress)
            {
                ComponentId component = components[progress];
                AtomicIncrement32(&mWriteComponents[component]);
            }
        }
        return success;
    }
}

// Execute:
    //      If Not CanWrite(Item.WriteComponents)
    //          Return False
    //      If Not CanRead(Item.ReadComponents)
    //          Return False
    //      
    //      LockWrite(Item.WriteComponents)
    //      LockRead(Item.ReadComponents)
    //      Scheduler.Execute( 
    //              [task](){
    //                  task->Running
    //                  task->Update()
    //                  task->Finished
    //              })
bool WorldImpl::ExecuteNonSerialUpdate(FenceConstantUpdate* updateData)
{
    if (!AcquireLock(true, updateData->mWriteComponents))
    {
        return false;
    }
    if (!AcquireLock(false, updateData->mReadComponents))
    {
        ReleaseLock(true, updateData->mWriteComponents);
        return false;
    }

    // todo: run on separate thread.
    AtomicStore(&updateData->mTaskState, TS_RUNNING);
    if (LogFenceUpdateVerbose())
    {
        gSysLog.Info(LogMessage("ConstantUpdate.Invoke for ") << updateData->mName.CStr());
    }
    updateData->mUpdateCallback();

    ReleaseLock(false, updateData->mReadComponents);
    ReleaseLock(true, updateData->mWriteComponents);

    AtomicStore(&updateData->mTaskState, TS_FINISHED);
    return true;
}
bool WorldImpl::ExecuteNonSerialUpdate(FenceUpdate* updateData)
{
    if (!AcquireLock(true, updateData->mWriteComponents))
    {
        return false;
    }
    if (!AcquireLock(false, updateData->mReadComponents))
    {
        ReleaseLock(true, updateData->mWriteComponents);
        return false;
    }

    // todo: run on separate thread.
    AtomicStore(&updateData->mTaskState, TS_RUNNING);
    if (LogFenceUpdateVerbose())
    {
        gSysLog.Info(LogMessage("Update.Invoke"));
    }
    updateData->mUpdateCallback();
    AtomicStore(&updateData->mTaskState, TS_FINISHED);
    return true;
}

} // namespace lf