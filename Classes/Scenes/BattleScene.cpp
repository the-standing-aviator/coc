#include "Scenes/BattleScene.h"
#include "Scenes/MainScene.h"
#include "Scenes/LoginScene.h"
#include "Data/SaveSystem.h"
#include "GameObjects/Buildings/Building.h"
#include "GameObjects/Buildings/TroopBuilding.h"
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
    _buildingScaleById.assign(10, 1.0f);
    _buildingOffsetById.assign(10, Vec2::ZERO);
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

    auto children = _world->getChildren();
    for (auto child : children)
    {
        if (child != _background) child->removeFromParent();
    }

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

    // Cache enemy village resources as the maximum lootable amount.
    _lootGoldTotal = data.gold;
    _lootElixirTotal = data.elixir;
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

    // Cache deployable diamond corners (node space of _world).
    _deployTop = top;
    _deployRight = right;
    _deployBottom = bottom;
    _deployLeft = left;
    _deployAreaReady = true;

    std::sort(data.buildings.begin(), data.buildings.end(), [](const SaveBuilding& a, const SaveBuilding& b) {
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
        int idx = std::max(1, std::min(9, bInfo.id));
        Vec2 off = _buildingOffsetById[idx];
        sprite->setPosition(pos + off);
        sprite->setScale(_buildingScale * _buildingScaleById[idx]);
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
        _background->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
        _world->addChild(_background, 0);
    }
    else
    {
        auto fallback = LayerColor::create(Color4B(50, 50, 50, 255));
        this->addChild(fallback, -1);
    }

    // Enable zoom (mouse wheel) and pan (mouse drag) for viewing the enemy village.
    _minZoom = 0.5f;
    _maxZoom = 2.5f;
    setZoom(1.0f);

    auto mouse = EventListenerMouse::create();
    mouse->onMouseScroll = [this](Event* e) {
        auto m = static_cast<EventMouse*>(e);
        float delta = m->getScrollY() * 0.10f;
        setZoom(_zoom + delta);
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
    touch->setSwallowTouches(false);
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

    // Battle HUD & countdown
    setupBattleHUD();
    setupTroopBar();
        _troopCounts = SaveSystem::getBattleReadyTroops();
    refreshTroopBar();
    startPhase(Phase::Scout, 45.0f);
    scheduleUpdate();

    // ESC menu toggle
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode code, Event*) {
        if (code == EventKeyboard::KeyCode::KEY_ESCAPE) {
            if (_settingsMask) {
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

    setupLootHUD();

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

void BattleScene::update(float dt)
{
    if (_phase == Phase::End)
        return;

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
            _phase = Phase::End;
            _phaseRemaining = 0.0f;
            updateBattleHUD();
            this->unscheduleUpdate();
            showReturnButton();
            return;
        }
    }

    updateBattleHUD();
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

        // Count label at the top-right corner (inside the icon).
        auto cntLbl = Label::createWithSystemFont("x" + std::to_string(count), "Arial", 18);
        cntLbl->setAnchorPoint(Vec2(1.0f, 1.0f));
        cntLbl->setColor(Color3B::WHITE);
        cntLbl->enableOutline(Color4B::BLACK, 2);
        cntLbl->setPosition(Vec2(isz.width - 2.0f, isz.height - 2.0f));
        slotRoot->addChild(cntLbl, 2);

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

    // Always test troop slots first. Troop icons may visually extend above the bar background.
    Vec2 local = _troopBar->convertToNodeSpace(glPos);
    for (const auto& slot : _troopSlots) {
        if (!slot.root) continue;
        Rect r = slot.root->getBoundingBox();
        if (r.containsPoint(local)) {
            if (_selectedTroopType == slot.type) setSelectedTroop(-1);
            else setSelectedTroop(slot.type);
            return true;
        }
    }

    // Consume clicks on the bar background so they do not deploy troops.
    Rect barRect = _troopBar->getBoundingBox();
    if (barRect.containsPoint(glPos)) return true;

    return false;
}

bool BattleScene::isPosInTroopBar(const cocos2d::Vec2& glPos) const
{
    if (!_troopBar) return false;

    // Test troop slots first. Icons may extend above the bar background.
    cocos2d::Vec2 local = _troopBar->convertToNodeSpace(glPos);
    for (const auto& slot : _troopSlots) {
        if (!slot.root) continue;
        cocos2d::Rect r = slot.root->getBoundingBox();
        if (r.containsPoint(local)) return true;
    }

    // Test bar background in its own local space (robust even if parent transforms).
    cocos2d::Size s = _troopBar->getContentSize();
    cocos2d::Rect bg(0, 0, s.width, s.height);
    return bg.containsPoint(local);
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
        DelayTime::create(2.0f),
        FadeOut::create(0.25f),
        RemoveSelf::create(),
        nullptr
    ));
}

void BattleScene::deploySelectedTroop(const cocos2d::Vec2& glPos)
{
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
        showBattleToast(std::string("\xE5\xB7\xB2\xE5\xAE\x8C\xE5\x85\xA8\xE9\x87\x8A\xE6\x94\xBE"));
        return;
    }

    bool deployed = false;

    // Spawn a simple sprite at the clicked position (world space).
    if (_world) {
        Vec2 worldLocal = _world->convertToNodeSpace(glPos);

        // Only allow deployment inside the main diamond area (map tiles).
        if (!isPosInDeployArea(worldLocal)) return;
        const char* path = nullptr;
        if (_selectedTroopType == (int)TrainingCamp::TROOP_BARBARIAN) path = "barbarian/barbarian_stand.png";
        else if (_selectedTroopType == (int)TrainingCamp::TROOP_ARCHER) path = "archor/archor_stand.png";
        else if (_selectedTroopType == (int)TrainingCamp::TROOP_GIANT) path = "giant/giant_stand.png";
        else if (_selectedTroopType == (int)TrainingCamp::TROOP_WALLBREAKER) path = "wall_breaker/wall_breaker_stand.png";
        else path = "barbarian/barbarian_stand.png";
        auto troop = Sprite::create(path);
        if (troop) {
            troop->setPosition(worldLocal);
            {
            float maxW = (_tileW > 0.0f) ? (_tileW * 0.55f) : 48.0f;
            float maxH = (_tileH > 0.0f) ? (_tileH * 0.55f) : 48.0f;
            Size cs = troop->getContentSize();
            float s1 = (cs.width > 0.0f) ? (maxW / cs.width) : 1.0f;
            float s2 = (cs.height > 0.0f) ? (maxH / cs.height) : 1.0f;
            float s = std::min(s1, s2);
            s = std::min(s, 1.0f);
            s = std::max(s, 0.05f);
            troop->setScale(s);
        }
            _world->addChild(troop, 20);
            deployed = true;
        }
    }

    if (!deployed) return;

    // Play deploy SFX (Resources/music).
    if (_selectedTroopType == (int)TrainingCamp::TROOP_ARCHER) {
        SoundManager::playSfx("music/archer_deploy_09.ogg", 1.0f);
    }
    else if (_selectedTroopType == (int)TrainingCamp::TROOP_GIANT) {
        SoundManager::playSfx("music/giant_deploy_04v3.ogg", 1.0f);
    }
    else if (_selectedTroopType == (int)TrainingCamp::TROOP_WALLBREAKER) {
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

