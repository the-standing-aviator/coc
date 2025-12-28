#pragma once
#include "GameObjects/Buildings/Building.h"
#include "Managers/ConfigManager.h"

class ArrowTower : public Building {
public:
    ArrowTower() { image = "buildings/buildings1.png"; }
    float damagePerHit = 0.f;
    float attacksPerSecond = 0.f;
    int rangeCells = 0;
    void setupStats(int level) {
        auto st = ConfigManager::getArrowTowerStats(level);
        hpMax = st.hp;
        if (hp > hpMax) hp = hpMax;
        damagePerHit = st.damagePerHit;
        attacksPerSecond = st.attacksPerSecond;
        rangeCells = st.rangeCells;
    }
};
class Cannon : public Building {
public:
    Cannon() { image = "buildings/buildings2.png"; }
    float damagePerHit = 0.f;
    float attacksPerSecond = 0.f;
    int rangeCells = 0;
    void setupStats(int level) {
        auto st = ConfigManager::getCannonStats(level);
        hpMax = st.hp;
        if (hp > hpMax) hp = hpMax;
        damagePerHit = st.damagePerHit;
        attacksPerSecond = st.attacksPerSecond;
        rangeCells = st.rangeCells;
    }
};
class Wall : public Building {
public:
    Wall() { image = "buildings/buildings9.png"; }
    // Wall stats are level-dependent.
    // Values are aligned with the simplified table used by this project.
    void setupStats(int level) {
        // Clamp to supported range in this project.
        if (level < 1) level = 1;
        if (level > 5) level = 5;

        // Hitpoints by level:
        // L1: 100, L2: 200, L3: 400, L4: 800, L5: 1200
        static const int kHp[6] = { 0, 100, 200, 400, 800, 1200 };
        hpMax = kHp[level];
        if (hp > hpMax) hp = hpMax;
    }
};