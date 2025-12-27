#include "GameObjects/Units/Barbarian.h"

// NOTE:
// These stats are "good enough" placeholders for your current milestone.
// If later you add JSON configs, you can move these numbers into Configs/*.json
// and load them in UnitFactory::create().

Barbarian::Barbarian()
{
    unitId = 1;
    name = "Barbarian";

    // Default lvl1 stats (CoC-style placeholders)
    hpMax = 45;
    hp = hpMax;
    damage = 9;  // Level 1: 9 damage per hit (CoC reference)
    attackInterval = 1.0f;
    attackRange = 24.0f;   // melee range (pixels)
    moveSpeed = 70.0f;

    housingSpace = 1;
    costElixir = 25;
    trainingTimeSec = 20;

    // If you don't have this path, change it to your real troop texture.
    // Example alternatives:
    //   "Textures/Troops/barbarian.png"
    //   "barbarian.png"
    image = "barbarian/barbarian_stand.png";
}