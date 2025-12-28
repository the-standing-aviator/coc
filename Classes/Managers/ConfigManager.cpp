#include "Managers/ConfigManager.h"
#include <algorithm>
namespace ConfigManager {

    // Helper: convert time values used in wiki-like tables to seconds.
    static int sec(int s) { return s; }
    static int min_(int m) { return m * 60; }
    static int hour(int h) { return h * 3600; }

    // Helper: returns the required Town Hall level for a building to reach the given level.
    // If not specified, defaults to 1.
    static int getTownHallRequiredForLevel(int id, int level) {
        level = std::max(1, std::min(5, level));

        // Resource producers
        if (id == 3 || id == 5) {
            // Level 1..5 all available from TH1 in the simplified tables.
            return 1;
        }
        // Resource storages
        if (id == 4 || id == 6) {
            if (level == 1) return 1;
            if (level == 2 || level == 3) return 2;
            return 3; // level 4-5
        }
        // Cannon
        if (id == 2) {
            if (level <= 2) return 1;
            if (level == 3) return 2;
            if (level == 4) return 3;
            return 4;
        }
        // Archer Tower
        if (id == 1) {
            if (level <= 2) return 2;
            if (level == 3) return 3;
            if (level == 4) return 4;
            return 5;
        }
        // Army camp (Barracks in this project)
        if (id == 7) {
            return level; // 1..5
        }
        // Training Camp
        if (id == 8) {
            if (level <= 3) return 1;
            if (level == 4) return 2;
            return 3;
        }
        // Town Hall upgrade should NOT require the *next* Town Hall level.
        // To upgrade Town Hall from (L) -> (L+1), the current Town Hall level is already L,
        // so the requirement for reaching `level` should be (level - 1).
        if (id == 9) return std::max(1, level - 1);

        // Wall
        if (id == 10) return 1;

        // Laboratory
        if (id == 11) {
            // L1 requires TH2; L2->TH3; L3->TH4; L4-5->TH5 (project requirement).
            static const int req[6] = { 0, 2, 3, 4, 5, 5 };
            return req[level];
        }

        return 1;
    }
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
        case 11: return "Laboratory";
        default: return "Building";
        }
    }
    UpgradeCost getUpgradeCost(int id, int nextLevel) {
        nextLevel = std::max(1, std::min(5, nextLevel));

        // Costs are based on the wiki-like tables (cost to reach that level).
        // NOTE: In this project, `useGold=true` means spending GOLD, `useGold=false` means spending ELIXIR.

        // Elixir Collector costs GOLD
        if (id == 3) {
            static const int kCost[6] = { 0, 150, 300, 700, 1400, 3000 };
            return { true, kCost[nextLevel] };
        }
        // Gold Mine costs ELIXIR
        if (id == 5) {
            static const int kCost[6] = { 0, 150, 300, 700, 1400, 3000 };
            return { false, kCost[nextLevel] };
        }
        // Elixir Storage costs GOLD
        if (id == 4) {
            static const int kCost[6] = { 0, 300, 750, 1500, 3000, 6000 };
            return { true, kCost[nextLevel] };
        }
        // Gold Storage costs ELIXIR
        if (id == 6) {
            static const int kCost[6] = { 0, 300, 750, 1500, 3000, 6000 };
            return { false, kCost[nextLevel] };
        }
        // Cannon costs GOLD
        if (id == 2) {
            static const int kCost[6] = { 0, 250, 1000, 4000, 16000, 50000 };
            return { true, kCost[nextLevel] };
        }
        // Archer Tower costs GOLD
        if (id == 1) {
            static const int kCost[6] = { 0, 1000, 2000, 5000, 20000, 70000 };
            return { true, kCost[nextLevel] };
        }
        // Barracks (Army Camp) costs ELIXIR
        if (id == 7) {
            static const int kCost[6] = { 0, 200, 2000, 10000, 100000, 250000 };
            return { false, kCost[nextLevel] };
        }
        // Training Camp costs ELIXIR
        if (id == 8) {
            // Original unlock order includes Goblin at level 4, Wall Breaker at level 5.
            // In this project we REMOVE Goblin and move Wall Breaker unlock to level 4,
            // but we keep the original cost curve.
            static const int kCost[6] = { 0, 100, 500, 2500, 5000, 20000 };
            // NOTE: Goblin is removed; Wall Breaker unlock is moved to level 4.
            // Level 5 has no new unit in this simplified project, but we keep a cost for upgrade testing.
            return { false, kCost[nextLevel] };
        }
        // Town Hall costs GOLD (upgrade only)
        if (id == 9) {
            static const int kCost[6] = { 0, 0, 1000, 4000, 25000, 150000 };
            return { true, kCost[nextLevel] };
        }
        // Wall costs GOLD (instant)
        if (id == 10) {
            // Costs to reach each level (GOLD).
            // L1: 0, L2: 1000, L3: 5000, L4: 10000, L5: 20000
            static const int kCost[6] = { 0, 0, 1000, 5000, 10000, 20000 };
            return { true, kCost[nextLevel] };
        }

        // Laboratory costs ELIXIR
        if (id == 11) {
            // Costs to reach each level (ELIXIR).
            // L1: 5000, L2: 25000, L3: 50000, L4: 100000, L5: 200000
            static const int kCost[6] = { 0, 5000, 25000, 50000, 100000, 200000 };
            return { false, kCost[nextLevel] };
        }

        return { true, 0 };
    }
    int getMaxLevel() { return 5; }
    CollectorStats getElixirCollectorStats(int level) {
        switch (level) {
        case 1: return { 200, 1000, 1, 75, 0 };
        case 2: return { 400, 2000, 1, 150, 0 };
        case 3: return { 600, 3000, 1, 300, 0 };
        case 4: return { 800, 5000, 1, 400, 0 };
        case 5: return { 1000, 10000, 1, 500, 0 };
        default: return { 200, 1000, 1, 75, 0 };
        }
    }
    CollectorStats getGoldMineStats(int level) {
        switch (level) {
        case 1: return { 200, 1000, 1, 75, 0 };
        case 2: return { 400, 2000, 1, 150, 0 };
        case 3: return { 600, 3000, 1, 300, 0 };
        case 4: return { 800, 5000, 1, 400, 0 };
        case 5: return { 1000, 10000, 1, 500, 0 };
        default: return { 200, 1000, 1, 75, 0 };
        }
    }
    StorageStats getElixirStorageStats(int level) {
        switch (level) {
        case 1: return { 1500, 150, 0 };
        case 2: return { 3000, 300, 0 };
        case 3: return { 6000, 450, 0 };
        case 4: return { 12000, 600, 0 };
        case 5: return { 150000, 800, 0 };
        default: return { 1500, 150, 0 };
        }
    }
    StorageStats getGoldStorageStats(int level) {
        switch (level) {
        case 1: return { 1500, 150, 0 };
        case 2: return { 3000, 300, 0 };
        case 3: return { 6000, 450, 0 };
        case 4: return { 12000, 600, 0 };
        case 5: return { 150000, 800, 0 };
        default: return { 1500, 150, 0 };
        }
    }
    UpgradeCost getBuildCost(int id) {
        // Build cost is the cost to reach level 1.
        if (id == 1) return getUpgradeCost(id, 1);
        if (id == 2) return getUpgradeCost(id, 1);
        if (id == 3) return getUpgradeCost(id, 1);
        if (id == 4) return getUpgradeCost(id, 1);
        if (id == 5) return getUpgradeCost(id, 1);
        if (id == 6) return getUpgradeCost(id, 1);
        if (id == 7) return getUpgradeCost(id, 1);
        if (id == 8) return getUpgradeCost(id, 1);
        if (id == 10) return getUpgradeCost(id, 1);
        if (id == 11) return getUpgradeCost(id, 1);
        // Town Hall is not bought in the shop in this project.
        return { true, 0 };
    }

    int getBuildTimeSec(int id, int level) {
        level = std::max(1, std::min(5, level));

        // Walls are instant.
        if (id == 10) return 0;

        // Elixir Collector / Gold Mine
        if (id == 3 || id == 5) {
            static const int t[6] = { 0, sec(5), sec(15), min_(1), min_(2), min_(5) };
            return t[level];
        }
        // Storages
        if (id == 4 || id == 6) {
            static const int t[6] = { 0, sec(10), min_(2), min_(5), min_(15), min_(30) };
            return t[level];
        }
        // Cannon
        if (id == 2) {
            static const int t[6] = { 0, sec(5), sec(30), min_(2), min_(20), min_(30) };
            return t[level];
        }
        // Archer Tower
        if (id == 1) {
            static const int t[6] = { 0, sec(15), min_(2), min_(20), hour(1), hour(1) + min_(30) };
            return t[level];
        }
        // Town Hall (upgrade times)
        if (id == 9) {
            static const int t[6] = { 0, 0, sec(10), min_(30), hour(3), hour(6) };
            return t[level];
        }
        // Army Camp (Barracks in this project)
        if (id == 7) {
            static const int t[6] = { 0, min_(1), min_(5), min_(30), hour(2), hour(6) };
            return t[level];
        }
        // Training Camp
        if (id == 8) {
            static const int t[6] = { 0, sec(10), sec(15), min_(2), min_(30), hour(2) };
            return t[level];
        }

        // Laboratory
        if (id == 11) {
            // Time to reach each level.
            // L1: 1m, L2: 30m, L3: 2h, L4: 4h, L5: 8h
            static const int t[6] = { 0, min_(1), min_(30), hour(2), hour(4), hour(8) };
            return t[level];
        }

        return 0;
    }
    TownHallStats getTownHallStats(int level) {
        switch (level) {
        case 1: return { 400, 0, 0 };
        case 2: return { 800, 0, 0 };
        case 3: return { 1600, 0, 0 };
        case 4: return { 2000, 0, 0 };
        case 5: return { 2400, 0, 0 };
        default: return { 400, 0, 0 };
        }
    }
    DefenseStats getArrowTowerStats(int level) {
        switch (level) {
        case 1: return { 380, 5.5f, 11.0f / 5.5f, 10 };
        case 2: return { 420, 7.5f, 15.0f / 7.5f, 10 };
        case 3: return { 460, 9.5f, 19.0f / 9.5f, 10 };
        case 4: return { 500, 12.5f, 25.0f / 12.5f, 10 };
        case 5: return { 540, 15.0f, 30.0f / 15.0f, 10 };
        default: return { 380, 5.5f, 11.0f / 5.5f, 10 };
        }
    }
    DefenseStats getCannonStats(int level) {
        switch (level) {
        case 1: return { 300, 5.6f, 7.0f / 5.6f, 9 };
        case 2: return { 360, 8.0f, 10.0f / 8.0f, 9 };
        case 3: return { 420, 10.4f, 13.0f / 10.4f, 9 };
        case 4: return { 500, 13.6f, 17.0f / 13.6f, 9 };
        case 5: return { 600, 18.4f, 23.0f / 18.4f, 9 };
        default: return { 300, 5.6f, 7.0f / 5.6f, 9 };
        }
    }
    BarracksStats getBarracksStats(int level) {
        switch (level) {
        case 1: return { 20, 100 };
        case 2: return { 30, 150 };
        case 3: return { 35, 200 };
        case 4: return { 40, 250 };
        case 5: return { 45, 300 };
        default: return { 20, 100 };
        }
    }
    TrainingCampStats getTrainingCampStats(int level) {
        switch (level) {
        case 1: return { 100 };
        case 2: return { 200 };
        case 3: return { 250 };
        case 4: return { 300 };
        case 5: return { 360 };
        default: return { 100 };
        }
    }

    LaboratoryStats getLaboratoryStats(int level) {
        switch (level) {
        case 1: return { 500 };
        case 2: return { 550 };
        case 3: return { 600 };
        case 4: return { 650 };
        case 5: return { 700 };
        default: return { 500 };
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

        if (id == 11) {
            // Laboratory can be built from Town Hall level 2; only 1 allowed.
            if (th < 2) return 0;
            return 1;
        }
        return 1;
    }
    bool isUpgradeAllowed(int id, int nextLevel, int townHallLevel, int maxBarracksLevel) {
        // Town Hall requirement gating is based on the table screenshots.
        // This function is used both for upgrade checks and build (level 1) checks.
        int requiredTH = getTownHallRequiredForLevel(id, nextLevel);
        if (townHallLevel < requiredTH) return false;

        // Optional extra gating for TrainingCamp can be added here if needed.
        (void)maxBarracksLevel;
        return true;
    }
}

// ===== Troop research (Laboratory) =====
// Research timing/cost rule (user requirement):
// - Level 1 -> 2 : 0.5 hour
// - Level 2 -> 3 : 1.0 hour
// - Each further level adds +0.5 hour
// - Cost uses elixir only: L1->2 costs 10000, each next level doubles.
int ConfigManager::getTroopResearchTimeSec(int unitId, int targetLevel)
{
	(void)unitId;
	if (targetLevel <= 1) return 0;
	// targetLevel=2 => 0.5h, 3 => 1h, 4 => 1.5h ...
	const int halfHourSec = 30 * 60;
	return (targetLevel - 1) * halfHourSec;
}

int ConfigManager::getTroopResearchCostElixir(int unitId, int targetLevel)
{
	(void)unitId;
	if (targetLevel <= 1) return 0;
	// targetLevel=2 => 10000, 3 => 20000, 4 => 40000 ...
	int cost = 10000;
	for (int lv = 2; lv < targetLevel; ++lv) {
		cost *= 2;
	}
	return cost;
}
