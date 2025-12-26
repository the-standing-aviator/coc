#include "GameObjects/Units/Archer.h"

// NOTE:
// These stats are placeholders for your current milestone.
// Later you can make them data-driven via JSON.

Archer::Archer()
{
    unitId = 2;
    name = "Archer";

    // Default lvl1 stats (CoC-style placeholders)
    hpMax = 22; // Level 1: 22 HP (CoC reference)
    hp = hpMax;
    damage = 8;  // Level 1: 8 damage per hit (CoC reference)
    attackInterval = 1.0f;
    attackRange = 220.0f; // Ranged: about 3.5 tiles (approx. in pixels) // ranged (pixels)
    moveSpeed = 75.0f;

    housingSpace = 1;
    costElixir = 50;
    trainingTimeSec = 25;

    // If you don't have this path, change it to your real troop texture.
    image = "Textures/Troops/archer.png";
}
