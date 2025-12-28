#include "GameObjects/Units/Giant.h"

#include <algorithm>

Giant::Giant()
{
    unitId = 3;
    name = "Giant";

    image = "giant/giant_stand.png";

    applyLevel(1);
}

void Giant::applyLevel(int lvl)
{
    // Reference table (Lv1~5):
    // Damage per attack: 24, 30, 40, 48, 62
    // Hitpoints:        400,500,600,700,900
    static const int kHp[5] = { 400, 500, 600, 700, 900 };
    static const int kDmg[5] = { 24, 30, 40, 48, 62 };

    lvl = std::max(1, std::min(5, lvl));
    level = lvl;

    hpMax = kHp[lvl - 1];
    hp = hpMax;
    damage = kDmg[lvl - 1];

    // From reference: attack speed 2s, range 1 tile, movement speed 12
    attackInterval = 2.0f;
    attackRangeTiles = 1.0f;
    moveSpeedStat = 12.0f;

    housingSpace = 5;
    costElixir = 0;
    trainingTimeSec = 0;
}
