#pragma once
#include "GameObjects/Buildings/Building.h"
#include "Managers/ConfigManager.h"
#include "Managers/ResourceManager.h"
class TownHall : public Building {
public:
    TownHall() { image = "buildings/buildings0.png"; }
    int capAddElixir = 0;
    int capAddGold = 0;
    int capAppliedElixir = 0;
    int capAppliedGold = 0;
    void setupStats(int level) {
        auto st = ConfigManager::getTownHallStats(level);
        hpMax = st.hp;
        if (hp > hpMax) hp = hpMax;
        capAddElixir = st.capAddElixir;
        capAddGold = st.capAddGold;
    }
    void applyCap() {
        int dElixir = capAddElixir - capAppliedElixir;
        if (dElixir > 0) {
            ResourceManager::setElixirCap(ResourceManager::getElixirCap() + dElixir);
            capAppliedElixir = capAddElixir;
        }
        int dGold = capAddGold - capAppliedGold;
        if (dGold > 0) {
            ResourceManager::setGoldCap(ResourceManager::getGoldCap() + dGold);
            capAppliedGold = capAddGold;
        }
    }
};
