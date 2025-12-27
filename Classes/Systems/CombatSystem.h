#pragma once
#include "cocos2d.h"
#include <vector>

#include "GameObjects/Units/UnitBase.h"
#include "GameObjects/Buildings/Building.h"
#include "Systems/AISystem.h"

namespace CombatSystem {

    // Base building priority (smaller = higher priority)
    int getBuildingPriority(int buildingId);

    // Per-unit building priority (e.g. Giant prefers defenses)
    int getBuildingPriorityForUnit(const UnitBase& unit, int buildingId, bool defensesExist);

    // Range check
    bool isInRange(const cocos2d::Vec2& a, const cocos2d::Vec2& b, float range);

    // Ensure a hp bar exists on sprite and update it.
    // For units: isUnit=true; for buildings: false.
    void ensureHpBar(cocos2d::Sprite* sprite, int hp, int hpMax, bool isUnit);

    // Unit tries to attack a building (range + cooldown + damage + hp bar update + float text)
    bool tryUnitAttackBuilding(UnitBase& attacker,
        cocos2d::Sprite* attackerSprite,
        Building& target,
        cocos2d::Sprite* targetSprite);

    // Bomber special:
    // - Target must be a wall (id==10)
    // - When bomber reaches attack range, it destroys a 3x3 wall area centered at that wall
    //   and then self-destructs.
    bool tryBomberExplode(UnitBase& bomber,
        cocos2d::Sprite* bomberSprite,
        EnemyBuildingRuntime& targetWall,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

    // Defense building shoots units (range + cooldown + damage + hp bar update + float text)
    bool tryDefenseShoot(float dt,
        Building& defense,
        cocos2d::Sprite* defenseSprite,
        std::vector<BattleUnitRuntime>& units,
        float& cooldown,
        float cellSizePx);
}
