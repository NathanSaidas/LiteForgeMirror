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
#include "ComponentSystem.h"
#include "AbstractEngine/World/World.h"
#include "Core/Utility/Log.h"

namespace lf
{
DEFINE_ABSTRACT_CLASS(lf::ComponentSystemFence) { NO_REFLECTION; }
DEFINE_ABSTRACT_CLASS(lf::ComponentSystemRegisterFence) { NO_REFLECTION; }
DEFINE_ABSTRACT_CLASS(lf::ComponentSystemUpdateFence) { NO_REFLECTION; }
DEFINE_ABSTRACT_CLASS(lf::ComponentSystemUnregisterFence) { NO_REFLECTION; }

DEFINE_ABSTRACT_CLASS(lf::ComponentSystem) { NO_REFLECTION; }

ComponentSystem::~ComponentSystem()
{}
bool ComponentSystem::Initialize(World* world)
{
    mWorld = world;
    if (GetWorld()->LogFenceUpdateVerbose())
    {
        gTestLog.Info(LogMessage("[System Call] Initialize ") << GetType()->GetFullName());
    }

    return OnInitialize();
}

void ComponentSystem::BindTuples()
{
    if (GetWorld()->LogFenceUpdateVerbose())
    {
        gTestLog.Info(LogMessage("[System Call] Bind Tuples ") << GetType()->GetFullName());
    }

    OnBindTuples();
}
void ComponentSystem::ScheduleUpdates()
{
    if (GetWorld()->LogFenceUpdateVerbose())
    {
        gTestLog.Info(LogMessage("[System Call] Schedule Updates ") << GetType()->GetFullName());
    }

    OnScheduleUpdates();
}

bool ComponentSystem::IsEnabled() const
{
    return true;
}

APIResult<bool> ComponentSystem::ScheduleUpdate(
    const String& name, 
    const ECSUtil::UpdateCallback& callback, 
    const Type* fence, 
    ECSUtil::UpdateType updateType)
{
    World::UpdateInfo info;
    info.mName = CreateUpdateName(name);
    info.mSystem = this;
    info.mUpdateCallback = callback;
    info.mUpdateType = updateType;
    info.mFenceType = fence ? fence : typeof(ComponentSystemUpdateFence);
    return GetWorld()->ScheduleUpdate(info);
}
APIResult<bool> ComponentSystem::StartConstantUpdate(
    const ECSUtil::UpdateCallback& callback, 
    const Type* fence, 
    ECSUtil::UpdateType updateType,
    const TVector<const Type*>& readComponents,
    const TVector<const Type*>& writeComponents)
{
    return StartConstantUpdate(String(), callback, fence, updateType, readComponents, writeComponents);
}
APIResult<bool> ComponentSystem::StartConstantUpdate(
    const String& name, 
    const ECSUtil::UpdateCallback& callback, 
    const Type* fence, 
    ECSUtil::UpdateType updateType,
    const TVector<const Type*>& readComponents,
    const TVector<const Type*>& writeComponents)
{
    World::UpdateInfo info;
    info.mName = CreateUpdateName(name);
    info.mSystem = this;
    info.mUpdateCallback = callback;
    info.mUpdateType = updateType;
    info.mFenceType = fence ? fence : typeof(ComponentSystemUpdateFence);
    info.mReadComponents = readComponents;
    info.mWriteComponents = writeComponents;
    return GetWorld()->StartConstantUpdate(info);
}
APIResult<bool> ComponentSystem::StopConstantUpdate(const String& name)
{
    return GetWorld()->StopConstantUpdate(CreateUpdateName(name));
}

Token ComponentSystem::CreateUpdateName(const String& name)
{
    if (name.Empty())
    {
        return Token(String(GetType()->GetFullName().CStr()) + ".Update");
    }
    return Token(String(GetType()->GetFullName().CStr()) + "." + name);
}

bool ComponentSystem::OnInitialize()
{
    return true;
}

void ComponentSystem::OnBindTuples() // 
{

}
void ComponentSystem::OnScheduleUpdates()
{

}

} // namespace lf