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
#include "AbstractEngine/World/ECSCommon.h"
#include "Game/Artherion/GameTypes/SeasonType.h"
#include "Game/Artherion/GameTypes/AgeType.h"

namespace lf
{

// NOTE: Unit Regen = UpgradeValue( Base Regen * Season Modifier(+- 2%) * Age Modifier(+-5%) )
// NOTE: 
struct StaticUnitPropertiesData : public ComponentData
{
    void Serialize(Stream& s);

    //** The base health a unit should have
    Int32 mBaseHealth;
    //** The base movement speed a unit should have ( meters/sec )
    Int32 mBaseMovementSpeed;
    //** Unit base health regen
    Float32 mBaseHealthRegen;
    //** Unit seasonal health regen value
    Float32 mSeasonHealthRegenModifier[SeasonType::MAX_VALUE];
    //** Unit age health regen modifier
    Float32 mAgeHealthRegenModifier[SeasonType::MAX_VALUE];
    //** Unit base energy regen
    Int32 mBaseEnergyRegen;
    //** Unit base armor
    Int32 mBaseArmor;
    //** Unit base magic resist
    Int32 mBaseMagicResist;
};

class StaticUnitProperties : public TComponentImpl<StaticUnitProperties, StaticUnitPropertiesData>
{
    DECLARE_CLASS(StaticUnitProperties, Component);
};

}
