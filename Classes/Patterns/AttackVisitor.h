#pragma once
#include "GameObjects/Units/UnitBase.h"
#include "GameObjects/Buildings/DefenseBuilding.h"
#include "GameObjects/Buildings/TownHall.h"
#include "GameObjects/Buildings/ResourceBuilding.h"
#include "GameObjects/Buildings/TroopBuilding.h"

class AttackVisitor {
public:
    static int computeDamage(const UnitBase& attacker, const Building& target);
};
