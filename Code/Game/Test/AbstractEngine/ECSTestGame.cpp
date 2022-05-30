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
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include "AbstractEngine/World/ComponentSystem.h"
#include "AbstractEngine/World/Entity.h"
#include "Engine/World/ComponentSystemTuple.h"
#include "Engine/World/WorldImpl.h"
#include "Game/Test/TestUtils.h"

#include "Core/Math/Vector.h"
#include "Core/Math/Random.h"
#include "Core/Utility/Log.h"

// This file will contain the types to create a very basic game deterministic simulation.
// 
// Position Component { Vector Position }           : TestPositionComponent
// Health Component { Int32 Health }                : TestHealthComponent
// Stats Component { Int32 Armor, Int32 Score }     : TestStatsComponent
// 
// Update Movement System : TestUpdateMovementSystem
//      Increment a position to a 'goal'
// Random Damage System : TestRandomDamageSystem
//      Iterate through entities apply 'random' damage to them (reduced by armor)
// Death System : TestDeathSystem
//      Check if entity is dead, if so move them back to spawn
// Score System : TestScoreSystem
//      Check if entity is alive
//
// Move => Damage => [ Death,Score ]
// 
//
// // Components:
//   Foo { Vector Position, Vector, Int32, bool }
//   Bar { Int32, bool, bool, bool, Vector }
//   Baz { 
//   
//
// System A: ( Update Movements )  [Write Position]
// System B: ( Random Damage )     [Write Health, Read Stats]
// System C: ( Death Handler )     [Write Position, Read Health]
// System D: ( Score Handler )     [Write Stats, Read Health]

namespace lf
{

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

template<typename ComponentT, typename ComponentDataT>
class TComponentTestImpl : public Component
{
public:
    using ComponentDataType = ComponentDataT;

    TComponentTestImpl() : mData(nullptr), mFactory() {}
    virtual ~TComponentTestImpl() {}
    
    void Serialize(Stream& s) override
    {
        Super::Serialize(s);
        if (mData)
        {
            mData->Serialize(s);
        }
    }

    void BeginSerialize(ComponentData* data) override { mData = static_cast<ComponentDataT*>(data); }
    void EndSerialize() { mData = nullptr; }

    ComponentFactory* GetFactory() const override { return &mFactory; }
protected:
    typename ComponentDataT* mData;
    mutable TComponentFactory<ComponentT>   mFactory;
};

/// ==========================
///  Components
/// ==========================

struct TestPositionComponentData : public ComponentData
{
    TestPositionComponentData() 
    : mPosition()
    {}
    void Serialize(Stream& s)
    {
        SERIALIZE(s, mPosition, "");
    }
    Vector mPosition;
};
class TestPositionComponent : public TComponentTestImpl<TestPositionComponent, TestPositionComponentData>
{
    DECLARE_CLASS(TestPositionComponent, Component);
};
DEFINE_CLASS(lf::TestPositionComponent) { NO_REFLECTION; }

struct TestHealthComponentData : public ComponentData
{
    TestHealthComponentData()
    : mHealth(100)
    {}
    void Serialize(Stream& s)
    {
        SERIALIZE(s, mHealth, "");
    }
    Int32 mHealth;
};

class TestHealthComponent : public TComponentTestImpl< TestHealthComponent, TestHealthComponentData>
{
    DECLARE_CLASS(TestHealthComponent, Component);
};
DEFINE_CLASS(lf::TestHealthComponent) { NO_REFLECTION; }


// Stats Component { Int32 Armor, Int32 Score }     : TestStatsComponent
struct TestStatsComponentData : public ComponentData
{
    TestStatsComponentData()
    : mArmor(75)
    , mScore(0)
    {}
    void Serialize(Stream& s)
    {
        SERIALIZE(s, mArmor, "");
        SERIALIZE(s, mScore, "");
    }
    Int32 mArmor;
    Int32 mScore;
};

class TestStatsComponent : public TComponentTestImpl< TestStatsComponent, TestStatsComponentData>
{
    DECLARE_CLASS(TestStatsComponent, Component);
};
DEFINE_CLASS(lf::TestStatsComponent) { NO_REFLECTION; }


/// ==========================
///  Systems
/// ==========================

// Update Movement System : TestUpdateMovementSystem
//      Increment a position to a 'goal'
// Random Damage System : TestRandomDamageSystem
//      Iterate through entities apply 'random' damage to them (reduced by armor)
// Death System : TestDeathSystem
//      Check if entity is dead, if so move them back to spawn
// Score System : TestScoreSystem
//      Check if entity is alive


struct TestGameTuple
{
    using TupleType = TComponentSystemTuple<TestPositionComponent, TestHealthComponent, TestStatsComponent>;

    TComponentTupleType<TestPositionComponent>  mPositions;
    TComponentTupleType<TestHealthComponent>    mHealth;
    TComponentTupleType<TestStatsComponent>     mStats;
    TVector<EntityCollection*>                   mCollections;
};

class TestDeathFence : public ComponentSystemFence
{
    DECLARE_CLASS(TestDeathFence, ComponentSystemFence);
};
DEFINE_CLASS(lf::TestDeathFence) { NO_REFLECTION; }
class TestPostDeathFence : public ComponentSystemFence
{
    DECLARE_CLASS(TestPostDeathFence, ComponentSystemFence);
};
DEFINE_CLASS(lf::TestPostDeathFence) { NO_REFLECTION; }

class TestUpdateMoveSystem : public ComponentSystem, public TSystemTestAttributes<TestUpdateMoveSystem>
{
    DECLARE_CLASS(TestUpdateMoveSystem, ComponentSystem);
public:
    bool mRegistered;
    TestGameTuple mTuple;

    bool IsEnabled() const override { return sEnable; }

    bool OnInitialize() override
    {
        mRegistered = false;
        return true;
    }

    void OnBindTuples() override
    {
        BindTuple(mTuple);
    }

    void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            TVector<const Type*> reads = { };
            TVector<const Type*> writes = { typeof(TestPositionComponent) };

            TEST(StartConstantUpdate(UpdateCallback::Make(this, &TestUpdateMoveSystem::Update), nullptr, sUpdateType, reads, writes));
            mRegistered = true;
        }
    }

    void Update()
    {
        ForEach(mTuple, [this](TestPositionComponentData* position, TestHealthComponentData*, TestStatsComponentData*) { UpdateEntity(position, nullptr, nullptr); });
    }

    void UpdateEntity(TestPositionComponentData* position, TestHealthComponentData*, TestStatsComponentData*)
    {
        position->mPosition += Vector::Forward;
    }

};
DEFINE_CLASS(lf::TestUpdateMoveSystem) { NO_REFLECTION; }

class TestRandomDamageSystem : public ComponentSystem, public TSystemTestAttributes<TestRandomDamageSystem>
{
    DECLARE_CLASS(TestRandomDamageSystem, ComponentSystem);
public:
    bool mRegistered;
    TestGameTuple mTuple;
    Int32 mRand;

    bool IsEnabled() const override { return sEnable; }

    bool OnInitialize() override
    {
        mRegistered = false;
        mRand = 0x78929893;
        return true;
    }

    void OnBindTuples() override
    {
        BindTuple(mTuple);
    }

    void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            TVector<const Type*> reads = { };
            TVector<const Type*> writes = { typeof(TestHealthComponent) };

            TEST(StartConstantUpdate(UpdateCallback::Make(this, &TestRandomDamageSystem::Update), nullptr, sUpdateType, reads, writes));
            mRegistered = true;
        }
    }

    void Update()
    {
        ForEach(mTuple, [this](TestPositionComponentData* , TestHealthComponentData* health, TestStatsComponentData*) { UpdateEntity(nullptr, health, nullptr); });
    }

    void UpdateEntity(TestPositionComponentData* , TestHealthComponentData* health, TestStatsComponentData*)
    {
        if (Random::Range(mRand, 1, 100) > 85)
        {
            health->mHealth = Max(0, health->mHealth - 5);
        }
    }

};
DEFINE_CLASS(lf::TestRandomDamageSystem) { NO_REFLECTION; }

class TestDeathSystem : public ComponentSystem, public TSystemTestAttributes<TestDeathSystem>
{
    DECLARE_CLASS(TestDeathSystem, ComponentSystem);
public:
    bool mRegistered;
    TestGameTuple mTuple;

    bool IsEnabled() const override { return sEnable; }

    bool OnInitialize() override
    {
        mRegistered = false;
        if (!GetWorld()->CreateFenceAfter(typeof(TestDeathFence), typeof(ComponentSystemUpdateFence)))
        {
            return false;
        }
        if (!GetWorld()->CreateFenceAfter(typeof(TestPostDeathFence), typeof(TestDeathFence)))
        {
            return false;
        }
        return true;
    }

    void OnBindTuples() override
    {
        BindTuple(mTuple);

    }

    void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            {
                TVector<const Type*> reads = { typeof(TestHealthComponent)  };
                TVector<const Type*> writes = { typeof(TestPositionComponent) };

                TEST(StartConstantUpdate("Teleport", UpdateCallback::Make(this, &TestDeathSystem::UpdateTeleport), typeof(TestDeathFence), ECSUtil::UpdateType::CONCURRENT, reads, writes));
            }
            {
                TVector<const Type*> reads = { };
                TVector<const Type*> writes = { typeof(TestHealthComponent) };

                TEST(StartConstantUpdate("ResetHealth", UpdateCallback::Make(this, &TestDeathSystem::UpdateResetHealth), typeof(TestPostDeathFence), ECSUtil::UpdateType::SERIAL, reads, writes));
            }

            mRegistered = true;
        }
    }

    void UpdateTeleport()
    {
        ForEach(mTuple, [this](TestPositionComponentData* position, TestHealthComponentData* health, TestStatsComponentData*) { UpdateTeleportEntity(position, health, nullptr); });
    }

    void UpdateResetHealth()
    {
        ForEachEntity(mTuple, [this](EntityId id, TestPositionComponentData* , TestHealthComponentData* health, TestStatsComponentData*) { UpdateResetEntityHealth(id, nullptr, health, nullptr); });
    }

    void UpdateTeleportEntity(TestPositionComponentData* position, TestHealthComponentData* health, TestStatsComponentData*)
    {
        if (health->mHealth == 0)
        {
            // gTestLog.Warning(LogMessage("Reset position for dead entity. "));
            position->mPosition = Vector();
        }
    }

    void UpdateResetEntityHealth(EntityId id,TestPositionComponentData* , TestHealthComponentData* health, TestStatsComponentData*)
    {
        (id);
        if (health->mHealth == 0) 
        {
            // gTestLog.Warning(LogMessage("Reset health for dead entity. Id=") << id);
            health->mHealth = 100;
        }
    }

};
DEFINE_CLASS(lf::TestDeathSystem) { NO_REFLECTION; }

class TestScoreSystem : public ComponentSystem, public TSystemTestAttributes<TestScoreSystem>
{
    DECLARE_CLASS(TestScoreSystem, ComponentSystem);
public:
    bool mRegistered;
    TestGameTuple mTuple;

    bool IsEnabled() const override { return sEnable; }

    bool OnInitialize() override
    {
        mRegistered = false;
        return true;
    }

    void OnBindTuples() override
    {
        BindTuple(mTuple);

    }

    void OnScheduleUpdates() override
    {
        if (!mRegistered)
        {
            TVector<const Type*> reads = { typeof(TestHealthComponent) };
            TVector<const Type*> writes = { typeof(TestStatsComponent) };

            TEST(StartConstantUpdate(UpdateCallback::Make(this, &TestScoreSystem::Update), typeof(TestDeathFence), sUpdateType, reads, writes));
            mRegistered = true;
        }
    }

    void Update()
    {
        ForEach(mTuple, [this](TestPositionComponentData* , TestHealthComponentData* health, TestStatsComponentData* stats) { UpdateEntity(nullptr, health, stats); });
    }

    void UpdateEntity(TestPositionComponentData* , TestHealthComponentData* health, TestStatsComponentData* stats)
    {
        if (health->mHealth == 0)
        {
            ++stats->mScore;
        }
    }

};
DEFINE_CLASS(lf::TestScoreSystem) { NO_REFLECTION; }

static void ServiceUpdateSim(ServiceContainer& container)
{
    TEST_CRITICAL(container.BeginFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.FrameUpdate() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.EndFrame() == ServiceResult::SERVICE_RESULT_SUCCESS);
}

static EntityDefinition GetMobType()
{
    EntityDefinition def;
    def.SetComponentTypes({ typeof(TestHealthComponent), typeof(TestPositionComponent), typeof(TestStatsComponent) });
    return def;
}

REGISTER_TEST(World_SerialGameSim_Test, "AbstractEngine.World")
{
    TestEnableSystem enableMove(TestUpdateMoveSystem::sEnable);
    TestEnableSystem enableDamage(TestRandomDamageSystem::sEnable);
    TestEnableSystem enableDeath(TestDeathSystem::sEnable);
    TestEnableSystem enableScore(TestScoreSystem::sEnable);

    const ECSUtil::UpdateType UPDATE_TYPE = ECSUtil::UpdateType::CONCURRENT;

    TestOverrideUpdateType serialMove(TestUpdateMoveSystem::sUpdateType, UPDATE_TYPE);
    TestOverrideUpdateType serialDamage(TestRandomDamageSystem::sUpdateType, UPDATE_TYPE);
    TestOverrideUpdateType serialDeath(TestDeathSystem::sUpdateType, UPDATE_TYPE);
    TestOverrideUpdateType serialScore(TestScoreSystem::sUpdateType, UPDATE_TYPE);

    TStrongPointer<WorldImpl> world(LFNew<WorldImpl>());
    world->SetType(typeof(WorldImpl));

    // Simulate Services...
    ServiceContainer container({ typeof(World) });
    container.Register(world);

    TEST_CRITICAL(container.Start() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.TryInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    TEST_CRITICAL(container.PostInitialize() == ServiceResult::SERVICE_RESULT_SUCCESS);
    ServiceUpdateSim(container);

    auto mobType = GetMobType();
    world->RegisterStaticEntityDefinition(&mobType);
    world->CreateEntity(&mobType);

    ServiceUpdateSim(container);

    for (SizeT i = 0; i < 10; ++i)
    {
        world->CreateEntity(&mobType);
    }

    for (SizeT i = 0; i < 10000; ++i)
    {
        ServiceUpdateSim(container);
    }


    TEST(container.Shutdown(ServiceShutdownMode::SHUTDOWN_NORMAL) == ServiceResult::SERVICE_RESULT_SUCCESS);
}

} // namespace lf