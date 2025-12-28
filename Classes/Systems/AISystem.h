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

    // Main target (non-wall). When the unit temporarily breaks walls, it will keep
    // returning to this target after the wall is destroyed.
    int mainTargetIndex = -1;
    bool breakingWall = false;

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
    // Index of the corresponding SaveBuilding inside the target SaveData.buildings.
    // Used by BattleScene to persist resource changes (stored resources in mines/collectors).
    int saveIndex = -1;
    cocos2d::Vec2 pos;            // cached world pos (node space of BattleScene::_world)

    std::unique_ptr<Building> building;
    cocos2d::Sprite* sprite = nullptr;

    // Defense shooting state
    float defenseCooldown = 0.0f;

    // When a non-wall building is destroyed, we keep its sprite and swap it to a ruin texture
    // (requested behavior). This flag prevents repeated swaps.
    bool ruinShown = false;

    // Loot is collected once when the building is destroyed.
    bool lootCollected = false;

// Loot model (50% rule):
// - loot*Max: maximum loot obtainable from this building.
// - loot*Taken: already granted loot based on damage progression.
// - lastHp: used for debugging / future extensions (not required for the current formula).
int lootGoldMax = 0;
int lootElixirMax = 0;
int lootGoldTaken = 0;
int lootElixirTaken = 0;
int lastHp = 0;

    // Death animation state
    bool dying = false;
    float dyingTimer = 0.0f;
};

class AISystem {
public:
    AISystem() = default;

    // For converting between iso-grid and world positions (must be set by BattleScene after tile params are known)
    void setIsoGrid(int rows, int cols, float tileW, float tileH, const cocos2d::Vec2& anchor);

    // Used for defenses (rangeCells->pixels) and for mapping some unit stats.
    void setCellSizePx(float cellSizePx) { _cellSizePx = cellSizePx; }

    // Main update: troops auto-fight, defenses counterattack, play death animations & cleanup.
    void update(float dt,
        std::vector<BattleUnitRuntime>& units,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

private:
    // Unit ids (keep consistent with your UnitFactory)
    static constexpr int UNIT_BARBARIAN = 1;
    static constexpr int UNIT_ARCHER    = 2;
    static constexpr int UNIT_GIANT     = 3;
    static constexpr int UNIT_BOMBER    = 4;

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
    bool isArcher(const UnitBase& u) const { return u.unitId == UNIT_ARCHER; }

    // Build a grid blocked map.
    // - wallsBlocked: whether wall cells are solid obstacles.
    // - centerOnly: if true, only the CENTER cell of each non-wall building is blocked
    //              (used by Giant per user requirement).
    void buildBlockedMap(const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        std::vector<unsigned char>& blocked,
        bool wallsBlocked,
        bool centerOnly) const;

    bool anyDefenseAlive(const std::vector<EnemyBuildingRuntime>& enemyBuildings) const;

    void getFootprintCells(const EnemyBuildingRuntime& target,
        std::vector<Pathfinding::GridPos>& out) const;

    // Returns the center grid cell of a building (for walls this is the wall cell itself).
    Pathfinding::GridPos getCenterCell(const EnemyBuildingRuntime& target) const;

    bool inAttackRangeCells(const UnitBase& unit,
        const Pathfinding::GridPos& unitCell,
        const EnemyBuildingRuntime& target) const;

    void collectApproachCells(const UnitBase& unit,
        const EnemyBuildingRuntime& target,
        const std::vector<unsigned char>& blocked,
        std::vector<Pathfinding::GridPos>& out) const;

    // Pick a reachable wall to break when the main target is blocked by walls.
    // Returns an index into enemyBuildings, or -1 if none.
    int pickWallToBreak(const UnitBase& unit,
        const Pathfinding::GridPos& unitCell,
        const Pathfinding::GridPos& mainTargetCenter,
        const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        const std::vector<unsigned char>& blockedHard) const;

    int pickTargetIndex(const UnitBase& unit,
        const Pathfinding::GridPos& unitCell,
        const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        const std::vector<unsigned char>& blocked) const;

    // Build the best A* path for a given unit -> target pair, using the same
    // goal rules as recomputePath(). Returns true if a path exists.
    bool buildBestPathForTarget(const UnitBase& unit,
        const Pathfinding::GridPos& start,
        const EnemyBuildingRuntime& target,
        const std::vector<unsigned char>& blockedHard,
        std::vector<Pathfinding::GridPos>& outPath) const;

    // Pick the nearest reachable (by A*) building according to unit priority rules.
    // If outBestPath is not nullptr, it will be filled with the chosen target path.
    int pickReachableTargetIndex(const UnitBase& unit,
        const Pathfinding::GridPos& unitCell,
        const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        const std::vector<unsigned char>& blockedHard,
        std::vector<Pathfinding::GridPos>* outBestPath) const;

    void recomputePath(BattleUnitRuntime& u,
        const EnemyBuildingRuntime& target,
        const std::vector<unsigned char>& blocked);

    void stepAlongPath(BattleUnitRuntime& u,
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
