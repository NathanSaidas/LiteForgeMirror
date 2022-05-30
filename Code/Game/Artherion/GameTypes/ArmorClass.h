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

#include "Core/Common/Enum.h"
#include "Game/Artherion/GameTypes/TypeDefs.h"
#include "Game/Artherion/GameTypes/DamageType.h"

namespace lf
{

DECLARE_ENUM(ArmorClass,
ARMOR_CLASS_UNARMORED,
ARMOR_CLASS_CLOTH,
ARMOR_CLASS_LEATHER,
ARMOR_CLASS_MAIL,
ARMOR_CLASS_PLATE);

// Unarmored is weak to physical attacks.

namespace ArmorClass 
{


// struct ArmorClassConstantsArray
// {
//     ArmorClassConstants DAMAGE_TYPE[DamageType::MAX_VALUE];
// };

constexpr DamageNumber ARMOR_CLASS_CONSTANTS[ArmorClass::MAX_VALUE][DamageType::MAX_VALUE] =
{
    // Unarmored, Bonus Damage from Physical Attacks
    {
        { DamageNumber(1.0f + 0.06) }, // PHYSICAL_SLASH
        { DamageNumber(1.0f + 0.06) }, // PHYSICAL_PIERCE
        { DamageNumber(1.0f + 0.06) }, // PHYSICAL_BLUNT
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_FIRE
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_WATER
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_EARTH
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_AIR
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_SHADOW
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_LIGHT
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_ARCANE
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_BLOOD
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_NECROMANCY
    },
    // Cloth, Reduced damage from water/earth/air/arcane/necromancy
    //        Bonus damage from fire Damage
    {
        { DamageNumber(1.0f - 0.00) }, // PHYSICAL_SLASH
        { DamageNumber(1.0f - 0.00) }, // PHYSICAL_PIERCE
        { DamageNumber(1.0f - 0.00) }, // PHYSICAL_BLUNT
        { DamageNumber(1.0f + 0.05) }, // MAGICAL_FIRE
        { DamageNumber(1.0f - 0.03) }, // MAGICAL_WATER
        { DamageNumber(1.0f - 0.03) }, // MAGICAL_EARTH
        { DamageNumber(1.0f - 0.03) }, // MAGICAL_AIR
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_SHADOW
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_LIGHT
        { DamageNumber(1.0f - 0.03) }, // MAGICAL_ARCANE
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_BLOOD
        { DamageNumber(1.0f - 0.03) }, // MAGICAL_NECROMANCY
    },
    // Leather, Reduced damage from physical/earth/blood (small),
    {
        { DamageNumber(1.0f - 0.03) }, // PHYSICAL_SLASH
        { DamageNumber(1.0f - 0.03) }, // PHYSICAL_PIERCE
        { DamageNumber(1.0f - 0.03) }, // PHYSICAL_BLUNT
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_FIRE
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_WATER
        { DamageNumber(1.0f - 0.03) }, // MAGICAL_EARTH
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_AIR
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_SHADOW
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_LIGHT
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_ARCANE
        { DamageNumber(1.0f - 0.03) }, // MAGICAL_BLOOD
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_NECROMANCY
    },
    // Mail, Reduced damage from physical, resist to 
    {
        { DamageNumber(1.0f - 0.05) }, // PHYSICAL_SLASH
        { DamageNumber(1.0f - 0.05) }, // PHYSICAL_PIERCE
        { DamageNumber(1.0f - 0.05) }, // PHYSICAL_BLUNT
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_FIRE
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_WATER
        { DamageNumber(1.0f - 0.05) }, // MAGICAL_EARTH
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_AIR
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_SHADOW
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_LIGHT
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_ARCANE
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_BLOOD
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_NECROMANCY
    },
    // Plate reduced damage from physical/earth
    //       increased damage from arcane
    {
        { DamageNumber(1.0f - 0.07) }, // PHYSICAL_SLASH
        { DamageNumber(1.0f - 0.07) }, // PHYSICAL_PIERCE
        { DamageNumber(1.0f - 0.07) }, // PHYSICAL_BLUNT
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_FIRE
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_WATER
        { DamageNumber(1.0f - 0.05) }, // MAGICAL_EARTH
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_AIR
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_SHADOW
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_LIGHT
        { DamageNumber(1.0f + 0.05) }, // MAGICAL_ARCANE
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_BLOOD
        { DamageNumber(1.0f - 0.00) }, // MAGICAL_NECROMANCY
    },
};

constexpr DamageNumber GetConstant(const ArmorClass::Value armorClass, const DamageType::Value damageType)
{
    return ARMOR_CLASS_CONSTANTS[armorClass][damageType];
}
} // namespace ArmorClass


} // namespace lf
