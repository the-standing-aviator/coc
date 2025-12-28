#pragma once
#include "cocos2d.h"
#include <vector>
#include <memory>

#include "GameObjects/Units/UnitBase.h"
#include "GameObjects/Buildings/Building.h"
#include "Systems/Pathfinding.h"


// BattleUnitRuntime encapsulates related behavior and state.


struct BattleUnitRuntime {
    std::unique_ptr<UnitBase> unit;
    cocos2d::Sprite* sprite = nullptr;

    
    int targetIndex = -1;

    
    
    int mainTargetIndex = -1;
    bool breakingWall = false;

    
    std::vector<Pathfinding::GridPos> path;
    int pathCursor = 0;
    float repathCD = 0.0f;

    
    bool dying = false;
    float dyingTimer = 0.0f;
};

// EnemyBuildingRuntime encapsulates related behavior and state.

struct EnemyBuildingRuntime {
    int id = 0;
    int r = 0, c = 0;             
    
    
    int saveIndex = -1;
    cocos2d::Vec2 pos;            

    std::unique_ptr<Building> building;
    cocos2d::Sprite* sprite = nullptr;

    
    float defenseCooldown = 0.0f;

    
    
    bool ruinShown = false;

    
    bool lootCollected = false;





int lootGoldMax = 0;
int lootElixirMax = 0;
int lootGoldTaken = 0;
int lootElixirTaken = 0;
int lastHp = 0;

    
    bool dying = false;
    float dyingTimer = 0.0f;
};

// AISystem encapsulates related behavior and state.

class AISystem {
public:
    AISystem() = default;

    
    // Sets the IsoGrid.

    
    void setIsoGrid(int rows, int cols, float tileW, float tileH, const cocos2d::Vec2& anchor);

    
    void setCellSizePx(float cellSizePx) { _cellSizePx = cellSizePx; }

    
    // Updates the object state.

    
    void update(float dt,
        std::vector<BattleUnitRuntime>& units,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

private:
    
    static constexpr int UNIT_BARBARIAN = 1;
    static constexpr int UNIT_ARCHER    = 2;
    static constexpr int UNIT_GIANT     = 3;
    static constexpr int UNIT_BOMBER    = 4;

    
    bool _gridReady = false;
    int _rows = 0, _cols = 0;
    float _tileW = 0.0f, _tileH = 0.0f;
    cocos2d::Vec2 _anchor = cocos2d::Vec2::ZERO;
    float _cellSizePx = 32.0f;

    // TODO: Add a brief description.

    cocos2d::Vec2 gridToWorld(int r, int c) const;
    // TODO: Add a brief description.
    Pathfinding::GridPos worldToGrid(const cocos2d::Vec2& p) const;

    bool isBomber(const UnitBase& u) const { return u.unitId == UNIT_BOMBER; }
    bool isGiant(const UnitBase& u) const { return u.unitId == UNIT_GIANT; }
    bool isBarbarian(const UnitBase& u) const { return u.unitId == UNIT_BARBARIAN; }
    bool isArcher(const UnitBase& u) const { return u.unitId == UNIT_ARCHER; }

    
    
    
    
    // Builds and configures resources.

    
    
    
    
    void buildBlockedMap(const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        std::vector<unsigned char>& blocked,
        bool wallsBlocked,
        bool centerOnly) const;

    // TODO: Add a brief description.

    bool anyDefenseAlive(const std::vector<EnemyBuildingRuntime>& enemyBuildings) const;

    // Returns the FootprintCells.

    void getFootprintCells(const EnemyBuildingRuntime& target,
        std::vector<Pathfinding::GridPos>& out) const;

    
    // Returns the CenterCell.

    
    Pathfinding::GridPos getCenterCell(const EnemyBuildingRuntime& target) const;

    // TODO: Add a brief description.

    bool inAttackRangeCells(const UnitBase& unit,
        const Pathfinding::GridPos& unitCell,
        const EnemyBuildingRuntime& target) const;

    // TODO: Add a brief description.

    void collectApproachCells(const UnitBase& unit,
        const EnemyBuildingRuntime& target,
        const std::vector<unsigned char>& blocked,
        std::vector<Pathfinding::GridPos>& out) const;

    
    
    // TODO: Add a brief description.

    
    
    int pickWallToBreak(const UnitBase& unit,
        const Pathfinding::GridPos& unitCell,
        const Pathfinding::GridPos& mainTargetCenter,
        const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        const std::vector<unsigned char>& blockedHard) const;

    // TODO: Add a brief description.

    int pickTargetIndex(const UnitBase& unit,
        const Pathfinding::GridPos& unitCell,
        const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        const std::vector<unsigned char>& blocked) const;

    
    
    // Builds and configures resources.

    
    
    bool buildBestPathForTarget(const UnitBase& unit,
        const Pathfinding::GridPos& start,
        const EnemyBuildingRuntime& target,
        const std::vector<unsigned char>& blockedHard,
        std::vector<Pathfinding::GridPos>& outPath) const;

    
    
    // TODO: Add a brief description.

    
    
    int pickReachableTargetIndex(const UnitBase& unit,
        const Pathfinding::GridPos& unitCell,
        const std::vector<EnemyBuildingRuntime>& enemyBuildings,
        const std::vector<unsigned char>& blockedHard,
        std::vector<Pathfinding::GridPos>* outBestPath) const;

    // TODO: Add a brief description.

    void recomputePath(BattleUnitRuntime& u,
        const EnemyBuildingRuntime& target,
        const std::vector<unsigned char>& blocked);

    // TODO: Add a brief description.

    void stepAlongPath(BattleUnitRuntime& u,
        float dt);

    // Updates the object state.

    void updateOneUnit(float dt, BattleUnitRuntime& u,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

    // Updates the object state.

    void updateDefenses(float dt,
        std::vector<BattleUnitRuntime>& units,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);

    // TODO: Add a brief description.

    void cleanup(float dt,
        std::vector<BattleUnitRuntime>& units,
        std::vector<EnemyBuildingRuntime>& enemyBuildings);
};
