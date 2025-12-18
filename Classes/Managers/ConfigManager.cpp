#include "Managers/ConfigManager.h"
namespace ConfigManager {
    std::string getBuildingName(int id) {
        switch (id) {
        case 1: return "Arrow Tower";
        case 2: return "Cannon";
        case 3: return "Elixir Collector";
        case 4: return "Elixir Storage";
        case 5: return "Gold Mine";
        case 6: return "Gold Storage";
        case 7: return "Barracks";
        case 8: return "Training Camp";
        case 9: return "Town Hall";
        case 10: return "Wall";
        default: return "Building";
        }
    }
    UpgradeCost getUpgradeCost(int id, int nextLevel) {
        if (id == 1) {
            if (nextLevel == 2) return { true, 2000 };
            if (nextLevel == 3) return { true, 5000 };
            return { true, 0 };
        }
        if (id == 2) {
            if (nextLevel == 2) return { true, 1000 };
            if (nextLevel == 3) return { true, 4000 };
            return { true, 0 };
        }
        if (id == 7) {
            if (nextLevel == 2) return { false, 2000 };
            if (nextLevel == 3) return { false, 2000 };
            return { false, 0 };
        }
        if (id == 8) {
            if (nextLevel == 2) return { false, 500 };
            if (nextLevel == 3) return { false, 2500 };
            return { false, 0 };
        }
        if (id == 3) {
            int currentLevel = nextLevel - 1;
            if (currentLevel < 1) return { true, 0 };
            auto st = getElixirCollectorStats(currentLevel);
            return { true, st.upgradeGold };
        }
        if (id == 5) {
            int currentLevel = nextLevel - 1;
            if (currentLevel < 1) return { false, 0 };
            auto st = getGoldMineStats(currentLevel);
            return { false, st.upgradeGold };
        }
        if (id == 9) {
            if (nextLevel == 2) return { true, 1000 };
            if (nextLevel == 3) return { true, 4000 };
            return { true, 0 };
        }
        if (id == 4) {
            int currentLevel = nextLevel - 1;
            if (currentLevel < 1) return { true, 0 };
            auto st = getElixirStorageStats(currentLevel);
            return { true, st.upgradeCost };
        }
        if (id == 6) {
            int currentLevel = nextLevel - 1;
            if (currentLevel < 1) return { false, 0 };
            auto st = getGoldStorageStats(currentLevel);
            return { false, st.upgradeCost };
        }
        int base = 500;
        int amount = base * nextLevel;
        bool useGold = true;
        if (id == 7 || id == 8) useGold = false;
        return { useGold, amount };
    }
    int getMaxLevel() { return 3; }
    CollectorStats getElixirCollectorStats(int level) {
        switch (level) {
        case 1: return { 200, 1000, 6, 75, 300 };
        case 2: return { 400, 2000, 15, 150, 700 };
        case 3: return { 600, 3000, 25, 300, 0 };
        default: return { 200, 1000, 6, 75, 300 };
        }
    }
    CollectorStats getGoldMineStats(int level) {
        switch (level) {
        case 1: return { 200, 1000, 6, 75, 300 };
        case 2: return { 400, 2000, 15, 150, 700 };
        case 3: return { 600, 3000, 25, 300, 0 };
        default: return { 200, 1000, 6, 75, 300 };
        }
    }
    StorageStats getElixirStorageStats(int level) {
        switch (level) {
        case 1: return { 1500, 150, 750 };
        case 2: return { 3000, 300, 1500 };
        case 3: return { 6000, 450, 0 };
        default: return { 1500, 150, 750 };
        }
    }
    StorageStats getGoldStorageStats(int level) {
        switch (level) {
        case 1: return { 1500, 150, 750 };
        case 2: return { 3000, 300, 1500 };
        case 3: return { 6000, 450, 0 };
        default: return { 1500, 150, 750 };
        }
    }
    UpgradeCost getBuildCost(int id) {
        if (id == 1) return { true, 1000 };
        if (id == 2) return { true, 250 };
        if (id == 3) return { true, 150 };
        if (id == 5) return { false, 150 };
        if (id == 4) return { true, 300 };
        if (id == 6) return { false, 300 };
        if (id == 7) return { false, 500 };
        if (id == 8) return { false, 200 };
        if (id == 10) return { true, 0 };
        return { true, 0 };
    }
    TownHallStats getTownHallStats(int level) {
        switch (level) {
        case 1: return { 100, 0, 0 };
        case 2: return { 100, 1500, 1500 };
        case 3: return { 100, 9000, 9000 };
        default: return { 100, 0, 0 };
        }
    }
    DefenseStats getArrowTowerStats(int level) {
        switch (level) {
        case 1: return { 380, 5.5f, 2.0f, 10 };
        case 2: return { 420, 7.5f, 2.0f, 10 };
        case 3: return { 460, 9.5f, 2.0f, 10 };
        default: return { 380, 5.5f, 2.0f, 10 };
        }
    }
    DefenseStats getCannonStats(int level) {
        switch (level) {
        case 1: return { 420, 9.0f, 1.0f, 0 };
        case 2: return { 470, 11.0f, 1.0f, 0 };
        case 3: return { 520, 12.0f, 1.0f, 0 };
        default: return { 420, 9.0f, 1.0f, 0 };
        }
    }
    BarracksStats getBarracksStats(int level) {
        switch (level) {
        case 1: return { 20, 100 };
        case 2: return { 30, 150 };
        case 3: return { 35, 200 };
        default: return { 20, 100 };
        }
    }
    TrainingCampStats getTrainingCampStats(int level) {
        switch (level) {
        case 1: return { 100 };
        case 2: return { 200 };
        case 3: return { 250 };
        default: return { 100 };
        }
    }
    int getBuildLimit(int id, int townHallLevel) {
        int th = townHallLevel;
        if (id == 1) {
            if (th <= 1) return 0;
            if (th == 2) return 1;
            return 2;
        }
        if (id == 2) {
            if (th <= 1) return 1;
            if (th == 2) return 2;
            return 3;
        }
        if (id == 7) {
            if (th <= 1) return 1;
            if (th == 2) return 2;
            return 3;
        }
        if (id == 3 || id == 5) {
            if (th <= 1) return 1;
            if (th == 2) return 2;
            return 3;
        }
        if (id == 4 || id == 6) {
            if (th <= 2) return 1;
            return 2;
        }
        if (id == 10) {
            if (th <= 1) return 25;
            if (th == 2) return 50;
            return 75;
        }
        return 1;
    }
    bool isUpgradeAllowed(int id, int nextLevel, int townHallLevel, int maxBarracksLevel) {
        if (id == 9) return true;
        if (nextLevel > townHallLevel) return false;
        return true;
    }
}