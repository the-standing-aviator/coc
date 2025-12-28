#pragma once

#include "cocos2d.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

struct SaveMeta
{
    int slot = 0;
    bool exists = false;
    std::string name;
    std::string path;
    int64_t updatedAt = 0;
};

struct SaveBuilding
{
    int id = 0;
    int level = 1;
    int r = 0;
    int c = 0;
    int hp = 0;
    float stored = 0.0f;

    // Build/upgrade persistence
    int buildState = 0;          // Building::BuildState
    float buildTotalSec = 0.0f;  // total seconds
    float buildRemainSec = 0.0f; // remaining seconds
    int upgradeTargetLevel = 0;  // if upgrading
};


struct SaveTrainedTroop
{
    int type = 0; // TrainingCamp::TroopType (1..4)
    int r = 0;    // grid row (MainScene cell)
    int c = 0;    // grid col (MainScene cell)
};

struct SaveData
{
    int slot = 0;
    std::string name;
    int gold = 0;
    int elixir = 0;
    int population = 0;
    float timeScale = 1.0f;
    int64_t lastRealTime = 0; // unix seconds
    std::vector<SaveBuilding> buildings;
    std::vector<SaveTrainedTroop> trainedTroops;

    // Troop levels (research results). Key is UnitFactory unitId (1..4).
    std::unordered_map<int, int> troopLevels;

    // Laboratory research state (only one research at a time).
    int researchUnitId = 0;
    int researchTargetLevel = 0;
    float researchTotalSec = 0.0f;
    float researchRemainSec = 0.0f;


};

class SaveSystem
{
public:
    static void setCurrentSlot(int slot);
    static int getCurrentSlot();

    static void setBattleTargetSlot(int slot);
    static int getBattleTargetSlot();

    // READY troops passed from MainScene to BattleScene.
    static void setBattleReadyTroops(const std::unordered_map<int, int>& troops);
    static const std::unordered_map<int, int>& getBattleReadyTroops();

    // Troop levels passed from MainScene to BattleScene.
    static void setBattleTroopLevels(const std::unordered_map<int, int>& levels);
    static const std::unordered_map<int, int>& getBattleTroopLevels();

    static std::string getSaveDir();
    static std::string getSavePath(int slot);
    static bool exists(int slot);
    static std::vector<SaveMeta> listAllSlots();

    static bool load(int slot, SaveData& outData);
    static bool save(const SaveData& data);
    static bool remove(int slot);
    static SaveData makeDefault(int slot, const std::string& name);

private:
    static constexpr int kMaxSlots = 20;
    static int s_currentSlot;
    static int s_battleTargetSlot;
    static std::unordered_map<int, int> s_battleReadyTroops;
    static std::unordered_map<int, int> s_battleTroopLevels;
};