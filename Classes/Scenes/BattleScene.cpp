#include "Scenes/BattleScene.h"
#include "Scenes/MainScene.h"
#include "Scenes/LoginScene.h"
#include "Data/SaveSystem.h"

#include "GameObjects/Buildings/Building.h"
#include "GameObjects/Buildings/TroopBuilding.h"
#include "GameObjects/Units/UnitBase.h"

#include "Managers/SoundManager.h"
#include "ui/CocosGUI.h"

#include <algorithm>
#include <cmath>
#include <chrono>

USING_NS_CC;

static inline float clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

Scene* BattleScene::createScene()
{
    return BattleScene::create();
}

void BattleScene::setBuildingVisualParams()
{
    _buildingScale = 0.33f;
    _buildingScaleById.assign(11, 1.0f);
    _buildingOffsetById.assign(11, Vec2::ZERO);

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

    _buildingScaleById[10] = 0.6f;
    _buildingOffsetById[10] = Vec2(0, 0);
}

Vec2 BattleScene::gridToWorld(int r, int c) const
{
    float x = _anchor.x + (c - r) * (_tileW * 0.5f);
    float y = _anchor.y - (c + r) * (_tileH * 0.5f);
    return Vec2(x, y);
}

Vec2 BattleScene::worldToGridLocal(const Vec2& localPos) const
{
    float dx = localPos.x - _anchor.x;
    float dy = _anchor.y - localPos.y;

    float r = (dy / _tileH) - (dx / _tileW);
    float c = (dy / _tileH) + (dx / _tileW);
    return Vec2(r, c);
}

bool BattleScene::isPointInCellDiamondLocal(const Vec2& localPos, int r, int c) const
{
    if (_tileW <= 1e-4f || _tileH <= 1e-4f) return true;

    Vec2 center = gridToWorld(r, c);
    Vec2 d = localPos - center;

    float nx = std::abs(d.x) / (_tileW * 0.5f);
    float ny = std::abs(d.y) / (_tileH * 0.5f);
    return (nx + ny) <= 1.08f;
}

void BattleScene::buildEnemyOccupyGrid(bool outOcc[30][30]) const
{
    for (int r = 0; r < 30; ++r)
        for (int c = 0; c < 30; ++c)
            outOcc[r][c] = false;

    for (const auto& b : _enemyBuildings)
    {
        if (!b.building) continue;
        if (b.building->hp <= 0) continue;

        int br = b.r;
        int bc = b.c;
        if (br < 0 || br >= 30 || bc < 0 || bc >= 30) continue;

        if (b.id == 10)
        {
            outOcc[br][bc] = true;
        }
        else
        {
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc)
                {
                    int rr = br + dr;
                    int cc = bc + dc;
                    if (rr < 0 || rr >= 30 || cc < 0 || cc >= 30) continue;
                    outOcc[rr][cc] = true;
                }
        }
    }
}

bool BattleScene::isAdjacentToOccupied(const bool occ[30][30], int r, int c) const
{
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
        {
            if (dr == 0 && dc == 0) continue;
            int rr = r + dr;
            int cc = c + dc;
            if (rr < 0 || rr >= 30 || cc < 0 || cc >= 30) continue;
            if (occ[rr][cc]) return true;
        }
    return false;
}

bool BattleScene::findDeployCellFromClick(const Vec2& clickLocalPos, int& outR, int& outC) const
{
    Vec2 rc = worldToGridLocal(clickLocalPos);
    int r0 = (int)std::round(rc.x);
    int c0 = (int)std::round(rc.y);

    if (r0 < 0 || r0 >= _rows || c0 < 0 || c0 >= _cols) return false;
    if (!isPointInCellDiamondLocal(clickLocalPos, r0, c0)) return false;

    bool occ[30][30];
    buildEnemyOccupyGrid(occ);

    auto isValidDeploy = [&](int r, int c) -> bool {
        if (r < 0 || r >= _rows || c < 0 || c >= _cols) return false;
        if (occ[r][c]) return false;
        if (!isAdjacentToOccupied(occ, r, c)) return false;
        return true;
        };

    if (isValidDeploy(r0, c0))
    {
        outR = r0;
        outC = c0;
        return true;
    }

    float bestDist = 1e30f;
    int bestR = -1, bestC = -1;

    for (int rad = 1; rad <= 8; ++rad)
    {
        int rMin = std::max(0, r0 - rad);
        int rMax = std::min(_rows - 1, r0 + rad);
        int cMin = std::max(0, c0 - rad);
        int cMax = std::min(_cols - 1, c0 + rad);

        for (int r = rMin; r <= rMax; ++r)
            for (int c = cMin; c <= cMax; ++c)
            {
                if (!isValidDeploy(r, c)) continue;
                Vec2 center = gridToWorld(r, c);
                float d = center.distance(clickLocalPos);
                if (d < bestDist)
                {
                    bestDist = d;
                    bestR = r;
                    bestC = c;
                }
            }

        if (bestR != -1) break;
    }

    if (bestR != -1)
    {
        outR = bestR;
        outC = bestC;
        return true;
    }

    return false;
}

void BattleScene::renderTargetVillage()
{
    if (!_world) return;

    auto children = _world->getChildren();
    for (auto child : children)
    {
        if (child != _background) child->removeFromParent();
    }
    _enemyBuildings.clear();

    int targetSlot = SaveSystem::getBattleTargetSlot();
    if (targetSlot < 0 || !SaveSystem::exists(targetSlot))
    {
        auto msg = Label::createWithSystemFont("No target selected", "Arial", 30);
        msg->setPosition(Director::getInstance()->getVisibleOrigin() + Director::getInstance()->getVisibleSize() / 2);
        this->addChild(msg, 5);
        return;
    }

    SaveData data;
    if (!SaveSystem::load(targetSlot, data))
    {
        auto msg = Label::createWithSystemFont("Failed to load target", "Arial", 30);
        msg->setPosition(Director::getInstance()->getVisibleOrigin() + Director::getInstance()->getVisibleSize() / 2);
        this->addChild(msg, 5);
        return;
    }

    auto imageUvToWorld = [this](const Vec2& uv) {
        if (!_background) return Vec2::ZERO;
        Size s = _background->getContentSize();
        float sx = _background->getScaleX();
        float sy = _background->getScaleY();
        Vec2 local((uv.x - 0.5f) * s.width, (uv.y - 0.5f) * s.height);
        Vec2 parentSpace = _background->getPosition() + Vec2(local.x * sx, local.y * sy);
        return _background->getParent()->convertToWorldSpace(parentSpace);
        };

    Vec2 uvTop(0.51f, 0.20f);
    Vec2 uvRight(0.83f, 0.49f);
    Vec2 uvBottom(0.51f, 0.92f);
    Vec2 uvLeft(0.17f, 0.49f);

    Vec2 top = _world->convertToNodeSpace(imageUvToWorld(uvTop));
    Vec2 right = _world->convertToNodeSpace(imageUvToWorld(uvRight));
    Vec2 bottom = _world->convertToNodeSpace(imageUvToWorld(uvBottom));
    Vec2 left = _world->convertToNodeSpace(imageUvToWorld(uvLeft));

    float Lr = right.x - left.x;
    float Lt = top.y - bottom.y;

    _tileW = (2.0f * Lr) / (_cols + _rows);
    _tileH = (2.0f * Lt) / (_cols + _rows);
    _anchor = top;

    _cellSizePx = std::max(8.0f, (_tileW + _tileH) * 0.25f);
    _ai.setCellSizePx(_cellSizePx);

    std::sort(data.buildings.begin(), data.buildings.end(),
        [](const SaveBuilding& a, const SaveBuilding& b) {
            int sa = a.r + a.c;
            int sb = b.r + b.c;
            if (sa == sb) return a.r < b.r;
            return sa < sb;
        });

    for (const auto& bInfo : data.buildings)
    {
        auto b = BuildingFactory::create(bInfo.id, std::max(1, bInfo.level));
        if (!b) continue;

        if (bInfo.hp > 0) b->hp = std::min(bInfo.hp, b->hpMax);
        b->stored = bInfo.stored;

        auto sprite = b->createSprite();
        Vec2 pos = gridToWorld(bInfo.r, bInfo.c);

        int idx = std::max(1, std::min(10, bInfo.id));
        Vec2 off = _buildingOffsetById[idx];

        sprite->setPosition(pos + off);
        sprite->setScale(_buildingScale * _buildingScaleById[idx]);

        EnemyBuildingRuntime rt;
        rt.id = bInfo.id;
        rt.r = bInfo.r;
        rt.c = bInfo.c;
        rt.pos = pos + off;
        rt.building = std::move(b);
        rt.sprite = sprite;

        _enemyBuildings.push_back(std::move(rt));
        _world->addChild(sprite, 3 + bInfo.r + bInfo.c);
    }
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
        _background->setPosition(origin + visibleSize / 2);
        _world->addChild(_background, 0);
    }
    else
    {
        auto fallback = LayerColor::create(Color4B(50, 50, 50, 255));
        this->addChild(fallback, -1);
    }

    setBuildingVisualParams();
    renderTargetVillage();

    setupBattleHUD();
    setupTroopBar();

    _troopCounts = SaveSystem::getBattleReadyTroops();
    refreshTroopBar();

    auto troopKey = EventListenerKeyboard::create();
    troopKey->onKeyPressed = [this](EventKeyboard::KeyCode code, Event*) {
        if (code == EventKeyboard::KeyCode::KEY_1) setSelectedTroop(1);
        if (code == EventKeyboard::KeyCode::KEY_2) setSelectedTroop(2);
        if (code == EventKeyboard::KeyCode::KEY_3) setSelectedTroop(3);
        if (code == EventKeyboard::KeyCode::KEY_4) setSelectedTroop(4);
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(troopKey, this);

    auto touch = EventListenerTouchOneByOne::create();
    touch->setSwallowTouches(false);
    touch->onTouchBegan = [this](Touch* t, Event*) {
        Vec2 glPos = t->getLocation();
        if (handleTroopBarClick(glPos)) return true;
        deploySelectedTroop(glPos);
        return false;
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touch, this);

    startPhase(Phase::Scout, 45.0f);
    scheduleUpdate();

    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode code, Event*) {
        if (code == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            if (_settingsMask)
            {
                _settingsMask->removeFromParent();
                _settingsMask = nullptr;
                return;
            }
            if (_escMask) closeEscMenu();
            else openEscMenu();
        }
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

    auto returnLabel = Label::createWithSystemFont("Return", "Arial", 44);
    auto returnItem = MenuItemLabel::create(returnLabel, [this](Ref*) {
        SaveSystem::setBattleTargetSlot(-1);
        Director::getInstance()->replaceScene(TransitionFade::create(0.35f, MainScene::createScene()));
        });
    returnItem->setPosition(origin + visibleSize / 2);

    _returnMenu = Menu::create(returnItem, nullptr);
    _returnMenu->setPosition(Vec2::ZERO);
    _returnMenu->setVisible(false);
    this->addChild(_returnMenu, 100);
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
    this->addChild(bg, 60);
    _troopBar = bg;
}

void BattleScene::refreshTroopBar()
{
    if (!_troopBar) return;

    _troopBar->removeAllChildren();
    _troopSlots.clear();

    if (_troopCounts.empty())
    {
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
        icon->setScale(0.7f);
        slotRoot->addChild(icon, 1);

        Size isz = icon->getContentSize() * icon->getScale();
        slotRoot->setContentSize(isz);

        auto cntLbl = Label::createWithSystemFont("x" + std::to_string(count), "Arial", 18);
        cntLbl->setAnchorPoint(Vec2(1.0f, 1.0f));
        cntLbl->setColor(Color3B::WHITE);
        cntLbl->enableOutline(Color4B::BLACK, 2);
        cntLbl->setPosition(Vec2(isz.width - 2.0f, isz.height - 2.0f));
        slotRoot->addChild(cntLbl, 2);

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

    setSelectedTroop(_selectedTroopType);
}

bool BattleScene::handleTroopBarClick(const cocos2d::Vec2& glPos)
{
    if (!_troopBar) return false;

    Vec2 local = _troopBar->convertToNodeSpace(glPos);
    for (const auto& slot : _troopSlots)
    {
        if (!slot.root) continue;
        Rect r = slot.root->getBoundingBox();
        if (r.containsPoint(local))
        {
            if (_selectedTroopType == slot.type) setSelectedTroop(-1);
            else setSelectedTroop(slot.type);
            return true;
        }
    }

    Size s = _troopBar->getContentSize();
    Rect bg(0, 0, s.width, s.height);
    if (bg.containsPoint(local)) return true;

    return false;
}

bool BattleScene::isPosInTroopBar(const cocos2d::Vec2& glPos) const
{
    if (!_troopBar) return false;
    Vec2 local = _troopBar->convertToNodeSpace(glPos);

    for (const auto& slot : _troopSlots)
    {
        if (!slot.root) continue;
        Rect r = slot.root->getBoundingBox();
        if (r.containsPoint(local)) return true;
    }

    Size s = _troopBar->getContentSize();
    Rect bg(0, 0, s.width, s.height);
    return bg.containsPoint(local);
}

void BattleScene::setSelectedTroop(int troopType)
{
    _selectedTroopType = troopType;

    for (auto& slot : _troopSlots)
    {
        bool selected = (slot.type == _selectedTroopType) && (_selectedTroopType != -1);
        if (slot.selectedLabel) slot.selectedLabel->setVisible(selected);
        if (slot.root) slot.root->setScale(selected ? 1.05f : 1.0f);
    }
}

void BattleScene::showBattleToast(const std::string& msg)
{
    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto label = Label::createWithSystemFont(msg, "Arial", 30);
    label->setColor(Color3B(220, 40, 40));
    label->enableOutline(Color4B::BLACK, 2);
    label->setPosition(origin + vs * 0.5f);
    this->addChild(label, 200);

    label->runAction(Sequence::create(
        DelayTime::create(1.5f),
        FadeOut::create(0.25f),
        RemoveSelf::create(),
        nullptr
    ));
}

void BattleScene::deploySelectedTroop(const cocos2d::Vec2& glPos)
{
    if (!_deployEnabled) return;
    if (_battleEnded) return;

    long long nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (nowMs - _lastDeployMs < 50) return;
    _lastDeployMs = nowMs;

    if (isPosInTroopBar(glPos)) return;
    if (_selectedTroopType == -1) return;

    auto it = _troopCounts.find(_selectedTroopType);
    if (it == _troopCounts.end() || it->second <= 0)
    {
        showBattleToast("No troops");
        return;
    }

    if (!_world) return;
    Vec2 local = _world->convertToNodeSpace(glPos);

    if (!spawnUnit(_selectedTroopType, local)) return;

    int left = it->second - 1;
    if (left <= 0)
    {
        _troopCounts.erase(it);
        if (_selectedTroopType != -1 && _troopCounts.find(_selectedTroopType) == _troopCounts.end())
            _selectedTroopType = -1;
    }
    else
    {
        it->second = left;
    }

    refreshTroopBar();
}

void BattleScene::enableDeployInput(bool enabled)
{
    _deployEnabled = enabled;
    if (_troopBar) _troopBar->setVisible(true);
}

bool BattleScene::spawnUnit(int unitId, const cocos2d::Vec2& clickLocalPos)
{
    if (_battleEnded) return false;

    auto it = _troopCounts.find(unitId);
    if (it == _troopCounts.end() || it->second <= 0) return false;

    int r = -1, c = -1;
    if (!findDeployCellFromClick(clickLocalPos, r, c))
        return false;

    int level = 1; // TODO: read actual level from save
    std::unique_ptr<UnitBase> u = UnitFactory::create(unitId, level);
    if (!u)
    {
        showBattleToast("Unit not supported");
        return false;
    }

    auto spr = u->createSprite();
    if (!spr) spr = Sprite::create();

    // ===== 关键：大小按 MainScene 训练后一样的规则 =====
    spr->setAnchorPoint(Vec2(0.5f, 0.0f));

    Vec2 center = gridToWorld(r, c);
    float jx = CCRANDOM_MINUS1_1() * (_tileW * 0.15f);
    float jy = CCRANDOM_MINUS1_1() * (_tileH * 0.10f);
    Vec2 pos = center + Vec2(jx, _tileH * 0.10f + jy);

    float desiredW = _tileW * 0.60f;
    float s = desiredW / std::max(1.0f, spr->getContentSize().width);
    spr->setScale(s);
    spr->setPosition(pos);

    int z = 5 + r + c;
    _world->addChild(spr, z);

    BattleUnitRuntime rt;
    rt.unit = std::move(u);
    rt.sprite = spr;
    rt.targetIndex = -1;
    _units.push_back(std::move(rt));

    return true;
}

void BattleScene::startPhase(Phase p, float durationSec)
{
    _phase = p;
    _phaseTotal = std::max(0.001f, durationSec);
    _phaseRemaining = durationSec;

    if (_phase == Phase::Battle) enableDeployInput(true);
    else enableDeployInput(false);

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

    float percent = clamp01(_phaseRemaining / _phaseTotal);
    _barFill->setScaleX(percent);
}

void BattleScene::showReturnButton()
{
    if (_returnMenu) _returnMenu->setVisible(true);
}

void BattleScene::checkBattleResult(bool timeUp)
{
    if (_battleEnded) return;

    bool anyNonWallAlive = false;
    for (auto& b : _enemyBuildings)
    {
        if (!b.building) continue;
        if (b.building->hp > 0 && b.id != 10)
        {
            anyNonWallAlive = true;
            break;
        }
    }

    bool win = !anyNonWallAlive;
    bool lose = timeUp && anyNonWallAlive;

    if (!win && !lose) return;

    _battleEnded = true;
    _phase = Phase::End;
    _phaseRemaining = 0;
    updateBattleHUD();
    this->unscheduleUpdate();

    enableDeployInput(false);
    showReturnButton();

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto result = Label::createWithSystemFont(win ? "VICTORY!" : "DEFEAT!", "Arial", 96);
    result->setPosition(origin + visibleSize / 2 + Vec2(0, 90));
    this->addChild(result, 150);

    result->setScale(0.2f);
    result->runAction(EaseBackOut::create(ScaleTo::create(0.35f, 1.0f)));
}

void BattleScene::update(float dt)
{
    if (_phase == Phase::End) return;

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
            checkBattleResult(true);
            return;
        }
    }

    if (_phase == Phase::Battle && !_battleEnded)
    {
        _ai.update(dt, _units, _enemyBuildings);
        checkBattleResult(false);
    }

    updateBattleHUD();
}

// ===== ESC menu / settings（保持你之前逻辑，不影响本次问题）=====
void BattleScene::openEscMenu()
{
    if (_escMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    _escMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_escMask, 200);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
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
    auto settingsItem = MenuItemLabel::create(settingsLabel, [this](Ref*) { this->openSettings(); });

    auto backLabel = Label::createWithSystemFont("Back to Start", "Arial", 42);
    auto backItem = MenuItemLabel::create(backLabel, [this](Ref*) {
        closeEscMenu();
        Director::getInstance()->replaceScene(TransitionFade::create(0.35f, LoginScene::createScene()));
        });

    auto exitLabel = Label::createWithSystemFont("Exit Game", "Arial", 42);
    auto exitItem = MenuItemLabel::create(exitLabel, [](Ref*) { Director::getInstance()->end(); });

    settingsItem->setPosition(Vec2(panelW / 2, panelH / 2 + 70));
    backItem->setPosition(Vec2(panelW / 2, panelH / 2));
    exitItem->setPosition(Vec2(panelW / 2, panelH / 2 - 70));

    auto menu = Menu::create(settingsItem, backItem, exitItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    panel->addChild(menu, 1);

    auto closeLabel = Label::createWithSystemFont("X", "Arial", 42);
    auto closeItem = MenuItemLabel::create(closeLabel, [this](Ref*) { closeEscMenu(); });
    closeItem->setPosition(Vec2(panelW - 40, panelH - 40));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu);
}

void BattleScene::closeEscMenu()
{
    if (_escMask)
    {
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
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
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
        if (_settingsMask)
        {
            _settingsMask->removeFromParent();
            _settingsMask = nullptr;
        }
        });
    closeItem->setPosition(Vec2(panelW - 40, panelH - 40));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu);
}
