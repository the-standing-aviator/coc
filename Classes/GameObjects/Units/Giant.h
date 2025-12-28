#pragma once

#include "GameObjects/Units/UnitBase.h"

// Giant
// -----
// Tanky melee troop.
// Targeting preference is implemented in AISystem:
//   - If any defense buildings exist, Giants target the nearest defense first.
class Giant : public UnitBase {
public:
    Giant();
    void applyLevel(int lvl);
    virtual ~Giant() = default;
};
