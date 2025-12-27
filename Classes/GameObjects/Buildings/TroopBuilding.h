#pragma once

#include "GameObjects/Buildings/Building.h"
#include "Managers/ConfigManager.h"
#include "Managers/ResourceManager.h"
#include "cocos2d.h"               
#include "ui/CocosGUI.h"    
#include <unordered_map>

// Training Camp - level only determines which troop types can be trained
// Capacity is controlled ONLY by Barracks (sum of all Barracks)
// No training time or cost - clicking troop buttons adds directly to READY troops

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

    // Applies this Barracks' capacity delta to the global population cap.
    void applyCap() {
        int currentCap = ResourceManager::getPopulationCap();
        int delta = capAdd - capApplied;
        if (delta != 0) {
            ResourceManager::setPopulationCap(currentCap + delta);
            capApplied = capAdd;
        }
    }

    int getHousingCap() const { return capAdd; }

    static int getHousingCapByLevel(int level) {
        auto st = ConfigManager::getBarracksStats(level);
        return st.capAdd;
    }
};

class TrainingCamp : public Building {
public:
    enum TroopType {
        TROOP_BARBARIAN = 1,
        TROOP_ARCHER = 2,
        TROOP_GIANT = 3,
        TROOP_WALLBREAKER = 4,
    };

    TrainingCamp() { image = "buildings/buildings8.png"; }

    void setupStats(int level) {
        auto st = ConfigManager::getTrainingCampStats(level);
        hpMax = st.hp;
        if (hp > hpMax) hp = hpMax;
    }

    // Troop housing sizes
    static int getTroopHousing(TroopType t) {
        switch (t) {
        case TROOP_BARBARIAN:   return 1;
        case TROOP_ARCHER:      return 1;
        case TROOP_GIANT:       return 5;
        case TROOP_WALLBREAKER: return 2;
        default:                return 1;
        }
    }

    // UI icons
    static const char* getTroopIcon(TroopType t) {
        switch (t) {
        case TROOP_BARBARIAN:   return "ui/barbarian_choose_button.png";
        case TROOP_ARCHER:      return "ui/archer_choose_button.png";
        case TROOP_GIANT:       return "ui/giant_choose_buuton.png";
        case TROOP_WALLBREAKER: return "ui/wallbreaker_choose_button.png";
        default:                return "ui/barbarian_choose_button.png";
        }
    }

    static int getUnlockLevel(TroopType t) {
        switch (t) {
        case TROOP_BARBARIAN:   return 1;
        case TROOP_ARCHER:      return 2;
        case TROOP_GIANT:       return 3;
        case TROOP_WALLBREAKER: return 4;
        default:                return 1;
        }
    }

    bool isUnlocked(TroopType t) const { return level >= getUnlockLevel(t); }

    // READY troop management
    int getReadyCount(TroopType t) const {
        auto it = readyTroops.find((int)t);
        return it == readyTroops.end() ? 0 : it->second;
    }

    void setReadyCount(TroopType t, int count) {
        int key = (int)t;
        if (count <= 0) readyTroops.erase(key);
        else readyTroops[key] = count;
    }

    const std::unordered_map<int, int>& getAllReadyCounts() const { return readyTroops; }
    void clearAllReadyCounts() { readyTroops.clear(); }

    // Calculate used housing (READY troops only)
    int getUsedHousing() const {
        int sum = 0;
        for (const auto& kv : readyTroops) {
            sum += kv.second * getTroopHousing((TroopType)kv.first);
        }
        return sum;
    }

    // Try to add READY troop (direct add)
    bool tryAddReadyTroop(TroopType t) {
        if (!isUnlocked(t)) return false;

        // Get global capacity (from all Barracks)
        int totalCap = ResourceManager::getPopulationCap();
        if (totalCap <= 0) return false;

        // Calculate new used capacity
        int newUsed = getUsedHousing() + getTroopHousing(t);
        if (newUsed > totalCap) return false;

        // Add troop
        int key = (int)t;
        readyTroops[key] = getReadyCount(t) + 1;
        return true;
    }

    // Try to remove READY troop
    bool tryRemoveReadyTroop(TroopType t) {
        int key = (int)t;
        auto it = readyTroops.find(key);
        if (it == readyTroops.end() || it->second <= 0) return false;

        it->second -= 1;
        if (it->second <= 0) readyTroops.erase(it);
        return true;
    }

    // For compatibility with existing code
    void updateTraining(float dt) {} // No training time needed
    int getQueuedCount(TroopType t) const { return 0; } // No training queue
    int getActiveType() const { return 0; } // No active training
    float getActiveProgress01() const { return 0.0f; } // No progress bar
    bool tryAddTroop(TroopType t) { return tryAddReadyTroop(t); } // Compatible
    bool tryRemoveQueued(TroopType t) { return false; } // No queue
    std::vector<int> getQueueOrder() const { return {}; } // No queue order

private:
    std::unordered_map<int, int> readyTroops; // troop type -> ready count
};