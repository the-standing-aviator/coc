#pragma once
#include "cocos2d.h"
#include <vector>

#include "GameObjects/Units/UnitBase.h"
#include "GameObjects/Buildings/Building.h"
#include "Systems/AISystem.h"

namespace CombatSystem {

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

    // Unit hits a building WITHOUT any range check.
    // AISystem is responsible for deciding whether the unit is in range.
    bool unitHitBuildingNoRange(UnitBase& attacker,
        cocos2d::Sprite* attackerSprite,
        Building& target,
        cocos2d::Sprite* targetSprite);

    // WallBreaker special:
    // - Target must be a wall (id==10)
    // - When it reaches attack range, it deals high damage to nearby walls
    //   (radius is defined in WallBreaker stats) and then self-destructs.
    bool tryBomberExplode(UnitBase& bomber,
        cocos2d::Sprite* bomberSprite,
        EnemyBuildingRuntime& targetWall,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

    // Bomber explodes WITHOUT any range check.
    // AISystem is responsible for deciding whether the bomber is in range.
    bool bomberExplodeNoRange(UnitBase& bomber,
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
