#include "GameObjects/Units/Archer.h"

// NOTE:
// These stats are placeholders for your current milestone.
// Later you can make them data-driven via JSON.

Archer::Archer()
{
    unitId = 2;
    name = "Archer";

    // Default lvl1 stats (CoC-style placeholders)
    hpMax = 20;
    hp = hpMax;
    damage = 7;
    attackInterval = 1.0f;
    attackRange = 120.0f; // ranged (pixels)
    moveSpeed = 75.0f;

    housingSpace = 1;
    costElixir = 50;
    trainingTimeSec = 25;

    // If you don't have this path, change it to your real troop texture.
    image = "Textures/Troops/archer.png";
}
