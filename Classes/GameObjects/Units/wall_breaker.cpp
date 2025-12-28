#include "GameObjects/Units/wall_breaker.h"

#include <algorithm>

WallBreaker::WallBreaker()
{
    unitId = 4;
    name = "WallBreaker";

    image = "wall_breaker/wall_breaker_stand.png";

    applyLevel(1);
}

void WallBreaker::applyLevel(int lvl)
{
    // Reference table (Lv1~5):
    // Damage: 10,20,25,30,43
    // Hitpoints: 20,24,29,35,53
    // Damage when destroyed: 6,9,13,16,23
    static const int kHp[5] = { 20, 24, 29, 35, 53 };
    static const int kDmg[5] = { 10, 20, 25, 30, 43 };
    static const int kDeath[5] = { 6, 9, 13, 16, 23 };

    lvl = std::max(1, std::min(5, lvl));
    level = lvl;

    hpMax = kHp[lvl - 1];
    hp = hpMax;
    damage = kDmg[lvl - 1];
    deathDamage = kDeath[lvl - 1];

    // From reference: attack speed 1s, range 1 tile, movement speed 24
    attackInterval = 1.0f;
    attackRangeTiles = 1.0f;
    moveSpeedStat = 24.0f;

    housingSpace = 2;
    costElixir = 0;
    trainingTimeSec = 0;

    wallDamageMultiplier = 40;
    damageRadiusTiles = 2.0f;
}
