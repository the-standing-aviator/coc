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
    
    
    
    float dx = p.x - _anchor.x;
    float dy = _anchor.y - p.y;

    float cr = 2.0f * dx / _tileW;  
    float cs = 2.0f * dy / _tileH;  

    float cf = (cr + cs) * 0.5f;    
    float rf = (cs - cr) * 0.5f;    

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

        
        if (e.id == 10)
        {
            if (wallsBlocked) markCell(rr, cc);
            continue;
        }

        if (centerOnly)
        {
            
            markCell(rr, cc);
        }
        else
        {
            
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

    
    
    
    

    Pathfinding::GridPos center = getCenterCell(target);
    int dr = std::abs(unitCell.r - center.r);
    int dc = std::abs(unitCell.c - center.c);
    int cheb = std::max(dr, dc);

    if (target.id == 10)
    {
        
        if (isArcher(unit)) return (cheb <= 3);
        return (cheb <= 1);
    }

    if (isArcher(unit)) return (cheb <= 3);
    
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

    
    
    const int range = isArcher(unit) ? 3 : 1;
    for (const auto& bc : footprint)
    {
        for (int dr = -range; dr <= range; ++dr)
        {
            for (int dc = -range; dc <= range; ++dc)
            {
                
                if (std::max(std::abs(dr), std::abs(dc)) > range) continue;

                int rr = bc.r + dr;
                int cc = bc.c + dc;
                if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) continue;
                size_t idx = (size_t)rr * (size_t)_cols + (size_t)cc;
                if (idx < isFoot.size() && isFoot[idx]) continue; 
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

    
    if (isArcher(unit) || isBomber(unit)) return -1;

    std::vector<unsigned char> blk = blockedHard;
    if ((int)blk.size() != _rows * _cols)
        blk.assign((size_t)_rows * (size_t)_cols, 0);

    
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

    
    for (int i = 0; i < (int)enemyBuildings.size(); ++i)
    {
        const auto& e = enemyBuildings[i];
        if (e.id != 10) continue;
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;

        Pathfinding::GridPos wcell = getCenterCell(e);

        
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
    // Selects the most appropriate target index based on unit type and distance.
    //
    // Targeting policy:
    // - WallBreaker (bomber): prefers the nearest wall; if no walls exist, attacks the nearest
    //   non-wall building.
    // - Giant: if at least one defense is alive, prefers the nearest defense building; otherwise
    //   attacks the nearest non-wall building.
    // - Archer: attacks the nearest non-wall building (NO special preference for resource
    //   buildings). This matches the requirement that archers should focus on the closest building
    //   excluding walls.
    // - All other units: attacks the nearest non-wall building.
    (void)blocked;  // This function uses a fast heuristic and does not require a blocked map.

    const bool defensesExist = anyDefenseAlive(enemyBuildings);

    auto isValid = [&](const EnemyBuildingRuntime& e) -> bool {
        return (e.building && e.building->hp > 0 && e.sprite);
    };

    auto chebDistTo = [&](const EnemyBuildingRuntime& e) -> int {
        const Pathfinding::GridPos c = getCenterCell(e);
        return std::max(std::abs(unitCell.r - c.r), std::abs(unitCell.c - c.c));
    };

    // Helper that returns the nearest matching building index.
    auto findNearest = [&](int* outBest, int* outBestD, bool (*match)(const EnemyBuildingRuntime&)) {
        *outBest = -1;
        *outBestD = INT_MAX;
        for (int i = 0; i < (int)enemyBuildings.size(); ++i)
        {
            const auto& e = enemyBuildings[i];
            if (!isValid(e)) continue;
            if (!match(e)) continue;
            const int d = chebDistTo(e);
            if (d < *outBestD)
            {
                *outBestD = d;
                *outBest = i;
            }
        }
    };

    int best = -1;
    int bestD = INT_MAX;

    // 1) Bomber: nearest wall, fallback to any non-wall.
    if (isBomber(unit))
    {
        auto matchWall = [](const EnemyBuildingRuntime& e) -> bool { return e.id == 10; };
        auto matchNonWall = [](const EnemyBuildingRuntime& e) -> bool { return e.id != 10; };

        findNearest(&best, &bestD, matchWall);
        if (best >= 0) return best;

        findNearest(&best, &bestD, matchNonWall);
        return best;
    }

    // 2) Giant: nearest defense if any exist.
    if (isGiant(unit) && defensesExist)
    {
        auto matchDefense = [](const EnemyBuildingRuntime& e) -> bool {
            return (e.id == 1 || e.id == 2);  // Arrow tower or cannon.
        };

        findNearest(&best, &bestD, matchDefense);
        if (best >= 0) return best;
    }

    // 3) Archer: explicitly nearest non-wall (no resource priority).
    if (isArcher(unit))
    {
        auto matchNonWall = [](const EnemyBuildingRuntime& e) -> bool { return e.id != 10; };
        findNearest(&best, &bestD, matchNonWall);
        return best;
    }

    // 4) Default: nearest non-wall.
    auto matchNonWall = [](const EnemyBuildingRuntime& e) -> bool { return e.id != 10; };
    findNearest(&best, &bestD, matchNonWall);
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

    
    std::vector<unsigned char> blk = blockedHard;
    if ((int)blk.size() != _rows * _cols)
        blk.assign((size_t)_rows * (size_t)_cols, 0);

    
    int sidx = start.r * _cols + start.c;
    if (sidx >= 0 && sidx < (int)blk.size()) blk[sidx] = 0;

    std::vector<Pathfinding::GridPos> goals;

    if (target.id == 10)
    {
        
        collectApproachCells(unit, target, blk, goals);
    }
    else if (isArcher(unit))
    {
        
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

    
    
    std::vector<int> candidates;
    candidates.reserve(enemyBuildings.size());

    if (isBomber(unit))
    {
        
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

    
    std::vector<unsigned char> blk = blocked;
    if ((int)blk.size() != _rows * _cols)
        blk.assign((size_t)_rows * (size_t)_cols, 0);

    
    int sidx = start.r * _cols + start.c;
    if (sidx >= 0 && sidx < (int)blk.size()) blk[sidx] = 0;

    
    std::vector<Pathfinding::GridPos> goals;

    if (target.id == 10)
    {
        
        collectApproachCells(*u.unit, target, blk, goals);
    }
    else if (isArcher(*u.unit))
    {
        
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

    
    u.repathCD = u.path.empty() ? 0.80f : 0.35f;
}

void AISystem::stepAlongPath(BattleUnitRuntime& u,
    float dt)
{
    if (!u.sprite || !u.unit) return;
    if (!_gridReady) return;

    if (u.path.empty() || u.pathCursor >= (int)u.path.size())
    {
        
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

    
    std::vector<unsigned char> blockedHard;
    if (_gridReady)
    {
        
        buildBlockedMap(enemyBuildings, blockedHard, true, isGiant(*u.unit));
        
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

    
    if (isBomber(*u.unit))
    {
        u.breakingWall = false;
        u.mainTargetIndex = -1;

        if (!isValidIndex(u.targetIndex))
        {
            
            
            
            int bestWall = -1;
            int bestLen = INT_MAX;
            std::vector<Pathfinding::GridPos> bestPath;

            for (int i = 0; i < (int)enemyBuildings.size(); ++i)
            {
                const auto& e = enemyBuildings[i];
        
        
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

    
    if (!isValidIndex(u.mainTargetIndex))
    {
        
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

    
    if (u.breakingWall)
    {
        if (!isValidIndex(u.targetIndex) || enemyBuildings[u.targetIndex].id != 10)
        {
            
            
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

    
    int curIdx = u.breakingWall ? u.targetIndex : u.mainTargetIndex;
    if (!isValidIndex(curIdx))
    {
        
        u.targetIndex = -1;
        u.mainTargetIndex = -1;
        u.breakingWall = false;
        return;
    }
    u.targetIndex = curIdx;
    auto& tgt = enemyBuildings[curIdx];

    
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
                        
                        u.targetIndex = u.mainTargetIndex;
                    }
                }
                else
                {
                    
                    u.targetIndex = -1;
                    u.mainTargetIndex = -1;
                }
            }
        }
        return;
    }

    
    u.repathCD -= dt;
    if (u.repathCD <= 0.0f)
    {
        if (!u.breakingWall)
        {
            
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

                
                if (u.path.empty()) u.repathCD = 0.60f;
            }
        }
        else
        {
            
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

        
        if (!(e.id == 1 || e.id == 2)) continue;

        CombatSystem::tryDefenseShoot(dt, *e.building, e.sprite, units, e.defenseCooldown, _cellSizePx);
    }
}

void AISystem::cleanup(float dt,
    std::vector<BattleUnitRuntime>& units,
    std::vector<EnemyBuildingRuntime>& enemyBuildings)
{
    
    for (int i = (int)units.size() - 1; i >= 0; --i)
    {
        auto& u = units[i];
        if (!u.unit) { units.erase(units.begin() + i); continue; }

        if (u.unit->isDead())
        {
            if (!u.dying)
            {
                
                switch (u.unit->unitId)
                {
                case 2: 
                    SoundManager::playSfxRandom("archer_death", 1.0f);
                    break;
                case 3: 
                    SoundManager::playSfxRandom("giant_death", 1.0f);
                    break;
                case 4: 
                    SoundManager::playSfxRandom("wall_breaker_death", 1.0f);
                    break;
                default: 
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

    
    
    
    for (auto& e : enemyBuildings)
    {
        if (!e.building || !e.sprite) continue;
        if (e.building->hp > 0) continue;

        
        if (e.id != 10)
        {
            if (!e.ruinShown)
            {
                
                SoundManager::playSfxRandom("building_destroyed", 1.0f);
                e.ruinShown = true;
                e.sprite->stopAllActions();
                e.sprite->setOpacity(255);

                
                e.sprite->removeChildByName("__hpbar");

                
                auto tex = cocos2d::Director::getInstance()->getTextureCache()->addImage("ruin.png");
                if (tex)
                {
                    e.sprite->setTexture(tex);
                    cocos2d::Size ts = tex->getContentSize();
                    e.sprite->setTextureRect(cocos2d::Rect(0, 0, ts.width, ts.height));

                    
                    
                    
                    float desiredW = std::max(1.0f, _tileW * 3.0f * 0.95f);
                    float desiredH = std::max(1.0f, _tileH * 3.0f * 0.95f);
                    float denomW = std::max(1.0f, ts.width);
                    float denomH = std::max(1.0f, ts.height);
                    float s = std::min(desiredW / denomW, desiredH / denomH);
                    
                    if (_tileW <= 0.01f || _tileH <= 0.01f) s = 1.0f;
                    e.sprite->setScale(s);
                }
                
                e.dying = true;
            }
            continue;
        }

        
        if (!e.dying)
        {
            
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
