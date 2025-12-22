#pragma once

#include "GameObjects/Units/UnitBase.h"

// Archer
// ------
// Ranged troop.
//
// Suggested sprite path (change if your Resources name differs):
//   Resources/Textures/Troops/archer.png
class Archer : public UnitBase {
public:
    Archer();
    virtual ~Archer() = default;
};
