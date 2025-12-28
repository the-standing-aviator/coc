#include "Data/SaveSystem.h"
#include "cocos2d.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"
#include <ctime>
#include <cstdio>

USING_NS_CC;

int SaveSystem::s_currentSlot = 0;
int SaveSystem::s_battleTargetSlot = -1;
std::unordered_map<int, int> SaveSystem::s_battleReadyTroops;
std::unordered_map<int, int> SaveSystem::s_battleTroopLevels;

void SaveSystem::setCurrentSlot(int slot)
{
    s_currentSlot = slot;
}

int SaveSystem::getCurrentSlot()
{
    return s_currentSlot;
}

void SaveSystem::setBattleTargetSlot(int slot)
{
    s_battleTargetSlot = slot;
}

int SaveSystem::getBattleTargetSlot()
{
    return s_battleTargetSlot;
}

std::string SaveSystem::getSaveDir()
{
    auto dir = FileUtils::getInstance()->getWritablePath() + std::string("saves/");
    FileUtils::getInstance()->createDirectory(dir);
    return dir;
}

std::string SaveSystem::getSavePath(int slot)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "save_%02d.json", slot);
    return getSaveDir() + buf;
}

bool SaveSystem::exists(int slot)
{
    if (slot < 0 || slot >= kMaxSlots) return false;
    return FileUtils::getInstance()->isFileExist(getSavePath(slot));
}

std::vector<SaveMeta> SaveSystem::listAllSlots()
{
    std::vector<SaveMeta> metas;
    metas.reserve(kMaxSlots);
    for (int i = 0; i < kMaxSlots; ++i)
    {
        SaveMeta meta;
        meta.slot = i;
        meta.path = getSavePath(i);
        meta.exists = exists(i);
        meta.name = cocos2d::StringUtils::format("Save %02d", i + 1);
        if (meta.exists)
        {
            SaveData tmp;
            if (load(i, tmp))
            {
                meta.name = tmp.name.empty() ? meta.name : tmp.name;
            }
        }
        metas.push_back(meta);
    }
    return metas;
}

bool SaveSystem::load(int slot, SaveData& outData)
{
    if (slot < 0 || slot >= kMaxSlots) return false;
    std::string path = getSavePath(slot);
    if (!FileUtils::getInstance()->isFileExist(path)) return false;
    std::string content = FileUtils::getInstance()->getStringFromFile(path);
    rapidjson::Document doc;
    doc.Parse<0>(content.c_str());
    if (doc.HasParseError() || !doc.IsObject()) return false;

    outData = SaveData();
    outData.slot = slot;
    outData.name = cocos2d::StringUtils::format("Save %02d", slot + 1);

    if (doc.HasMember("meta") && doc["meta"].IsObject())
    {
        const auto& meta = doc["meta"];
        if (meta.HasMember("name") && meta["name"].IsString())
        {
            outData.name = meta["name"].GetString();
        }
        // Offline production timestamp
        if (meta.HasMember("lastRealTime") && meta["lastRealTime"].IsInt64())
        {
            outData.lastRealTime = meta["lastRealTime"].GetInt64();
        }
    }

    if (doc.HasMember("resources") && doc["resources"].IsObject())
    {
        const auto& res = doc["resources"];
        if (res.HasMember("gold") && res["gold"].IsInt()) outData.gold = res["gold"].GetInt();
        if (res.HasMember("elixir") && res["elixir"].IsInt()) outData.elixir = res["elixir"].GetInt();
        if (res.HasMember("population") && res["population"].IsInt()) outData.population = res["population"].GetInt();
    }

    // IMPORTANT:
    // Time-scale is a *temporary cheat* only.
    // Requirement: initial state must be NO acceleration; only enable after clicking the cheat button.
    // Therefore we intentionally DO NOT load persisted timeScale from the save file.
    outData.timeScale = 1.0f;

    if (doc.HasMember("buildings") && doc["buildings"].IsArray())
    {
        const auto& arr = doc["buildings"];
        for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
        {
            const auto& v = arr[i];
            if (!v.IsObject()) continue;
            if (!v.HasMember("id") || !v.HasMember("level") || !v.HasMember("r") || !v.HasMember("c")) continue;
            SaveBuilding b;
            if (v["id"].IsInt()) b.id = v["id"].GetInt();
            if (v["level"].IsInt()) b.level = v["level"].GetInt();
            if (v["r"].IsInt()) b.r = v["r"].GetInt();
            if (v["c"].IsInt()) b.c = v["c"].GetInt();
            if (v.HasMember("hp") && v["hp"].IsInt()) b.hp = v["hp"].GetInt();
            if (v.HasMember("stored") && v["stored"].IsNumber()) b.stored = v["stored"].GetFloat();

            // Build/upgrade fields (backward compatible: missing fields -> default values).
            if (v.HasMember("buildState") && v["buildState"].IsInt()) b.buildState = v["buildState"].GetInt();
            if (v.HasMember("buildTotalSec") && v["buildTotalSec"].IsNumber()) b.buildTotalSec = v["buildTotalSec"].GetFloat();
            if (v.HasMember("buildRemainSec") && v["buildRemainSec"].IsNumber()) b.buildRemainSec = v["buildRemainSec"].GetFloat();
            if (v.HasMember("upgradeTargetLevel") && v["upgradeTargetLevel"].IsInt()) b.upgradeTargetLevel = v["upgradeTargetLevel"].GetInt();
            outData.buildings.push_back(b);
        }
    }

    
// Trained troops (stand units in village)
if (doc.HasMember("trainedTroops") && doc["trainedTroops"].IsArray())
{
    const auto& arr = doc["trainedTroops"];
    for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
    {
        const auto& it = arr[i];
        if (!it.IsObject()) continue;

        SaveTrainedTroop t;
        if (it.HasMember("type") && it["type"].IsInt()) t.type = it["type"].GetInt();
        if (it.HasMember("r") && it["r"].IsInt()) t.r = it["r"].GetInt();
        if (it.HasMember("c") && it["c"].IsInt()) t.c = it["c"].GetInt();
        if (t.type > 0) outData.trainedTroops.push_back(t);
    }
}


    // Troop levels (backward compatible).
    outData.troopLevels.clear();
    // Default: all troops are level 1.
    for (int id = 1; id <= 4; ++id) outData.troopLevels[id] = 1;

    if (doc.HasMember("troopLevels") && doc["troopLevels"].IsArray())
    {
        const auto& arr = doc["troopLevels"];
        for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
        {
            const auto& it = arr[i];
            if (!it.IsObject()) continue;
            if (!it.HasMember("id") || !it.HasMember("level")) continue;
            if (!it["id"].IsInt() || !it["level"].IsInt()) continue;
            int tid = it["id"].GetInt();
            int tlv = it["level"].GetInt();
            if (tid >= 1 && tid <= 4) outData.troopLevels[tid] = std::max(1, tlv);
        }
    }

    // Research state (backward compatible).
    outData.researchUnitId = 0;
    outData.researchTargetLevel = 0;
    outData.researchTotalSec = 0.0f;
    outData.researchRemainSec = 0.0f;
    if (doc.HasMember("research") && doc["research"].IsObject())
    {
        const auto& r = doc["research"];
        if (r.HasMember("unitId") && r["unitId"].IsInt()) outData.researchUnitId = r["unitId"].GetInt();
        if (r.HasMember("targetLevel") && r["targetLevel"].IsInt()) outData.researchTargetLevel = r["targetLevel"].GetInt();
        if (r.HasMember("totalSec") && r["totalSec"].IsNumber()) outData.researchTotalSec = r["totalSec"].GetFloat();
        if (r.HasMember("remainSec") && r["remainSec"].IsNumber()) outData.researchRemainSec = r["remainSec"].GetFloat();

        // Clamp invalid values.
        if (outData.researchRemainSec < 0.0f) outData.researchRemainSec = 0.0f;
        if (outData.researchTotalSec < 0.0f) outData.researchTotalSec = 0.0f;
        if (outData.researchRemainSec > outData.researchTotalSec && outData.researchTotalSec > 0.0f)
            outData.researchRemainSec = outData.researchTotalSec;
        if (outData.researchUnitId < 1 || outData.researchUnitId > 4)
        {
            outData.researchUnitId = 0;
            outData.researchTargetLevel = 0;
            outData.researchTotalSec = 0.0f;
            outData.researchRemainSec = 0.0f;
        }
    }

return true;
}

bool SaveSystem::save(const SaveData& data)
{
    if (data.slot < 0 || data.slot >= kMaxSlots) return false;

    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    rapidjson::Value meta(rapidjson::kObjectType);
    meta.AddMember("name", rapidjson::Value(data.name.c_str(), alloc), alloc);
    meta.AddMember("updatedAt", static_cast<int64_t>(std::time(nullptr)), alloc);
    // Persist real-world timestamp for offline production.
    meta.AddMember("lastRealTime", static_cast<int64_t>(std::time(nullptr)), alloc);
    doc.AddMember("meta", meta, alloc);

    rapidjson::Value res(rapidjson::kObjectType);
    res.AddMember("gold", data.gold, alloc);
    res.AddMember("elixir", data.elixir, alloc);
    res.AddMember("population", data.population, alloc);
    doc.AddMember("resources", res, alloc);

    // Time-scale cheat is not persisted. Always save default 1.0 to overwrite any old saves.
    doc.AddMember("timeScale", 1.0, alloc);

    rapidjson::Value arr(rapidjson::kArrayType);
    for (const auto& b : data.buildings)
    {
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.AddMember("id", b.id, alloc);
        obj.AddMember("level", b.level, alloc);
        obj.AddMember("r", b.r, alloc);
        obj.AddMember("c", b.c, alloc);
        obj.AddMember("hp", b.hp, alloc);
        obj.AddMember("stored", b.stored, alloc);

        obj.AddMember("buildState", b.buildState, alloc);
        obj.AddMember("buildTotalSec", b.buildTotalSec, alloc);
        obj.AddMember("buildRemainSec", b.buildRemainSec, alloc);
        obj.AddMember("upgradeTargetLevel", b.upgradeTargetLevel, alloc);
        arr.PushBack(obj, alloc);
    }
    doc.AddMember("buildings", arr, alloc);

// Trained troops (stand units in village)
rapidjson::Value tArr(rapidjson::kArrayType);
for (const auto& t : data.trainedTroops)
{
    rapidjson::Value o(rapidjson::kObjectType);
    o.AddMember("type", t.type, alloc);
    o.AddMember("r", t.r, alloc);
    o.AddMember("c", t.c, alloc);
    tArr.PushBack(o, alloc);
}
doc.AddMember("trainedTroops", tArr, alloc);


    // Troop levels
    rapidjson::Value lvArr(rapidjson::kArrayType);
    // Ensure default values exist (backward compatible callers).
    std::unordered_map<int,int> levels = data.troopLevels;
    for (int id = 1; id <= 4; ++id) {
        if (levels.find(id) == levels.end()) levels[id] = 1;
    }
    for (int id = 1; id <= 4; ++id)
    {
        rapidjson::Value o(rapidjson::kObjectType);
        o.AddMember("id", id, alloc);
        o.AddMember("level", levels[id], alloc);
        lvArr.PushBack(o, alloc);
    }
    doc.AddMember("troopLevels", lvArr, alloc);

    // Research state
    rapidjson::Value rObj(rapidjson::kObjectType);
    rObj.AddMember("unitId", data.researchUnitId, alloc);
    rObj.AddMember("targetLevel", data.researchTargetLevel, alloc);
    rObj.AddMember("totalSec", data.researchTotalSec, alloc);
    rObj.AddMember("remainSec", data.researchRemainSec, alloc);
    doc.AddMember("research", rObj, alloc);


    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::string output = buffer.GetString();
    return FileUtils::getInstance()->writeStringToFile(output, getSavePath(data.slot));
}

bool SaveSystem::remove(int slot)
{
    if (slot < 0 || slot >= kMaxSlots) return false;
    return FileUtils::getInstance()->removeFile(getSavePath(slot));
}

SaveData SaveSystem::makeDefault(int slot, const std::string& name)
{
    SaveData data;
    data.slot = slot;
    data.name = name;
    data.gold = 1000;
    data.elixir = 1000;
    data.population = 0;
    data.timeScale = 1.0f;
    data.lastRealTime = static_cast<int64_t>(std::time(nullptr));
    SaveBuilding th;
    th.id = 9;
    th.level = 1;
    th.r = 15;
    th.c = 15;
    th.hp = 100;
    th.stored = 0.0f;
    data.buildings.push_back(th);
    
    // Default troop levels
    for (int id = 1; id <= 4; ++id) data.troopLevels[id] = 1;
    // No active research
    data.researchUnitId = 0;
    data.researchTargetLevel = 0;
    data.researchTotalSec = 0.0f;
    data.researchRemainSec = 0.0f;
return data;
}

void SaveSystem::setBattleReadyTroops(const std::unordered_map<int, int>& troops)
{
    s_battleReadyTroops = troops;
}

const std::unordered_map<int, int>& SaveSystem::getBattleReadyTroops()
{
    return s_battleReadyTroops;
}

void SaveSystem::setBattleTroopLevels(const std::unordered_map<int, int>& levels)
{
    s_battleTroopLevels = levels;
}

const std::unordered_map<int, int>& SaveSystem::getBattleTroopLevels()
{
    return s_battleTroopLevels;
}

