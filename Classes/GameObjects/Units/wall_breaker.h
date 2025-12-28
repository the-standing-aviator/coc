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

    WallBreaker();
    void applyLevel(int lvl);

    virtual ~WallBreaker() = default;
};
