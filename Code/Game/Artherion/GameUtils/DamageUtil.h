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

#include "Core/Utility/Utility.h"
#include "Core/Math/MathFunctions.h"
#include "Game/Artherion/GameTypes/ArmorClass.h"
#include "Game/Artherion/GameTypes/TypeDefs.h"

// DECLARE_ENUM(ArmorClass,
// ARMOR_CLASS_UNARMORED,
// ARMOR_CLASS_CLOTH,
// ARMOR_CLASS_LEATHER,
// ARMOR_CLASS_MAIL,
// ARMOR_CLASS_PLATE);

namespace lf
{
namespace DamageUtil
{
// Armor Class Constants:
// 
// Unarmored Bonus Constant


constexpr DamageNumber CalcResistance(const DamageNumber resistanceRating, const DamageNumber penetrationRating)
{
    return resistanceRating - penetrationRating;
}

DamageNumber CalcResistance(DamageNumber resistanceRating, DamageNumber penetrationRating, const DiminishingTable& resistanceTable, const DiminishingTable& penetrationTable)
{
    DamageNumber percentSum = DamageNumber(0);
    DamageNumber rating = resistanceRating - penetrationRating;
    const bool penetrated = rating < DamageNumber(0);
    const DiminishingTable& tbl = penetrated ? penetrationTable : resistanceTable;
    rating = Abs(rating);
    for (const DiminishingValue& value : tbl.mValues)
    {
        percentSum += Max(Min(rating / value.mRating, DamageNumber(1)) * value.mPercentage, DamageNumber(0));
        rating = Max(rating - value.mRating, DamageNumber(0));
    }
    return penetrated ? 1.0f + (percentSum / DamageNumber(100)) : 1.0f - (percentSum / DamageNumber(100));
}


struct DamageDefenseData
{
    ArmorClass::Value   mArmorClass;
    DamageNumber        mArmorAndResistance[DamageType::MAX_VALUE];
};

struct DamageAttackData
{
    DamageNumber mDamage[DamageType::MAX_VALUE];
    DamageNumber mArmorPenetration[DamageType::MAX_VALUE];
};

struct DamageCombined
{
    DamageNumber mFinalDamage[DamageType::MAX_VALUE];
};



} // namespace DamageUtil
} // namespace lf
