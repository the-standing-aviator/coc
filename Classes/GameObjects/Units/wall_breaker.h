#pragma once

#include "GameObjects/Units/UnitBase.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
// Wall Breaker
// ------------
// Specialized troop that deals massive damage to Walls (x40 in the reference table).
// Combat logic for "wall preference" and "death damage" is not implemented yet;
// we store the reference values for future use.
class WallBreaker : public UnitBase {
public:
    // Reference extras
    int wallDamageMultiplier = 40;
    float damageRadiusTiles = 2.0f;
    int deathDamage = 6; // damage when destroyed (reference)

    WallBreaker()
    {
        unitId = 4;
        name = "WallBreaker";

        // Level 1 (reference):
        // Housing Space: 2
        // Attack Speed: 1s
        // Damage: 10 (normal), Damage vs Walls: 400 (x40)
        // Hitpoints: 20
        hpMax = 20;
        hp = hpMax;

        damage = 10;
        attackInterval = 1.0f;

        // Melee-ish range. Approximate.
        attackRange = 64.0f;

        // Fast movement (pixel-based approximation)
        moveSpeed = 95.0f;

        housingSpace = 2;
        costElixir = 0;
        trainingTimeSec = 0;

        image = "troops/wall_breaker.png"; // Optional: change to your actual resource path
    }

    virtual ~WallBreaker() = default;
};
