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
    void setupStats() {
        hpMax = 300;
        if (hp > hpMax) hp = hpMax;
    }
};