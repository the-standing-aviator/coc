#pragma once

#include "GameObjects/Units/UnitBase.h"

// Barbarian
// ---------
// Melee troop. Simple stats only (no special ability yet).
//
// Suggested sprite path (change if your Resources name differs):
//   Resources/Textures/Troops/barbarian.png
class Barbarian : public UnitBase {
public:
    Barbarian();
    virtual ~Barbarian() = default;
};
