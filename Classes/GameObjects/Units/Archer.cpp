#include "GameObjects/Units/Archer.h"

#include <algorithm>

// NOTE:
// These stats are placeholders for your current milestone.
// Later you can make them data-driven via JSON.

Archer::Archer()
{
    unitId = 2;
    name = "Archer";

    // Resource path (adjust if your folder name differs)
    image = "archor/archor_stand.png";

    applyLevel(1);
}

void Archer::applyLevel(int lvl)
{
    // Reference table (Lv1~5):
    static const int kHp[5] = { 22, 26, 29, 33, 40 };
    static const int kDmg[5] = { 8, 10, 13, 16, 20 };

    lvl = std::max(1, std::min(5, lvl));
    level = lvl;

    hpMax = kHp[lvl - 1];
    hp = hpMax;
    damage = kDmg[lvl - 1];
    attackInterval = 1.0f;

    // Requirement for this project milestone:
    // Archer can attack 3 tiles away.
    attackRangeTiles = 3.0f;
    moveSpeedStat = 24.0f;

    housingSpace = 1;
    costElixir = 0;
    trainingTimeSec = 0;
}