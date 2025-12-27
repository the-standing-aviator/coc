#include "Patterns/AttackVisitor.h"
#include <algorithm>

int AttackVisitor::computeDamage(const UnitBase& attacker, const Building& target)
{
    int dmg = std::max(1, attacker.damage);

    // Giant example: bonus vs defenses
    if (attacker.unitId == 3)
    {
        if (dynamic_cast<const ArrowTower*>(&target) || dynamic_cast<const Cannon*>(&target))
            dmg = dmg * 2;
    }

    // Wall example: more resistant
    if (dynamic_cast<const Wall*>(&target))
    {
        dmg = std::max(1, dmg / 2);
    }

    return dmg;
}
