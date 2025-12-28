#include "Patterns/AttackVisitor.h"
#include <algorithm>

int AttackVisitor::computeDamage(const UnitBase& attacker, const Building& target)
{
    (void)target;
    // Use raw unit damage. Targeting preference is handled by AISystem.
    return std::max(1, attacker.damage);
}
