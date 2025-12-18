#pragma once
#include "GameObjects/Buildings/Building.h"
#include "Managers/ConfigManager.h"
#include "Managers/ResourceManager.h"

class Barracks : public Building {
public:
    Barracks() { image = "buildings/buildings7.png"; }
    int capAdd = 0;
    int capApplied = 0;
    void setupStats(int level) {
        auto st = ConfigManager::getBarracksStats(level);
        capAdd = st.capAdd;
        hpMax = st.hp;
        if (hp > hpMax) hp = hpMax;
    }
    void applyCap() {
        int currentCap = ResourceManager::getPopulationCap();
        int delta = capAdd - capApplied;
        if (delta != 0) {
            ResourceManager::setPopulationCap(currentCap + delta);
            capApplied = capAdd;
        }
    }
};
class TrainingCamp : public Building {
public:
    TrainingCamp() { image = "buildings/buildings8.png"; }
    void setupStats(int level) {
        auto st = ConfigManager::getTrainingCampStats(level);
        hpMax = st.hp;
        if (hp > hpMax) hp = hpMax;
    }
};