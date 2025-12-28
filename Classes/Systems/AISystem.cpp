#include "Systems/AISystem.h"

#include "Systems/CombatSystem.h"
#include "Systems/Pathfinding.h"

#include "GameObjects/Buildings/DefenseBuilding.h"
#include "Managers/SoundManager.h"

#include <algorithm>
#include <cmath>

using namespace cocos2d;

void AISystem::setIsoGrid(int rows, int cols, float tileW, float tileH, const Vec2& anchor)
{
    _rows = rows;
    _cols = cols;
    _tileW = tileW;
    _tileH = tileH;
    _anchor = anchor;
    _gridReady = (_rows > 0 && _cols > 0 && _tileW > 0.001f && _tileH > 0.001f);
}

Vec2 AISystem::gridToWorld(int r, int c) const
{
    float x = _anchor.x + (c - r) * (_tileW * 0.5f);
    float y = _anchor.y - (c + r) * (_tileH * 0.5f);
    return Vec2(x, y);
}

Pathfinding::GridPos AISystem::worldToGrid(const Vec2& p) const
{
    // Inverse of:
    // x = anchor.x + (c-r)*(tileW/2)
    // y = anchor.y - (c+r)*(tileH/2)
    float dx = p.x - _anchor.x;
    float dy = _anchor.y - p.y;

    float cr = 2.0f * dx / _tileW;  // (c - r)
    float cs = 2.0f * dy / _tileH;  // (c + r)

    float cf = (cr + cs) * 0.5f;    // c
    float rf = (cs - cr) * 0.5f;    // r

    int c = (int)std::round(cf);
    int r = (int)std::round(rf);

    if (r < 0) r = 0;
    if (c < 0) c = 0;
    if (r >= _rows) r = _rows - 1;
    if (c >= _cols) c = _cols - 1;

    return { r, c };
}

bool AISystem::anyDefenseAlive(const std::vector<EnemyBuildingRuntime>& enemyBuildings) const
{
    for (const auto& e : enemyBuildings)
    {
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;
        if (e.id == 1 || e.id == 2) return true;
    }
    return false;
}

void AISystem::buildBlockedMap(const std::vector<EnemyBuildingRuntime>& enemyBuildings,
    std::vector<unsigned char>& blocked,
    bool wallsBlocked,
    bool centerOnly) const
{
    if (!_gridReady) {
        blocked.clear();
        return;
    }

    blocked.assign((size_t)_rows * (size_t)_cols, 0);

    auto markCell = [&](int rr, int cc) {
        if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) return;
        blocked[(size_t)rr * (size_t)_cols + (size_t)cc] = 1;
    };

    for (const auto& e : enemyBuildings)
    {
        if (!e.building || e.building->hp <= 0) continue;

        int rr = e.r;
        int cc = e.c;
        if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols)
        {
            auto g = worldToGrid(e.pos);
            rr = g.r;
            cc = g.c;
        }

        // Wall occupies 1 cell.
        if (e.id == 10)
        {
            if (wallsBlocked) markCell(rr, cc);
            continue;
        }

        if (centerOnly)
        {
            // Giant special rule: only the building center cell is blocked.
            markCell(rr, cc);
        }
        else
        {
            // Default: buildings occupy 3x3 around center (same as MainScene placement rule).
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc)
                    markCell(rr + dr, cc + dc);
        }
    }
}

void AISystem::getFootprintCells(const EnemyBuildingRuntime& target,
    std::vector<Pathfinding::GridPos>& out) const
{
    out.clear();
    if (!_gridReady) return;

    int tr = target.r;
    int tc = target.c;
    if (tr < 0 || tr >= _rows || tc < 0 || tc >= _cols)
    {
        auto g = worldToGrid(target.pos);
        tr = g.r;
        tc = g.c;
    }

    auto pushIfInside = [&](int rr, int cc) {
        if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) return;
        out.push_back({ rr, cc });
    };

    if (target.id == 10)
    {
        pushIfInside(tr, tc);
        return;
    }

    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
            pushIfInside(tr + dr, tc + dc);
}

Pathfinding::GridPos AISystem::getCenterCell(const EnemyBuildingRuntime& target) const
{
    if (!_gridReady) return { 0, 0 };

    int tr = target.r;
    int tc = target.c;
    if (tr < 0 || tr >= _rows || tc < 0 || tc >= _cols)
    {
        auto g = worldToGrid(target.pos);
        tr = g.r;
        tc = g.c;
    }
    return { tr, tc };
}

bool AISystem::inAttackRangeCells(const UnitBase& unit,
    const Pathfinding::GridPos& unitCell,
    const EnemyBuildingRuntime& target) const
{
    if (!_gridReady) return false;

    // User requirement:
    // - Melee units (Barbarian/Giant) must enter the target building's CENTER cell to attack.
    // - Archer range is measured ONLY against the target building's CENTER cell (<=3 tiles).
    // - Walls are a special case: units cannot enter a wall cell, so melee attacks walls from 1 tile away.

    Pathfinding::GridPos center = getCenterCell(target);
    int dr = std::abs(unitCell.r - center.r);
    int dc = std::abs(unitCell.c - center.c);
    int cheb = std::max(dr, dc);

    if (target.id == 10)
    {
        // Wall
        if (isArcher(unit)) return (cheb <= 3);
        return (cheb <= 1);
    }

    if (isArcher(unit)) return (cheb <= 3);
    // Melee must be exactly at center.
    return (cheb == 0);
}

void AISystem::collectApproachCells(const UnitBase& unit,
    const EnemyBuildingRuntime& target,
    const std::vector<unsigned char>& blocked,
    std::vector<Pathfinding::GridPos>& out) const
{
    out.clear();
    if (!_gridReady) return;

    std::vector<Pathfinding::GridPos> footprint;
    getFootprintCells(target, footprint);
    if (footprint.empty()) return;

    auto isPassable = [&](int rr, int cc) -> bool {
        if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) return false;
        size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
        if (idx >= blocked.size()) return false;
        return blocked[idx] == 0;
    };

    // Build a quick lookup for footprint cells.
    std::vector<unsigned char> isFoot((size_t)_rows * (size_t)_cols, 0);
    for (const auto& bc : footprint)
    {
        size_t idx = (size_t)bc.r * (size_t)_cols + (size_t)bc.c;
        if (idx < isFoot.size()) isFoot[idx] = 1;
    }

    std::vector<unsigned char> visited((size_t)_rows * (size_t)_cols, 0);
    auto pushUnique = [&](int rr, int cc) {
        if (!isPassable(rr, cc)) return;
        size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
        if (idx >= visited.size()) return;
        if (visited[idx]) return;
        visited[idx] = 1;
        out.push_back({ rr, cc });
    };

    // Collect all walkable cells where the unit is able to attack the target.
    // Archer: within 3 tiles; Other units: within 1 tile.
    const int range = isArcher(unit) ? 3 : 1;
    for (const auto& bc : footprint)
    {
        for (int dr = -range; dr <= range; ++dr)
        {
            for (int dc = -range; dc <= range; ++dc)
            {
                // Tile-based Chebyshev distance.
                if (std::max(std::abs(dr), std::abs(dc)) > range) continue;

                int rr = bc.r + dr;
                int cc = bc.c + dc;
                if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) continue;
                size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
                if (idx < isFoot.size() && isFoot[idx]) continue; // cannot stand inside the target
                pushUnique(rr, cc);
            }
        }
    }
}

int AISystem::pickWallToBreak(const UnitBase& unit,
    const Pathfinding::GridPos& unitCell,
    const Pathfinding::GridPos& mainTargetCenter,
    const std::vector<EnemyBuildingRuntime>& enemyBuildings,
    const std::vector<unsigned char>& blockedHard) const
{
    if (!_gridReady) return -1;

    // Melee units are allowed to break walls when the main target is unreachable.
    if (isArcher(unit) || isBomber(unit)) return -1;

    std::vector<unsigned char> blk = blockedHard;
    if ((int)blk.size() != _rows * _cols)
        blk.assign((size_t)_rows * (size_t)_cols, 0);

    // Ensure start is passable.
    int sidx = unitCell.r * _cols + unitCell.c;
    if (sidx >= 0 && sidx < (int)blk.size()) blk[sidx] = 0;

    auto isPassable = [&](int rr, int cc) -> bool {
        if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) return false;
        size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
        if (idx >= blk.size()) return false;
        return blk[idx] == 0;
    };

    auto chebDist = [&](const Pathfinding::GridPos& a, const Pathfinding::GridPos& b) -> int {
        return std::max(std::abs(a.r - b.r), std::abs(a.c - b.c));
    };

    int bestIdx = -1;
    int bestScore = INT_MAX;
    int bestWallDist = INT_MAX;

    // For each alive wall, try to find a reachable approach cell (4-neighborhood) and score it.
    for (int i = 0; i < (int)enemyBuildings.size(); ++i)
    {
        const auto& e = enemyBuildings[i];
        if (e.id != 10) continue;
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;

        Pathfinding::GridPos wcell = getCenterCell(e);

        // Candidate approach cells (4-dir). We don't allow entering the wall cell.
        Pathfinding::GridPos neigh[4] = {
            { wcell.r - 1, wcell.c },
            { wcell.r + 1, wcell.c },
            { wcell.r, wcell.c - 1 },
            { wcell.r, wcell.c + 1 }
        };

        int bestLen = INT_MAX;
        for (const auto& g : neigh)
        {
            if (!isPassable(g.r, g.c)) continue;
            auto path = Pathfinding::findPathAStar(_rows, _cols, unitCell, g, blk, false, 20000);
            if (path.empty()) continue;
            int len = (int)path.size();
            if (len < bestLen) bestLen = len;
            if (bestLen <= 2) break;
        }

        if (bestLen == INT_MAX) continue;

        int wallDistToTarget = chebDist(wcell, mainTargetCenter);

        // Score: prioritize reachable short paths, then walls closer to the target.
        int score = bestLen * 10 + wallDistToTarget;
        if (score < bestScore || (score == bestScore && wallDistToTarget < bestWallDist))
        {
            bestScore = score;
            bestWallDist = wallDistToTarget;
            bestIdx = i;
        }
    }

    return bestIdx;
}

int AISystem::pickTargetIndex(const UnitBase& unit,
    const Pathfinding::GridPos& unitCell,
    const std::vector<EnemyBuildingRuntime>& enemyBuildings,
    const std::vector<unsigned char>& blocked) const
{
    // IMPORTANT CHANGE:
    // Use a stable distance measure (Chebyshev to the target CENTER cell) instead of requiring
    // an existing A* path. This prevents "no target" when buildings are fully enclosed by walls.
    // Melee units will break walls dynamically when needed.

    (void)blocked; // unused (kept for signature compatibility)

    const bool defensesExist = anyDefenseAlive(enemyBuildings);

    auto chebDist = [&](const Pathfinding::GridPos& a, const Pathfinding::GridPos& b) -> int {
        return std::max(std::abs(a.r - b.r), std::abs(a.c - b.c));
    };

    auto isValid = [&](const EnemyBuildingRuntime& e) -> bool {
        return (e.building && e.building->hp > 0 && e.sprite);
    };

    int best = -1;
    int bestD = INT_MAX;

    // Bomber: prefers walls.
    if (isBomber(unit))
    {
        for (int i = 0; i < (int)enemyBuildings.size(); ++i)
        {
            const auto& e = enemyBuildings[i];
            if (!isValid(e)) continue;
            if (e.id != 10) continue;
            int d = chebDist(unitCell, getCenterCell(e));
            if (d < bestD) { bestD = d; best = i; }
        }
        if (best >= 0) return best;
        // Fallback: any non-wall.
        for (int i = 0; i < (int)enemyBuildings.size(); ++i)
        {
            const auto& e = enemyBuildings[i];
            if (!isValid(e)) continue;
            if (e.id == 10) continue;
            int d = chebDist(unitCell, getCenterCell(e));
            if (d < bestD) { bestD = d; best = i; }
        }
        return best;
    }

    // Giant: prefers defenses if any exist.
    if (isGiant(unit) && defensesExist)
    {
        for (int i = 0; i < (int)enemyBuildings.size(); ++i)
        {
            const auto& e = enemyBuildings[i];
            if (!isValid(e)) continue;
            if (!(e.id == 1 || e.id == 2)) continue;
            int d = chebDist(unitCell, getCenterCell(e));
            if (d < bestD) { bestD = d; best = i; }
        }
        if (best >= 0) return best;
    }

    // Default: any non-wall building.
    for (int i = 0; i < (int)enemyBuildings.size(); ++i)
    {
        const auto& e = enemyBuildings[i];
        if (!isValid(e)) continue;
        if (e.id == 10) continue;
        int d = chebDist(unitCell, getCenterCell(e));
        if (d < bestD) { bestD = d; best = i; }
    }

    return best;
}

bool AISystem::buildBestPathForTarget(const UnitBase& unit,
    const Pathfinding::GridPos& start,
    const EnemyBuildingRuntime& target,
    const std::vector<unsigned char>& blockedHard,
    std::vector<Pathfinding::GridPos>& outPath) const
{
    outPath.clear();
    if (!_gridReady) return false;

    // Build a local blocked map that we can tweak (same idea as recomputePath).
    std::vector<unsigned char> blk = blockedHard;
    if ((int)blk.size() != _rows * _cols)
        blk.assign((size_t)_rows * (size_t)_cols, 0);

    // Start must be passable.
    int sidx = start.r * _cols + start.c;
    if (sidx >= 0 && sidx < (int)blk.size()) blk[sidx] = 0;

    std::vector<Pathfinding::GridPos> goals;

    if (target.id == 10)
    {
        // Wall: use normal approach cells.
        collectApproachCells(unit, target, blk, goals);
    }
    else if (isArcher(unit))
    {
        // Archer: any passable cell within 3 tiles to the target CENTER cell.
        Pathfinding::GridPos center = getCenterCell(target);
        auto isPassable = [&](int rr, int cc) -> bool {
            if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) return false;
            size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
            if (idx >= blk.size()) return false;
            return blk[idx] == 0;
        };

        for (int dr = -3; dr <= 3; ++dr)
        {
            for (int dc = -3; dc <= 3; ++dc)
            {
                if (std::max(std::abs(dr), std::abs(dc)) > 3) continue;
                int rr = center.r + dr;
                int cc = center.c + dc;
                if (!isPassable(rr, cc)) continue;
                goals.push_back({ rr, cc });
            }
        }
    }
    else
    {
        // Melee: must enter the target CENTER cell to attack.
        // Temporarily mark the target 3x3 footprint as passable.
        Pathfinding::GridPos center = getCenterCell(target);
        for (int dr = -1; dr <= 1; ++dr)
        {
            for (int dc = -1; dc <= 1; ++dc)
            {
                int rr = center.r + dr;
                int cc = center.c + dc;
                if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) continue;
                size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
                if (idx < blk.size()) blk[idx] = 0;
            }
        }
        goals.push_back(center);
    }

    std::vector<Pathfinding::GridPos> bestPath;
    int bestLen = INT_MAX;

    // Keep the node cap moderate because we may call this for multiple candidates.
    const int maxNodes = 8000;
    for (const auto& g : goals)
    {
        auto path = Pathfinding::findPathAStar(_rows, _cols, start, g, blk, false, maxNodes);
        if (path.empty()) continue;
        int len = (int)path.size();
        if (len < bestLen)
        {
            bestLen = len;
            bestPath = std::move(path);
        }
        if (bestLen <= 2) break;
    }

    if (bestPath.empty()) return false;

    outPath = std::move(bestPath);
    return true;
}

int AISystem::pickReachableTargetIndex(const UnitBase& unit,
    const Pathfinding::GridPos& unitCell,
    const std::vector<EnemyBuildingRuntime>& enemyBuildings,
    const std::vector<unsigned char>& blockedHard,
    std::vector<Pathfinding::GridPos>* outBestPath) const
{
    if (!_gridReady) return -1;

    auto isValid = [&](const EnemyBuildingRuntime& e) -> bool {
        return (e.building && e.building->hp > 0 && e.sprite);
    };

    const bool defensesExist = anyDefenseAlive(enemyBuildings);

    // Build candidate list following the same priority rules as pickTargetIndex(),
    // but only accept those that are actually reachable by A*.
    std::vector<int> candidates;
    candidates.reserve(enemyBuildings.size());

    if (isBomber(unit))
    {
        // Bomber uses its own special behavior and should not enter this function.
        return -1;
    }

    if (isGiant(unit) && defensesExist)
    {
        for (int i = 0; i < (int)enemyBuildings.size(); ++i)
        {
            const auto& e = enemyBuildings[i];
            if (!isValid(e)) continue;
            if (e.id == 10) continue;
            if (e.id == 1 || e.id == 2) candidates.push_back(i);
        }
    }

    // Fallback / default set: all non-wall buildings.
    if (candidates.empty())
    {
        for (int i = 0; i < (int)enemyBuildings.size(); ++i)
        {
            const auto& e = enemyBuildings[i];
            if (!isValid(e)) continue;
            if (e.id == 10) continue;
            candidates.push_back(i);
        }
    }

    if (candidates.empty()) return -1;

    // Sort candidates by a cheap heuristic first (Chebyshev to center), then test reachability.
    auto chebDist = [&](int idx) -> int {
        Pathfinding::GridPos c = getCenterCell(enemyBuildings[idx]);
        return std::max(std::abs(unitCell.r - c.r), std::abs(unitCell.c - c.c));
    };
    std::sort(candidates.begin(), candidates.end(), [&](int a, int b) {
        return chebDist(a) < chebDist(b);
    });

    int bestIdx = -1;
    int bestLen = INT_MAX;
    std::vector<Pathfinding::GridPos> bestPath;

    for (int idx : candidates)
    {
        std::vector<Pathfinding::GridPos> path;
        if (!buildBestPathForTarget(unit, unitCell, enemyBuildings[idx], blockedHard, path))
            continue;

        int len = (int)path.size();
        if (len < bestLen)
        {
            bestLen = len;
            bestIdx = idx;
            bestPath = std::move(path);
        }

        // Early stop: very close target.
        if (bestLen <= 3) break;
    }

    if (bestIdx >= 0 && outBestPath)
        *outBestPath = std::move(bestPath);

    return bestIdx;
}

void AISystem::recomputePath(BattleUnitRuntime& u,
    const EnemyBuildingRuntime& target,
    const std::vector<unsigned char>& blocked)
{
    if (!_gridReady || !u.sprite || !u.unit) return;

    Pathfinding::GridPos start = worldToGrid(u.sprite->getPosition());

    // Build a local blocked map we can tweak
    std::vector<unsigned char> blk = blocked;
    if ((int)blk.size() != _rows * _cols)
        blk.assign((size_t)_rows * (size_t)_cols, 0);

    // Start must be passable
    int sidx = start.r * _cols + start.c;
    if (sidx >= 0 && sidx < (int)blk.size()) blk[sidx] = 0;

    // Build goal cells based on unit type and the "center-cell" attack rule.
    std::vector<Pathfinding::GridPos> goals;

    if (target.id == 10)
    {
        // Wall: use normal approach cells (stand next to the wall, or within range for archer).
        collectApproachCells(*u.unit, target, blk, goals);
    }
    else if (isArcher(*u.unit))
    {
        // Archer: any passable cell within 3 tiles to the target CENTER cell.
        Pathfinding::GridPos center = getCenterCell(target);
        auto isPassable = [&](int rr, int cc) -> bool {
            if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) return false;
            size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
            if (idx >= blk.size()) return false;
            return blk[idx] == 0;
        };

        for (int dr = -3; dr <= 3; ++dr)
        {
            for (int dc = -3; dc <= 3; ++dc)
            {
                if (std::max(std::abs(dr), std::abs(dc)) > 3) continue;
                int rr = center.r + dr;
                int cc = center.c + dc;
                if (!isPassable(rr, cc)) continue;
                goals.push_back({ rr, cc });
            }
        }
    }
    else
    {
        // Melee: must enter the target CENTER cell to attack.
        // To allow this, temporarily mark the target footprint as passable.
        Pathfinding::GridPos center = getCenterCell(target);
        for (int dr = -1; dr <= 1; ++dr)
        {
            for (int dc = -1; dc <= 1; ++dc)
            {
                int rr = center.r + dr;
                int cc = center.c + dc;
                if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) continue;
                size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
                if (idx < blk.size()) blk[idx] = 0;
            }
        }
        goals.push_back(center);
    }

    std::vector<Pathfinding::GridPos> bestPath;
    int bestLen = INT_MAX;

    for (const auto& g : goals)
    {
        auto path = Pathfinding::findPathAStar(_rows, _cols, start, g, blk, false, 20000);
        if (path.empty()) continue;
        int len = (int)path.size();
        if (len < bestLen)
        {
            bestLen = len;
            bestPath = std::move(path);
        }
        if (bestLen <= 2) break;
    }

    u.path = std::move(bestPath);
    u.pathCursor = 0;

    if (!u.path.empty() && u.path[0].r == start.r && u.path[0].c == start.c)
        u.pathCursor = 1;

    // If no path exists, slow down repathing a bit to avoid thrashing.
    u.repathCD = u.path.empty() ? 0.80f : 0.35f;
}

void AISystem::stepAlongPath(BattleUnitRuntime& u,
    float dt)
{
    if (!u.sprite || !u.unit) return;
    if (!_gridReady) return;

    if (u.path.empty() || u.pathCursor >= (int)u.path.size())
    {
        // No path: do not move (do NOT walk through walls/buildings).
        return;
    }

    Vec2 cur = u.sprite->getPosition();
    auto cell = u.path[u.pathCursor];
    Vec2 waypoint = gridToWorld(cell.r, cell.c);

    Vec2 next = Pathfinding::stepTowards(cur, waypoint, u.unit->moveSpeed * dt, 0.0f);
    u.sprite->setPosition(next);

    if (next.distance(waypoint) <= 6.0f)
    {
        u.pathCursor++;
    }
}

void AISystem::updateOneUnit(float dt, BattleUnitRuntime& u,
    std::vector<EnemyBuildingRuntime>& enemyBuildings)
{
    if (!u.unit || !u.sprite) return;
    if (u.unit->isDead()) return;

    u.unit->tickAttack(dt);

    // Build a blocked map: walls are solid obstacles (units can NOT enter wall cells).
    std::vector<unsigned char> blockedHard;
    if (_gridReady)
    {
        // Giant special path rule: only building center cells are blocked.
        buildBlockedMap(enemyBuildings, blockedHard, true, isGiant(*u.unit));
        // Ensure unit's own cell is passable.
        auto st = worldToGrid(u.sprite->getPosition());
        int idx = st.r * _cols + st.c;
        if (idx >= 0 && idx < (int)blockedHard.size()) blockedHard[idx] = 0;
    }

    auto unitCell = worldToGrid(u.sprite->getPosition());

    auto isValidIndex = [&](int idx) -> bool {
        if (idx < 0 || idx >= (int)enemyBuildings.size()) return false;
        const auto& t = enemyBuildings[idx];
        return (t.building && t.building->hp > 0 && t.sprite);
    };

    // Special: bomber keeps old behavior (only targets walls, no main-target logic).
    if (isBomber(*u.unit))
    {
        u.breakingWall = false;
        u.mainTargetIndex = -1;

        if (!isValidIndex(u.targetIndex))
        {
            // Bomber (WallBreaker): ALWAYS look for the nearest REACHABLE wall.
            // The old "nearest by distance" rule could pick a wall that has no A* path
            // (e.g. inner walls), causing the unit to stall.
            int bestWall = -1;
            int bestLen = INT_MAX;
            std::vector<Pathfinding::GridPos> bestPath;

            for (int i = 0; i < (int)enemyBuildings.size(); ++i)
            {
                const auto& e = enemyBuildings[i];
        // NOTE: "isValid" was a leftover identifier and caused a build error on MSVC.
        // We should validate by index using the local helper.
        if (!isValidIndex(i) || e.id != 10) continue;

                std::vector<Pathfinding::GridPos> path;
                if (!buildBestPathForTarget(*u.unit, unitCell, e, blockedHard, path))
                    continue;

                int len = (int)path.size();
                if (len < bestLen)
                {
                    bestLen = len;
                    bestWall = i;
                    bestPath = std::move(path);
                }
                if (bestLen <= 2) break;
            }

            if (bestWall >= 0)
            {
                u.targetIndex = bestWall;
                u.path = std::move(bestPath);
                u.pathCursor = 0;
                if (!u.path.empty() && u.path[0].r == unitCell.r && u.path[0].c == unitCell.c)
                    u.pathCursor = 1;
                u.repathCD = 0.35f;
            }
            else
            {
                // Fallback: keep the old distance-based pick rule.
                u.targetIndex = pickTargetIndex(*u.unit, unitCell, enemyBuildings, blockedHard);
                u.path.clear();
                u.pathCursor = 0;
                u.repathCD = 0.0f;
            }
        }

        if (!isValidIndex(u.targetIndex)) return;
        auto& tgt = enemyBuildings[u.targetIndex];

        if (tgt.id == 10 && inAttackRangeCells(*u.unit, unitCell, tgt))
        {
            if (CombatSystem::bomberExplodeNoRange(*u.unit, u.sprite, tgt, enemyBuildings))
            {
                u.path.clear();
                u.pathCursor = 0;
                u.repathCD = 0.0f;
                u.targetIndex = -1;
                return;
            }
        }

        u.repathCD -= dt;
        if (u.repathCD <= 0.0f)
        {
            recomputePath(u, tgt, blockedHard);
            u.repathCD = std::max(u.repathCD, 0.05f);
        }
        stepAlongPath(u, dt);
        return;
    }

    // Ensure we have a valid main target.
    if (!isValidIndex(u.mainTargetIndex))
    {
        // Prefer a target that is actually reachable by A*.
        std::vector<Pathfinding::GridPos> bestPath;
        int reachable = pickReachableTargetIndex(*u.unit, unitCell, enemyBuildings, blockedHard, &bestPath);
        if (reachable >= 0)
        {
            u.mainTargetIndex = reachable;
            u.breakingWall = false;
            u.targetIndex = reachable;
            u.path = std::move(bestPath);
            u.pathCursor = 0;
            if (!u.path.empty() && u.path[0].r == unitCell.r && u.path[0].c == unitCell.c)
                u.pathCursor = 1;
            u.repathCD = 0.35f;
        }
        else
        {
            // No reachable building right now (likely fully enclosed). Fall back to
            // the priority rule target and start wall-breaking if needed.
            u.mainTargetIndex = pickTargetIndex(*u.unit, unitCell, enemyBuildings, blockedHard);
            u.breakingWall = false;
            u.targetIndex = u.mainTargetIndex;
            u.path.clear();
            u.pathCursor = 0;
            u.repathCD = 0.0f;
        }
    }

    if (!isValidIndex(u.mainTargetIndex))
    {
        u.targetIndex = -1;
        return;
    }

    // If we are breaking a wall but the wall is gone, return to the main target.
    if (u.breakingWall)
    {
        if (!isValidIndex(u.targetIndex) || enemyBuildings[u.targetIndex].id != 10)
        {
            // A wall we were targeting got destroyed (possibly by other units).
            // Re-evaluate the best reachable priority target immediately.
            u.breakingWall = false;
            std::vector<Pathfinding::GridPos> bestPath;
            int reachable = pickReachableTargetIndex(*u.unit, unitCell, enemyBuildings, blockedHard, &bestPath);
            if (reachable >= 0)
            {
                u.mainTargetIndex = reachable;
                u.targetIndex = reachable;
                u.path = std::move(bestPath);
                u.pathCursor = 0;
                if (!u.path.empty() && u.path[0].r == unitCell.r && u.path[0].c == unitCell.c)
                    u.pathCursor = 1;
                u.repathCD = 0.35f;
            }
            else
            {
                u.targetIndex = u.mainTargetIndex;
                u.path.clear();
                u.pathCursor = 0;
                u.repathCD = 0.0f;
            }
        }
    }

    // Current target = wall (temporary) or main target.
    int curIdx = u.breakingWall ? u.targetIndex : u.mainTargetIndex;
    if (!isValidIndex(curIdx))
    {
        // If current target invalid, reset and pick again next tick.
        u.targetIndex = -1;
        u.mainTargetIndex = -1;
        u.breakingWall = false;
        return;
    }
    u.targetIndex = curIdx;
    auto& tgt = enemyBuildings[curIdx];

    // 1) Attack if in range (center-based rules).
    if (inAttackRangeCells(*u.unit, unitCell, tgt))
    {
        if (tgt.building && tgt.sprite)
        {
            CombatSystem::unitHitBuildingNoRange(*u.unit, u.sprite, *tgt.building, tgt.sprite);
            if (tgt.building->hp <= 0)
            {
                u.path.clear();
                u.pathCursor = 0;
                u.repathCD = 0.0f;

                if (u.breakingWall)
                {
                    // Wall destroyed: immediately switch to the nearest REACHABLE priority target.
                    // This prevents wasting time hitting additional walls once a path exists.
                    Pathfinding::GridPos wcell = getCenterCell(tgt);
                    size_t widx = (size_t)wcell.r * (size_t)_cols + (size_t)wcell.c;
                    if (widx < blockedHard.size()) blockedHard[widx] = 0;

                    u.breakingWall = false;

                    std::vector<Pathfinding::GridPos> bestPath;
                    int reachable = pickReachableTargetIndex(*u.unit, unitCell, enemyBuildings, blockedHard, &bestPath);
                    if (reachable >= 0)
                    {
                        u.mainTargetIndex = reachable;
                        u.targetIndex = reachable;
                        u.path = std::move(bestPath);
                        u.pathCursor = 0;
                        if (!u.path.empty() && u.path[0].r == unitCell.r && u.path[0].c == unitCell.c)
                            u.pathCursor = 1;
                        u.repathCD = 0.35f;
                    }
                    else
                    {
                        // Still no reachable target, keep the main target and continue later.
                        u.targetIndex = u.mainTargetIndex;
                    }
                }
                else
                {
                    // Main target destroyed: repick.
                    u.targetIndex = -1;
                    u.mainTargetIndex = -1;
                }
            }
        }
        return;
    }

    // 2) Move with A*.
    u.repathCD -= dt;
    if (u.repathCD <= 0.0f)
    {
        if (!u.breakingWall)
        {
            // Always prefer a reachable priority target once any path exists.
            std::vector<Pathfinding::GridPos> bestPath;
            int reachable = pickReachableTargetIndex(*u.unit, unitCell, enemyBuildings, blockedHard, &bestPath);
            if (reachable >= 0)
            {
                u.mainTargetIndex = reachable;
                u.targetIndex = reachable;
                u.breakingWall = false;
                u.path = std::move(bestPath);
                u.pathCursor = 0;
                if (!u.path.empty() && u.path[0].r == unitCell.r && u.path[0].c == unitCell.c)
                    u.pathCursor = 1;
                u.repathCD = 0.35f;
            }
            else
            {
                // No reachable building -> keep the priority target and break walls if needed.
                if (!isValidIndex(u.mainTargetIndex))
                    u.mainTargetIndex = pickTargetIndex(*u.unit, unitCell, enemyBuildings, blockedHard);
                if (!isValidIndex(u.mainTargetIndex))
                {
                    u.targetIndex = -1;
                    return;
                }

                u.targetIndex = u.mainTargetIndex;
                recomputePath(u, enemyBuildings[u.mainTargetIndex], blockedHard);

                if (u.path.empty() && (isBarbarian(*u.unit) || isGiant(*u.unit)))
                {
                    Pathfinding::GridPos center = getCenterCell(enemyBuildings[u.mainTargetIndex]);
                    int wallIdx = pickWallToBreak(*u.unit, unitCell, center, enemyBuildings, blockedHard);
                    if (wallIdx >= 0)
                    {
                        u.breakingWall = true;
                        u.targetIndex = wallIdx;
                        u.path.clear();
                        u.pathCursor = 0;
                        u.repathCD = 0.0f;
                        recomputePath(u, enemyBuildings[wallIdx], blockedHard);
                    }
                }

                // If still no path, keep trying later (do not become permanently idle).
                if (u.path.empty()) u.repathCD = 0.60f;
            }
        }
        else
        {
            // While breaking a wall, keep walking to it.
            recomputePath(u, tgt, blockedHard);
            if (u.path.empty()) u.repathCD = 0.60f;
        }
    }

    stepAlongPath(u, dt);
}

void AISystem::updateDefenses(float dt,
    std::vector<BattleUnitRuntime>& units,
    std::vector<EnemyBuildingRuntime>& enemyBuildings)
{
    for (auto& e : enemyBuildings)
    {
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;

        // only defenses attack
        if (!(e.id == 1 || e.id == 2)) continue;

        CombatSystem::tryDefenseShoot(dt, *e.building, e.sprite, units, e.defenseCooldown, _cellSizePx);
    }
}

void AISystem::cleanup(float dt,
    std::vector<BattleUnitRuntime>& units,
    std::vector<EnemyBuildingRuntime>& enemyBuildings)
{
    // Units death animation and removal
    for (int i = (int)units.size() - 1; i >= 0; --i)
    {
        auto& u = units[i];
        if (!u.unit) { units.erase(units.begin() + i); continue; }

        if (u.unit->isDead())
        {
            if (!u.dying)
            {
                // SFX: troop death (play once when entering dying state).
                switch (u.unit->unitId)
                {
                case 2: // archer
                    SoundManager::playSfxRandom("archer_death", 1.0f);
                    break;
                case 3: // giant
                    SoundManager::playSfxRandom("giant_death", 1.0f);
                    break;
                case 4: // wall breaker
                    SoundManager::playSfxRandom("wall_breaker_death", 1.0f);
                    break;
                default: // barbarian
                    SoundManager::playSfxRandom("barbarian_death", 1.0f);
                    break;
                }

                u.dying = true;
                u.dyingTimer = 0.30f;

                if (u.sprite)
                {
                    u.sprite->stopAllActions();
                    u.sprite->runAction(Spawn::create(
                        FadeOut::create(0.30f),
                        ScaleTo::create(0.30f, u.sprite->getScaleX() * 0.5f, u.sprite->getScaleY() * 0.5f),
                        nullptr
                    ));
                }
            }
            else
            {
                u.dyingTimer -= dt;
                if (u.dyingTimer <= 0.0f)
                {
                    if (u.sprite) u.sprite->removeFromParent();
                    units.erase(units.begin() + i);
                }
            }
        }
    }

    // Buildings death behavior:
    // - Non-wall buildings: keep the sprite and swap it to a ruin texture (requested).
    // - Walls: keep the old fade-out + remove behavior.
    for (auto& e : enemyBuildings)
    {
        if (!e.building || !e.sprite) continue;
        if (e.building->hp > 0) continue;

        // Non-wall -> show ruin and keep it.
        if (e.id != 10)
        {
            if (!e.ruinShown)
            {
                // SFX: building destroyed (play once).
                SoundManager::playSfxRandom("building_destroyed", 1.0f);
                e.ruinShown = true;
                e.sprite->stopAllActions();
                e.sprite->setOpacity(255);

                // Remove HP bar if it exists.
                e.sprite->removeChildByName("__hpbar");

                // Swap to ruin texture. Keep original transform (pos/scale/rotation).
                auto tex = cocos2d::Director::getInstance()->getTextureCache()->addImage("ruin.png");
                if (tex)
                {
                    e.sprite->setTexture(tex);
                    cocos2d::Size ts = tex->getContentSize();
                    e.sprite->setTextureRect(cocos2d::Rect(0, 0, ts.width, ts.height));

                    // Requirement: ruin sprite must be a fixed 3x3 tiles size (not too large).
                    // We scale the ruin texture to fit INSIDE the projected 3x3 footprint.
                    // Slightly smaller than the full 3x3 footprint to avoid looking oversized.
                    float desiredW = std::max(1.0f, _tileW * 3.0f * 0.95f);
                    float desiredH = std::max(1.0f, _tileH * 3.0f * 0.95f);
                    float denomW = std::max(1.0f, ts.width);
                    float denomH = std::max(1.0f, ts.height);
                    float s = std::min(desiredW / denomW, desiredH / denomH);
                    // Guard: avoid crazy scales when grid metrics are not ready.
                    if (_tileW <= 0.01f || _tileH <= 0.01f) s = 1.0f;
                    e.sprite->setScale(s);
                }
                // Mark as "handled" so the wall-only death animation below won't run.
                e.dying = true;
            }
            continue;
        }

        // Wall -> old fade-out behavior.
        if (!e.dying)
        {
            // SFX: wall destroyed (reuse the same destroyed sound).
            SoundManager::playSfxRandom("building_destroyed", 1.0f);
            e.dying = true;
            e.dyingTimer = 0.35f;
            e.sprite->stopAllActions();
            e.sprite->runAction(cocos2d::Spawn::create(
                cocos2d::FadeOut::create(0.35f),
                cocos2d::ScaleTo::create(0.35f, e.sprite->getScaleX() * 0.7f, e.sprite->getScaleY() * 0.7f),
                nullptr
            ));
        }
        else
        {
            e.dyingTimer -= dt;
            if (e.dyingTimer <= 0.0f && e.sprite)
            {
                e.sprite->removeFromParent();
                e.sprite = nullptr;
            }
        }
    }
}

void AISystem::update(float dt,
    std::vector<BattleUnitRuntime>& units,
    std::vector<EnemyBuildingRuntime>& enemyBuildings)
{
    if (dt <= 0.0f) return;

    for (auto& u : units)
        updateOneUnit(dt, u, enemyBuildings);

    updateDefenses(dt, units, enemyBuildings);

    cleanup(dt, units, enemyBuildings);
}
