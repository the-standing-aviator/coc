#pragma once

#include "GameObjects/Units/UnitBase.h"

// Giant
// -----
// Tanky melee troop. Preferred target: Defenses (not implemented in combat logic yet).
// Stats are based on the reference table provided by the user (level 1).
class Giant : public UnitBase {
public:
    Giant()
    {
        unitId = 3;
        name = "Giant";

        // Level 1 (reference):
        // Housing Space: 5
        // Attack Speed: 2s
        // Damage per attack: 24
        // Hitpoints: 400
        hpMax = 400;
        hp = hpMax;
        damage = 24;
        attackInterval = 2.0f;

        // Melee range: about 1 tile. We approximate with pixels.
        attackRange = 64.0f;

        // Slow movement (pixel-based approximation)
        moveSpeed = 55.0f;

        housingSpace = 5;
        costElixir = 0;
        trainingTimeSec = 0;

        image = "giant/giant_stand.png"; // Optional: change to your actual resource path
    }

    virtual ~Giant() = default;
};