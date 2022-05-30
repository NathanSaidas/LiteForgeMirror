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
#include "DamageUtil.h"
#include "Game/Artherion/GameTypes/ArmorClass.h"
#include "Game/Artherion/GameTypes/MagicSchool.h"

#include "Core/Test/Test.h"
#include "Core/Utility/Log.h"

namespace lf
{
REGISTER_TEST(DamageUtilTest, "Game.Artherion")
{
    TArmorClass armorClass = ArmorClass::ARMOR_CLASS_UNARMORED;
    TDamageType damageType = DamageType::DAMAGE_TYPE_MAGICAL_AIR;

    // DamageFloat constant = ArmorClass::GetConstant(armorClass, damageType);
    // DamageFloat effectiveness = ArmorClass::GetEffectiveness(armorClass, damageType);
    // DamageFloat maxBonus = ArmorClass::GetMaxBonus(armorClass, damageType);
    // DamageFloat maxReduction = ArmorClass::GetMaxReduction(armorClass, damageType);
    // 
    // gSysLog.Info(LogMessage("ArmorClass=") << armorClass.GetString());
    // gSysLog.Info(LogMessage("DamageType=") << damageType.GetString());
    // gSysLog.Info(LogMessage("Constant=") << constant);
    // gSysLog.Info(LogMessage("Effectiveness=") << effectiveness);
    // gSysLog.Info(LogMessage("Max Bonus=") << maxBonus);
    // gSysLog.Info(LogMessage("Max Reduction=") << maxReduction);
    // 
    // DamageUtil::CalcDefensiveConstant(constant, effectiveness, 8.0f, 13.0f);

    DiminishingTable resistanceTbl;
    resistanceTbl.mValues.push_back({  15, 40 });
    resistanceTbl.mValues.push_back({  28, 20 });
    resistanceTbl.mValues.push_back({ 400,  5 });

    DiminishingTable penetrationTbl;
    penetrationTbl.mValues.push_back({ 8, 50 });
    penetrationTbl.mValues.push_back({ 20, 25 });
    penetrationTbl.mValues.push_back({ 100, 15 });


    DamageUtil::DamageDefenseData defense;
    DamageUtil::DamageAttackData attack;


    for (SizeT i = 0; i < ArmorClass::MAX_VALUE; ++i)
    {
        TArmorClass ac(static_cast<ArmorClass::Value>(i));
        memset(defense.mArmorAndResistance, 0, sizeof(defense.mArmorAndResistance));
        defense.mArmorClass = ac;


        memset(attack.mArmorPenetration, 0, sizeof(attack.mArmorPenetration));
        memset(attack.mDamage, 0, sizeof(attack.mDamage));



        gSysLog.Info(LogMessage("Armor Class=") << ac.GetString());
        for (SizeT k = 0; k < DamageType::MAX_VALUE; ++k)
        {
            TDamageType dmgType(static_cast<DamageType::Value>(k));
            
            defense.mArmorAndResistance[dmgType] = DamageNumber(5);
            attack.mArmorPenetration[dmgType] = DamageNumber(5);
            attack.mDamage[dmgType] = 10;

            const DamageNumber DMG(10);

            DamageNumber resist = DamageUtil::CalcResistance(0, 7, resistanceTbl, penetrationTbl);
            DamageNumber value = DMG * resist;
            value *= ArmorClass::GetConstant(ac, dmgType);
            value = static_cast<DamageNumber>(Round(value));


            gSysLog.Info(LogMessage("") << StreamFillLeft(40) << dmgType.GetString() << StreamFillLeft() << ": " << ArmorClass::GetConstant(ac, dmgType) << ", " << value);
        }
    }

    // TODO: How do we compute these inputs
    // Inputs:
    
    // Outputs:
    // DamageUtil::DamageCombined combined = { 0 };
    // TODO: This gets added to the Apply

    // result = AtomicSub(Absorb[i], damage);
    // if (result < 0)
    // {
    //     AtomicAdd(Absorb[i], -result);
    //     Absorbed = damage + result;
    // }
    // else
    // {
    //     Absorbed = damage;
    // }


    // Damage Fences:
    //
    // Apply Upgrades: (Single thread)
    // Generate Attack & Defense Cache: (Multithread Read-Only)
    // Generate Damage Combined: (Multithread Read-Only)
    // Apply Concurrent: (Multithread Read-Only)

}
}