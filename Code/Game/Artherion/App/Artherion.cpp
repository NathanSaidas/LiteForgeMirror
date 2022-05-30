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
#include "Engine/App/GameApp.h"
#include "AbstractEngine/App/AppService.h"
#include "Engine/DX11/DX11GfxDevice.h"
#include "Engine/Win32Input/Win32InputMgr.h"

#include "AbstractEngine/World/ComponentList.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/Entity.h"
#include "Engine/World/WorldImpl.h"
#include "Core/Math/Vector.h"
#include "Core/Math/Quaternion.h"

#include "Game/Artherion/ComponentTypes/TransformComponent.h"
#include "Game/Artherion/ComponentTypes/BoundsComponent.h"
#include "Game/Artherion/ComponentTypes/ModelComponent.h"

#include "Runtime/Reflection/ReflectionMgr.h"
#include "Core/Utility/Log.h"

#include "Game/Artherion/ComponentSystems/RenderSubmitComponentSystem.h"

#include <algorithm>
#include <random>

namespace lf
{
// struct EntityAGroup
// {
//     TVector<EntityId> mEntities;
//     TComponentList<TransformComponent> mTransforms;
//     TComponentList<EquipmentComponent> mEquipments;
// };
// 
// struct UntypedEntityTuple
// {
//     ComponentList* GetList(const Type* type)
//     {
//         for (ComponentList* list : mComponents)
//         {
//             if (list->GetType() == type)
//             {
//                 return list;
//             }
//         }
//         return nullptr;
//     }
// 
//     template<typename ComponentT>
//     TVector<typename ComponentT::ComponentDataType>* GetArray()
//     {
//         ComponentList* list = GetList(typeof(ComponentT));
//         if (!list)
//         {
//             return nullptr;
//         }
// 
//         TComponentList<ComponentT>* componentList = static_cast<TComponentList<ComponentT>*>(list);
//         return componentList->GetArray();
//     }
// 
//     TVector<EntityId>         mEntities;
//     TVector<ComponentListPtr> mComponents;
// };
// 
// DECLARE_PTR(ComponentFactory);
// struct TestWorld
// {
//     TVector<ComponentFactoryPtr> mFactories;
// 
//     template<typename ComponentT>
//     void Add()
//     {
//         ComponentFactoryPtr factory(LFNew<TComponentFactory<ComponentT>>());
//         mFactories.push_back(factory);
//     }
// 
//     ComponentFactory* GetFactory(const Type* type)
//     {
//         for (ComponentFactory* factory : mFactories)
//         {
//             if (factory->GetType() == type)
//             {
//                 return factory;
//             }
//         }
//         return nullptr;
//     }
// 
//     UntypedEntityTuple MakeTuple(TVector<const Type*> types)
//     {
//         std::sort(types.begin(), types.end(), [](const Type* a, const Type* b) { return a->GetName() < b->GetName(); });
// 
//         UntypedEntityTuple tuple;
//         for (const Type* type : types)
//         {
//             tuple.mComponents.push_back(GetFactory(type)->Create());
//         }
//         return tuple;
//     }
// 
// };
// 
// 
// 
// struct AbstractEntityGroup
// {
// 
// };
// 
// struct ComponentListIteratorVoid
// {
// };
// 
// template<typename ComponentT, typename ... ArgsT>
// struct TComponentListIteratorBase
// {
//     using ComponentType = ComponentT;
//     using ComponentDataType = typename ComponentT::ComponentDataType;
//     using ArrayType = TVector<ComponentDataType>;
// 
//     TComponentListIteratorBase(UntypedEntityTuple& tuple)
//     : mComponents(tuple.GetArray<ComponentT>())
//     , mNext(tuple)
//     {}
// 
//     template<typename CallbackT, typename ... ArgsT>
//     void Execute(CallbackT& callback, ArgsT... args)
//     {
//         mNext.Execute(callback, args..., mComponents);
//     }
// 
//     ArrayType* mComponents;
//     TComponentListIteratorBase<ArgsT...> mNext;
// };
// template<>
// struct TComponentListIteratorBase<ComponentListIteratorVoid>
// {
//     TComponentListIteratorBase(UntypedEntityTuple&)
//     {}
// 
//     template<typename CallbackT, typename ... ArgsT>
//     void Execute(CallbackT& callback, ArgsT ... args)
//     {
//         callback.Invoke(args...);
//     }
// };
// 
// template<typename ... ArgsT>
// using TComponentListIterator = TComponentListIteratorBase<ArgsT..., ComponentListIteratorVoid>;
// 
// struct SystemComponentTuple
// {
//     TVector<TransformComponentData>* mTransforms;
//     TVector<EquipmentComponentData>* mEquipments;
// };

static bool ForceUpdate(const char* string)
{
    AssetPath path(string);
    return GetAssetMgr().Wait(GetAssetMgr().UpdateCacheData(GetAssetMgr().FindType(path)));;;
}

static bool ForceImport(const char* string)
{
    AssetPath path(string);
    if(GetAssetMgr().FindType(path) != nullptr)
    {
        return ForceUpdate(string);
    }
    if (!GetAssetMgr().Wait(GetAssetMgr().Import(path)))
    {
        return false;
    }

    return ForceUpdate(string);
}

template<EntityId TOTAL, EntityId NUM_HIGH, EntityId NUM_NORMAL>
TVector<EntityId> GenId()
{
    const EntityId NUM_LOW = TOTAL - (NUM_HIGH + NUM_NORMAL);
    LF_STATIC_ASSERT(TOTAL == (NUM_HIGH + NUM_NORMAL + NUM_LOW));

    SizeT numHigh = NUM_HIGH;
    SizeT numNormal = NUM_NORMAL;

    TVector<UInt32> ids;
    for (UInt32 i = 0; i < TOTAL; ++i)
    {
        UInt32 id = i;
        if (numHigh > 0)
        {
            ECSUtil::SetHighPriority(id);
            --numHigh;
        }
        else if (numNormal > 0)
        {
            ECSUtil::SetNormalPriority(id);
            --numNormal;
        }
        else
        {
            ECSUtil::SetLowPriority(id);
        }
        ids.push_back(i);

    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ids.begin(), ids.end(), g);

    return ids;
}

void SortTest()
{
    const EntityId TOTAL = 1000;
    const EntityId NUM_HIGH = 200;
    const EntityId NUM_NORMAL = 480;

    for (SizeT i = 0; i < 100; ++i)
    {
        TVector<UInt32> ids = GenId<TOTAL, NUM_HIGH, NUM_NORMAL>();

        Timer t;
        // -- Using Sort
        // std::stable_sort(ids.begin(), ids.end(),
        //     [](UInt32 a, UInt32 b) -> bool
        //     {
        //         return GetPriority(a) < GetPriority(b);
        //     });

        t.Start();
        TVector<UInt32> sorted;
        sorted.reserve(ids.size());
        for (UInt32 high : ids)
        {
            if (ECSUtil::GetPriority(high) == ECSUtil::EntityPriority::HIGH)
            {
                sorted.push_back(high);
            }
        }
        for (UInt32 mid : ids)
        {
            if (ECSUtil::GetPriority(mid) == ECSUtil::EntityPriority::NORMAL)
            {
                sorted.push_back(mid);
            }
        }
        for (UInt32 low : ids)
        {
            if (ECSUtil::GetPriority(low) == ECSUtil::EntityPriority::LOW)
            {
                sorted.push_back(low);
            }
        }
        sorted.swap(ids);
        sorted.clear();
        t.Stop();

        TimeTypes::Milliseconds ms(ToMilliseconds(TimeTypes::Seconds(t.GetDelta())));
        gSysLog.Info(LogMessage("Sorted ") << TOTAL << " entities in " << ms.mValue << "; HIGH=" << NUM_HIGH << ", NORMAL=" << NUM_NORMAL << ", LOW=" << (TOTAL - (NUM_HIGH + NUM_NORMAL)));
    }

}

void ClearCache()
{
    TVector<ByteT> data;
    data.resize(ToMB<SizeT>(16));
    for (SizeT i = 0; i < data.size(); ++i)
    {
        data[i] = (i * 2) & 0xFF;
    }
}
void TestFind()
{
    const EntityId TOTAL = 1000;
    const EntityId NUM_HIGH = 200;
    const EntityId NUM_NORMAL = 480;
    for (SizeT k = 0; k < 100; ++k)
    {
        TVector<EntityId> ids = GenId<TOTAL, NUM_HIGH, NUM_NORMAL>();
        TVector<EntityId> maskedIds;
        maskedIds.resize(ids.size());
        for (SizeT i = 0; i < ids.size(); ++i)
        {
            maskedIds[i] = (ids[i] & ECSUtil::ENTITY_ID_BITMASK);
        }

        Timer t;

        ClearCache();
        t.Start();
        for (SizeT i = 0; i < ids.size(); ++i)
        {
            EntityId id = (ids[i] & ECSUtil::ENTITY_ID_BITMASK);
            (void)std::find_if(ids.begin(), ids.end(), [id](EntityId entity) { return (entity & ECSUtil::ENTITY_ID_BITMASK) == id; });
        }
        t.Stop();

        TimeTypes::Milliseconds findIf(ToMilliseconds(TimeTypes::Seconds(t.GetDelta())));

        ClearCache();
        t.Start();
        for (SizeT i = 0; i < ids.size(); ++i)
        {
            EntityId id = (ids[i] & ECSUtil::ENTITY_ID_BITMASK);
            (void)std::find(maskedIds.begin(), maskedIds.end(), id);
        }
        t.Stop();
        TimeTypes::Milliseconds find(ToMilliseconds(TimeTypes::Seconds(t.GetDelta())));
        gSysLog.Info(LogMessage("FindIf=") << findIf.mValue << ", Find=" << find.mValue);
    }
}

// How do I get component specific data of an entity? (Do we return pointers?)
  
class ArtherionAppService : public Service
{
    DECLARE_CLASS(ArtherionAppService, Service);
public:
    AppService* mAppService = nullptr;
    GfxDevice* mGfxService = nullptr;
    InputMgr* mInputService = nullptr;

    ThreadFence mFrameFence;

    APIResult<ServiceResult::Value> OnStart() override
    {
        APIResult<ServiceResult::Value> result = Super::OnStart();
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            return result;
        }

        Assert(mFrameFence.Initialize());
        mFrameFence.Set(true);

        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }

    APIResult<ServiceResult::Value> OnPostInitialize() override
    {
        APIResult<ServiceResult::Value> result = Super::OnPostInitialize();
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            return result;
        }

        mAppService = GetServices()->GetService<AppService>();
        if (!mAppService)
        {
            return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_FAILED);
        }

        mGfxService = GetServices()->GetService<GfxDevice>();
        if (!mGfxService)
        {
            return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_FAILED);
        }

        mInputService = GetServices()->GetService<InputMgr>();

        // mWindow = mGfxService->CreateWindow("Artherion", 640, 640);
        // Assert(mWindow->Open());

        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }
    APIResult<ServiceResult::Value> OnFrameUpdate()
    {
        mFrameFence.Signal();

        APIResult<ServiceResult::Value> result = Super::OnFrameUpdate();
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            return result;
        }

        // if (!mWindow || !mWindow->IsOpen())
        // {
        //     mAppService->Stop();
        // }

        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }
    APIResult<ServiceResult::Value> OnShutdown(ServiceShutdownMode mode)
    {
        // if (mWindow && mode != ServiceShutdownMode::SHUTDOWN_FAST)
        // {
        //     // mWindow->Close();
        //     mWindow.Release();
        // }

        mFrameFence.Set(false);

        mFrameFence.Destroy();

        APIResult<ServiceResult::Value> result = Super::OnShutdown(mode);
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            return result;
        }
        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }
};
DEFINE_CLASS(lf::ArtherionAppService) { NO_REFLECTION; }

// TODO:
// TODO: 
class ArtherionApp : public GameApp
{
    DECLARE_CLASS(ArtherionApp, GameApp);
public:
    ArtherionApp() = default;
    virtual ~ArtherionApp() = default;

    ServiceResult::Value RegisterServices() override
    {
        auto appService = MakeService<AppService>();
        if (!GetServices().Register(appService))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        // if (!GetServices().Register(MakeService<DX11GfxDevice>()))
        // {
        //     return ServiceResult::SERVICE_RESULT_FAILED;
        // }
        if (!GetServices().Register(MakeService<ArtherionAppService>()))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        if (!GetServices().Register(MakeService<Win32InputMgr>()))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }
        if (!GetServices().Register(MakeService<WorldImpl>()))
        {
            return ServiceResult::SERVICE_RESULT_FAILED;
        }

        appService->SetRunning();

        return ServiceResult::SERVICE_RESULT_SUCCESS;
    }

    void BackgroundRun()
    {
        // // Make a call to get the dispatcher, since we're not running on the game thread.
        // dispatcher = GetServices()->GetService<Dispatcher>();
        // dispatcher->Call(ThreadID::Game, [this]()
        // {
        //     // Since we're a game, we'll want to create our main window.
        //     graphics = GetServices()->GetService<DX11GfxDevice>();
        //     window = graphics->CreateWindow();
        //     graphics->SetMainWindow(window);
        // 
        //     // Then we want to load the splash screen level.
        //     // This will create an object that will then begin loading
        //     // the main menu asynchronously.
        //     world = GetServices()->GetService<World>();
        //
        //     struct LevelLoadArgs {
        //          Type: "lf::SplashLevelArgs" {
        //              TargetLevelOverride: "Engine//Levels/NathanSandbox.level"
        //          }
        //     }
        // 
        //     world->Load("Engine//Levels/Splash.level");
        // });

        // 
        
    }
};
DEFINE_CLASS(lf::ArtherionApp) { NO_REFLECTION; }


} // namespace lf