#pragma once
#include <string>
namespace ConfigManager {
	struct UpgradeCost { bool useGold; int amount; };
	std::string getBuildingName(int id);
	UpgradeCost getUpgradeCost(int id, int nextLevel);
	int getMaxLevel();
	struct CollectorStats { int ratePerHour; int capacity; int chunkMinutes; int hp; int upgradeGold; };
	CollectorStats getElixirCollectorStats(int level);
	CollectorStats getGoldMineStats(int level);
	struct StorageStats { int capAdd; int hp; int upgradeCost; };
	StorageStats getElixirStorageStats(int level);
	StorageStats getGoldStorageStats(int level);
	UpgradeCost getBuildCost(int id);
	// Build/upgrade time in seconds for given building id and level.
	// For new buildings, level is always 1.
	// Walls should return 0.
	int getBuildTimeSec(int id, int level);
	struct TownHallStats { int hp; int capAddElixir; int capAddGold; };
	TownHallStats getTownHallStats(int level);
	struct DefenseStats { int hp; float damagePerHit; float attacksPerSecond; int rangeCells; };
	DefenseStats getArrowTowerStats(int level);
	DefenseStats getCannonStats(int level);
	struct BarracksStats { int capAdd; int hp; };
	BarracksStats getBarracksStats(int level);
	struct TrainingCampStats { int hp; };
	TrainingCampStats getTrainingCampStats(int level);
	// Laboratory (TroopBuilding) stats
	struct LaboratoryStats { int hp; };
	LaboratoryStats getLaboratoryStats(int level);
	int getBuildLimit(int id, int townHallLevel);
	bool isUpgradeAllowed(int id, int nextLevel, int townHallLevel, int maxBarracksLevel);

	// Troop research (Laboratory)
	int getTroopResearchTimeSec(int unitId, int targetLevel);
	int getTroopResearchCostElixir(int unitId, int targetLevel);
}