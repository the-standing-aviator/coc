#include "Scenes/BattleScene.h"
#include "Scenes/MainScene.h"
#include "Scenes/LoginScene.h"
#include "Data/SaveSystem.h"
#include "GameObjects/Buildings/Building.h"
#include "GameObjects/Buildings/TroopBuilding.h"
#include "Managers/ConfigManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/SoundManager.h"

#include "ui/CocosGUI.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>
#include <cmath>
#include <chrono>

USING_NS_CC;

Scene* BattleScene::createScene()
{
    return BattleScene::create();
}

void BattleScene::setBuildingVisualParams()
{
    _buildingScale = 0.33f;
    // Indexed by building id. Reserve a bit more space for new building types.
    _buildingScaleById.assign(12, 1.0f);
    _buildingOffsetById.assign(12, Vec2::ZERO);
    _buildingScaleById[1] = 0.4f;
    _buildingOffsetById[1] = Vec2(0, 20 / 3);
    _buildingOffsetById[2] = Vec2(-2 / 3, 14 / 3);
    _buildingOffsetById[3] = Vec2(-10 / 3, 14 / 3);
    _buildingOffsetById[4] = Vec2(4 / 3, 4 / 3);
    _buildingScaleById[7] = 0.7f;
    _buildingOffsetById[7] = Vec2(-4 / 3, 0);
    _buildingScaleById[8] = 1.3f;
    _buildingOffsetById[8] = Vec2(4 / 3, 10 / 3);
    _buildingScaleById[9] = 0.8f;
    _buildingOffsetById[9] = Vec2(4 / 3, 10 / 3);

    // Wall (id=10) visuals
    if ((int)_buildingScaleById.size() > 10) {
        _buildingScaleById[10] = 0.8f;
        _buildingOffsetById[10] = Vec2(0, 4 / 3);
    }
}

Vec2 BattleScene::gridToWorld(int r, int c) const
{
    float x = _anchor.x + (c - r) * (_tileW * 0.5f);
    float y = _anchor.y - (c + r) * (_tileH * 0.5f);
    return Vec2(x, y);
}

void BattleScene::renderTargetVillage()
{
    if (!_world) return;

    // Load both attacker/defender save data once.
    if (!_savesLoaded) {
        loadBattleSaves();
    }

    auto children = _world->getChildren();
    for (auto child : children)
    {
        if (child != _background) child->removeFromParent();
    }

    int targetSlot = SaveSystem::getBattleTargetSlot();
    if (targetSlot < 0 || !SaveSystem::exists(targetSlot) || !_savesLoaded)
    {
        auto msg = Label::createWithSystemFont("No target selected", "Arial", 30);
        msg->setPosition(Director::getInstance()->getVisibleOrigin() + Director::getInstance()->getVisibleSize() / 2);
        this->addChild(msg, 5);
        return;
    }

    // _defenderData was already loaded by loadBattleSaves().
    SaveData& data = _defenderData;

// Loot model is initialized after enemy buildings are spawned (needs storage caps).
_lootGoldTotal = 0;
_lootElixirTotal = 0;
_lootedGold = 0;
_lootedElixir = 0;
if (_lootHud) updateLootHUD();

    auto imageUvToWorld = [this](const Vec2& uv) {
        if (!_background) return Vec2::ZERO;
        Size s = _background->getContentSize();
        float sx = _background->getScaleX();
        float sy = _background->getScaleY();
        Vec2 local((uv.x - 0.5f) * s.width, (uv.y - 0.5f) * s.height);
        Vec2 parentSpace = _background->getPosition() + Vec2(local.x * sx, local.y * sy);
        return _background->getParent()->convertToWorldSpace(parentSpace);
    };

    // Sample 4 points from the map image to estimate the isometric diamond.
// NOTE: The UV values are approximate. We re-order the resulting points by
// their extremums (top/bottom/left/right) so the math is robust even if the
// texture or coordinate system is vertically flipped.
Vec2 uvA(0.51f, 0.20f);
Vec2 uvB(0.83f, 0.49f);
Vec2 uvC(0.51f, 0.92f);
Vec2 uvD(0.17f, 0.49f);

Vec2 pA = _world->convertToNodeSpace(imageUvToWorld(uvA));
Vec2 pB = _world->convertToNodeSpace(imageUvToWorld(uvB));
Vec2 pC = _world->convertToNodeSpace(imageUvToWorld(uvC));
Vec2 pD = _world->convertToNodeSpace(imageUvToWorld(uvD));

std::vector<Vec2> pts;
pts.reserve(4);
pts.push_back(pA);
pts.push_back(pB);
pts.push_back(pC);
pts.push_back(pD);

Vec2 top = *std::max_element(pts.begin(), pts.end(), [](const Vec2& a, const Vec2& b) { return a.y < b.y; });
Vec2 bottom = *std::min_element(pts.begin(), pts.end(), [](const Vec2& a, const Vec2& b) { return a.y < b.y; });
Vec2 left = *std::min_element(pts.begin(), pts.end(), [](const Vec2& a, const Vec2& b) { return a.x < b.x; });
Vec2 right = *std::max_element(pts.begin(), pts.end(), [](const Vec2& a, const Vec2& b) { return a.x < b.x; });

float Lr = std::fabs(right.x - left.x);
float Lt = std::fabs(top.y - bottom.y);

_tileW = (2.0f * Lr) / (_cols + _rows);
_tileH = (2.0f * Lt) / (_cols + _rows);
_anchor = top;

// Cache deployable diamond corners (node space of _world).
_deployTop = top;
_deployRight = right;
_deployBottom = bottom;
_deployLeft = left;
_deployAreaReady = true;

    // Initialize battle AI grid conversion.
    _cellSizePx = std::max(8.0f, (_tileW + _tileH) * 0.25f);
    _ai.setCellSizePx(_cellSizePx);
    _ai.setIsoGrid(_rows, _cols, _tileW, _tileH, _anchor);

    std::sort(data.buildings.begin(), data.buildings.end(), [](const SaveBuilding& a, const SaveBuilding& b) {
        int sa = a.r + a.c;
        int sb = b.r + b.c;
        if (sa == sb) return a.r < b.r;
        return sa < sb;
    });

    // Cache enemy building cells for deployment blocking.
    _enemyBuildings.clear();
    _enemyBuildings.reserve(data.buildings.size());

    for (int bi = 0; bi < (int)data.buildings.size(); ++bi)
    {
        const auto& bInfo = data.buildings[bi];
        auto b = BuildingFactory::create(bInfo.id, std::max(1, bInfo.level), true, false);
        if (!b) continue;

        // Battle starts with a fresh enemy village: all buildings are full HP.
        // (HP bars are shown only after taking damage.)
        b->hp = b->hpMax;

        b->stored = bInfo.stored;
        auto sprite = b->createSprite();

        Vec2 pos = gridToWorld(bInfo.r, bInfo.c);
        int idx = std::max(1, std::min(11, bInfo.id));
        Vec2 off = _buildingOffsetById[idx];

        sprite->setPosition(pos + off);
        sprite->setScale(_buildingScale * _buildingScaleById[idx]);

        EnemyBuildingRuntime rt;
        rt.id = bInfo.id;
        rt.r = bInfo.r;
        rt.c = bInfo.c;
        rt.saveIndex = bi;
        rt.pos = pos + off;
        rt.building = std::move(b);
        rt.sprite = sprite;
        rt.lastHp = rt.building ? rt.building->hp : 0;

        _enemyBuildings.push_back(std::move(rt));
        _world->addChild(sprite, 3 + bInfo.r + bInfo.c);
    }


// Lootable resources: 50% of the enemy village's current resources.
// - Gold/Elixir storages: loot is proportional to HP lost (up to 50% from that storage).
// - TownHall: loot can only be obtained when the TownHall is completely destroyed.
{
    int goldBase = std::max(0, data.gold);
    int elixirBase = std::max(0, data.elixir);
    int extraGoldFromMines = 0;
    int extraElixirFromCollectors = 0;
    int targetGold = 0;
    int targetElixir = 0;

    _lootedGold = 0;
    _lootedElixir = 0;

    // Reset per-building loot state.
    for (auto& eb : _enemyBuildings)
    {
        eb.lootGoldMax = 0;
        eb.lootElixirMax = 0;
        eb.lootGoldTaken = 0;
        eb.lootElixirTaken = 0;
        eb.lootCollected = false;
        eb.lastHp = eb.building ? eb.building->hp : 0;
    }

    // Collect storages and TownHall indices.
    std::vector<int> goldStorIdx;
    std::vector<int> goldStorMaxLoot;
    std::vector<int> elixirStorIdx;
    std::vector<int> elixirStorMaxLoot;
    std::vector<int> goldMineIdx;
    std::vector<int> goldMineMaxLoot;
    std::vector<int> elixirCollectorIdx;
    std::vector<int> elixirCollectorMaxLoot;
    int townHallIdx = -1;

    for (int i = 0; i < (int)_enemyBuildings.size(); ++i)
    {
        auto& eb = _enemyBuildings[i];
        if (eb.id == 6) // Gold Storage
        {
            int lv = eb.building ? eb.building->level : 1;
            auto st = ConfigManager::getGoldStorageStats(lv);
            int cap = std::max(0, st.capAdd);
            goldStorIdx.push_back(i);
            goldStorMaxLoot.push_back(cap / 2);
        }
        else if (eb.id == 4) // Elixir Storage
        {
            int lv = eb.building ? eb.building->level : 1;
            auto st = ConfigManager::getElixirStorageStats(lv);
            int cap = std::max(0, st.capAdd);
            elixirStorIdx.push_back(i);
            elixirStorMaxLoot.push_back(cap / 2);
        }
        else if (eb.id == 5) // Gold Mine
        {
            int stored = (int)std::floor(std::max(0.0f, eb.building ? eb.building->stored : 0.0f) + 1e-6f);
            extraGoldFromMines += stored;
            goldMineIdx.push_back(i);
            goldMineMaxLoot.push_back(stored / 2);
        }
        else if (eb.id == 3) // Elixir Collector
        {
            int stored = (int)std::floor(std::max(0.0f, eb.building ? eb.building->stored : 0.0f) + 1e-6f);
            extraElixirFromCollectors += stored;
            elixirCollectorIdx.push_back(i);
            elixirCollectorMaxLoot.push_back(stored / 2);
        }
        else if (eb.id == 9) // TownHall
        {
            townHallIdx = i;
        }
    }


    // Total loot is 50% of the enemy's currently available resources.
    // This includes uncollected resources stored inside mines/collectors.
    targetGold = std::max(0, (goldBase + extraGoldFromMines) / 2);
    targetElixir = std::max(0, (elixirBase + extraElixirFromCollectors) / 2);

    _lootGoldTotal = targetGold;
    _lootElixirTotal = targetElixir;
    auto distributeToStorages = [](int total, const std::vector<int>& maxPer, std::vector<int>& out)
    {
        int n = (int)maxPer.size();
        out.assign(n, 0);
        if (n == 0 || total <= 0) return;

        long long sumMax = 0;
        for (int v : maxPer) sumMax += std::max(0, v);
        if (sumMax <= 0) return;

        int totalForStor = (int)std::min<long long>(sumMax, total);
        std::vector<double> frac(n, 0.0);

        int used = 0;
        for (int i = 0; i < n; ++i)
        {
            double exact = (double)totalForStor * (double)std::max(0, maxPer[i]) / (double)sumMax;
            int base = (int)std::floor(exact);
            base = std::min(base, std::max(0, maxPer[i]));
            out[i] = base;
            used += base;
            frac[i] = exact - (double)base;
        }

        int remain = totalForStor - used;
        // Distribute the small remainder based on fractional parts.
        while (remain > 0)
        {
            int best = -1;
            double bestFrac = -1.0;
            for (int i = 0; i < n; ++i)
            {
                if (out[i] >= std::max(0, maxPer[i])) continue;
                if (frac[i] > bestFrac)
                {
                    bestFrac = frac[i];
                    best = i;
                }
            }
            if (best < 0) break;
            out[best] += 1;
            frac[best] = 0.0;
            remain -= 1;
        }
    };

    // Assign loot to Gold Storages, remainder goes to TownHall.
    // Assign loot to Gold Mines first (proportional to their stored resources),
    // then Gold Storages. Any remainder goes to the TownHall.
    int remainGold = targetGold;
    {
        std::vector<int> alloc;
        distributeToStorages(remainGold, goldMineMaxLoot, alloc);
        int used = 0;
        for (int k = 0; k < (int)alloc.size(); ++k)
        {
            used += alloc[k];
            _enemyBuildings[goldMineIdx[k]].lootGoldMax = alloc[k];
        }
        remainGold = std::max(0, remainGold - used);
    }
    {
        std::vector<int> alloc;
        distributeToStorages(remainGold, goldStorMaxLoot, alloc);
        int used = 0;
        for (int k = 0; k < (int)alloc.size(); ++k)
        {
            used += alloc[k];
            _enemyBuildings[goldStorIdx[k]].lootGoldMax = alloc[k];
        }
        remainGold = std::max(0, remainGold - used);
    }
    if (townHallIdx >= 0) _enemyBuildings[townHallIdx].lootGoldMax += remainGold;

    // Assign loot to Elixir Collectors first (proportional to their stored resources),
    // then Elixir Storages. Any remainder goes to the TownHall.
    int remainElixir = targetElixir;
    {
        std::vector<int> alloc;
        distributeToStorages(remainElixir, elixirCollectorMaxLoot, alloc);
        int used = 0;
        for (int k = 0; k < (int)alloc.size(); ++k)
        {
            used += alloc[k];
            _enemyBuildings[elixirCollectorIdx[k]].lootElixirMax = alloc[k];
        }
        remainElixir = std::max(0, remainElixir - used);
    }
    {
        std::vector<int> alloc;
        distributeToStorages(remainElixir, elixirStorMaxLoot, alloc);
        int used = 0;
        for (int k = 0; k < (int)alloc.size(); ++k)
        {
            used += alloc[k];
            _enemyBuildings[elixirStorIdx[k]].lootElixirMax = alloc[k];
        }
        remainElixir = std::max(0, remainElixir - used);
    }
    if (townHallIdx >= 0) _enemyBuildings[townHallIdx].lootElixirMax += remainElixir;

    if (_lootHud) updateLootHUD();
    // Build blocked map and overlay after we have _tileW/_tileH/_anchor.
    rebuildDeployBlockedMap();
    rebuildDeployOverlay();
    updateDeployOverlayVisibility();
}
}

void BattleScene::loadBattleSaves()
{
    _savesLoaded = false;
    _savesDirty = false;

    _attackerSlot = SaveSystem::getCurrentSlot();
    _defenderSlot = SaveSystem::getBattleTargetSlot();

    if (_attackerSlot < 0 || !SaveSystem::exists(_attackerSlot)) return;
    if (_defenderSlot < 0 || !SaveSystem::exists(_defenderSlot)) return;

    if (!SaveSystem::load(_attackerSlot, _attackerData)) return;
    if (!SaveSystem::load(_defenderSlot, _defenderData)) return;

    _savesLoaded = true;
}

void BattleScene::persistBattleSaves()
{
    if (!_savesLoaded || !_savesDirty) return;

    // Persist resource changes to both villages.
    SaveSystem::save(_attackerData);
    SaveSystem::save(_defenderData);
    _savesDirty = false;
}

int BattleScene::calcGoldCapFromSave(const SaveData& data) const
{
    // NOTE: ResourceManager base cap defaults to 5000 in Resources().
    // Storages add their capAdd on top of the base.
    int cap = 5000;
    for (const auto& b : data.buildings)
    {
        if (b.id != 6) continue; // Gold Storage
        int lv = std::max(1, std::min(5, b.level));
        cap += std::max(0, ConfigManager::getGoldStorageStats(lv).capAdd);
    }
    return std::max(0, cap);
}

int BattleScene::calcElixirCapFromSave(const SaveData& data) const
{
    int cap = 5000;
    for (const auto& b : data.buildings)
    {
        if (b.id != 4) continue; // Elixir Storage
        int lv = std::max(1, std::min(5, b.level));
        cap += std::max(0, ConfigManager::getElixirStorageStats(lv).capAdd);
    }
    return std::max(0, cap);
}

void BattleScene::settleBattleLoot()
{
    if (_lootSettled) return;
    if (!_savesLoaded) return;

    _lootSettled = true;

    // Clamp attacker gain by attacker's storage caps.
    int goldCap = calcGoldCapFromSave(_attackerData);
    int elixirCap = calcElixirCapFromSave(_attackerData);

    int addGold = std::max(0, _lootedGold);
    int addElixir = std::max(0, _lootedElixir);

    int goldRoom = std::max(0, goldCap - std::max(0, _attackerData.gold));
    int elixirRoom = std::max(0, elixirCap - std::max(0, _attackerData.elixir));

    int g = std::min(addGold, goldRoom);
    int e = std::min(addElixir, elixirRoom);

    if (g > 0) _attackerData.gold = std::max(0, _attackerData.gold) + g;
    if (e > 0) _attackerData.elixir = std::max(0, _attackerData.elixir) + e;

    // Defender loses (directly deduct, as requested).
    if (addGold > 0) _defenderData.gold = std::max(0, _defenderData.gold - addGold);
    if (addElixir > 0) _defenderData.elixir = std::max(0, _defenderData.elixir - addElixir);

    _savesDirty = true;
}

bool BattleScene::isTownHallDestroyed() const
{
    for (const auto& b : _enemyBuildings)
    {
        if (b.id != 9) continue;
        if (!b.building) continue;
        if (b.building->hp <= 0) return true;
    }
    return false;
}

bool BattleScene::areAllNonWallBuildingsDestroyed() const
{
    for (const auto& b : _enemyBuildings)
    {
        if (!b.building) continue;
        if (b.id == 10) continue; // walls ignored
        if (b.building->hp > 0) return false;
    }
    return true;
}

bool BattleScene::areAllTroopsDeployedAndDead() const
{
    // All ready troops deployed?
    for (const auto& kv : _troopCounts)
    {
        if (kv.second > 0) return false;
    }

    // Any alive unit?
    for (const auto& u : _units)
    {
        if (!u.unit) continue;
        if (!u.unit->isDead()) return false;
    }

    return true;
}

bool BattleScene::init()
{
    if (!Scene::init()) return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    _world = Node::create();
    this->addChild(_world, 0);

    _background = Sprite::create("backgrounds/village_map.jpg");
    if (_background)
    {
        Size bg = _background->getContentSize();
        float scale = std::max(visibleSize.width / bg.width, visibleSize.height / bg.height);
        _background->setScale(scale);
        _background->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
        _world->addChild(_background, 0);
    }
    else
    {
        auto fallback = LayerColor::create(Color4B(50, 50, 50, 255));
        this->addChild(fallback, -1);
    }

    // Enable zoom (mouse wheel) and pan (mouse drag) for viewing the enemy village.
    _maxZoom = 2.5f;
    _minZoom = 0.1f;
    if (_background)
    {
        // Match MainScene logic: prevent zooming out so far that black borders appear.
        Size img = _background->getContentSize();
        float base = _background->getScaleX();
        float minZ = std::max(visibleSize.width / (img.width * base), visibleSize.height / (img.height * base));
        _minZoom = std::max(minZ, 0.1f);
    }
    setZoom(1.0f);

    auto mouse = EventListenerMouse::create();
    mouse->onMouseScroll = [this](Event* e) {
        auto m = static_cast<EventMouse*>(e);
        float scroll = m->getScrollY();
        if (scroll > 0) setZoom(_zoom * 1.10f);
        else if (scroll < 0) setZoom(_zoom / 1.10f);
    };

    // Left click:
    //  - click troop icon in bottom bar -> toggle "selected"
    //  - click on map (without dragging) -> deploy 1 selected troop
    //  - drag on map -> pan camera
    mouse->onMouseDown = [this](Event* e) {
        auto m = static_cast<EventMouse*>(e);
        if (m->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) return;

        Vec2 glPos = Director::getInstance()->convertToGL(Vec2(m->getCursorX(), m->getCursorY()));

        _mouseDown = true;
        _mouseConsumed = false;
        _mouseMoved = false;
        _mouseDownPos = glPos;

        _dragging = false;
        _dragLast = Vec2(m->getCursorX(), m->getCursorY());

        // If user clicked on troop bar, consume the click (no panning / no deployment).
        if (handleTroopBarClick(glPos)) {
            _mouseConsumed = true;
        }
    };

    mouse->onMouseUp = [this](Event* e) {
        auto m = static_cast<EventMouse*>(e);
        if (m->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) return;

        Vec2 glPos = Director::getInstance()->convertToGL(Vec2(m->getCursorX(), m->getCursorY()));

        if (!_mouseDown) return;

        // Stop dragging if we were panning.
        if (_dragging) {
            _dragging = false;
        } else {
            // Treat as a click on the map if it wasn't consumed by UI and wasn't a drag.
            if (!_mouseConsumed && !_mouseMoved) {
                deploySelectedTroop(glPos);
            }
        }

        _mouseDown = false;
        _mouseConsumed = false;
        _mouseMoved = false;
    };

    mouse->onMouseMove = [this](Event* e) {
        if (!_mouseDown || _mouseConsumed) return;

        auto m = static_cast<EventMouse*>(e);

        Vec2 curScreen(m->getCursorX(), m->getCursorY());
        Vec2 glCur = Director::getInstance()->convertToGL(curScreen);

        // Start dragging only if the mouse moved enough.
        const float kDragThreshold = 6.0f;
        if (!_dragging) {
            if ((glCur - _mouseDownPos).length() < kDragThreshold) return;
            _dragging = true;
        }

        _mouseMoved = true;

        Vec2 d = curScreen - _dragLast;
        _dragLast = curScreen;

        if (_world) {
            _world->setPosition(_world->getPosition() + d);
            clampWorld();
        }
    };

    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouse, this);

    // Touch support (some devices don't generate mouse events reliably).
    auto touch = EventListenerTouchOneByOne::create();
    // We handle both UI and map interactions inside this listener. Swallow touches to prevent
    // accidental propagation that may cause duplicate deployments.
    touch->setSwallowTouches(true);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        Vec2 glPos = t->getLocation();
        _touchDown = true;
        _touchConsumed = false;
        _touchMoved = false;
        _touchDownPos = glPos;
        // First: troop bar selection.
        if (handleTroopBarClick(glPos)) {
            _touchConsumed = true;
            return true;
        }
        return true;
    };
    touch->onTouchMoved = [this](Touch* t, Event*) {
        if (!_touchDown || _touchConsumed) return;
        Vec2 glCur = t->getLocation();
        const float kDragThreshold = 8.0f;
        if ((glCur - _touchDownPos).length() < kDragThreshold) return;
        _touchMoved = true;

        // Use view/screen coordinates (same space as EventMouse cursorX/Y) for panning.
        Vec2 curScreen = t->getLocationInView();
        if (!_dragging) {
            _dragging = true;
            _dragLast = curScreen;
            return;
        }
        Vec2 d = curScreen - _dragLast;
        _dragLast = curScreen;
        if (_world) {
            _world->setPosition(_world->getPosition() + d);
            clampWorld();
        }
    };
    touch->onTouchEnded = [this](Touch* t, Event*) {
        Vec2 glPos = t->getLocation();
        if (!_touchDown) return;
        if (_dragging) {
            _dragging = false;
        } else {
            if (!_touchConsumed && !_touchMoved) {
                deploySelectedTroop(glPos);
            }
        }
        _touchDown = false;
        _touchConsumed = false;
        _touchMoved = false;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);


    setBuildingVisualParams();
    renderTargetVillage();

    _battleEnded = false;
    _units.clear();

    // Battle HUD & countdown
    setupBattleHUD();
    setupTroopBar();
        _troopCounts = SaveSystem::getBattleReadyTroops();
        _troopLevels = SaveSystem::getBattleTroopLevels();
        for (int id = 1; id <= 4; ++id) {
            if (_troopLevels.find(id) == _troopLevels.end()) _troopLevels[id] = 1;
        }
    refreshTroopBar();
    startPhase(Phase::Scout, 45.0f);
    scheduleUpdate();

    // ESC behavior (requested): do NOT open settings.
    // Only show an "abandon?" confirmation popup and pause the battle timer while it is visible.
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode code, Event*) {
        if (code != EventKeyboard::KeyCode::KEY_ESCAPE) return;
        if (_battleEnded || _phase == Phase::End) return;

        // If a result popup is showing, ignore ESC.
        if (_resultMask) return;

        // Toggle abandon popup.
        if (_abandonMask) {
            // Do not resume by ESC; user must pick a button.
            return;
        }
        openAbandonConfirmPopup();
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    return true;
}

void BattleScene::setupBattleHUD()
{
    if (_hud) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    _hud = Node::create();
    this->addChild(_hud, 50);

    _phaseLabel = Label::createWithSystemFont("Scout Phase", "Arial", 26);
    _phaseLabel->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height - 26));
    _hud->addChild(_phaseLabel);

    _timeLabel = Label::createWithSystemFont("45", "Arial", 26);
    _timeLabel->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height - 52));
    _hud->addChild(_timeLabel);

    float barW = 520.0f;
    float barH = 14.0f;
    Vec2 barPos = origin + Vec2(visibleSize.width * 0.5f - barW * 0.5f, visibleSize.height - 80);

    _barBg = LayerColor::create(Color4B(40, 40, 40, 200), barW, barH);
    _barBg->setAnchorPoint(Vec2(0.0f, 0.5f));
    _barBg->setPosition(barPos + Vec2(0, barH * 0.5f));
    _hud->addChild(_barBg);

    _barFill = LayerColor::create(Color4B(70, 200, 70, 220), barW, barH);
    _barFill->setAnchorPoint(Vec2(0.0f, 0.5f));
    _barFill->setPosition(barPos + Vec2(0, barH * 0.5f));
    _barFill->setScaleX(1.0f);
    _hud->addChild(_barFill);

    setupLootHUD();

    auto returnLabel = Label::createWithSystemFont("Return", "Arial", 44);
    auto returnItem = MenuItemLabel::create(returnLabel, [this](Ref*) {
        // Return after battle: ensure settlement is applied, then persist.
        settleBattleLoot();
        persistBattleSaves();
        SaveSystem::setBattleTargetSlot(-1);
        Director::getInstance()->replaceScene(TransitionFade::create(0.35f, MainScene::createScene()));
    });
    returnItem->setPosition(origin + visibleSize / 2);
    _returnMenu = Menu::create(returnItem, nullptr);
    _returnMenu->setPosition(Vec2::ZERO);
    _returnMenu->setVisible(false);
    this->addChild(_returnMenu, 100);
}

void BattleScene::startPhase(Phase p, float durationSec)
{
    _phase = p;

    // Phase music mapping (Resources/music).
    if (p == Phase::Scout) {
        SoundManager::play("music/capital_pre_battle_music.ogg", true, 0.6f);
    }
    else if (p == Phase::Battle) {
        SoundManager::play("music/capital_battle_music.ogg", true, 0.6f);
    }
    else if (p == Phase::End) {
        SoundManager::playSfx("music/capital_battle_end.ogg", 1.0f);
    }
    _phaseTotal = std::max(0.001f, durationSec);
    _phaseRemaining = durationSec;
    updateBattleHUD();
}

void BattleScene::updateBattleHUD()
{
    if (!_phaseLabel || !_timeLabel || !_barFill) return;

    switch (_phase)
    {
    case Phase::Scout:
        _phaseLabel->setString("Scout Phase (observe) - 45s");
        break;
    case Phase::Battle:
        _phaseLabel->setString("Battle Phase - 180s");
        break;
    case Phase::End:
        _phaseLabel->setString("Battle End");
        break;
    }

    int sec = (int)std::ceil(std::max(0.0f, _phaseRemaining));
    _timeLabel->setString(StringUtils::format("%d", sec));

    float percent = _phaseRemaining / _phaseTotal;
    percent = std::max(0.0f, std::min(1.0f, percent));
    _barFill->setScaleX(percent);

    updateLootHUD();
}

void BattleScene::showReturnButton()
{
    if (_returnMenu) _returnMenu->setVisible(true);
}

void BattleScene::checkBattleResult(bool timeUp)
{
    if (_battleEnded) return;

    // End conditions (as required):
    // 1) Time up.
    // 2) All buildings destroyed (excluding walls).
    // 3) All troops deployed and all are dead.
    // NOTE: Destroying the TownHall does NOT end the battle immediately.
    bool townHallDestroyed = isTownHallDestroyed();
    bool allNonWallDestroyed = areAllNonWallBuildingsDestroyed();
    bool allTroopsGone = areAllTroopsDeployedAndDead();

    // Avoid ending immediately if player hasn't deployed anything yet.
    if (!_hasDeployedAnyTroop) {
        allTroopsGone = false;
    }

    bool shouldEnd = timeUp || allNonWallDestroyed || allTroopsGone;
    if (!shouldEnd) return;

    // Result logic: TownHall destroyed -> victory, otherwise defeated.
    bool win = townHallDestroyed;

    endBattleAndShowResult(win);
}

void BattleScene::update(float dt)
{
    if (_phase == Phase::End) return;

    // Pause battle logic and countdown when a modal popup is open.
    if (_pausedByPopup) return;

    _phaseRemaining -= dt;

    if (_phaseRemaining <= 0.0f)
    {
        if (_phase == Phase::Scout)
        {
            startPhase(Phase::Battle, 180.0f);
            return;
        }
        if (_phase == Phase::Battle)
        {
            // Time up -> determine win/lose.
            checkBattleResult(true);
            return;
        }
    }

    if (_phase == Phase::Battle && !_battleEnded)
    {
        _ai.update(dt, _units, _enemyBuildings);

        // Collect loot from newly destroyed buildings (one-time).
        
// Loot progression:
// - Gold/Elixir storages: loot is proportional to HP lost (up to 50% from that storage).
// - TownHall: loot can only be obtained when the TownHall is completely destroyed.
// NOTE: We DO NOT apply loot to village saves during the battle.
// The real resource changes happen only once, at battle settlement.
for (auto& eb : _enemyBuildings)
{
    if (!eb.building) continue;

    int hpNow = eb.building->hp;
    int hpMax = std::max(1, eb.building->hpMax);

    if (eb.id == 9) // TownHall
    {
        if (hpNow <= 0 && !eb.lootCollected)
        {
            int addGold = std::max(0, eb.lootGoldMax - eb.lootGoldTaken);
            int addElixir = std::max(0, eb.lootElixirMax - eb.lootElixirTaken);

            int remainGold = std::max(0, _lootGoldTotal - _lootedGold);
            int remainElixir = std::max(0, _lootElixirTotal - _lootedElixir);

            int g = std::min(addGold, remainGold);
            int e = std::min(addElixir, remainElixir);

            _lootedGold += g;
            _lootedElixir += e;

            eb.lootGoldTaken += g;
            eb.lootElixirTaken += e;
            eb.lootCollected = true;

        }
        continue;
    }

    if (eb.id == 4 || eb.id == 6 || eb.id == 3 || eb.id == 5) // Elixir Storage / Gold Storage / Collector / Mine
    {
        float destroyed = (float)(hpMax - hpNow) / (float)hpMax;
        destroyed = std::max(0.0f, std::min(1.0f, destroyed));

        int shouldGold = (int)std::floor((float)eb.lootGoldMax * destroyed + 1e-6f);
        int shouldElixir = (int)std::floor((float)eb.lootElixirMax * destroyed + 1e-6f);

        int addGold = std::max(0, shouldGold - eb.lootGoldTaken);
        int addElixir = std::max(0, shouldElixir - eb.lootElixirTaken);

        int gDelta = 0;
        int eDelta = 0;

        if (addGold > 0)
        {
            int remain = std::max(0, _lootGoldTotal - _lootedGold);
            gDelta = std::min(addGold, remain);
            _lootedGold += gDelta;
            eb.lootGoldTaken += gDelta;
        }

        if (addElixir > 0)
        {
            int remain = std::max(0, _lootElixirTotal - _lootedElixir);
            eDelta = std::min(addElixir, remain);
            _lootedElixir += eDelta;
            eb.lootElixirTaken += eDelta;
        }
    }

    eb.lastHp = hpNow;
}

        

        checkBattleResult(false);
    }

    updateBattleHUD();
}

void BattleScene::endBattleAndShowResult(bool win)
{
    if (_battleEnded) return;

    _battleEnded = true;
    _phase = Phase::End;
    _phaseRemaining = 0.0f;

    // Stop the battle loop.
    this->unscheduleUpdate();
    updateBattleHUD();

    // Close any modal popups.
    if (_abandonMask) {
        _abandonMask->removeFromParent();
        _abandonMask = nullptr;
    }
    _pausedByPopup = false;

    // Apply loot to both villages and persist once at battle settlement.
    settleBattleLoot();
    persistBattleSaves();

    // SFX: battle result.
    if (win) SoundManager::playSfxRandom("battle_win", 1.0f);
    else     SoundManager::playSfxRandom("battle_lost", 1.0f);

    showBattleResultPopup(win);
}

void BattleScene::showBattleResultPopup(bool win)
{
    if (_resultMask) return;

    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    _resultMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_resultMask, 500);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [this](Touch*, Event*) { return _resultMask && _resultMask->isVisible(); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _resultMask);

    const float panelW = 720.0f;
    const float panelH = 520.0f;
    auto panel = LayerColor::create(Color4B(255, 255, 255, 255), panelW, panelH);
    panel->setPosition(origin.x + vs.width * 0.5f - panelW * 0.5f,
        origin.y + vs.height * 0.5f - panelH * 0.5f);
    _resultMask->addChild(panel);

    auto panelListener = EventListenerTouchOneByOne::create();
    panelListener->setSwallowTouches(true);
    panelListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(panelListener, panel);

    // Title
    auto title = Label::createWithSystemFont(win ? "victory" : "defeated", "Arial", 78);
    title->setColor(Color3B::BLACK);
    title->setPosition(Vec2(panelW * 0.5f, panelH - 80.0f));
    panel->addChild(title);

    // Loot summary (raw looted values).
    auto goldLbl = Label::createWithSystemFont(StringUtils::format("Gold: %d", std::max(0, _lootedGold)), "Arial", 36);
    goldLbl->setColor(Color3B::BLACK);
    goldLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
    goldLbl->setPosition(Vec2(panelW * 0.5f, panelH - 165.0f));
    panel->addChild(goldLbl);

    auto elixirLbl = Label::createWithSystemFont(StringUtils::format("Elixir: %d", std::max(0, _lootedElixir)), "Arial", 36);
    elixirLbl->setColor(Color3B::BLACK);
    elixirLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
    elixirLbl->setPosition(Vec2(panelW * 0.5f, panelH - 215.0f));
    panel->addChild(elixirLbl);

    // Troops deployed = troop losses (as requested).
    std::string lines;
    auto troopName = [](int type) -> std::string {
        switch (type) {
        case 1: return "barbarian";
        case 2: return "archer";
        case 3: return "giant";
        case 4: return "wall_breaker";
        default: return "troop";
        }
    };

    for (int type = 1; type <= 4; ++type)
    {
        int cnt = 0;
        auto it = _deployedCounts.find(type);
        if (it != _deployedCounts.end()) cnt = it->second;
        if (cnt <= 0) continue;
        if (!lines.empty()) lines += "\n";
        lines += troopName(type);
        lines += "  ";
        // Use a unicode escape to avoid source-encoding issues on Windows (MSVC).
        // "\u00D7" = multiplication sign.
        lines += u8"\u00D7";
        lines += StringUtils::format("%d", cnt);
    }
    if (lines.empty()) lines = "none";

    auto lossTitle = Label::createWithSystemFont("Troops Lost:", "Arial", 32);
    lossTitle->setColor(Color3B::BLACK);
    lossTitle->setAnchorPoint(Vec2(0.5f, 0.5f));
    lossTitle->setPosition(Vec2(panelW * 0.5f, panelH - 285.0f));
    panel->addChild(lossTitle);

    auto lossLbl = Label::createWithSystemFont(lines, "Arial", 28);
    lossLbl->setColor(Color3B::BLACK);
    lossLbl->setAnchorPoint(Vec2(0.5f, 1.0f));
    lossLbl->setPosition(Vec2(panelW * 0.5f, panelH - 320.0f));
    panel->addChild(lossLbl);

    // Return button
    auto retLabel = Label::createWithSystemFont("return", "Arial", 46);
    retLabel->setColor(Color3B::BLACK);
    auto retItem = MenuItemLabel::create(retLabel, [this](Ref*) {
        // Ensure settlement is applied, then persist.
        settleBattleLoot();
        persistBattleSaves();
        SaveSystem::setBattleTargetSlot(-1);
        Director::getInstance()->replaceScene(TransitionFade::create(0.35f, MainScene::createScene()));
    });
    retItem->setPosition(Vec2(panelW * 0.5f, 70.0f));
    auto menu = Menu::create(retItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    panel->addChild(menu, 2);
}

void BattleScene::openAbandonConfirmPopup()
{
    if (_abandonMask || _battleEnded || _phase == Phase::End) return;

    _pausedByPopup = true;

    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    _abandonMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_abandonMask, 450);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [this](Touch*, Event*) { return _abandonMask && _abandonMask->isVisible(); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _abandonMask);

    const float panelW = 640.0f;
    const float panelH = 320.0f;
    auto panel = LayerColor::create(Color4B(255, 255, 255, 255), panelW, panelH);
    panel->setPosition(origin.x + vs.width * 0.5f - panelW * 0.5f,
        origin.y + vs.height * 0.5f - panelH * 0.5f);
    _abandonMask->addChild(panel);

    auto panelListener = EventListenerTouchOneByOne::create();
    panelListener->setSwallowTouches(true);
    panelListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(panelListener, panel);

    auto title = Label::createWithSystemFont("Abandon battle?", "Arial", 50);
    title->setColor(Color3B::BLACK);
    title->setPosition(Vec2(panelW * 0.5f, panelH - 90.0f));
    panel->addChild(title);

    auto yesLbl = Label::createWithSystemFont("Yes", "Arial", 44);
    yesLbl->setColor(Color3B::BLACK);
    auto yesItem = MenuItemLabel::create(yesLbl, [this](Ref*) {
        closeAbandonConfirmPopup(false);
        // Abandon is treated as defeated.
        endBattleAndShowResult(false);
    });

    auto noLbl = Label::createWithSystemFont("No", "Arial", 44);
    noLbl->setColor(Color3B::BLACK);
    auto noItem = MenuItemLabel::create(noLbl, [this](Ref*) {
        closeAbandonConfirmPopup(true);
    });

    yesItem->setPosition(Vec2(panelW * 0.35f, 85.0f));
    noItem->setPosition(Vec2(panelW * 0.65f, 85.0f));

    auto menu = Menu::create(yesItem, noItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    panel->addChild(menu, 2);
}

void BattleScene::closeAbandonConfirmPopup(bool resumeBattle)
{
    if (_abandonMask) {
        _abandonMask->removeFromParent();
        _abandonMask = nullptr;
    }

    if (resumeBattle) {
        _pausedByPopup = false;
    }
}

void BattleScene::openEscMenu()
{
    if (_escMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    _escMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_escMask, 200);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [this](cocos2d::Touch*, cocos2d::Event*) { return _escMask && _escMask->isVisible(); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _escMask);

    const float panelW = 560.0f;
    const float panelH = 420.0f;
    auto panel = LayerColor::create(Color4B(70, 70, 70, 255), panelW, panelH);
    panel->setPosition(origin.x + visibleSize.width / 2 - panelW / 2,
        origin.y + visibleSize.height / 2 - panelH / 2);
    _escMask->addChild(panel);

    auto panelListener = EventListenerTouchOneByOne::create();
    panelListener->setSwallowTouches(true);
    panelListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(panelListener, panel);

    panel->setScale(0.1f);
    panel->runAction(EaseBackOut::create(ScaleTo::create(0.22f, 1.0f)));

    auto title = Label::createWithSystemFont("Menu", "Arial", 52);
    title->setPosition(Vec2(panelW / 2, panelH - 60));
    panel->addChild(title);

    auto settingsLabel = Label::createWithSystemFont("Settings", "Arial", 48);
    auto settingsItem = MenuItemLabel::create(settingsLabel, [this](Ref*) {
        this->openSettings();
    });

    auto backLabel = Label::createWithSystemFont("Back to Start", "Arial", 42);
    auto backItem = MenuItemLabel::create(backLabel, [this](Ref*) {
        // Leaving early: treat as settlement with current looted values.
        settleBattleLoot();
        persistBattleSaves();
        closeEscMenu();
        Director::getInstance()->replaceScene(
            TransitionFade::create(0.35f, LoginScene::createScene())
        );
    });

    auto exitLabel = Label::createWithSystemFont("Exit Game", "Arial", 42);
    auto exitItem = MenuItemLabel::create(exitLabel, [](Ref*) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
        ExitProcess(0);
#else
        Director::getInstance()->end();
#endif
    });

    settingsItem->setPosition(Vec2(panelW / 2, panelH / 2 + 70));
    backItem->setPosition(Vec2(panelW / 2, panelH / 2));
    exitItem->setPosition(Vec2(panelW / 2, panelH / 2 - 70));

    auto menu = Menu::create(settingsItem, backItem, exitItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    panel->addChild(menu, 1);

    auto closeLabel = Label::createWithSystemFont("X", "Arial", 42);
    auto closeItem = MenuItemLabel::create(closeLabel, [this](Ref*) {
        closeEscMenu();
    });
    closeItem->setPosition(Vec2(panelW - 40, panelH - 40));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu);
}

void BattleScene::closeEscMenu()
{
    if (_escMask) {
        _escMask->removeFromParent();
        _escMask = nullptr;
    }
}

void BattleScene::openSettings()
{
    if (_settingsMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    _settingsMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_settingsMask, 300);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [this](cocos2d::Touch*, cocos2d::Event*) { return _settingsMask && _settingsMask->isVisible(); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _settingsMask);

    const float panelW = 640.0f;
    const float panelH = 420.0f;
    auto panel = LayerColor::create(Color4B(70, 70, 70, 255), panelW, panelH);
    panel->setPosition(origin.x + visibleSize.width / 2 - panelW / 2,
        origin.y + visibleSize.height / 2 - panelH / 2);
    _settingsMask->addChild(panel);

    auto panelListener = EventListenerTouchOneByOne::create();
    panelListener->setSwallowTouches(true);
    panelListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(panelListener, panel);

    panel->setScale(0.1f);
    panel->runAction(EaseBackOut::create(ScaleTo::create(0.22f, 1.0f)));

    auto title = Label::createWithSystemFont("Settings", "Arial", 50);
    title->setPosition(Vec2(panelW / 2, panelH - 50));
    panel->addChild(title);

    auto volLabel = Label::createWithSystemFont("Master Volume", "Arial", 32);
    volLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    volLabel->setPosition(Vec2(60, 260));
    panel->addChild(volLabel);

    float curVol = SoundManager::getVolume();

    auto volSlider = ui::Slider::create();
    volSlider->loadBarTexture("ui/sliderTrack.png");
    volSlider->loadSlidBallTextures("ui/sliderThumb.png", "ui/sliderThumb.png", "");
    volSlider->loadProgressBarTexture("ui/sliderProgress.png");
    volSlider->setPosition(Vec2(420, 260));
    volSlider->setPercent((int)(curVol * 100.0f + 0.5f));
    panel->addChild(volSlider);

    auto mute = ui::CheckBox::create("ui/checkbox_off.png", "ui/checkbox_on.png");
    mute->setPosition(Vec2(110, 160));
    mute->setSelected(curVol <= 0.001f);
    panel->addChild(mute);

    auto muteLabel = Label::createWithSystemFont("Mute", "Arial", 32);
    muteLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    muteLabel->setPosition(Vec2(150, 160));
    panel->addChild(muteLabel);

    volSlider->addEventListener([=](Ref* sender, ui::Slider::EventType type) {
        if (type == ui::Slider::EventType::ON_PERCENTAGE_CHANGED)
        {
            auto s = static_cast<ui::Slider*>(sender);
            float v = s->getPercent() / 100.0f;
            SoundManager::setVolume(v);
            mute->setSelected(v <= 0.001f);
        }
    });

    mute->addEventListener([=](Ref*, ui::CheckBox::EventType type) {
        bool isMute = (type == ui::CheckBox::EventType::SELECTED);
        if (isMute)
        {
            SoundManager::setVolume(0.0f);
            volSlider->setPercent(0);
        }
        else
        {
            float v = volSlider->getPercent() / 100.0f;
            SoundManager::setVolume(v);
        }
    });

    auto closeLabel = Label::createWithSystemFont("X", "Arial", 40);
    auto closeItem = MenuItemLabel::create(closeLabel, [this](Ref*) {
        if (_settingsMask) {
            _settingsMask->removeFromParent();
            _settingsMask = nullptr;
        }
    });
    closeItem->setPosition(Vec2(panelW - 40, panelH - 40));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu);
}


void BattleScene::setZoom(float z)
{
    z = std::max(_minZoom, std::min(_maxZoom, z));
    _zoom = z;
    if (_world) _world->setScale(_zoom);
    clampWorld();
}

void BattleScene::clampWorld()
{
    if (!_background || !_world) return;

    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    float bgW = _background->getContentSize().width * _background->getScale();
    float bgH = _background->getContentSize().height * _background->getScale();

    float worldW = bgW * _world->getScale();
    float worldH = bgH * _world->getScale();

    float minX = origin.x + vs.width - worldW;
    float maxX = origin.x;
    float minY = origin.y + vs.height - worldH;
    float maxY = origin.y;

    Vec2 p = _world->getPosition();
    if (worldW < vs.width) p.x = origin.x + (vs.width - worldW) * 0.5f;
    else p.x = std::max(minX, std::min(maxX, p.x));

    if (worldH < vs.height) p.y = origin.y + (vs.height - worldH) * 0.5f;
    else p.y = std::max(minY, std::min(maxY, p.y));

    _world->setPosition(p);
}

void BattleScene::setupTroopBar()
{
    if (_troopBar) return;

    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    float barH = 90.0f;
    auto bg = LayerColor::create(Color4B(0, 0, 0, 120), vs.width, barH);
    bg->setAnchorPoint(Vec2(0, 0));
    bg->setPosition(origin);
    this->addChild(bg, 50);
    _troopBar = bg;
}

void BattleScene::refreshTroopBar()
{
    if (!_troopBar) return;

    _troopBar->removeAllChildren();
    _troopSlots.clear();

    if (_troopCounts.empty()) {
        auto tip = Label::createWithSystemFont("No troops", "Arial", 20);
        tip->setAnchorPoint(Vec2(0, 0.5f));
        tip->setColor(Color3B::WHITE);
        tip->enableOutline(Color4B::BLACK, 2);
        tip->setPosition(Vec2(20.0f, 45.0f));
        _troopBar->addChild(tip, 2);
        return;
    }

    float x = 20.0f;
    float y = 12.0f;
    float gap = 10.0f;
    const float kBarIconSide = 70.0f; // clamp icon size (Wall Breaker icon can be huge)

    for (int type = 1; type <= 4; ++type)
    {
        auto it = _troopCounts.find(type);
        if (it == _troopCounts.end()) continue;
        int count = it->second;

        auto iconPath = TrainingCamp::getTroopIcon((TrainingCamp::TroopType)type);
        auto icon = Sprite::create(iconPath);
        if (!icon) continue;

        auto slotRoot = Node::create();
        slotRoot->setAnchorPoint(Vec2(0, 0));
        slotRoot->setPosition(Vec2(x, y));
        _troopBar->addChild(slotRoot, 1);

        icon->setAnchorPoint(Vec2(0, 0));
        icon->setPosition(Vec2::ZERO);
        float iconScale = 1.0f;
        {
            cocos2d::Size cs = icon->getContentSize();
            float denom = std::max(1.0f, std::max(cs.width, cs.height));
            iconScale = std::min(1.0f, kBarIconSide / denom);
        }
        icon->setScale(iconScale);
        slotRoot->addChild(icon, 1);

        Size isz = icon->getContentSize() * iconScale;
        slotRoot->setContentSize(isz);

        // Count label at the top-right corner (inside the icon).
        auto cntLbl = Label::createWithSystemFont("x" + std::to_string(count), "Arial", 18);
        cntLbl->setAnchorPoint(Vec2(1.0f, 1.0f));
        cntLbl->setColor(Color3B::WHITE);
        cntLbl->enableOutline(Color4B::BLACK, 2);
        cntLbl->setPosition(Vec2(isz.width - 2.0f, isz.height - 2.0f));
        slotRoot->addChild(cntLbl, 2);

        
// Troop level label (center).
{
    int lv = 1;
    auto itLv = _troopLevels.find(type);
    if (itLv != _troopLevels.end()) lv = itLv->second;

    auto lvLbl = Label::createWithSystemFont(StringUtils::format("LV%d", lv), "Arial", 28);
    lvLbl->setColor(Color3B::BLACK);
    // Use outline to make the text thicker while keeping a black fill.
    lvLbl->enableOutline(Color4B::BLACK, 3);
    lvLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
    lvLbl->setPosition(Vec2(isz.width * 0.5f, isz.height * 0.5f));

    // Keep the label inside the icon.
    float maxW = isz.width * 0.95f;
    float maxH = isz.height * 0.60f;
    float bw = lvLbl->getContentSize().width;
    float bh = lvLbl->getContentSize().height;
    float s = 1.0f;
    if (bw > maxW) s = std::min(s, maxW / std::max(1.0f, bw));
    if (bh > maxH) s = std::min(s, maxH / std::max(1.0f, bh));
    lvLbl->setScale(s);

    slotRoot->addChild(lvLbl, 2);
}
        // "selected" indicator. Visible only when this troop is selected.
        auto selLbl = Label::createWithSystemFont("selected", "Arial", 16);
        selLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
        selLbl->setPosition(Vec2(isz.width * 0.5f, isz.height * 0.5f));
        selLbl->setColor(Color3B(255, 255, 120));
        selLbl->enableOutline(Color4B::BLACK, 2);
        selLbl->setVisible(false);
        slotRoot->addChild(selLbl, 3);

        TroopSlot slot;
        slot.type = type;
        slot.root = slotRoot;
        slot.countLabel = cntLbl;
        slot.selectedLabel = selLbl;
        _troopSlots.push_back(slot);

        x += isz.width + gap;
    }

    // Re-apply selected state after rebuilding UI.
    setSelectedTroop(_selectedTroopType);
}


bool BattleScene::handleTroopBarClick(const cocos2d::Vec2& glPos)
{
    if (!_troopBar) return false;

    auto rectToWorld = [this](const cocos2d::Rect& localRect) {
        cocos2d::Vec2 a = _troopBar->convertToWorldSpace(localRect.origin);
        cocos2d::Vec2 b = _troopBar->convertToWorldSpace(localRect.origin + cocos2d::Vec2(localRect.size.width, localRect.size.height));
        float minX = std::min(a.x, b.x);
        float minY = std::min(a.y, b.y);
        float maxX = std::max(a.x, b.x);
        float maxY = std::max(a.y, b.y);
        return cocos2d::Rect(minX, minY, maxX - minX, maxY - minY);
    };

    // IMPORTANT CHANGE:
    // The whole troop-bar row area must block map interactions (no troop deployment on this row).
    // Only the troop icons are clickable; clicking empty space in the row does nothing but is consumed.

    // _troopBar->getBoundingBox() is in its PARENT space (HUD), so convert it to world.
    cocos2d::Rect barLocal = _troopBar->getBoundingBox();
    cocos2d::Node* hud = _troopBar->getParent();
    cocos2d::Vec2 barA = hud ? hud->convertToWorldSpace(barLocal.origin) : barLocal.origin;
    cocos2d::Vec2 barB = hud ? hud->convertToWorldSpace(barLocal.origin + cocos2d::Vec2(barLocal.size.width, barLocal.size.height))
                             : (barLocal.origin + cocos2d::Vec2(barLocal.size.width, barLocal.size.height));
    cocos2d::Rect barRect(
        std::min(barA.x, barB.x),
        std::min(barA.y, barB.y),
        std::fabs(barB.x - barA.x),
        std::fabs(barB.y - barA.y)
    );

    // Build a union rect that covers: the bar background + all icon rectangles (icons may extend above).
    float minX = barRect.getMinX();
    float minY = barRect.getMinY();
    float maxX = barRect.getMaxX();
    float maxY = barRect.getMaxY();

    // First: check troop icons (the only clickable elements in this row).
    for (const auto& slot : _troopSlots)
    {
        if (!slot.root) continue;
        cocos2d::Rect localRect = slot.root->getBoundingBox();
        cocos2d::Rect worldRect = rectToWorld(localRect);

        minX = std::min(minX, worldRect.getMinX());
        minY = std::min(minY, worldRect.getMinY());
        maxX = std::max(maxX, worldRect.getMaxX());
        maxY = std::max(maxY, worldRect.getMaxY());

        if (worldRect.containsPoint(glPos))
        {
            SoundManager::playSfxRandom("button_click", 1.0f);
            if (_selectedTroopType == slot.type) setSelectedTroop(-1);
            else setSelectedTroop(slot.type);
            return true; // consumed
        }
    }

    cocos2d::Rect rowRect(minX, minY, maxX - minX, maxY - minY);
    if (rowRect.containsPoint(glPos))
    {
        // Clicked on empty space in the troop row: consume but do nothing.
        return true;
    }

    return false;
}

bool BattleScene::isPosInTroopBar(const cocos2d::Vec2& glPos) const
{
    if (!_troopBar) return false;

    auto rectToWorld = [this](const cocos2d::Rect& localRect) {
        cocos2d::Vec2 a = _troopBar->convertToWorldSpace(localRect.origin);
        cocos2d::Vec2 b = _troopBar->convertToWorldSpace(localRect.origin + cocos2d::Vec2(localRect.size.width, localRect.size.height));
        float minX = std::min(a.x, b.x);
        float minY = std::min(a.y, b.y);
        float maxX = std::max(a.x, b.x);
        float maxY = std::max(a.y, b.y);
        return cocos2d::Rect(minX, minY, maxX - minX, maxY - minY);
    };

    // Build a union rect for the entire troop row area (icons + background).
    cocos2d::Rect barLocal = _troopBar->getBoundingBox();
    cocos2d::Node* hud = _troopBar->getParent();
    cocos2d::Vec2 barA = hud ? hud->convertToWorldSpace(barLocal.origin) : barLocal.origin;
    cocos2d::Vec2 barB = hud ? hud->convertToWorldSpace(barLocal.origin + cocos2d::Vec2(barLocal.size.width, barLocal.size.height))
                             : (barLocal.origin + cocos2d::Vec2(barLocal.size.width, barLocal.size.height));

    float minX = std::min(barA.x, barB.x);
    float minY = std::min(barA.y, barB.y);
    float maxX = std::max(barA.x, barB.x);
    float maxY = std::max(barA.y, barB.y);

    for (const auto& slot : _troopSlots)
    {
        if (!slot.root) continue;
        cocos2d::Rect localRect = slot.root->getBoundingBox();
        cocos2d::Rect worldRect = rectToWorld(localRect);
        minX = std::min(minX, worldRect.getMinX());
        minY = std::min(minY, worldRect.getMinY());
        maxX = std::max(maxX, worldRect.getMaxX());
        maxY = std::max(maxY, worldRect.getMaxY());
        if (worldRect.containsPoint(glPos)) return true;
    }

    cocos2d::Rect rowRect(minX, minY, maxX - minX, maxY - minY);
    return rowRect.containsPoint(glPos);
}

static float cross2(const cocos2d::Vec2& a, const cocos2d::Vec2& b)
{
    return a.x * b.y - a.y * b.x;
}

static bool pointInTriangle(const cocos2d::Vec2& p, const cocos2d::Vec2& a, const cocos2d::Vec2& b, const cocos2d::Vec2& c)
{
    float c1 = cross2(b - a, p - a);
    float c2 = cross2(c - b, p - b);
    float c3 = cross2(a - c, p - c);
    bool hasNeg = (c1 < 0.0f) || (c2 < 0.0f) || (c3 < 0.0f);
    bool hasPos = (c1 > 0.0f) || (c2 > 0.0f) || (c3 > 0.0f);
    return !(hasNeg && hasPos);
}

bool BattleScene::isPosInDeployArea(const cocos2d::Vec2& worldLocal) const
{
    if (!_deployAreaReady) return true; // if not ready, do not block deployment

    // Diamond as 2 triangles: (top, right, bottom) and (top, bottom, left).
    if (pointInTriangle(worldLocal, _deployTop, _deployRight, _deployBottom)) return true;
    if (pointInTriangle(worldLocal, _deployTop, _deployBottom, _deployLeft)) return true;
    return false;
}

cocos2d::Vec2 BattleScene::worldToGridFloat(const cocos2d::Vec2& worldLocal) const
{
    // Inverse transform of gridToWorld:
    // x = anchor.x + (c - r) * (tileW/2)
    // y = anchor.y - (c + r) * (tileH/2)
    float hw = std::max(0.0001f, _tileW * 0.5f);
    float hh = std::max(0.0001f, _tileH * 0.5f);
    float dx = (worldLocal.x - _anchor.x) / hw;
    float dy = (_anchor.y - worldLocal.y) / hh;
    float c = (dx + dy) * 0.5f;
    float r = (dy - dx) * 0.5f;
    return cocos2d::Vec2(r, c);
}

bool BattleScene::worldToGrid(const cocos2d::Vec2& worldLocal, int& outR, int& outC) const
{
    cocos2d::Vec2 rc = worldToGridFloat(worldLocal);
    int r = (int)std::round(rc.x);
    int c = (int)std::round(rc.y);
    if (r < 0 || r >= _rows || c < 0 || c >= _cols) return false;
    outR = r;
    outC = c;
    return true;
}

bool BattleScene::isGridBlocked(int r, int c) const
{
    if (r < 0 || r >= (int)_deployBlocked.size()) return false;
    if (c < 0 || c >= (int)_deployBlocked[r].size()) return false;
    return _deployBlocked[r][c] != 0;
}

void BattleScene::rebuildDeployBlockedMap()
{
    _deployBlocked.assign(_rows, std::vector<uint8_t>(_cols, 0));

    auto markBlocked = [this](int r, int c) {
        if (r < 0 || r >= _rows || c < 0 || c >= _cols) return;
        _deployBlocked[r][c] = 1;
    };

    for (const auto& eb : _enemyBuildings)
    {
        // Footprint rule matches MainScene:
        //  - Wall (id==10): 1x1
        //  - Other buildings: 3x3 centered at (r,c)
        std::vector<cocos2d::Vec2> footprint;
        if (eb.id == 10) {
            footprint.push_back(cocos2d::Vec2(eb.r, eb.c));
        } else {
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc)
                    footprint.push_back(cocos2d::Vec2(eb.r + dr, eb.c + dc));
        }

        // Expand outward by 1 tile (outer ring) -> cannot deploy.
        for (const auto& cell : footprint) {
            int rr = (int)cell.x;
            int cc = (int)cell.y;
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc)
                    markBlocked(rr + dr, cc + dc);
        }
    }
}

void BattleScene::ensureDeployOverlay()
{
    if (_deployOverlay || !_world) return;
    _deployOverlay = cocos2d::DrawNode::create();
    _deployOverlay->setVisible(false);
    // Draw above background but below buildings.
    _world->addChild(_deployOverlay, 2);
}

void BattleScene::rebuildDeployOverlay()
{
    ensureDeployOverlay();
    if (!_deployOverlay) return;
    _deployOverlay->clear();

    if (_tileW <= 0.0f || _tileH <= 0.0f) return;

    const cocos2d::Color4F kRed(1.0f, 0.0f, 0.0f, 0.35f);

    auto drawCell = [this](int r, int c, const cocos2d::Color4F& color) {
        cocos2d::Vec2 center = gridToWorld(r, c);
        cocos2d::Vec2 top(center.x, center.y + _tileH * 0.5f);
        cocos2d::Vec2 right(center.x + _tileW * 0.5f, center.y);
        cocos2d::Vec2 bottom(center.x, center.y - _tileH * 0.5f);
        cocos2d::Vec2 left(center.x - _tileW * 0.5f, center.y);
        cocos2d::Vec2 verts[4] = { top, right, bottom, left };
        _deployOverlay->drawSolidPoly(verts, 4, color);
    };

    // Only draw blocked cells inside the deployable diamond.
    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            if (!isGridBlocked(r, c)) continue;
            cocos2d::Vec2 center = gridToWorld(r, c);
            if (!isPosInDeployArea(center)) continue;
            drawCell(r, c, kRed);
        }
    }
}

void BattleScene::updateDeployOverlayVisibility()
{
    if (!_deployOverlay) return;

    bool hasSelection = (_selectedTroopType != -1);
    bool hasCount = false;
    if (hasSelection) {
        auto it = _troopCounts.find(_selectedTroopType);
        hasCount = (it != _troopCounts.end() && it->second > 0);
    }
    _deployOverlay->setVisible(hasSelection && hasCount);
}

void BattleScene::setSelectedTroop(int troopType)
{
    _selectedTroopType = troopType;

    for (auto& slot : _troopSlots) {
        bool selected = (slot.type == _selectedTroopType) && (_selectedTroopType != -1);
        if (slot.selectedLabel) slot.selectedLabel->setVisible(selected);
        if (slot.root) {
            // Slight highlight by scaling when selected
            slot.root->setScale(selected ? 1.05f : 1.0f);
        }
    }

    // Show/hide blocked deployment overlay based on selection state.
    updateDeployOverlayVisibility();
}

void BattleScene::showBattleToast(const std::string& msg, float seconds)
{
    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto label = Label::createWithSystemFont(msg, "Arial", 30);
    label->setColor(Color3B(220, 40, 40));
    label->enableOutline(Color4B::BLACK, 2);
    label->setPosition(origin + vs * 0.5f);
    this->addChild(label, 200);

    label->runAction(Sequence::create(
        DelayTime::create(std::max(0.0f, seconds)),
        FadeOut::create(0.25f),
        RemoveSelf::create(),
        nullptr
    ));
}

void BattleScene::deploySelectedTroop(const cocos2d::Vec2& glPos)
{
    if (_battleEnded || _phase == Phase::End) return;

    // Prevent duplicate deployment when multiple input systems fire for a single click/tap.
    long long nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (nowMs - _lastDeployMs < 50) return;
    _lastDeployMs = nowMs;

    // Safety guard: clicking on troop UI should never deploy or consume troops.
    if (isPosInTroopBar(glPos)) return;

    if (_selectedTroopType == -1) return;

    int& cnt = _troopCounts[_selectedTroopType];
    if (cnt <= 0) {
        showBattleToast("No troops left.", 1.0f);
        return;
    }

    if (!_world) return;
    cocos2d::Vec2 worldLocal = _world->convertToNodeSpace(glPos);

    // Only allow deployment inside the main diamond area.
    if (!isPosInDeployArea(worldLocal)) return;

    // Block deployment near enemy buildings (footprint expanded by 1 tile).
    int gr = 0, gc = 0;
    if (!worldToGrid(worldLocal, gr, gc)) return;
    if (isGridBlocked(gr, gc)) {
        showBattleToast("Cannot deploy here.", 1.0f);
        return;
    }

    if (!spawnUnit(_selectedTroopType, worldLocal)) return;

    // Record deployed troop count (used as "losses" in the result popup).
    _deployedCounts[_selectedTroopType] += 1;

    // Play deploy SFX (Resources/music).
    if (_selectedTroopType == 2) {
        SoundManager::playSfx("music/archer_deploy_09.ogg", 1.0f);
    }
    else if (_selectedTroopType == 3) {
        SoundManager::playSfx("music/giant_deploy_04v3.ogg", 1.0f);
    }
    else if (_selectedTroopType == 4) {
        SoundManager::playSfx("music/troop_housing_placing_08.ogg", 1.0f);
    }
    else {
        SoundManager::playSfx("music/troop_housing_placing_06.ogg", 1.0f);
    }

    // First successful deployment skips scout phase immediately.
    if (!_hasDeployedAnyTroop) {
        _hasDeployedAnyTroop = true;
        if (_phase == Phase::Scout) {
            startPhase(Phase::Battle, 180.0f);
        }
    }

    cnt = std::max(0, cnt - 1);
    refreshTroopBar();
}

bool BattleScene::spawnUnit(int unitId, const cocos2d::Vec2& worldLocalPos)
{
    if (_battleEnded) return false;
    if (!_world) return false;

    // Create a unit with the attacker's laboratory level.
    int unitLv = 1;
    auto itLv = _troopLevels.find(unitId);
    if (itLv != _troopLevels.end()) unitLv = itLv->second;
    std::unique_ptr<UnitBase> u = UnitFactory::create(unitId, unitLv);
    if (!u) {
        showBattleToast("Unit not supported.", 1.0f);
        return false;
    }

    // Convert tile-based stats into pixels (range/move).
    // _cellSizePx is computed from the current enemy map diamond.
    float cell = (_cellSizePx > 0.001f) ? _cellSizePx : 32.0f;
    u->attackRange = std::max(8.0f, u->attackRangeTiles * cell);
    // Movement speed stat is CoC-style (bigger = faster). We map it to px/s.
    // Requirement: all troop movement speeds are divided by 2.
    u->moveSpeed = std::max(2.0f, (u->moveSpeedStat * (cell * 0.25f)) / 2.0f);

    cocos2d::Sprite* spr = u->createSprite();
    if (!spr) spr = cocos2d::Sprite::create();

    // Foot anchor looks much better on isometric ground.
    spr->setAnchorPoint(cocos2d::Vec2(0.5f, 0.0f));

    // Fit the unit roughly into one tile.
    float maxW = (_tileW > 0.0f) ? (_tileW * 0.55f) : 48.0f;
    float maxH = (_tileH > 0.0f) ? (_tileH * 0.55f) : 48.0f;
    cocos2d::Size cs = spr->getContentSize();
    float s1 = (cs.width > 0.0f) ? (maxW / cs.width) : 1.0f;
    float s2 = (cs.height > 0.0f) ? (maxH / cs.height) : 1.0f;
    float s = std::min(s1, s2);
    s = std::min(s, 1.0f);
    s = std::max(s, 0.05f);
    spr->setScale(s);

    spr->setPosition(worldLocalPos);

    // Use r+c as z-order so troops stay correctly layered.
    int r = 0, c = 0;
    int z = 20;
    if (worldToGrid(worldLocalPos, r, c)) {
        z = 5 + r + c;
    }

    _world->addChild(spr, z);

    BattleUnitRuntime rt;
    rt.unit = std::move(u);
    rt.sprite = spr;
    rt.targetIndex = -1;
    _units.push_back(std::move(rt));

    return true;
}




void BattleScene::setupLootHUD()
{
    if (_lootHud) return;
    if (!_hud) return;

    auto visibleSize = cocos2d::Director::getInstance()->getVisibleSize();
    auto origin = cocos2d::Director::getInstance()->getVisibleOrigin();

    _lootHud = cocos2d::Node::create();
    _hud->addChild(_lootHud, 1);

    const float marginX = 10.0f;
    const float marginY = 10.0f;
    const float textW = 95.0f;
    const float barW = 145.0f;
    const float barH = 10.0f;
    const float rowH = 18.0f;
    const float gapY = 4.0f;

    float x0 = origin.x + marginX;
    float yTop = origin.y + visibleSize.height - marginY;

    // Section titles
    _lootableTitle = cocos2d::Label::createWithSystemFont("Lootable", "Arial", 18);
    _lootableTitle->setAnchorPoint(cocos2d::Vec2(0.0f, 1.0f));
    _lootableTitle->setPosition(cocos2d::Vec2(x0, yTop));
    _lootableTitle->setTextColor(cocos2d::Color4B::WHITE);
    _lootHud->addChild(_lootableTitle);

    float y = yTop - 24.0f;

    auto createRow = [&](cocos2d::Label*& text,
                         cocos2d::LayerColor*& bg,
                         cocos2d::LayerColor*& fill,
                         const cocos2d::Color4B& fillColor,
                         float yCenter)
    {
        text = cocos2d::Label::createWithSystemFont("", "Arial", 14);
        text->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
        text->setPosition(cocos2d::Vec2(x0, yCenter));
        text->setTextColor(cocos2d::Color4B::WHITE);
        _lootHud->addChild(text);

        bg = cocos2d::LayerColor::create(cocos2d::Color4B(40, 40, 40, 200), barW, barH);
        bg->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
        bg->setPosition(cocos2d::Vec2(x0 + textW, yCenter));
        _lootHud->addChild(bg);

        fill = cocos2d::LayerColor::create(fillColor, barW, barH);
        fill->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
        fill->setPosition(bg->getPosition());
        fill->setScaleX(1.0f);
        _lootHud->addChild(fill);
    };

    createRow(_lootableGoldText, _lootableGoldBg, _lootableGoldFill, cocos2d::Color4B(235, 200, 50, 220), y);
    y -= (rowH + gapY);
    createRow(_lootableElixirText, _lootableElixirBg, _lootableElixirFill, cocos2d::Color4B(170, 70, 200, 220), y);

    y -= 26.0f;
    _lootedTitle = cocos2d::Label::createWithSystemFont("Looted", "Arial", 18);
    _lootedTitle->setAnchorPoint(cocos2d::Vec2(0.0f, 1.0f));
    _lootedTitle->setPosition(cocos2d::Vec2(x0, y + 18.0f));
    _lootedTitle->setTextColor(cocos2d::Color4B::WHITE);
    _lootHud->addChild(_lootedTitle);

    y -= 6.0f;
    createRow(_lootedGoldText, _lootedGoldBg, _lootedGoldFill, cocos2d::Color4B(235, 200, 50, 220), y);
    y -= (rowH + gapY);
    createRow(_lootedElixirText, _lootedElixirBg, _lootedElixirFill, cocos2d::Color4B(170, 70, 200, 220), y);

    updateLootHUD();
}

void BattleScene::updateLootHUD()
{
    if (!_lootHud) return;

    int goldTotal = std::max(0, _lootGoldTotal);
    int elixirTotal = std::max(0, _lootElixirTotal);

    int goldLooted = std::max(0, _lootedGold);
    int elixirLooted = std::max(0, _lootedElixir);

    goldLooted = (goldTotal > 0) ? std::min(goldLooted, goldTotal) : 0;
    elixirLooted = (elixirTotal > 0) ? std::min(elixirLooted, elixirTotal) : 0;

    int goldRemain = std::max(0, goldTotal - goldLooted);
    int elixirRemain = std::max(0, elixirTotal - elixirLooted);

    float goldRemainRatio = (goldTotal > 0) ? (float)goldRemain / (float)goldTotal : 0.0f;
    float elixirRemainRatio = (elixirTotal > 0) ? (float)elixirRemain / (float)elixirTotal : 0.0f;
    float goldLootedRatio = (goldTotal > 0) ? (float)goldLooted / (float)goldTotal : 0.0f;
    float elixirLootedRatio = (elixirTotal > 0) ? (float)elixirLooted / (float)elixirTotal : 0.0f;

    if (_lootableGoldText) _lootableGoldText->setString(cocos2d::StringUtils::format("Gold %d/%d", goldRemain, goldTotal));
    if (_lootableElixirText) _lootableElixirText->setString(cocos2d::StringUtils::format("Elixir %d/%d", elixirRemain, elixirTotal));
    if (_lootedGoldText) _lootedGoldText->setString(cocos2d::StringUtils::format("Gold %d/%d", goldLooted, goldTotal));
    if (_lootedElixirText) _lootedElixirText->setString(cocos2d::StringUtils::format("Elixir %d/%d", elixirLooted, elixirTotal));

    if (_lootableGoldFill) _lootableGoldFill->setScaleX(std::min(1.0f, std::max(0.0f, goldRemainRatio)));
    if (_lootableElixirFill) _lootableElixirFill->setScaleX(std::min(1.0f, std::max(0.0f, elixirRemainRatio)));
    if (_lootedGoldFill) _lootedGoldFill->setScaleX(std::min(1.0f, std::max(0.0f, goldLootedRatio)));
    if (_lootedElixirFill) _lootedElixirFill->setScaleX(std::min(1.0f, std::max(0.0f, elixirLootedRatio)));
}