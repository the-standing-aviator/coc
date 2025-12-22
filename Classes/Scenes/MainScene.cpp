#include "Scenes/MainScene.h"
#include "Scenes/BattleScene.h"
#include "Scenes/MenuScene.h"
#include "Data/SaveSystem.h"
#include "Managers/ConfigManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/SoundManager.h"
#include "UI/ResourcePanel.h"
#include "UI/BuildingButton.h"
#include "UI/CustomButton.h"
#include "GameObjects/Buildings/Building.h"
#include "GameObjects/Buildings/DefenseBuilding.h"
#include "GameObjects/Buildings/ResourceBuilding.h"
#include "GameObjects/Buildings/TroopBuilding.h"
#include "GameObjects/Buildings/TownHall.h"
#include <memory>
#include <cmath>
#include <algorithm>

#include "ui/CocosGUI.h"

#ifdef _WIN32
#include <windows.h>
#endif
using namespace cocos2d;

Scene* MainScene::createScene()
{
    return MainScene::create();
}

bool MainScene::init()
{
    if (!Scene::init()) return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    _world = Node::create();
    this->addChild(_world, 0);

    _background = Sprite::create("backgrounds/village_map.jpg");
    Size bg = _background->getContentSize();
    Size screen = visibleSize;
    float scale = std::max(screen.width / bg.width, screen.height / bg.height);
    _background->setScale(scale);
    _background->setPosition(Vec2(origin.x + screen.width / 2, origin.y + screen.height / 2));
    _world->addChild(_background, 0);

    {
        Size img = _background->getContentSize();
        float base = _background->getScaleX();
        float minZ = std::max(screen.width / (img.width * base), screen.height / (img.height * base));
        _minZoom = std::max(minZ, 0.1f);
        setZoom(1.0f);
    }

    buildGrid();
    setGridVisible(false);
    _hint = DrawNode::create();
    _occupied = DrawNode::create();
    _world->addChild(_occupied, 2);
    _world->addChild(_hint, 2);
    _occupy.assign(_rows, std::vector<int>(_cols, 0));
    _buildingScaleById.assign(10, 1.0f);
    _buildingOffsetById.assign(10, Vec2::ZERO);

    setupInteraction();

    SoundManager::play("bgm/backgroundbgm.mp3", true, 0.7f);

    auto btn = BuildingButton::create();
    btn->setButtonScale(0.15f);
    btn->setButtonOffset(Vec2(16.f, 16.f));
    btn->onClicked = [this]() {
        auto vs = Director::getInstance()->getVisibleSize();
        auto origin = Director::getInstance()->getVisibleOrigin();
        auto panel = LayerColor::create(Color4B::WHITE);
        this->addChild(panel, 50);
        float titleH = 48.f;
        auto titleImg = Sprite::create("ui/shoptitle.png");
        if (titleImg) {
            float sx = vs.width / titleImg->getContentSize().width;
            titleImg->setScaleX(sx);
            titleImg->setAnchorPoint(Vec2(0.5f, 1.0f));
            titleImg->setPosition(Vec2(origin.x + vs.width * 0.5f, origin.y + vs.height));
            panel->addChild(titleImg, 1);
            titleH = titleImg->getContentSize().height * titleImg->getScaleY();
        }
        else {
            auto bar = LayerColor::create(Color4B(255, 199, 57, 255), vs.width, titleH);
            bar->setAnchorPoint(Vec2(0.0f, 1.0f));
            bar->setPosition(Vec2(origin.x, origin.y + vs.height));
            panel->addChild(bar, 1);
            auto t = Label::createWithSystemFont("Shop", "Arial", 20);
            t->setColor(Color3B::BLACK);
            t->setPosition(Vec2(origin.x + vs.width * 0.5f, origin.y + vs.height - titleH * 0.5f));
            panel->addChild(t, 2);
        }
        float margin = 20.f;
        Rect area(origin.x + margin, origin.y + margin, vs.width - margin * 2, vs.height - margin * 2 - titleH);
        int cols = 5, rows = 2;
        float gap = 12.f;
        float cw = (area.size.width - gap * (cols - 1)) / cols;
        float ch = (area.size.height - gap * (rows - 1)) / rows;
        auto menu = Menu::create();
        menu->setPosition(Vec2::ZERO);
        panel->addChild(menu, 2);
        int idx = 0;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                ++idx;
                float x = area.origin.x + c * (cw + gap);
                float y = area.origin.y + area.size.height - (r + 1) * ch - r * gap;
                std::string path = StringUtils::format("ui/build_button%d.png", idx);
                MenuItem* item = nullptr;
                if (FileUtils::getInstance()->isFileExist(path)) {
                    auto it = MenuItemImage::create(path, path, [this, idx, panel](Ref*) {
                        int buildId = (idx == 9 ? 10 : idx);
                        startPlacement(buildId);
                        panel->removeFromParent();
                        });
                    Size s = it->getContentSize();
                    float sx = std::min(cw / s.width, ch / s.height);
                    it->setScale(sx);
                    item = it;
                    int buildId = (idx == 9 ? 10 : idx);
                    int limit = buildLimitForId(buildId);
                    if (countById(buildId) >= limit) {
                        it->setEnabled(false);
                        if (auto normal = it->getNormalImage()) normal->setColor(Color3B(150, 150, 150));
                        if (auto selected = it->getSelectedImage()) selected->setColor(Color3B(150, 150, 150));
                    }
                }
                else {
                    auto label = Label::createWithSystemFont(StringUtils::format("%d", idx), "Arial", 20);
                    auto it = MenuItemLabel::create(label, [this, idx, panel](Ref*) {
                        int buildId = (idx == 9 ? 10 : idx);
                        startPlacement(buildId);
                        panel->removeFromParent();
                        });
                    it->setContentSize(Size(cw, ch));
                    item = it;
                    int buildId = (idx == 9 ? 10 : idx);
                    int limit = buildLimitForId(buildId);
                    if (countById(buildId) >= limit) {
                        it->setEnabled(false);
                        label->setColor(Color3B(150, 150, 150));
                    }
                }
                item->setPosition(Vec2(x + cw * 0.5f, y + ch * 0.5f));
                menu->addChild(item);
            }
        }
        auto closeLabel = Label::createWithSystemFont("X", "Arial", 28);
        closeLabel->setColor(Color3B::BLACK);
        auto closeItem = MenuItemLabel::create(closeLabel, [panel](Ref*) { panel->removeFromParent(); });
        closeItem->setPosition(Vec2(origin.x + vs.width - 20.f, origin.y + vs.height - 20.f));
        auto closeMenu = Menu::create(closeItem, nullptr);
        closeMenu->setPosition(Vec2::ZERO);
        panel->addChild(closeMenu, 3);
        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [](Touch*, Event*) { return true; };
        panel->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, panel);
        };
    this->addChild(btn, 40);
    auto resPanel = ResourcePanel::create();
    resPanel->setName("ResourcePanel");
    resPanel->onSetTimeScale = [this](float s) { setTimeScale(s); };
    this->addChild(resPanel, 45);
    this->scheduleUpdate();

    auto battleItem = MenuItemImage::create("ui/battle_button.png", "ui/battle_button.png", [this](Ref* sender) {
        this->openAttackTargetPicker();
        });
    _battleButton = battleItem;
    if (_battleButton) {
        setBattleButtonScale(0.065f);
        auto battleMenu = Menu::create(_battleButton, nullptr);
        battleMenu->setPosition(Vec2::ZERO);
        this->addChild(battleMenu, 50);
    }

    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode code, Event*) {
        // ESC: close settings first, otherwise toggle in-game menu.
        if (code == EventKeyboard::KeyCode::KEY_ESCAPE) {
            if (_settingsMask) {
                _settingsMask->removeFromParent();
                _settingsMask = nullptr;
                return;
            }
            if (_escMask) closeEscMenu();
            else openEscMenu();
            return;
        }
        if (code == EventKeyboard::KeyCode::KEY_G) {
            bool v = _grid && _grid->isVisible();
            setGridVisible(!v);
        }
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    //建筑大小接口
    setBuildingScale(0.33f);

    setBuildingScaleForId(1, 0.4f);
    setBuildingOffsetForId(1, Vec2(0, 20 / 3));

    setBuildingOffsetForId(2, Vec2(-2 / 3, 14 / 3));

    setBuildingOffsetForId(3, Vec2(-10 / 3, 14 / 3));

    setBuildingOffsetForId(4, Vec2(4 / 3, 4 / 3));

    setBuildingScaleForId(7, 0.7f);
    setBuildingOffsetForId(7, Vec2(-4 / 3, 0));

    setBuildingScaleForId(8, 1.3f);
    setBuildingOffsetForId(8, Vec2(4 / 3, 10 / 3));

    setBuildingScaleForId(9, 0.8f);
    setBuildingOffsetForId(9, Vec2(4 / 3, 10 / 3));
    setBuildingScaleForId(10, 0.8f);
    setBuildingOffsetForId(10, Vec2(0, 4 / 3));
    loadFromCurrentSaveOrCreate();

    return true;
}

void MainScene::openEscMenu()
{
    if (_escMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    _escMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_escMask, 200);

    // Swallow touches so the game below can't be clicked.
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

    // Allow touches to pass to panel children (buttons/scroll)
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
        // Close menu before switching scenes.
        closeEscMenu();
        Director::getInstance()->replaceScene(
            TransitionFade::create(0.35f, MenuScene::createScene())
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
    panel->addChild(closeMenu, 2);
}

void MainScene::closeEscMenu()
{
    if (_escMask) {
        _escMask->removeFromParent();
        _escMask = nullptr;
    }
}

void MainScene::openSettings()
{
    if (_settingsMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 1) Mask layer (swallow touches)
    _settingsMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_settingsMask, 300);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _settingsMask);

    // 2) Panel
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

    // 3) Title
    auto title = Label::createWithSystemFont("Settings", "Arial", 50);
    title->setPosition(Vec2(panelW / 2, panelH - 50));
    panel->addChild(title);

    // 4) Master volume slider (same as LoginScene)
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

    // 5) Mute checkbox
    auto mute = ui::CheckBox::create(
        "ui/checkbox_off.png",
        "ui/checkbox_on.png"
    );
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

    // 6) Close button
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

void MainScene::buildGrid()
{
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
    if (!_grid) {
        _grid = DrawNode::create();
        _world->addChild(_grid, 1);
    }
    _grid->clear();
    Color4F lineColor(1, 1, 1, 0.25f);
    for (int r = 0; r < _rows; ++r) {
        for (int c = 0; c < _cols; ++c) {
            Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
            Vec2 top(center.x, center.y + _tileH * 0.5f);
            Vec2 right(center.x + _tileW * 0.5f, center.y);
            Vec2 bottom(center.x, center.y - _tileH * 0.5f);
            Vec2 left(center.x - _tileW * 0.5f, center.y);
            _grid->drawLine(top, right, lineColor);
            _grid->drawLine(right, bottom, lineColor);
            _grid->drawLine(bottom, left, lineColor);
            _grid->drawLine(left, top, lineColor);
        }
    }
}

int MainScene::countById(int id) const
{
    int n = 0;
    for (const auto& b : _buildings) if (b.id == id) ++n;
    return n;
}
int MainScene::buildLimitForId(int id) const
{
    return ConfigManager::getBuildLimit(id, getTownHallLevel());
}

void MainScene::setGridVisible(bool visible)
{
    if (_grid) _grid->setVisible(visible);
}

cocos2d::Vec2 MainScene::gridToWorld(int r, int c) const
{
    float x = _anchor.x + (c - r) * (_tileW * 0.5f);
    float y = _anchor.y - (c + r) * (_tileH * 0.5f);
    return Vec2(x, y);
}

cocos2d::Vec2 MainScene::worldToGrid(const cocos2d::Vec2& pos) const
{
    Vec2 local = pos;
    if (_world) local = _world->convertToNodeSpace(pos);
    float dx = local.x - _anchor.x;
    float dy = _anchor.y - local.y;
    float r = (dy / _tileH) - (dx / _tileW);
    float c = (dy / _tileH) + (dx / _tileW);
    return Vec2(r, c);
}

cocos2d::Vec2 MainScene::imageUvToWorld(const Vec2& uv) const
{
    if (!_background) return Vec2::ZERO;
    Size s = _background->getContentSize();
    float sx = _background->getScaleX();
    float sy = _background->getScaleY();
    Vec2 local((uv.x - 0.5f) * s.width, (uv.y - 0.5f) * s.height);
    Vec2 parentSpace = _background->getPosition() + Vec2(local.x * sx, local.y * sy);
    return _background->getParent()->convertToWorldSpace(parentSpace);
}

void MainScene::setZoom(float z)
{
    _zoom = std::max(_minZoom, std::min(_maxZoom, z));
    if (_world) _world->setScale(_zoom);
    clampWorld();
}
void MainScene::setTimeScale(float s)
{
    float clamped = std::max(0.01f, s);
    if (std::fabs(clamped - _timeScale) > 1e-4f)
    {
        _timeScale = clamped;
        _saveDirty = true;
    }
    else
    {
        _timeScale = clamped;
    }
}

void MainScene::update(float dt)
{
    for (size_t i = 0; i < _buildings.size(); ++i) {
        auto& pb = _buildings[i];
        if (!pb.data) continue;
        if (auto ec = dynamic_cast<ElixirCollector*>(pb.data.get())) {
            float before = ec->stored;
            ec->updateProduction(dt, _timeScale);
            if (std::fabs(ec->stored - before) > 1e-3f) _saveDirty = true;
            ec->manageCollectPrompt(_world, pb.sprite);
        }
        else if (auto gm = dynamic_cast<GoldMine*>(pb.data.get())) {
            float before = gm->stored;
            gm->updateProduction(dt, _timeScale);
            if (std::fabs(gm->stored - before) > 1e-3f) _saveDirty = true;
            gm->manageCollectPrompt(_world, pb.sprite);
        }
    }
    _autosaveTimer += dt;
    if (_autosaveTimer >= 2.0f)
    {
        saveToCurrentSlot(false);
        _autosaveTimer = 0.0f;
    }
}
void MainScene::onExit()
{
    saveToCurrentSlot(true);
    Scene::onExit();
}

int MainScene::getTownHallLevel() const
{
    for (const auto& b : _buildings) {
        if (b.id == 9 && b.data) return b.data->level;
    }
    return 1;
}

void MainScene::openUpgradeWindowForIndex(int idx)
{
    if (idx < 0 || idx >= (int)_buildings.size()) return;
    auto& b = _buildings[idx];
    int curLv = b.data ? b.data->level : 1;
    int nextLv = std::min(ConfigManager::getMaxLevel(), curLv + 1);
    auto cost = ConfigManager::getUpgradeCost(b.id, nextLv);
    int thLevel = getTownHallLevel();
    bool isMax = (curLv >= ConfigManager::getMaxLevel());
    int maxBk = 0;
    for (const auto& it : _buildings) {
        if (it.id == 7 && it.data) {
            maxBk = std::max(maxBk, it.data->level);
        }
    }
    bool disabled = isMax || !ConfigManager::isUpgradeAllowed(b.id, nextLv, thLevel, maxBk);
    std::string title = cocos2d::StringUtils::format("%s (Level %d)", ConfigManager::getBuildingName(b.id).c_str(), curLv);
    std::string resName = cost.useGold ? "Gold" : "Elixir";
    auto modal = CustomButton::createUpgradePanel(title, resName, cost.amount, disabled, isMax,
        [this, idx, nextLv, cost]() {
            auto& pb = _buildings[idx];
            if (!pb.data) return;
            int curLv2 = pb.data->level;
            if (curLv2 >= ConfigManager::getMaxLevel()) { return; }
            {
                int th = getTownHallLevel();
                int maxBk2 = 0;
                for (const auto& it : _buildings) {
                    if (it.id == 7 && it.data) {
                        maxBk2 = std::max(maxBk2, it.data->level);
                    }
                }
                if (!ConfigManager::isUpgradeAllowed(pb.id, nextLv, th, maxBk2)) return;
            }
            bool ok = false;
            if (cost.useGold) ok = ResourceManager::spendGold(cost.amount);
            else ok = ResourceManager::spendElixir(cost.amount);
            if (ok) {
                pb.data->level = curLv2 + 1;
                BuildingFactory::applyStats(pb.data.get(), pb.id, pb.data->level);
                _saveDirty = true;
            }
        },
        []() {}
    );
    this->addChild(modal, 100);
}



void MainScene::setupInteraction()
{
    auto mouse = EventListenerMouse::create();
    mouse->onMouseScroll = [this](Event* e) {
        auto ev = static_cast<EventMouse*>(e);
        // When a modal overlay (attack target picker) is open, let the overlay handle mouse wheel.
        if (_attackMask && _attackMask->isVisible()) return;
float delta = ev->getScrollY();
        float k = 1.1f;
        if (delta > 0) setZoom(_zoom / k);
        else if (delta < 0) setZoom(_zoom * k);
        };
    mouse->onMouseDown = [this](Event* e) {
        auto ev = static_cast<EventMouse*>(e);
        if (ev->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            Vec2 cur(ev->getCursorX(), ev->getCursorY());
            Vec2 ij = worldToGrid(cur);
            int r = (int)std::round(ij.x);
            int c = (int)std::round(ij.y);
            if (_placing) {
                if (canPlace(r, c)) {
                    placeBuilding(r, c, _placingId);
                    if (_placingId == 10) {
                        int limit = buildLimitForId(10);
                        if (countById(10) >= limit) cancelPlacement();
                    }
                    else {
                        cancelPlacement();
                    }
                }
            }
            else if (_moving) {
                if (canPlaceIgnoring(r, c, _movingIndex)) commitMove(r, c);
                cancelMove();
            }
            else {
                int idx = findBuildingCenter(r, c);
                if (idx != -1) {
                    auto& pb = _buildings[idx];
                    if (pb.data) {
                        auto ec = dynamic_cast<ElixirCollector*>(pb.data.get());
                        if (ec) {
                            bool canCollect = ec->canCollect();
                            if (canCollect) {
                                int deliver = ec->collect();
                                if (deliver > 0) _saveDirty = true;
                            }
                            else {
                                _moving = true;
                                _movingIndex = idx;
                                _hint->clear();
                            }
                        }
                        else if (auto gm = dynamic_cast<GoldMine*>(pb.data.get())) {
                            bool canCollect = gm->canCollect();
                            if (canCollect) {
                                int deliver = gm->collect();
                                if (deliver > 0) _saveDirty = true;
                            }
                            else {
                                _moving = true;
                                _movingIndex = idx;
                                _hint->clear();
                            }
                        }
                        else {
                            _moving = true;
                            _movingIndex = idx;
                            _hint->clear();
                        }
                    }
                    else {
                        _dragging = true;
                        _lastMouse = Vec2(ev->getCursorX(), ev->getCursorY());
                    }
                }
                else {
                    _dragging = true;
                    _lastMouse = Vec2(ev->getCursorX(), ev->getCursorY());
                }
            }
        }
        else if (ev->getMouseButton() == EventMouse::MouseButton::BUTTON_RIGHT) {
            if (_placing) { cancelPlacement(); }
            if (_moving) { cancelMove(); }
            Vec2 cur(ev->getCursorX(), ev->getCursorY());
            Vec2 ij = worldToGrid(cur);
            int r = (int)std::round(ij.x);
            int c = (int)std::round(ij.y);
            int idx = findBuildingCenter(r, c);
            if (idx != -1) openUpgradeWindowForIndex(idx);
        }
        };
    mouse->onMouseUp = [this](Event* e) {
        auto ev = static_cast<EventMouse*>(e);
        if (ev->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            _dragging = false;
        }
        };
    mouse->onMouseMove = [this](Event* e) {
        auto ev = static_cast<EventMouse*>(e);
        Vec2 cur(ev->getCursorX(), ev->getCursorY());
        if (_placing) {
            _hint->clear();
            Vec2 ij = worldToGrid(cur);
            int r = (int)std::round(ij.x);
            int c = (int)std::round(ij.y);
            if (_placingId == 10) {
                bool ok = canPlace(r, c);
                Color4F col = ok ? Color4F(0.3f, 0.9f, 0.3f, 0.35f) : Color4F(0.9f, 0.3f, 0.3f, 0.35f);
                if (r >= 0 && r < _rows && c >= 0 && c < _cols)
                    drawCellFilled(r, c, col, _hint);
            }
            else {
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        int rr = r + dr, cc = c + dc;
                        if (rr >= 0 && rr < _rows && cc >= 0 && cc < _cols)
                            drawCellFilled(rr, cc, Color4F(0.3f, 0.9f, 0.3f, 0.35f), _hint);
                    }
                }
            }
        }
        else if (_moving) {
            _hint->clear();
            Vec2 ij = worldToGrid(cur);
            int r = (int)std::round(ij.x);
            int c = (int)std::round(ij.y);
            bool ok = canPlaceIgnoring(r, c, _movingIndex);
            Color4F col = ok ? Color4F(0.3f, 0.9f, 0.3f, 0.35f) : Color4F(0.9f, 0.3f, 0.3f, 0.35f);
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    int rr = r + dr, cc = c + dc;
                    if (rr >= 0 && rr < _rows && cc >= 0 && cc < _cols)
                        drawCellFilled(rr, cc, col, _hint);
                }
            }
        }
        else if (_dragging && _world) {
            Vec2 delta = cur - _lastMouse;
            _lastMouse = cur;
            _world->setPosition(_world->getPosition() + delta);
            clampWorld();
        }
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouse, this);
}

void MainScene::clampWorld()
{
    if (!_background || !_world) return;
    auto director = Director::getInstance();
    Size screen = director->getVisibleSize();
    Vec2 origin = director->getVisibleOrigin();
    Size img = _background->getContentSize();
    float base = _background->getScaleX();
    float s = base * _world->getScaleX();
    float halfW = img.width * s * 0.5f;
    float halfH = img.height * s * 0.5f;
    Vec2 center = _background->getParent()->convertToWorldSpace(_background->getPosition());
    Vec2 P = _world->getPosition();
    Vec2 center0 = center - P;
    float lowX = origin.x + screen.width - halfW - center0.x;
    float highX = origin.x + halfW - center0.x;
    float lowY = origin.y + screen.height - halfH - center0.y;
    float highY = origin.y + halfH - center0.y;
    P.x = std::max(lowX, std::min(highX, P.x));
    P.y = std::max(lowY, std::min(highY, P.y));
    _world->setPosition(P);
}

void MainScene::startPlacement(int id)
{
    _placing = true;
    _placingId = id;
}

void MainScene::cancelPlacement()
{
    _placing = false;
    _placingId = 0;
    if (_hint) _hint->clear();
}

bool MainScene::canPlace(int r, int c) const
{
    if (_placing && _placingId == 10) {
        if (r < 0 || r >= _rows || c < 0 || c >= _cols) return false;
        return _occupy[r][c] == 0;
    }
    else {
        if (r < 1 || r > _rows - 2 || c < 1 || c > _cols - 2) return false;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                if (_occupy[r + dr][c + dc] != 0) return false;
        return true;
    }
}

void MainScene::drawCellFilled(int r, int c, const Color4F& color, DrawNode* layer)
{
    Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
    Vec2 top(center.x, center.y + _tileH * 0.5f);
    Vec2 right(center.x + _tileW * 0.5f, center.y);
    Vec2 bottom(center.x, center.y - _tileH * 0.5f);
    Vec2 left(center.x - _tileW * 0.5f, center.y);
    Vec2 verts[4] = { top, right, bottom, left };
    layer->drawSolidPoly(verts, 4, color);
}

void MainScene::placeBuilding(int r, int c, int id)
{
    if (id >= 1 && id <= 10 && countById(id) >= buildLimitForId(id)) {
        return;
    }
    auto buildCost = ConfigManager::getBuildCost(id);
    if (buildCost.amount > 0) {
        bool ok = buildCost.useGold ? ResourceManager::spendGold(buildCost.amount)
            : ResourceManager::spendElixir(buildCost.amount);
        if (!ok) return;
    }
    if (id == 10) {
        _occupy[r][c] = id;
        drawCellFilled(r, c, Color4F(0.2f, 0.8f, 0.2f, 0.6f), _occupied);
    }
    else {
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                _occupy[r + dr][c + dc] = id;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                drawCellFilled(r + dr, c + dc, Color4F(0.2f, 0.8f, 0.2f, 0.6f), _occupied);
    }

    auto b = BuildingFactory::create(id, 1);
    auto s = b->createSprite();
    Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
    int idx = std::max(1, std::min(9, id));
    Vec2 off = _buildingOffsetById[idx];
    s->setPosition(center + off);
    s->setScale(_buildingScale * _buildingScaleById[idx]);
    _world->addChild(s, 3);
    std::shared_ptr<Building> ptr(b.release());
    _buildings.push_back({ id, r, c, s, ptr });
    _saveDirty = true;
}

void MainScene::setBuildingScale(float s)
{
    _buildingScale = std::max(0.1f, std::min(3.0f, s));
}

void MainScene::setBuildingScaleForId(int id, float s)
{
    if (id == 0) id = 9;
    if (id < 1 || id > 9) return;
    _buildingScaleById[id] = std::max(0.1f, std::min(3.0f, s));
}

void MainScene::setBuildingOffsetForId(int id, const Vec2& off)
{
    if (id == 0) id = 9;
    if (id < 1 || id > 9) return;
    _buildingOffsetById[id] = off;
}

void MainScene::setBattleButtonScale(float s)
{
    if (!_battleButton) return;
    _battleButton->setScale(s);
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    float margin = 20.0f;
    Size size = _battleButton->getContentSize();
    float x = origin.x + size.width * s * 0.5f + margin;
    float y = origin.y + size.height * s * 0.5f + margin;
    _battleButton->setPosition(Vec2(x, y));
}

bool MainScene::canPlaceIgnoring(int r, int c, int ignoreIndex) const
{
    int movingId = _buildings[ignoreIndex].id;
    if (movingId == 10) {
        if (r < 0 || r >= _rows || c < 0 || c >= _cols) return false;
        int br = _buildings[ignoreIndex].r;
        int bc = _buildings[ignoreIndex].c;
        bool inOld = (r == br && c == bc);
        if (!inOld && _occupy[r][c] != 0) return false;
        return true;
    }
    else {
        if (r < 1 || r > _rows - 2 || c < 1 || c > _cols - 2) return false;
        int br = _buildings[ignoreIndex].r;
        int bc = _buildings[ignoreIndex].c;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc) {
                int rr = r + dr, cc = c + dc;
                bool inOld = std::abs(rr - br) <= 1 && std::abs(cc - bc) <= 1;
                if (!inOld && _occupy[rr][cc] != 0) return false;
            }
        return true;
    }
}

int MainScene::findBuildingCenter(int r, int c) const
{
    for (size_t i = 0; i < _buildings.size(); ++i) {
        if (_buildings[i].r == r && _buildings[i].c == c) return (int)i;
    }
    return -1;
}

void MainScene::redrawOccupied()
{
    if (_occupied) _occupied->clear();
    for (const auto& b : _buildings) {
        if (b.id == 10) {
            drawCellFilled(b.r, b.c, Color4F(0.2f, 0.8f, 0.2f, 0.6f), _occupied);
        }
        else {
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc)
                    drawCellFilled(b.r + dr, b.c + dc, Color4F(0.2f, 0.8f, 0.2f, 0.6f), _occupied);
        }
    }
}

void MainScene::commitMove(int r, int c)
{
    auto& b = _buildings[_movingIndex];
    if (b.id == 10) {
        _occupy[b.r][b.c] = 0;
        _occupy[r][c] = b.id;
    }
    else {
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                _occupy[b.r + dr][b.c + dc] = 0;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                _occupy[r + dr][c + dc] = b.id;
    }
    Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
    int idx = std::max(1, std::min(9, b.id));
    Vec2 off = _buildingOffsetById[idx];
    if (b.sprite) b.sprite->setPosition(center + off);
    b.r = r; b.c = c;
    redrawOccupied();
    _saveDirty = true;
}

void MainScene::cancelMove()
{
    _moving = false;
    _movingIndex = -1;
    if (_hint) _hint->clear();
}
void MainScene::loadFromCurrentSaveOrCreate()
{
    int slot = SaveSystem::getCurrentSlot();
    if (slot < 0 || slot >= 20)
    {
        slot = 0;
        SaveSystem::setCurrentSlot(0);
    }

    SaveData data;
    bool loaded = SaveSystem::load(slot, data);
    if (!loaded)
    {
        std::string defName = StringUtils::format("Save %02d", slot + 1);
        data = SaveSystem::makeDefault(slot, defName);
        SaveSystem::save(data);
    }

    for (auto& b : _buildings)
    {
        if (b.sprite) b.sprite->removeFromParent();
    }
    _buildings.clear();
    _occupy.assign(_rows, std::vector<int>(_cols, 0));
    if (_occupied) _occupied->clear();

    ResourceManager::reset();
    _timeScale = data.timeScale > 0.0f ? data.timeScale : 1.0f;

    for (const auto& b : data.buildings)
    {
        placeBuildingLoaded(b.r, b.c, b);
    }

    if (_buildings.empty())
    {
        SaveBuilding th;
        th.id = 9;
        th.level = 1;
        th.r = _rows / 2;
        th.c = _cols / 2;
        th.hp = 100;
        th.stored = 0.0f;
        placeBuildingLoaded(th.r, th.c, th);
        data.buildings.clear();
        data.buildings.push_back(th);
        SaveSystem::save(data);
    }

    ResourceManager::setGold(data.gold);
    ResourceManager::setElixir(data.elixir);
    ResourceManager::setPopulation(data.population);

    _saveDirty = false;
    _autosaveTimer = 0.0f;
}

void MainScene::saveToCurrentSlot(bool force)
{
    int slot = SaveSystem::getCurrentSlot();
    if (slot < 0 || slot >= 20)
    {
        slot = 0;
        SaveSystem::setCurrentSlot(0);
    }

    if (!force && !_saveDirty) return;

    SaveData data;
    data.slot = slot;
    SaveData prev;
    if (SaveSystem::load(slot, prev)) data.name = prev.name;
    else data.name = StringUtils::format("Save %02d", slot + 1);

    data.gold = ResourceManager::getGold();
    data.elixir = ResourceManager::getElixir();
    data.population = ResourceManager::getPopulation();
    data.timeScale = _timeScale;

    for (const auto& b : _buildings)
    {
        SaveBuilding sb;
        sb.id = b.id;
        sb.r = b.r;
        sb.c = b.c;
        if (b.data)
        {
            sb.level = b.data->level;
            sb.hp = b.data->hp;
            sb.stored = b.data->stored;
        }
        data.buildings.push_back(sb);
    }

    if (SaveSystem::save(data))
    {
        _saveDirty = false;
    }
}

void MainScene::placeBuildingLoaded(int r, int c, const SaveBuilding& info)
{
    if (info.id <= 0) return;

    if (info.id == 10)
    {
        if (r < 0 || r >= _rows || c < 0 || c >= _cols) return;
        if (_occupy[r][c] != 0) return;
        _occupy[r][c] = info.id;
        drawCellFilled(r, c, Color4F(0.2f, 0.8f, 0.2f, 0.6f), _occupied);
    }
    else
    {
        if (r < 1 || r > _rows - 2 || c < 1 || c > _cols - 2) return;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                if (_occupy[r + dr][c + dc] != 0) return;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                _occupy[r + dr][c + dc] = info.id;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                drawCellFilled(r + dr, c + dc, Color4F(0.2f, 0.8f, 0.2f, 0.6f), _occupied);
    }

    auto b = BuildingFactory::create(info.id, std::max(1, info.level));
    if (!b) return;
    b->level = std::max(1, info.level);
    if (info.hp > 0) b->hp = std::min(info.hp, b->hpMax);
    b->stored = info.stored;
    auto s = b->createSprite();
    Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
    int idx = std::max(1, std::min(9, info.id));
    Vec2 off = _buildingOffsetById[idx];
    s->setPosition(center + off);
    s->setScale(_buildingScale * _buildingScaleById[idx]);
    _world->addChild(s, 3);
    std::shared_ptr<Building> ptr(b.release());
    _buildings.push_back({ info.id, r, c, s, ptr });
}

void MainScene::openAttackTargetPicker()
{
    if (_attackMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // Modal mask (swallow touches)
    _attackMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_attackMask, 250);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _attackMask);

    const float panelW = 640.0f;
    const float panelH = 520.0f;

    auto panel = LayerColor::create(Color4B(80, 80, 80, 235), panelW, panelH);
    panel->setIgnoreAnchorPointForPosition(false);
    panel->setAnchorPoint(Vec2(0, 0));
    panel->setPosition(origin.x + visibleSize.width / 2 - panelW / 2,
                       origin.y + visibleSize.height / 2 - panelH / 2);
    _attackMask->addChild(panel);

    _attackPanel = panel;

    auto title = Label::createWithSystemFont("Choose Target", "Arial", 48);
    title->setPosition(Vec2(panelW / 2, panelH - 50));
    panel->addChild(title);

    // Close button (X)
    auto closeLabel = Label::createWithSystemFont("X", "Arial", 44);
    closeLabel->setColor(Color3B::WHITE);
    auto closeItem = MenuItemLabel::create(closeLabel, [this](Ref*) {
        closeAttackTargetPicker();
    });
    closeItem->setPosition(Vec2(panelW - 30, panelH - 40));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu, 2);

    // Collect targets: all existing slots except current
    auto metasAll = SaveSystem::listAllSlots();
    std::vector<SaveMeta> targets;
    targets.reserve(metasAll.size());
    for (const auto& m : metasAll)
    {
        if (m.exists && m.slot != SaveSystem::getCurrentSlot())
            targets.push_back(m);
    }

    // ScrollView
    _attackScroll = ui::ScrollView::create();
    _attackScroll->setDirection(ui::ScrollView::Direction::VERTICAL);
    _attackScroll->setBounceEnabled(true);
    _attackScroll->setContentSize(Size(panelW - 40, panelH - 140));
    // NOTE: ui::ScrollView 默认忽略 anchorPoint（用左下角做定位）。
    // 这里直接用左下角定位，避免出现“偏到右边”的问题。
    _attackScroll->setAnchorPoint(Vec2::ZERO);
    _attackScroll->ignoreAnchorPointForPosition(true);
    _attackScroll->setPosition(Vec2(20, 60));
    _attackScroll->setScrollBarEnabled(true);
    panel->addChild(_attackScroll);

    _attackContent = Node::create();
    _attackContent->setAnchorPoint(Vec2(0, 0));
    _attackContent->setPosition(Vec2::ZERO);
    _attackScroll->addChild(_attackContent);

    if (targets.empty())
    {
        _attackScroll->setInnerContainerSize(_attackScroll->getContentSize());

        auto msg = Label::createWithSystemFont("No other saves to attack.", "Arial", 30);
        msg->setColor(Color3B::WHITE);
        msg->setPosition(Vec2(_attackScroll->getContentSize().width * 0.5f,
                              _attackScroll->getContentSize().height * 0.5f));
        _attackContent->addChild(msg);
    }
    else
    {
        const float rowH = 64.0f;
        float innerH = std::max(_attackScroll->getContentSize().height, rowH * targets.size());
        _attackScroll->setInnerContainerSize(Size(_attackScroll->getContentSize().width, innerH));

        for (size_t i = 0; i < targets.size(); ++i)
        {
            const auto& meta = targets[i];
            float y = innerH - rowH * (i + 1);

            auto rowBg = LayerColor::create(Color4B(120, 120, 120, 255),
                                            _attackScroll->getContentSize().width,
                                            rowH - 6);
            rowBg->setIgnoreAnchorPointForPosition(false);
            rowBg->setAnchorPoint(Vec2(0, 0));
            rowBg->setPosition(Vec2(0, y + 3));
            _attackContent->addChild(rowBg);

            std::string labelText = StringUtils::format("[%02d] %s", meta.slot + 1, meta.name.c_str());
            auto label = Label::createWithSystemFont(labelText, "Arial", 30);
            label->setAnchorPoint(Vec2(0.0f, 0.5f));
            label->setPosition(Vec2(16, (rowH - 6) * 0.5f));
            rowBg->addChild(label);

            auto attackLabel = Label::createWithSystemFont("Attack", "Arial", 26);
            auto attackItem = MenuItemLabel::create(attackLabel, [this, meta](Ref*) {
                SaveSystem::setBattleTargetSlot(meta.slot);
                closeAttackTargetPicker();
                auto scene = BattleScene::createScene();
                Director::getInstance()->replaceScene(TransitionFade::create(0.3f, scene));
            });
            attackItem->setPosition(Vec2(_attackScroll->getContentSize().width - 60, (rowH - 6) * 0.5f));
            auto rowMenu = Menu::create(attackItem, nullptr);
            rowMenu->setPosition(Vec2::ZERO);
            rowBg->addChild(rowMenu);
        }
    }

    // Mouse wheel support (Win32)
    if (!_attackMouseListener)
    {
        _attackMouseListener = EventListenerMouse::create();
        _attackMouseListener->onMouseScroll = [this](Event* e)
        {
            auto m = static_cast<EventMouse*>(e);
            if (!_attackMask || !_attackMask->isVisible() || !_attackScroll || !_attackPanel) return;

            Vec2 glPos = Director::getInstance()->convertToGL(Vec2(m->getCursorX(), m->getCursorY()));
            Vec2 local = _attackPanel->convertToNodeSpace(glPos);
            Rect r(0, 0, _attackPanel->getContentSize().width, _attackPanel->getContentSize().height);
            if (!r.containsPoint(local)) return;

            float dy = m->getScrollY() * 60.0f;
            Vec2 pos = _attackScroll->getInnerContainerPosition();
            pos.y += dy;

            float minY = _attackScroll->getContentSize().height - _attackScroll->getInnerContainerSize().height;
            if (minY > 0) minY = 0;

            if (pos.y > 0) pos.y = 0;
            if (pos.y < minY) pos.y = minY;

            _attackScroll->setInnerContainerPosition(pos);
            e->stopPropagation();
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(_attackMouseListener, _attackMask);
    }
}

void MainScene::closeAttackTargetPicker()
{
    if (_attackMask)
    {
        if (_attackMouseListener)
        {
            _eventDispatcher->removeEventListener(_attackMouseListener);
            _attackMouseListener = nullptr;
        }
        _attackPanel = nullptr;
        _attackScroll = nullptr;
        _attackContent = nullptr;
_attackMask->removeFromParent();
        _attackMask = nullptr;
    }
}