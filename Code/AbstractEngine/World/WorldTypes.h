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

#include "Core/Common/API.h"
#include "Core/Common/Enum.h"
#include "Core/Common/Types.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/SmartCallback.h"

namespace lf 
{

using EntityId = UInt32;
using EntityIdAtomic = AtomicU32;

using ComponentId = UInt16;
using ComponentSequence = TVector<ComponentId>;

enum : EntityId { INVALID_ENTITY_ID = INVALID32 };

namespace ECSUtil
{
constexpr EntityId ENTITY_ID_BITMASK = 0xFFFFFU;
constexpr EntityId ENTITY_FLAG_BITMASK = 0xFFF0000;
constexpr EntityId ENTITY_FLAG_HIGH_PRIORITY = 1U << 31U;
constexpr EntityId ENTITY_FLAG_LOW_PRIORITY = 1U << 30U;
constexpr EntityId ENTITY_FLAG_LIFE_MINOR_BIT = 1U << 29U;
constexpr EntityId ENTITY_FLAG_LIFE_MAJOR_BIT = 1U << 28U;
constexpr EntityId ENTITY_FLAG_RESERVED_2 = 1U << 27U;
constexpr EntityId ENTITY_FLAG_RESERVED_3 = 1U << 26U;
constexpr EntityId ENTITY_FLAG_RESERVED_4 = 1U << 25U;
constexpr EntityId ENTITY_FLAG_RESERVED_5 = 1U << 24U;
constexpr EntityId ENTITY_FLAG_RESERVED_6 = 1U << 23U;
constexpr EntityId ENTITY_FLAG_RESERVED_7 = 1U << 22U;
constexpr EntityId ENTITY_FLAG_RESERVED_8 = 1U << 21U;
constexpr EntityId ENTITY_FLAG_RESERVED_9 = 1U << 20U;

// Entity Priority is extra information used by systems.
// 
// High - The entity will update every frame
// Normal - The entity will update every frame (within system frame budget), otherwise with accumlated frame deltas
// Low - The entity will update with accumulated frame deltas (within entity frame budget)
//
// tldr; High priority updates always, normal usually always, low gets split between frames.
DECLARE_STRICT_ENUM(EntityPriority,
HIGH,
NORMAL,
LOW);

// Entity LifeState is used to determine 
// 
// Register - Entity was just created, not yet apart of the 'entity' collection
// Alive - Entity passed 1 frame of updates and is 'registered'
// Unregister - Entity was just destroyed, and will unregister
// Destroyed - Entity is no longer valid (can't query world/entity collection)
//
// Bit State [ Minor, Major ]
// 0 0 - Register
// 0 1 - Alive
// 1 0 - Unregister
// 1 1 - Destroyed
DECLARE_STRICT_ENUM(EntityLifeState,
REGISTER,
ALIVE,
UNREGISTER,
DESTROYED);

// Defines the type of update for a system
// Serial - Update called exclusively on main thread, no other updates can run while this runs
// Concurrent - Update called on system worker thread, concurrently with other systems (system update is serial)
// Serial Distributed - Update is called exclusively on main thread, but distributes internal system updates across multiple worker threads.
// Concurrent Distributed - Update called on system worker thread, concurrent with other systems and work is distributed for this system on multiple worker threads.
//
//      Y-Axis = Thread, X-Axis = Time
//
// Serial
//      A => B => C => D => ...
// Concurrent
//      A => ...
//      B => C => ...
//      D => ...
// Serial Distributed
//      
//           +=> a.0  |      |=> b.0
//      A ===|=> a.1  | B ===|=> b.1 
//           +=> a.2  |      |=> b.2
//
// Concurrent Distributed
//
//           +=> a.0  |      |=> b.0
//      A ===|=> a.1  | B ===|=> b.1 
//           +=> a.2  |      |=> b.2
//
//           +=> c.0  |      |=> d.0
//      C ===|=> c.1  | D ===|=> d.1 
//           +=> c.2  |      |=> d.2
//
DECLARE_STRICT_ENUM(UpdateType,
SERIAL,
CONCURRENT,
SERIAL_DISTRIBUTED,
CONCURRENT_DISTRIBUTED);

using UpdateCallback = TCallback<void>;

// Priority
LF_FORCE_INLINE EntityId SetHighPriority(EntityId id)
{
    return (id & ~(ENTITY_FLAG_LOW_PRIORITY)) | ENTITY_FLAG_HIGH_PRIORITY;
}
LF_FORCE_INLINE EntityId SetNormalPriority(EntityId id)
{
    return (id & ~(ENTITY_FLAG_HIGH_PRIORITY | ENTITY_FLAG_LOW_PRIORITY));
}
LF_FORCE_INLINE EntityId SetLowPriority(EntityId id)
{
    return (id & ~(ENTITY_FLAG_HIGH_PRIORITY)) | ENTITY_FLAG_LOW_PRIORITY;
}
// Lifetime
LF_FORCE_INLINE EntityId SetRegister(EntityId id)
{
    return (id & ~(ENTITY_FLAG_LIFE_MAJOR_BIT | ENTITY_FLAG_LIFE_MINOR_BIT));
}
LF_FORCE_INLINE EntityId SetAlive(EntityId id)
{
    return (id & ~(ENTITY_FLAG_LIFE_MINOR_BIT)) | ENTITY_FLAG_LIFE_MAJOR_BIT;
}
LF_FORCE_INLINE EntityId SetUnregister(EntityId id)
{
    return (id & ~(ENTITY_FLAG_LIFE_MAJOR_BIT)) | ENTITY_FLAG_LIFE_MINOR_BIT;
}
LF_FORCE_INLINE EntityId SetDestroyed(EntityId id)
{
    return id | (ENTITY_FLAG_LIFE_MAJOR_BIT | ENTITY_FLAG_LIFE_MINOR_BIT);
}
// Priority
LF_FORCE_INLINE bool IsHighPriority(EntityId id)
{
    return (id & ECSUtil::ENTITY_FLAG_HIGH_PRIORITY) > 0;
}
LF_FORCE_INLINE bool IsNormalPriority(EntityId id)
{
    return (id & (ECSUtil::ENTITY_FLAG_HIGH_PRIORITY | ECSUtil::ENTITY_FLAG_LOW_PRIORITY)) == 0;
}
LF_FORCE_INLINE bool IsLowPriority(EntityId id)
{
    return (id & ECSUtil::ENTITY_FLAG_LOW_PRIORITY) > 0;
}
// Lifetime
LF_FORCE_INLINE bool IsRegister(EntityId id)
{
    return (id & (ENTITY_FLAG_LIFE_MINOR_BIT | ENTITY_FLAG_LIFE_MAJOR_BIT)) == 0;
}
LF_FORCE_INLINE bool IsAlive(EntityId id)
{
    return (id & (ENTITY_FLAG_LIFE_MINOR_BIT | ENTITY_FLAG_LIFE_MAJOR_BIT)) == ENTITY_FLAG_LIFE_MAJOR_BIT;
}
LF_FORCE_INLINE bool IsUnregister(EntityId id)
{
    return (id & (ENTITY_FLAG_LIFE_MINOR_BIT | ENTITY_FLAG_LIFE_MAJOR_BIT)) == ENTITY_FLAG_LIFE_MINOR_BIT;
}
LF_FORCE_INLINE bool IsDestroyed(EntityId id)
{
    return (id & (ENTITY_FLAG_LIFE_MINOR_BIT | ENTITY_FLAG_LIFE_MAJOR_BIT)) == (ENTITY_FLAG_LIFE_MINOR_BIT | ENTITY_FLAG_LIFE_MAJOR_BIT);
}
LF_FORCE_INLINE bool IsLifeChanged(EntityId a, EntityId b)
{
    const EntityId LIFE_BITS = ENTITY_FLAG_LIFE_MINOR_BIT | ENTITY_FLAG_LIFE_MAJOR_BIT;
    return (a & LIFE_BITS) != (b & LIFE_BITS);
}
LF_FORCE_INLINE EntityPriority GetPriority(EntityId id)
{
    if(IsHighPriority(id))
    {
        return EntityPriority::HIGH;
    }
    else if(IsLowPriority(id))
    {
        return EntityPriority::LOW;
    }
    return EntityPriority::NORMAL;
}
LF_FORCE_INLINE EntityLifeState GetLifeState(EntityId id)
{
    if (IsRegister(id))
    {
        return EntityLifeState::REGISTER;
    }
    else if (IsAlive(id))
    {
        return EntityLifeState::ALIVE;
    }
    else if (IsUnregister(id))
    {
        return EntityLifeState::UNREGISTER;
    }
    return EntityLifeState::DESTROYED;
}
LF_FORCE_INLINE EntityId GetId(EntityId id)
{
    return (id & ENTITY_ID_BITMASK);
}

} // namespace ECSUtil

} // namespace lf