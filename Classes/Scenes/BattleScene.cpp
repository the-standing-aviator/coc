#include "Scenes/BattleScene.h"
#include "Scenes/MainScene.h"
#include "Scenes/LoginScene.h"
#include "Data/SaveSystem.h"
#include "GameObjects/Buildings/Building.h"
#include "Managers/SoundManager.h"

#include "ui/CocosGUI.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>
#include <cmath>

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

    setBuildingVisualParams();
    renderTargetVillage();

    // Battle HUD & countdown
    setupBattleHUD();
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
