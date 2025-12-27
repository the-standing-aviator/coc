#pragma once
#include "cocos2d.h"
#include <vector>
#include <memory>

#include "GameObjects/Units/UnitBase.h"
#include "GameObjects/Buildings/Building.h"

#include "Systems/Pathfinding.h"

// Runtime data structures used by battle.
struct BattleUnitRuntime {
    std::unique_ptr<UnitBase> unit;
    cocos2d::Sprite* sprite = nullptr;

    // Cached target (index in enemyBuildings)
    int targetIndex = -1;

    // Cached A* path in grid coordinates
    std::vector<Pathfinding::GridPos> path;
    int pathCursor = 0;
    float repathCD = 0.0f;

    // Death animation state
    bool dying = false;
    float dyingTimer = 0.0f;
};

struct EnemyBuildingRuntime {
    int id = 0;
    int r = 0, c = 0;             // grid coords from SaveData if available
    cocos2d::Vec2 pos;            // cached world pos (node space of BattleScene::_world)

    std::unique_ptr<Building> building;
    cocos2d::Sprite* sprite = nullptr;

    // Defense shooting state
    float defenseCooldown = 0.0f;

    // Death animation state
    bool dying = false;
    float dyingTimer = 0.0f;
};

class AISystem {
public:
    AISystem() = default;

    // For converting between iso-grid and world positions (must be set by BattleScene after tile params are known)
    void setIsoGrid(int rows, int cols, float tileW, float tileH, const cocos2d::Vec2& anchor);

    // Used to convert "rangeCells" to pixels for defenses, and for approximating attack range in cells.
    void setCellSizePx(float cellSizePx) { _cellSizePx = cellSizePx; }

    // Main update: troops auto-fight, defenses counterattack, play death animations & cleanup.
    void update(float dt,
        std::vector<BattleUnitRuntime>& units,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

private:
    // Unit ids (keep consistent with your UnitFactory)
    static constexpr int UNIT_BARBARIAN = 1;
    static constexpr int UNIT_ARCHER = 2;
    static constexpr int UNIT_GIANT = 3;
    static constexpr int UNIT_BOMBER = 4; // 炸弹人（你后续加兵种时建议用 4）

    // grid params (iso)
    bool _gridReady = false;
    int _rows = 0, _cols = 0;
    float _tileW = 0.0f, _tileH = 0.0f;
    cocos2d::Vec2 _anchor = cocos2d::Vec2::ZERO;
    float _cellSizePx = 32.0f;

    cocos2d::Vec2 gridToWorld(int r, int c) const;
    Pathfinding::GridPos worldToGrid(const cocos2d::Vec2& p) const;

    bool isBomber(const UnitBase& u) const { return u.unitId == UNIT_BOMBER; }
    bool isGiant(const UnitBase& u) const { return u.unitId == UNIT_GIANT; }
    bool isBarbarian(const UnitBase& u) const { return u.unitId == UNIT_BARBARIAN; }
    bool isMelee(const UnitBase& u) const { return u.attackRange <= std::max(8.0f, _cellSizePx * 0.90f); }

    void buildBlockedMap(const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        std::vector<unsigned char>& blocked,
        bool wallsBlocked) const;

    int findWallIndexAtCell(int r, int c,
        const std::vector<EnemyBuildingRuntime>& enemyBuildings) const;

    int pickTargetIndex(const UnitBase& unit,
        const cocos2d::Vec2& unitPos,
        const std::vector<EnemyBuildingRuntime>& enemyBuildings) const;

    bool anyDefenseAlive(const std::vector<EnemyBuildingRuntime>& enemyBuildings) const;

    Pathfinding::GridPos pickBestApproachCell(const UnitBase& unit,
        const EnemyBuildingRuntime& target,
        const std::vector<unsigned char>& blocked) const;

    void recomputePath(BattleUnitRuntime& u,
        const EnemyBuildingRuntime& target,
        const std::vector<unsigned char>& blocked);

    void stepAlongPath(BattleUnitRuntime& u,
        const EnemyBuildingRuntime& target,
        float dt);

    void updateOneUnit(float dt, BattleUnitRuntime& u,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

    void updateDefenses(float dt,
        std::vector<BattleUnitRuntime>& units,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

    void cleanup(float dt,
        std::vector<BattleUnitRuntime>& units,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);
};
