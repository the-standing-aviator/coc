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
            default: return "Building";
        }
    }
UpgradeCost getUpgradeCost(int id, int nextLevel) {
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
    if (id == 3) return { true, 150 };
    if (id == 5) return { false, 150 };
    if (id == 4) return { true, 300 };
    if (id == 6) return { false, 300 };
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
}
