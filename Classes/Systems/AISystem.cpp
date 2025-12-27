#include "Systems/AISystem.h"

#include "Systems/CombatSystem.h"
#include "Systems/Pathfinding.h"

#include "GameObjects/Buildings/DefenseBuilding.h"

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
    bool wallsBlocked) const
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

        // Other buildings occupy 3x3 around center (same as MainScene placement rule).
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                markCell(rr + dr, cc + dc);
    }
}

int AISystem::findWallIndexAtCell(int r, int c,
    const std::vector<EnemyBuildingRuntime>& enemyBuildings) const
{
    for (int i = 0; i < (int)enemyBuildings.size(); ++i)
    {
        const auto& e = enemyBuildings[i];
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;
        if (e.id != 10) continue;

        int rr = e.r;
        int cc = e.c;
        if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols)
        {
            auto g = worldToGrid(e.pos);
            rr = g.r;
            cc = g.c;
        }
        if (rr == r && cc == c) return i;
    }
    return -1;
}

int AISystem::pickTargetIndex(const UnitBase& unit,
    const Vec2& unitPos,
    const std::vector<EnemyBuildingRuntime>& enemyBuildings) const
{
    // Target preference rules (per your requirement):
    // 1) Bomber: nearest wall
    // 2) Giant: only defenses; if none left then nearest non-wall building
    // 3) Others: nearest non-wall building
    // 4) Barbarian exception: can also target walls if they are the nearest

    const bool defensesExist = anyDefenseAlive(enemyBuildings);

    int best = -1;
    float bestDist = 1e30f;

    auto accept = [&](int buildingId) -> bool {
        if (isBomber(unit)) return buildingId == 10;

        if (isGiant(unit))
        {
            if (defensesExist) return (buildingId == 1 || buildingId == 2);
            return buildingId != 10; // after defenses gone
        }

        if (isBarbarian(unit))
        {
            // Barbarian can hit everything.
            return true;
        }

        // default: exclude walls
        return buildingId != 10;
        };

    for (int i = 0; i < (int)enemyBuildings.size(); ++i)
    {
        const auto& e = enemyBuildings[i];
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;
        if (!accept(e.id)) continue;

        float d = e.pos.distance(unitPos);
        if (d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }

    // If Giant has no defenses AND there is no non-wall building left, do nothing.
    // If other (non-barbarian) has no non-wall building, do nothing.
    return best;
}

Pathfinding::GridPos AISystem::pickBestApproachCell(const UnitBase& unit,
    const EnemyBuildingRuntime& target,
    const std::vector<unsigned char>& blocked) const
{
    // Goal cell is blocked by the building itself.
    // We need an approach cell (passable) from which the unit can attack.
    // - Melee units: pick a passable cell adjacent to the building footprint (wall=1 cell, others=3x3)
    // - Ranged units: pick a cell within attackRange.
    int tr = target.r;
    int tc = target.c;

    if (tr < 0 || tr >= _rows || tc < 0 || tc >= _cols)
    {
        auto g = worldToGrid(target.pos);
        tr = g.r;
        tc = g.c;
    }

    Vec2 targetWorld = target.sprite ? target.sprite->getPosition() : target.pos;

    Pathfinding::GridPos best{ tr, tc };
    float bestScore = 1e30f;

    auto passable = [&](int rr, int cc) -> bool {
        if (rr < 0 || rr >= _rows || cc < 0 || cc >= _cols) return false;
        int idx = rr * _cols + cc;
        if (idx < 0 || idx >= (int)blocked.size()) return false;
        return blocked[idx] == 0;
        };

    // Melee: ring around footprint
    if (isMelee(unit))
    {
        int inner = (target.id == 10) ? 0 : 1; // wall: center only; others: 3x3
        int outer = inner + 1;                 // ring thickness 1

        // Enumerate ring cells around (tr,tc)
        for (int dr = -outer; dr <= outer; ++dr)
        {
            for (int dc = -outer; dc <= outer; ++dc)
            {
                // skip interior (occupied footprint)
                if (std::abs(dr) <= inner && std::abs(dc) <= inner) continue;

                int rr = tr + dr;
                int cc = tc + dc;
                if (!passable(rr, cc)) continue;

                Vec2 w = gridToWorld(rr, cc);
                float score = w.distance(targetWorld) + (float)(std::abs(dr) + std::abs(dc)) * 2.0f;
                if (score < bestScore)
                {
                    bestScore = score;
                    best = { rr, cc };
                }
            }
        }
        return best;
    }

    // Ranged: within attack range
    int k = (int)std::ceil(std::max(1.0f, unit.attackRange / std::max(8.0f, _cellSizePx)));
    k = std::min(k + 1, 8);

    for (int dr = -k; dr <= k; ++dr)
    {
        for (int dc = -k; dc <= k; ++dc)
        {
            int rr = tr + dr;
            int cc = tc + dc;
            if (!passable(rr, cc)) continue;

            Vec2 w = gridToWorld(rr, cc);
            if (w.distance(targetWorld) > unit.attackRange * 0.98f) continue;

            float score = w.distance(targetWorld) + (float)(std::abs(dr) + std::abs(dc)) * 3.0f;
            if (score < bestScore)
            {
                bestScore = score;
                best = { rr, cc };
            }
        }
    }
    return best;
}

void AISystem::recomputePath(BattleUnitRuntime& u,
    const EnemyBuildingRuntime& target,
    const std::vector<unsigned char>& blocked)
{
    if (!_gridReady || !u.sprite || !u.unit) return;

    Pathfinding::GridPos start = worldToGrid(u.sprite->getPosition());

    // Build a local blocked map we can tweak (e.g. allow goal cell passable)
    std::vector<unsigned char> blk = blocked;
    if ((int)blk.size() != _rows * _cols)
        blk.assign((size_t)_rows * (size_t)_cols, 0);

    // Normal: find an approach cell (melee: adjacent to footprint; ranged: within range)
    Pathfinding::GridPos goal = pickBestApproachCell(*u.unit, target, blk);

    // start must be passable
    int sidx = start.r * _cols + start.c;
    if (sidx >= 0 && sidx < (int)blk.size()) blk[sidx] = 0;

    auto path = Pathfinding::findPathAStar(_rows, _cols, start, goal, blk, false, 20000);

    u.path = std::move(path);
    u.pathCursor = 0;

    if (!u.path.empty() && u.path[0].r == start.r && u.path[0].c == start.c)
        u.pathCursor = 1;

    u.repathCD = 0.35f;
}

void AISystem::stepAlongPath(BattleUnitRuntime& u,
    const EnemyBuildingRuntime& target,
    float dt)
{
    if (!u.sprite || !u.unit) return;

    if (!_gridReady || u.path.empty() || u.pathCursor >= (int)u.path.size())
    {
        // fallback: direct step to target (still respects stop distance)
        Vec2 cur = u.sprite->getPosition();
        Vec2 dst = target.sprite ? target.sprite->getPosition() : target.pos;
        float stop = std::max(8.0f, u.unit->attackRange * 0.85f);
        Vec2 next = Pathfinding::stepTowards(cur, dst, u.unit->moveSpeed * dt, stop);
        u.sprite->setPosition(next);
        return;
    }

    Vec2 cur = u.sprite->getPosition();
    auto cell = u.path[u.pathCursor];
    Vec2 waypoint = gridToWorld(cell.r, cell.c);

    float stop = 0.0f; // waypoint is an intermediate; don't stop short
    Vec2 next = Pathfinding::stepTowards(cur, waypoint, u.unit->moveSpeed * dt, stop);
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

    // keep hp bar visible/updated
    CombatSystem::ensureHpBar(u.sprite, u.unit->hp, u.unit->hpMax, true);

    u.unit->tickAttack(dt);

    // Build blocked maps (hard: walls block; soft: walls treated passable for "break wall" detection)
    std::vector<unsigned char> blockedHard;
    std::vector<unsigned char> blockedSoft;
    if (_gridReady)
    {
        buildBlockedMap(enemyBuildings, blockedHard, true);
        buildBlockedMap(enemyBuildings, blockedSoft, false);

        // ensure unit's own cell is passable
        auto st = worldToGrid(u.sprite->getPosition());
        int idx = st.r * _cols + st.c;
        if (idx >= 0 && idx < (int)blockedHard.size()) blockedHard[idx] = 0;
        if (idx >= 0 && idx < (int)blockedSoft.size()) blockedSoft[idx] = 0;
    }

    // validate / repick target
    bool needPick = false;
    if (u.targetIndex < 0 || u.targetIndex >= (int)enemyBuildings.size()) needPick = true;
    else {
        auto& t = enemyBuildings[u.targetIndex];
        if (!t.building || t.building->hp <= 0 || !t.sprite) needPick = true;
    }

    if (needPick)
    {
        u.targetIndex = pickTargetIndex(*u.unit, u.sprite->getPosition(), enemyBuildings);
        u.path.clear();
        u.pathCursor = 0;
        u.repathCD = 0.0f;
    }

    if (u.targetIndex < 0) return;

    auto& tgt = enemyBuildings[u.targetIndex];

    // keep building hp bar visible/updated
    if (tgt.sprite && tgt.building)
        CombatSystem::ensureHpBar(tgt.sprite, tgt.building->hp, tgt.building->hpMax, false);

    // 0) Bomber special: explode at wall
    if (isBomber(*u.unit))
    {
        if (CombatSystem::tryBomberExplode(*u.unit, u.sprite, tgt, enemyBuildings))
        {
            u.path.clear();
            u.pathCursor = 0;
            u.repathCD = 0.0f;
            return;
        }
    }

    // 1) try normal attack (range + cooldown + damage + float text)
    if (!isBomber(*u.unit) && CombatSystem::tryUnitAttackBuilding(*u.unit, u.sprite, *tgt.building, tgt.sprite))
    {
        // if building dies, clear path soon
        if (tgt.building->hp <= 0)
        {
            u.path.clear();
            u.pathCursor = 0;
            u.repathCD = 0.0f;
        }
        return;
    }

    // 2) move with A*
    u.repathCD -= dt;
    if (u.repathCD <= 0.0f)
    {
        recomputePath(u, tgt, blockedHard);

        // -------- Break-wall behavior (melee units only, non-bomber) --------
        // If the "ideal" shortest path (treating walls as passable) crosses a wall,
        // we first attack that wall.
        if (_gridReady && !isBomber(*u.unit) && isMelee(*u.unit) && tgt.id != 10)
        {
            Pathfinding::GridPos start = worldToGrid(u.sprite->getPosition());
            Pathfinding::GridPos goalSoft = pickBestApproachCell(*u.unit, tgt, blockedSoft);
            auto pathSoft = Pathfinding::findPathAStar(_rows, _cols, start, goalSoft, blockedSoft, false, 20000);

            const int hardLen = (int)u.path.size();
            const int softLen = (int)pathSoft.size();

            // Decide whether wall likely blocks the optimal route
            bool shouldBreak = false;
            if (!pathSoft.empty())
            {
                if (u.path.empty()) shouldBreak = true;
                else if (hardLen > softLen + 4) shouldBreak = true;
            }

            if (shouldBreak)
            {
                int wallIdx = -1;
                for (const auto& cell : pathSoft)
                {
                    wallIdx = findWallIndexAtCell(cell.r, cell.c, enemyBuildings);
                    if (wallIdx >= 0) break;
                }

                if (wallIdx >= 0)
                {
                    u.targetIndex = wallIdx;
                    auto& wall = enemyBuildings[wallIdx];
                    recomputePath(u, wall, blockedHard);
                }
            }
        }

        // -------- Fallback: if still no path to a non-wall building, target nearest reachable wall --------
        if (_gridReady && !isBomber(*u.unit) && u.path.empty() && tgt.id != 10)
        {
            int bestWall = -1;
            int bestLen = 1e9;
            Pathfinding::GridPos start = worldToGrid(u.sprite->getPosition());

            for (int i = 0; i < (int)enemyBuildings.size(); ++i)
            {
                auto& w = enemyBuildings[i];
                if (!w.building || w.building->hp <= 0 || !w.sprite) continue;
                if (w.id != 10) continue;

                // compute approach to this wall
                auto goal = pickBestApproachCell(*u.unit, w, blockedHard);
                auto p = Pathfinding::findPathAStar(_rows, _cols, start, goal, blockedHard, false, 12000);
                if (!p.empty() && (int)p.size() < bestLen)
                {
                    bestLen = (int)p.size();
                    bestWall = i;
                    u.path = std::move(p);
                    u.pathCursor = 0;
                    if (!u.path.empty() && u.path[0].r == start.r && u.path[0].c == start.c)
                        u.pathCursor = 1;
                }
            }

            if (bestWall >= 0) u.targetIndex = bestWall;
        }
    }

    stepAlongPath(u, tgt, dt);
}

void AISystem::updateDefenses(float dt,
    std::vector<BattleUnitRuntime>& units,
    std::vector<EnemyBuildingRuntime>& enemyBuildings)
{
    for (auto& e : enemyBuildings)
    {
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;

        // keep hp bar updated
        CombatSystem::ensureHpBar(e.sprite, e.building->hp, e.building->hpMax, false);

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
                u.dying = true;
                u.dyingTimer = 0.30f;

                if (u.sprite)
                {
                    // placeholder death anim: fade + shrink
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

    // Buildings death animation (sprite only)
    for (auto& e : enemyBuildings)
    {
        if (!e.building || !e.sprite) continue;
        if (e.building->hp > 0) continue;

        if (!e.dying)
        {
            e.dying = true;
            e.dyingTimer = 0.35f;
            e.sprite->stopAllActions();
            e.sprite->runAction(Spawn::create(
                FadeOut::create(0.35f),
                ScaleTo::create(0.35f, e.sprite->getScaleX() * 0.7f, e.sprite->getScaleY() * 0.7f),
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
