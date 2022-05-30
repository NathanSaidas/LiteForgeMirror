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
#include "Core/Test/Test.h"
#include "AbstractEngine/World/Entity.h"
#include "AbstractEngine/World/ComponentSystem.h"
#include "Engine/World/WorldImpl.h"
#include "Game/Test/TestUtils.h"

#include "Game/Artherion/ComponentTypes/TransformComponent.h"
#include "Game/Artherion/ComponentTypes/BoundsComponent.h"
#include "Game/Artherion/ComponentTypes/ModelComponent.h"

// TODO: System Update Scheduling
// TODO: Soft Data Locks
// TODO: Entity External Modification
// TODO: Entity Serialization
// TODO: Solidify life-time management
//
// [Solidify life-time management]
// 
// function IsRegisterFrame(Entity)
//      return Valid( Entities.Find(EntityId) );
// function DestroyNextUpdate(Entity)
//      mDestroyerSystem->AddEntity(Entity);
//
// Create Entity [Register]
// Destroy Entity [Destroyed]
//
// Create Entity [Register]
// Update [Push from new list to current list, execute system update]
//   TestPreRegister.Update [ Destroy Entity] -- This will fail because we're trying to destroy inside the register frame
//   GlobalSystemRegisterFence.Update [ Change state from Register to Alive ]
//   TestSystem.Update [ Destroy Entity ] -- This will succeed and put the entity in the UnregisterState
//   GlobalSystemUnregisterFence.Update [ do nothing ]
//   GlobalSystemEndFrameFence.Update  -- This will change the state from unregister to destroy
// 
// ECS Update:
//
//   [Begin Frame]
//   GlobalRegister: System.Update => { World->ScheduleUpdateBefore( GlobalRegister ) }
// 
//   GlobalUnregister: System.Update => { World->ScheduleUpdateAfter( GlobalUnregister ) }
//   [End Frame]
// 
// Systems Updating Before Register, CANNOT destroyed entities
// Systems Updating After Unregister, CANNOT destroyed entities
// Entities are 'game objects' and should be destroyed through game-flow.
// External code trying to destroy a entity outside the world update frame are also
// instead external code could call World->DestroyNextUpdate( entity );
//
// eg; UpdateBefore( GlobalRegister ) {
//      Entity->Destroy(); // Generates an error (Assert)
// }
// eg; UpdateAfter( GlobalUnregister ) {
//      Entity->Destroy(); // Generates an error (Assert)
// }
// eg; UpdateAfter( GlobalRegister ) {
//      World->SafeDestroy(); // Can use this to check if an entity can be destroyed...
//      World->DestroyNextUpdate( Entity )
// }
// eg; ExternalCode ( ) {
//      World->DestroyNextUpdate( Entity )
// }
// 
// [Entity External Modification]
// 
// eg; ECS_Thread() {
//      Entity->GetComponent<T>(); // OK:
// }
// eg; ExternalCode() {
//      Entity->GetComponent<T>(); // Assert!

//      World->ReadComponent( Entity, [](Entity* entity) // OK!
//      {
//          Result = entity->GetComponent<T>().Property; // OK!
//          entity->GetComponent<T>().Property = Value;  // NOT OK! Won't assert though.
//      });
// 
//      World->WriteComponent( Entity, [](Entity* entity) // OK!
//      {
//          Result = entity->GetComponent<T>().Property; // OK!
//          entity->GetComponent<T>().Property = Value;  // OK!
//      });
// }
// 
// ECS At a global scale will provide scheduling such that updates
// can be multi-threaded without data races. (eg 2 threads attempting to write)
// this means we do 'soft' locks on the data.
// 
// The update itself shouldn't have to verify it has 'write' or 'read' access,
// but calling code should verify they have 'read' or 'write' access.
// 
// The following functions support single-writer, multi-reader.
// 
// External code should call ReadComponentAsync and WriteComponentAsync
//
// 
// World->LockReadComponent(type, thread);
// World->UnlockReadComponent(type, thread);
// World->HasReadAccess(type, thread);
// World->LockWriteComponent(type, thread);
// World->UnlockWriteComponent(type, thread);
// World->HasWriteAccess(type, thread);
// 
// [System Update Scheduling]
// OnInit:
// World->CreateFence( fence, reference [Before/After] )
//
// OnFrameBegin:
// World->Schedule( fence, readTypes, writeTypes, distributed )
// 
// 2 Systems can update concurrently as long as they don't have read/write conflicts.
// A single system can run updates in a distributed manner
//      NumThreads = T (8)
//      WorkItems = W (100)
//      WorkLoad = NumThreads(100) / WorkItems(8)
//      WorkLoad->ForEach( Thread, UpdateCallback )
// And the distributer can be smart and time 'last update' time and determine if the work
// needs to be less sparse (eg used all threads to generate all thread baseline, use 1 thread 
// to generate single thread baseline) then choose a thread count to find the best time to thread 
// perf.
// 
// eg; 1 thread = 2ms, 8 thread = 6ms, pick 1 thread over 8
// eg; 1 thread = 2ms, 2 thread = 1ms, pick 2 thread over 1
// eg; 1 thread = 2ms, 3 thread = 1ms, pick 3 thread over 1
// eg; 1 thread = 2ms, 4 thread = 2ms, pick 3 thread (from history) over 4 and 1
// 
// 
// *ScheduledUpdate*
// MFence()
// ExecuteUpdates(...)
// MFence()

namespace lf
{

// Fences define where updates are scheduled/grouped
// eg Correct;
// FenceA = After => DefaultUpdate
// FenceB = After => FenceA
// FenceC = Before => Fence B
// Result = A => C => B or C => A => B
// 
// eg Conflict; What if you need something to update after B but before A? Well thats impossible so it would be a conflict
// and by default we can't generate those conflicts.
class TestUpdateAFence : public ComponentSystemFence
{
    DECLARE_CLASS(TestUpdateAFence, ComponentSystemFence);
};
DEFINE_ABSTRACT_CLASS(lf::TestUpdateAFence) { NO_REFLECTION; }

class TestUpdateBFence : public ComponentSystemFence
{
    DECLARE_CLASS(TestUpdateBFence, ComponentSystemFence);
};
DEFINE_ABSTRACT_CLASS(lf::TestUpdateBFence) { NO_REFLECTION; }

class TestUpdateCFence : public ComponentSystemFence
{
    DECLARE_CLASS(TestUpdateCFence, ComponentSystemFence);
};
DEFINE_ABSTRACT_CLASS(lf::TestUpdateCFence) { NO_REFLECTION; }

class TestRegisterFence : public ComponentSystemFence
{
    DECLARE_CLASS(TestRegisterFence, ComponentSystemFence);
public:
    static bool Create(World& world)
    {
        return world.CreateFenceBefore(typeof(TestRegisterFence), typeof(ComponentSystemRegisterFence));
    }
};
DEFINE_ABSTRACT_CLASS(lf::TestRegisterFence) { NO_REFLECTION; }

class TestUnregisterFence : public ComponentSystemFence
{
    DECLARE_CLASS(TestUnregisterFence, ComponentSystemFence);
public:
    static bool Create(World& world)
    {
        return world.CreateFenceAfter(typeof(TestUnregisterFence), typeof(ComponentSystemUnregisterFence));
    }
};
DEFINE_ABSTRACT_CLASS(lf::TestUnregisterFence) { NO_REFLECTION; }

const Type* gUpdateList[]
{
    nullptr, // TestRegisterSystem
    nullptr, // TestUpdateASystem
    nullptr, // TestUpdateCSystem
    nullptr, // TestUpdateBSystem
    nullptr  // TestUnregisterSystem
};
SizeT gCurrentUpdate = 0;

static EntityDefinition GetMobType()
{
    EntityDefinition def;
    def.SetComponentTypes({ typeof(TransformComponent), typeof(BoundsComponent), typeof(ModelComponent) });
    return def;
}

// Example / Test class to show how to create a system that updates in the 'register' phase of a frame
class TestRegisterSystem : public ComponentSystem, public TSystemTestAttributes<TestRegisterSystem>
{
    DECLARE_CLASS(TestRegisterSystem, ComponentSystem);
public:
    bool mRegistered;
    virtual bool OnInitialize() override
    {
        mRegistered = false;
        return true;
    }
    virtual void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            if (StartConstantUpdate(String(), ECSUtil::UpdateCallback::Make(this, &TestRegisterSystem::Update), typeof(ComponentSystemRegisterFence), sUpdateType))
            {
                mRegistered = true;
            }
        }
    }
    void Update()
    {
        gUpdateList[gCurrentUpdate] = GetType();
        gCurrentUpdate = (gCurrentUpdate + 1) % LF_ARRAY_SIZE(gUpdateList);
    }
    virtual bool IsEnabled() const override { return sEnable; }
};
DEFINE_CLASS(lf::TestRegisterSystem) { NO_REFLECTION; }

// Example / Test class to show how to create a system that updates in the 'register' phase of a frame
class TestUnregisterSystem : public ComponentSystem, public TSystemTestAttributes<TestUnregisterSystem>
{
    DECLARE_CLASS(TestUnregisterSystem, ComponentSystem);
public:

    bool mRegistered;
    virtual bool OnInitialize() override
    {
        mRegistered = false;
        return true;
    }
    virtual void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            if (StartConstantUpdate(String(), ECSUtil::UpdateCallback::Make(this, &TestUnregisterSystem::Update), typeof(ComponentSystemUnregisterFence), sUpdateType))
            {
                mRegistered = true;
            }
        }
    }
    void Update()
    {
        gUpdateList[gCurrentUpdate] = GetType();
        gCurrentUpdate = (gCurrentUpdate + 1) % LF_ARRAY_SIZE(gUpdateList);
    }
    virtual bool IsEnabled() const override { return sEnable; }
};
DEFINE_CLASS(lf::TestUnregisterSystem) { NO_REFLECTION; }

class TestUpdateASystem : public ComponentSystem, public TSystemTestAttributes<TestUpdateASystem>
{
    DECLARE_CLASS(TestUpdateASystem, ComponentSystem);
public:

    bool mRegistered;
    virtual bool OnInitialize() override
    {
        TEST(GetWorld()->CreateFenceAfter(typeof(TestUpdateAFence), typeof(ComponentSystemUpdateFence)));
        mRegistered = false;
        return true;
    }
    virtual void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            if (StartConstantUpdate(String(), ECSUtil::UpdateCallback::Make(this, &TestUpdateASystem::Update), typeof(TestUpdateAFence), sUpdateType))
            {
                mRegistered = true;
            }
        }
    }
    void Update()
    {
        gUpdateList[gCurrentUpdate] = GetType();
        gCurrentUpdate = (gCurrentUpdate + 1) % LF_ARRAY_SIZE(gUpdateList);
    }
    virtual bool IsEnabled() const override { return sEnable; }
};
DEFINE_CLASS(lf::TestUpdateASystem) { NO_REFLECTION; }

class TestUpdateBSystem : public ComponentSystem, public TSystemTestAttributes<TestUpdateBSystem>
{
    DECLARE_CLASS(TestUpdateBSystem, ComponentSystem);
public:

    bool mRegistered;
    virtual bool OnInitialize()
    {
        TEST(GetWorld()->CreateFenceAfter(typeof(TestUpdateBFence), typeof(TestUpdateAFence)));
        mRegistered = false;
        return true;
    }
    virtual void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            if (StartConstantUpdate(String(), ECSUtil::UpdateCallback::Make(this, &TestUpdateBSystem::Update), typeof(TestUpdateBFence), sUpdateType))
            {
                mRegistered = true;
            }
        }
    }
    void Update()
    {
        gUpdateList[gCurrentUpdate] = GetType();
        gCurrentUpdate = (gCurrentUpdate + 1) % LF_ARRAY_SIZE(gUpdateList);
    }
    virtual bool IsEnabled() const override { return sEnable; }
};
DEFINE_CLASS(lf::TestUpdateBSystem) { NO_REFLECTION; }

class TestUpdateCSystem : public ComponentSystem, public TSystemTestAttributes<TestUpdateCSystem>
{
    DECLARE_CLASS(TestUpdateCSystem, ComponentSystem);
public:

    bool mRegistered;
    virtual bool OnInitialize()
    {
        TEST(GetWorld()->CreateFenceBefore(typeof(TestUpdateCFence), typeof(TestUpdateBFence)));
        mRegistered = false;
        return true;
    }
    virtual void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            if (StartConstantUpdate(String(), ECSUtil::UpdateCallback::Make(this, &TestUpdateCSystem::Update), typeof(TestUpdateCFence), sUpdateType))
            {
                mRegistered = true;
            }
        }
    }
    void Update()
    {
        gUpdateList[gCurrentUpdate] = GetType();
        gCurrentUpdate = (gCurrentUpdate + 1) % LF_ARRAY_SIZE(gUpdateList);
    }
    virtual bool IsEnabled() const override { return sEnable; }
};
DEFINE_CLASS(lf::TestUpdateCSystem) { NO_REFLECTION; }

class TestDeleteEntitySystem : public ComponentSystem, public TSystemTestAttributes<TestDeleteEntitySystem>
{
    DECLARE_CLASS(TestDeleteEntitySystem, ComponentSystem);
public:
    static EntityId sDestroyOnRegister;
    static EntityId sDestroyOnUnregister;

    bool mRegistered;
    bool IsEnabled() const override { return sEnable; }
    bool OnInitialize() override
    {
        return TestRegisterFence::Create(*GetWorld()) && TestUnregisterFence::Create(*GetWorld());
    }

    void OnScheduleUpdates() override
    {
        if (Valid(sDestroyOnRegister))
        {
            ScheduleUpdate("DestroyOnRegister", UpdateCallback::Make(this, &TestDeleteEntitySystem::DestroyOnRegister), typeof(TestRegisterFence));
        }
        if (Valid(sDestroyOnUnregister))
        {
            ScheduleUpdate("DestroyOnUnregister", UpdateCallback::Make(this, &TestDeleteEntitySystem::DestroyOnUnregister), typeof(TestUnregisterFence));
        }
    }

    void DestroyOnRegister()
    {
        EntityAtomicWPtr entity = GetWorld()->FindEntitySlow(sDestroyOnRegister).second;
        sDestroyOnRegister = INVALID_ENTITY_ID;
        if (entity)
        {
            entity->Destroy();
        }
    }

    void DestroyOnUnregister()
    {
        EntityAtomicWPtr entity = GetWorld()->FindEntitySlow(sDestroyOnUnregister).second;
        sDestroyOnUnregister = INVALID_ENTITY_ID;
        if (entity)
        {
            entity->Destroy();
        }
    }

};
DEFINE_CLASS(lf::TestDeleteEntitySystem) { NO_REFLECTION; }
EntityId TestDeleteEntitySystem::sDestroyOnRegister = INVALID_ENTITY_ID;
EntityId TestDeleteEntitySystem::sDestroyOnUnregister = INVALID_ENTITY_ID;

struct TestEnableSystem
{
    TestEnableSystem(bool& value) : mValue(value) { mValue = true; }
    ~TestEnableSystem() { mValue = false; }
    bool& mValue;
};

struct TestOverrideUpdateType
{
    TestOverrideUpdateType(ECSUtil::UpdateType& value, ECSUtil::UpdateType overrideValue) : mValue(value) { mValue = overrideValue; }
    ~TestOverrideUpdateType() { mValue = ECSUtil::UpdateType::SERIAL; }

    ECSUtil::UpdateType& mValue;
};

static SizeT GetUpdateIndex(const Type* type)
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(gUpdateList); ++i)
    {
        if (gUpdateList[i] == type)
        {
            return i;
        }
    }
    return INVALID;
}

static bool VerifyFlags(WorldImpl* world, Entity* entity)
{
    auto worldResult = world->FindEntitySlow(entity->GetId());
    if (!Valid(worldResult.first) || !worldResult.second || worldResult.second != entity)
    {
        return false;
    }
    SizeT index = entity->GetCollection()->GetIndexSlow(entity->GetId());
    if (Invalid(index))
    {
        return false;
    }
    EntityId collectionId = entity->GetCollection()->GetEntity(index);

    return entity->GetId() == worldResult.first && collectionId == entity->GetId();
}

static bool VerifyNewFlags(Entity* entity)
{
    SizeT index = entity->GetCollection()->GetNewIndexSlow(entity->GetId());
    if (Invalid(index))
    {
        return false;
    }
    EntityId collectionId = entity->GetCollection()->GetNewEntity(index);

    return collectionId == entity->GetId();
}

static EntityAtomicPtr CreateTestEntity(World* world)
{
    EntityDefinition def;
    def.SetComponentTypes({ typeof(TransformComponent), typeof(ModelComponent) });
    return world->CreateEntity(&def);
}

REGISTER_TEST(World_EntityFlag_Tests, "AbstractEngine.World")
{
    const EntityId DEFAULT_ID = 498203;
    LF_STATIC_ASSERT(DEFAULT_ID <= ECSUtil::ENTITY_ID_BITMASK);
    EntityId id = DEFAULT_ID;

    // By default entity ids are Normal Priority
    TEST(ECSUtil::IsNormalPriority(id) == true); 
    TEST(ECSUtil::IsHighPriority(id) == false);
    TEST(ECSUtil::IsLowPriority(id) == false);
    TEST(ECSUtil::GetPriority(id) == ECSUtil::EntityPriority::NORMAL);
    TEST(ECSUtil::GetId(id) == DEFAULT_ID);

    id = ECSUtil::SetLowPriority(id);
    TEST(ECSUtil::IsNormalPriority(id) == false);
    TEST(ECSUtil::IsHighPriority(id) == false);
    TEST(ECSUtil::IsLowPriority(id) == true);
    TEST(ECSUtil::GetPriority(id) == ECSUtil::EntityPriority::LOW);
    TEST(ECSUtil::GetId(id) == DEFAULT_ID);

    id = ECSUtil::SetHighPriority(id);
    TEST(ECSUtil::IsNormalPriority(id) == false);
    TEST(ECSUtil::IsHighPriority(id) == true);
    TEST(ECSUtil::IsLowPriority(id) == false);
    TEST(ECSUtil::GetPriority(id) == ECSUtil::EntityPriority::HIGH);
    TEST(ECSUtil::GetId(id) == DEFAULT_ID);

    id = ECSUtil::SetNormalPriority(id);
    TEST(ECSUtil::IsNormalPriority(id) == true);
    TEST(ECSUtil::IsHighPriority(id) == false);
    TEST(ECSUtil::IsLowPriority(id) == false);
    TEST(ECSUtil::GetPriority(id) == ECSUtil::EntityPriority::NORMAL);
    TEST(ECSUtil::GetId(id) == DEFAULT_ID);

    // By default entity ids are 'register' life state
    id = DEFAULT_ID;
    TEST(ECSUtil::IsRegister(id) == true);
    TEST(ECSUtil::IsAlive(id) == false);
    TEST(ECSUtil::IsUnregister(id) == false);
    TEST(ECSUtil::IsDestroyed(id) == false);
    TEST(ECSUtil::GetLifeState(id) == ECSUtil::EntityLifeState::REGISTER);

    id = ECSUtil::SetAlive(id);
    TEST(ECSUtil::IsRegister(id) == false);
    TEST(ECSUtil::IsAlive(id) == true);
    TEST(ECSUtil::IsUnregister(id) == false);
    TEST(ECSUtil::IsDestroyed(id) == false);
    TEST(ECSUtil::GetLifeState(id) == ECSUtil::EntityLifeState::ALIVE);
    TEST(ECSUtil::GetId(id) == DEFAULT_ID);

    id = ECSUtil::SetUnregister(id);
    TEST(ECSUtil::IsRegister(id) == false);
    TEST(ECSUtil::IsAlive(id) == false);
    TEST(ECSUtil::IsUnregister(id) == true);
    TEST(ECSUtil::IsDestroyed(id) == false);
    TEST(ECSUtil::GetLifeState(id) == ECSUtil::EntityLifeState::UNREGISTER);
    TEST(ECSUtil::GetId(id) == DEFAULT_ID);

    id = ECSUtil::SetDestroyed(id);
    TEST(ECSUtil::IsRegister(id) == false);
    TEST(ECSUtil::IsAlive(id) == false);
    TEST(ECSUtil::IsUnregister(id) == false);
    TEST(ECSUtil::IsDestroyed(id) == true);
    TEST(ECSUtil::GetLifeState(id) == ECSUtil::EntityLifeState::DESTROYED);
    TEST(ECSUtil::GetId(id) == DEFAULT_ID);
}

REGISTER_TEST(World_Fence_Test, "AbstractEngine.World")
{
    TestEnableSystem a(TestUpdateASystem::sEnable);
    TestEnableSystem b(TestUpdateBSystem::sEnable);
    TestEnableSystem c(TestUpdateCSystem::sEnable);


    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    SizeT defaultUpdate = world->GetFenceIndex(typeof(ComponentSystemUpdateFence));
    SizeT testUpdateA = world->GetFenceIndex(typeof(TestUpdateAFence));
    SizeT testUpdateB = world->GetFenceIndex(typeof(TestUpdateBFence));
    SizeT testUpdateC = world->GetFenceIndex(typeof(TestUpdateCFence));
    TEST(Valid(testUpdateA) && testUpdateA > defaultUpdate);
    TEST(Valid(testUpdateB) && testUpdateB > testUpdateA);
    TEST(Valid(testUpdateC) && testUpdateC < testUpdateB);
}
REGISTER_TEST(World_FenceFail_Test, "AbstractEngine.World")
{
    TestEnableSystem a(TestUpdateASystem::sEnable);
    TestEnableSystem c(TestUpdateCSystem::sEnable);

    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    SizeT testUpdateA = world->GetFenceIndex(typeof(TestUpdateAFence));
    SizeT testUpdateC = world->GetFenceIndex(typeof(TestUpdateCFence));
    TEST(Invalid(testUpdateA));
    TEST(Invalid(testUpdateC));
}

REGISTER_TEST(World_Update_Test, "AbstractEngine.World")
{
    gCurrentUpdate = 0;

    TestEnableSystem a(TestUpdateASystem::sEnable);
    TestEnableSystem b(TestUpdateBSystem::sEnable);
    TestEnableSystem c(TestUpdateCSystem::sEnable);
    TestEnableSystem registerEnable(TestRegisterSystem::sEnable);
    TestEnableSystem unregisterEnable(TestUnregisterSystem::sEnable);


    TestOverrideUpdateType updateA(TestUpdateASystem::sUpdateType, ECSUtil::UpdateType::CONCURRENT);

    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    SizeT registerUpdate = GetUpdateIndex(typeof(TestRegisterSystem));
    SizeT aUpdate = GetUpdateIndex(typeof(TestUpdateASystem));
    SizeT bUpdate = GetUpdateIndex(typeof(TestUpdateBSystem));
    SizeT cUpdate = GetUpdateIndex(typeof(TestUpdateCSystem));
    SizeT unregisterUpdate = GetUpdateIndex(typeof(TestUnregisterSystem));

    TEST(Valid(registerUpdate));
    TEST(Valid(aUpdate));
    TEST(Valid(bUpdate) && bUpdate > aUpdate);
    TEST(Valid(cUpdate) && cUpdate < bUpdate);
    TEST(Valid(unregisterUpdate));

    TEST(aUpdate > registerUpdate);
    TEST(bUpdate > registerUpdate);
    TEST(cUpdate > registerUpdate);

    TEST(aUpdate < unregisterUpdate);
    TEST(bUpdate < unregisterUpdate);
    TEST(cUpdate < unregisterUpdate);
}

REGISTER_TEST(World_EntityCommonLifeTime_Test, "AbstractEngine.World", TestFlags::TF_DISABLED)
{
    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // We create the entity, they will be in 'register state for one frame'
    EntityAtomicPtr entity = CreateTestEntity(world);
    TEST_CRITICAL(entity);
    TEST_CRITICAL(entity->GetCollection() != nullptr);
    TEST_CRITICAL(entity->GetWorld() != nullptr);

    // Verify create state is correct
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) == NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Invalid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Valid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyNewFlags(entity));

    /// Update!
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // Verify register state is correct, this is the frame we update w/ register
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) != NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Valid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Invalid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyFlags(world, entity));

    /// Update!
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // Verify register state is correct, this is the frame we're alive!
    TEST(ECSUtil::IsAlive(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) == NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Valid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Invalid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyFlags(world, entity));


    /// Update!
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    entity->Destroy();
    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) != NULL_PTR);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // Verify register state is correct, this is the frame we're alive!
    TEST(ECSUtil::IsUnregister(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) == NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Valid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Invalid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyFlags(world, entity));
}

REGISTER_TEST(World_EntityShortLifeTime_Test, "AbstractEngine.World", TestFlags::TF_DISABLED)
{
    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // We create the entity, they will be in 'register state for one frame'
    EntityAtomicPtr entity = CreateTestEntity(world);
    TEST_CRITICAL(entity);
    TEST_CRITICAL(entity->GetCollection() != nullptr);
    TEST_CRITICAL(entity->GetWorld() != nullptr);

    // Verify create state is correct
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) == NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Invalid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Valid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyNewFlags(entity));

    entity->Destroy();

    TEST(ECSUtil::IsDestroyed(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) == NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Invalid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Valid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyNewFlags(entity));

    /// Update!
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    TEST(ECSUtil::IsDestroyed(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) == NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Invalid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Invalid(entity->GetCollection()->GetNewIndex(entity->GetId())));
}

REGISTER_TEST(World_EntitySemiRegisterLifeTime_Test, "AbstractEngine.World", TestFlags::TF_DISABLED)
{
    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // We create the entity, they will be in 'register state for one frame'
    EntityAtomicPtr entity = CreateTestEntity(world);
    TEST_CRITICAL(entity);
    TEST_CRITICAL(entity->GetCollection() != nullptr);
    TEST_CRITICAL(entity->GetWorld() != nullptr);

    // Verify create state is correct
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) == NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Invalid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Valid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyNewFlags(entity));

    /// Update!
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // Verify register state is correct, this is the frame we update w/ register
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) != NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Valid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Invalid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyFlags(world, entity));

    entity->Destroy();

    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) != NULL_PTR);

    /// Update!
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // Verify register state is correct, this is the frame we're alive!
    TEST(ECSUtil::IsUnregister(entity->GetId()));
    TEST(world->FindNewEntity(entity->GetId()) == NULL_PTR);
    TEST(world->FindRegistered(entity->GetId()) == NULL_PTR);
    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);
    TEST(world->FindUnregistered(entity->GetId()) == NULL_PTR);
    TEST(Valid(entity->GetCollection()->GetIndex(entity->GetId())));
    TEST(Invalid(entity->GetCollection()->GetNewIndex(entity->GetId())));
    TEST(VerifyFlags(world, entity));
}

REGISTER_TEST(World_LifeTime_Tests, "AbstractEngine.World")
{
    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    auto mobType = GetMobType();
    world->RegisterStaticEntityDefinition(&mobType);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // Test Normal Create Flow
    EntityAtomicPtr entity = world->CreateEntity(&mobType);
    TEST_CRITICAL(entity != NULL_PTR);
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);

    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    TEST(ECSUtil::IsAlive(entity->GetId()));
    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);
    entity->Destroy();
    TEST(ECSUtil::IsUnregister(entity->GetId()));
    TEST(world->FindEntity(entity->GetId()) != NULL_PTR);

    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    TEST(ECSUtil::IsDestroyed(entity->GetId()));
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);
    EntityAtomicWPtr weakEntity = entity;
    entity = NULL_PTR;
    TEST(weakEntity == NULL_PTR);

    // Test Temp Create Flow
    entity = world->CreateEntity(&mobType);
    TEST_CRITICAL(entity != NULL_PTR);
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);
    
    entity->Destroy();
    TEST(ECSUtil::IsDestroyed(entity->GetId()));
    weakEntity = entity;
    entity = NULL_PTR;

    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    TEST(weakEntity == NULL_PTR);
}

REGISTER_TEST(World_DestroyRegister_Test, "AbtractEngine.World")
{
    TestEnableSystem deleteEntityEnable(TestDeleteEntitySystem::sEnable);

    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);


    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    auto mobType = GetMobType();
    world->RegisterStaticEntityDefinition(&mobType);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    // Test Destroy On Register Flow (Should go Register => Unregister => Destroy)
    EntityAtomicPtr entity = world->CreateEntity(&mobType);
    TEST_CRITICAL(entity != NULL_PTR);
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);

    TestDeleteEntitySystem::sDestroyOnRegister = entity->GetId();
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    TEST(ECSUtil::IsDestroyed(entity->GetId()));
    EntityAtomicWPtr weakEntity = entity;
    entity = NULL_PTR;
    TEST(weakEntity == NULL_PTR);

    entity = world->CreateEntity(&mobType);
    TEST_CRITICAL(entity != NULL_PTR);
    TEST(ECSUtil::IsRegister(entity->GetId()));
    TEST(world->FindEntity(entity->GetId()) == NULL_PTR);

    TestDeleteEntitySystem::sDestroyOnUnregister = entity->GetId();
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);

    TEST(ECSUtil::IsDestroyed(entity->GetId()));
    weakEntity = entity;
    entity = NULL_PTR;
    TEST(weakEntity == NULL_PTR);

}

REGISTER_TEST(WorldTests, "AbstractEngine")
{
    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);
    TestUtils::RegisterDefaultServices(container);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);


    // TEST_CRITICAL(entity);
    // TEST(ECSUtil::IsRegister(entity->GetId()));
    // TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    // TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    // TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    // world->FindCollections(...);
}

}