#include "GameObjects/Units/Barbarian.h"

#include <algorithm>

// NOTE:
// These stats are "good enough" placeholders for your current milestone.
// If later you add JSON configs, you can move these numbers into Configs/*.json
// and load them in UnitFactory::create().

Barbarian::Barbarian()
{
    unitId = 1;
    name = "Barbarian";

    // Resource path (adjust if your folder name differs)
    image = "barbarian/barbarian_stand.png";

    // Apply default level
    applyLevel(1);
}

void Barbarian::applyLevel(int lvl)
{
    // Reference table (Lv1~5):
    // DPS = Damage per attack (attack speed is 1s)
    static const int kHp[5] = { 45, 54, 65, 85, 105 };
    static const int kDmg[5] = { 9, 12, 15, 18, 23 };

    lvl = std::max(1, std::min(5, lvl));
    level = lvl;

    hpMax = kHp[lvl - 1];
    hp = hpMax;
    damage = kDmg[lvl - 1];
    attackInterval = 1.0f;

    // From reference: range 0.4 tiles, movement speed 18
    attackRangeTiles = 0.4f;
    moveSpeedStat = 18.0f;

    housingSpace = 1;
    costElixir = 0;
    trainingTimeSec = 0;
}