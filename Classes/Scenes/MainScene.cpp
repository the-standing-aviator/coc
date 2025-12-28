// File: MainScene.cpp
// Brief: Implements the MainScene component.
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
#include <vector>
#include <ctime>

namespace {
    static float randomRealSafe(float a, float b)
    {
        if (std::isnan(a) || std::isnan(b)) return 0.0f;
        if (a > b) std::swap(a, b);
        if (a == b) return a;
        return cocos2d::RandomHelper::random_real(a, b);
    }

    static void playPickupSfxForBuildingId(int buildId)
    {
        switch (buildId)
        {
        case 1:
            SoundManager::playSfxRandom("archer_tower_pick", 1.0f);
            break;
        case 2:
            SoundManager::playSfxRandom("cannon_pick", 1.0f);
            break;
        case 3:
            SoundManager::playSfxRandom("elixir_pump_pick", 1.0f);
            break;
        case 4:
            SoundManager::playSfxRandom("elixir_storage_pick", 1.0f);
            break;
        case 5:
            SoundManager::playSfxRandom("goldmine_pick", 1.0f);
            break;
        default:
            SoundManager::playSfxRandom("building_pickup", 1.0f);
            break;
        }
    }

    static void playPlaceSfxForBuildingId(int buildId)
    {
        switch (buildId)
        {
        case 1:
            SoundManager::playSfxRandom("archer_tower_place", 1.0f);
            break;
        case 2:
            SoundManager::playSfxRandom("cannon_drop", 1.0f);
            break;
        case 3:
            SoundManager::playSfxRandom("elixir_pump_drop", 1.0f);
            break;
        case 4:
            SoundManager::playSfxRandom("elixir_storage_drop", 1.0f);
            break;
        case 5:
            SoundManager::playSfxRandom("goldmine_drop", 1.0f);
            break;
        case 6:
            SoundManager::playSfxRandom("gold_storage_drop", 1.0f);
            break;
        default:
            SoundManager::playSfxRandom("building_place", 1.0f);
            break;
        }
    }
}

#include "ui/CocosGUI.h"

#ifdef _WIN32
#include <windows.h>
#include <cstdio>

namespace {


    static cocos2d::Node* safeCreateNode() {
        auto n = cocos2d::Node::create();
        if (n) return n;


        cocos2d::Node* raw = new (std::nothrow) cocos2d::Node();
        if (raw && raw->init()) {
            raw->autorelease();
            return raw;
        }
        CC_SAFE_DELETE(raw);
        return nullptr;
    }
}

#endif
using namespace cocos2d;





static bool CanResolveResource(const std::string& relPath)
{
    auto fu = FileUtils::getInstance();
    const std::string full = fu->fullPathForFilename(relPath);
    return !full.empty();
}

static std::string FallbackResourcePath(const std::string& relPath)
{

    if (CanResolveResource(relPath)) return relPath;


    const std::string tries[] = {
        std::string("Resources/") + relPath,
        std::string("resources/") + relPath,
        std::string("../Resources/") + relPath,
        std::string("../../Resources/") + relPath,
    };
    for (const auto& p : tries) {
        if (CanResolveResource(p)) return p;
    }
    return relPath;
}


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
    _backgroundOptions = {
    "backgrounds/village_map.jpg",
    "backgrounds/village_map_alt.jpg"
    };
    _backgroundIndex = 0;
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


    _standTroopLayer = Node::create();
    _world->addChild(_standTroopLayer, 4);
    _occupy.assign(_rows, std::vector<int>(_cols, 0));

    _buildingScaleById.assign(12, 1.0f);
    _buildingOffsetById.assign(12, Vec2::ZERO);

    setupInteraction();

    SoundManager::play("music/home_music_part_1.ogg", true, 0.6f);

    auto changeBgLabel = Label::createWithSystemFont("Change Background", "Arial", 20);
    changeBgLabel->setColor(Color3B::WHITE);
    auto changeBgItem = MenuItemLabel::create(changeBgLabel, [this, origin, visibleSize](Ref*) {
        if (!_background || _backgroundOptions.empty()) return;
        _backgroundIndex = (_backgroundIndex + 1) % _backgroundOptions.size();
        const auto& nextPath = _backgroundOptions[_backgroundIndex];
        auto texture = Director::getInstance()->getTextureCache()->addImage(nextPath);
        if (!texture) return;
        _background->setTexture(texture);
        _background->setTextureRect(Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height));
        Size bgSize = _background->getContentSize();
        float scale = std::max(visibleSize.width / bgSize.width, visibleSize.height / bgSize.height);
        _background->setScale(scale);
        _background->setPosition(Vec2(origin.x + visibleSize.width / 2, origin.y + visibleSize.height / 2));
        });
    changeBgItem->setAnchorPoint(Vec2(0.0f, 1.0f));
    changeBgItem->setPosition(Vec2(origin.x + 12.0f, origin.y + visibleSize.height - 12.0f));
    auto changeBgMenu = Menu::create(changeBgItem, nullptr);
    changeBgMenu->setPosition(Vec2::ZERO);
    this->addChild(changeBgMenu, 30);


    auto btn = BuildingButton::create();
    btn->setButtonScale(0.15f);
    btn->setButtonOffset(Vec2(16.f, 16.f));
    btn->onClicked = [this]() {
        SoundManager::playSfxRandom("button_click", 1.0f);
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




                int buttonIdx = idx;
                if (idx == 10) buttonIdx = 11;

                std::string rawPath = StringUtils::format("ui/build_button%d.png", buttonIdx);
                std::string path = FallbackResourcePath(rawPath);


                const int buildId = (idx == 9 ? 10 : (idx == 10 ? 11 : idx));

                MenuItem* item = nullptr;






                auto makeSprite = [](const std::string& p) -> Sprite* {
                    auto sp = Sprite::create(p);
                    if (!sp) return nullptr;

                    if (!sp->getTexture()) return nullptr;
                    Size s = sp->getContentSize();
                    if (s.width <= 0.0f || s.height <= 0.0f) return nullptr;
                    return sp;
                    };

                std::vector<std::string> candidates;
                candidates.push_back(rawPath);

                if (path != rawPath) candidates.push_back(path);

                candidates.push_back(std::string("Resources/") + rawPath);
                candidates.push_back(std::string("../Resources/") + rawPath);

                Sprite* normalSp = nullptr;
                std::string picked;
                for (const auto& p : candidates) {
                    normalSp = makeSprite(p);
                    if (normalSp) { picked = p; break; }
                }

                if (normalSp) {

                    auto selectedSp = makeSprite(picked);
                    if (!selectedSp) {

                        selectedSp = Sprite::createWithTexture(normalSp->getTexture());
                        selectedSp->setTextureRect(Rect(0, 0, normalSp->getContentSize().width, normalSp->getContentSize().height));
                    }
                    auto spriteItem = MenuItemSprite::create(normalSp, selectedSp, [this, buildId, panel](Ref*) {
                        SoundManager::playSfxRandom("button_click", 1.0f);
                        startPlacement(buildId);
                        panel->removeFromParent();
                        });
                    Size s = normalSp->getContentSize();
                    float sx = std::min(cw / s.width, ch / s.height);
                    spriteItem->setScale(sx);
                    item = spriteItem;
                }
                else {

                    auto label = Label::createWithSystemFont(StringUtils::format("ID %d", buttonIdx), "Arial", 18);
                    label->setColor(Color3B::BLACK);
                    auto fallbackItem = MenuItemLabel::create(label, [this, buildId, panel](Ref*) {
                        SoundManager::playSfxRandom("button_click", 1.0f);
                        startPlacement(buildId);
                        panel->removeFromParent();
                        });
                    item = fallbackItem;
                }


                const int limit = buildLimitForId(buildId);
                if (countById(buildId) >= limit) {
                    item->setEnabled(false);
                    if (auto ms = dynamic_cast<MenuItemSprite*>(item)) {
                        if (auto normal = ms->getNormalImage()) normal->setColor(Color3B(150, 150, 150));
                        if (auto selected = ms->getSelectedImage()) selected->setColor(Color3B(150, 150, 150));
                    }
                    else if (auto ml = dynamic_cast<MenuItemLabel*>(item)) {
                        if (auto lab = dynamic_cast<Label*>(ml->getLabel())) {
                            lab->setColor(Color3B(150, 150, 150));
                        }
                    }
                }

                item->setPosition(Vec2(x + cw * 0.5f, y + ch * 0.5f));
                menu->addChild(item);
            }
        }
        auto closeLabel = Label::createWithSystemFont("X", "Arial", 28);
        closeLabel->setColor(Color3B::BLACK);
        auto closeItem = MenuItemLabel::create(closeLabel, [panel](Ref*) {
            SoundManager::playSfxRandom("button_click", 1.0f);
            panel->removeFromParent();
            });
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

    for (size_t i = 0; i < _buildings.size(); ++i) {
        auto& pb = _buildings[i];
        if (!pb.data) continue;
        if (auto barracks = dynamic_cast<Barracks*>(pb.data.get())) {
            barracks->applyCap();
        }
    }
    return true;
}

void MainScene::openEscMenu()
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

void MainScene::showMainToast(const std::string& msg)
{
    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto label = Label::createWithSystemFont(msg, "Arial", 24);
    label->setColor(Color3B(200, 30, 30));
    label->enableOutline(Color4B::BLACK, 2);
    label->setPosition(Vec2(origin.x + vs.width * 0.5f, origin.y + vs.height * 0.6f));

    this->addChild(label, 5000);

    label->runAction(Sequence::create(
        DelayTime::create(2.0f),
        FadeOut::create(0.25f),
        RemoveSelf::create(),
        nullptr
    ));
}

int MainScene::getActiveBuilderCount() const
{
    int count = 0;
    for (const auto& pb : _buildings) {
        if (!pb.data) continue;

        if (pb.id == 10) continue;
        if (pb.data->buildState != Building::STATE_NORMAL && pb.data->buildTotalSec > 0.0f && pb.data->buildRemainSec > 0.0f) {
            count++;
        }
    }
    return count;
}

static const int kBuildUiTag = 2025122701;
static const int kBuildUiLabelTag = 2025122702;
static const int kBuildUiFillTag = 2025122703;
static const int kBuildUiBgTag = 2025122704;

void MainScene::attachBuildTimerUI(cocos2d::Sprite* sprite, float totalSec, float remainSec)
{
    if (!sprite) return;
    removeBuildTimerUI(sprite);

    auto ui = Node::create();
    ui->setTag(kBuildUiTag);


    auto label = Label::createWithSystemFont("00:00", "Arial", 26);
    label->setTag(kBuildUiLabelTag);
    label->setColor(Color3B::BLACK);
    label->enableOutline(Color4B::BLACK, 3);
    ui->addChild(label, 2);




    const float w = 72.0f;
    const float h = 8.0f;
    auto bg = LayerColor::create(Color4B(30, 30, 30, 200), w, h);
    bg->setTag(kBuildUiBgTag);


    bg->setIgnoreAnchorPointForPosition(false);
    bg->setAnchorPoint(Vec2(0.5f, 0.5f));
    bg->setPosition(Vec2(0.f, -12.f));
    ui->addChild(bg, 0);

    auto fill = LayerColor::create(Color4B(60, 200, 60, 220), w, h);
    fill->setTag(kBuildUiFillTag);
    fill->setIgnoreAnchorPointForPosition(false);
    fill->setAnchorPoint(Vec2(0.0f, 0.5f));
    fill->setPosition(Vec2(-w * 0.5f, -12.f));
    ui->addChild(fill, 1);


    ui->setPosition(Vec2(sprite->getContentSize().width * 0.5f, sprite->getContentSize().height + 20.f));
    sprite->addChild(ui, 200);

    updateBuildTimerUI(sprite, totalSec, remainSec);
}

void MainScene::updateBuildTimerUI(cocos2d::Sprite* sprite, float totalSec, float remainSec)
{
    if (!sprite) return;
    auto ui = sprite->getChildByTag(kBuildUiTag);
    if (!ui) return;

    int sec = (int)std::ceil(std::max(0.0f, remainSec));
    int mm = sec / 60;
    int ss = sec % 60;
    auto label = dynamic_cast<Label*>(ui->getChildByTag(kBuildUiLabelTag));
    if (label) {
        label->setString(StringUtils::format("%02d:%02d", mm, ss));
        label->setPosition(Vec2(0.f, 6.f));
    }

    float ratio = 0.0f;
    if (totalSec > 0.0f) ratio = 1.0f - std::max(0.0f, std::min(1.0f, remainSec / totalSec));
    auto bg = dynamic_cast<LayerColor*>(ui->getChildByTag(kBuildUiBgTag));
    auto fill = dynamic_cast<LayerColor*>(ui->getChildByTag(kBuildUiFillTag));
    if (bg && fill) {
        float w = bg->getContentSize().width;
        float h = bg->getContentSize().height;
        float clamped = std::max(0.0f, std::min(1.0f, ratio));
        fill->setContentSize(Size(w * clamped, h));

        fill->setPosition(Vec2(-w * 0.5f, bg->getPositionY()));
    }
}

void MainScene::removeBuildTimerUI(cocos2d::Sprite* sprite)
{
    if (!sprite) return;
    if (auto ui = sprite->getChildByTag(kBuildUiTag)) {
        ui->removeFromParent();
    }
}

void MainScene::updateBuildSystems(float dt)
{
    bool anyFinished = false;
    for (auto& pb : _buildings) {
        if (!pb.data || !pb.sprite) continue;
        if (pb.data->buildState == Building::STATE_NORMAL) {

            removeBuildTimerUI(pb.sprite);
            continue;
        }
        if (pb.data->buildTotalSec <= 0.0f || pb.data->buildRemainSec <= 0.0f) {

            pb.data->buildRemainSec = 0.0f;
        }
        else {
            pb.data->buildRemainSec -= dt * _timeScale;
            if (pb.data->buildRemainSec < 0.0f) pb.data->buildRemainSec = 0.0f;
        }

        if (pb.data->buildRemainSec <= 0.0f) {

            SoundManager::playSfxRandom("building_finished", 1.0f);
            if (pb.data->buildState == Building::STATE_UPGRADING && pb.data->upgradeTargetLevel > 0) {
                pb.data->level = pb.data->upgradeTargetLevel;
            }

            pb.data->buildState = Building::STATE_NORMAL;
            pb.data->buildTotalSec = 0.0f;
            pb.data->buildRemainSec = 0.0f;
            pb.data->upgradeTargetLevel = 0;


            BuildingFactory::applyStats(pb.data.get(), pb.id, pb.data->level, true, false);



            if (pb.sprite) {
                pb.sprite->removeFromParent();
                pb.sprite = nullptr;
            }
            {
                auto s = pb.data->createSprite();
                cocos2d::Vec2 center(_anchor.x + (pb.c - pb.r) * (_tileW * 0.5f),
                    _anchor.y - (pb.c + pb.r) * (_tileH * 0.5f));
                int idx = std::max(1, std::min(11, pb.id));
                cocos2d::Vec2 off = _buildingOffsetById[idx];
                s->setPosition(center + off);
                s->setScale(_buildingScale * _buildingScaleById[idx]);
                _world->addChild(s, 3);
                pb.sprite = s;
            }


            if (pb.sprite) {
                auto lvl = dynamic_cast<cocos2d::Label*>(pb.sprite->getChildByTag(20251225));
                if (lvl) lvl->setString(cocos2d::StringUtils::format("level%d", pb.data->level));
                pb.sprite->setOpacity(255);
                removeBuildTimerUI(pb.sprite);
            }

            anyFinished = true;
            _saveDirty = true;
        }
        else {

            if (!pb.sprite->getChildByTag(kBuildUiTag)) {
                attachBuildTimerUI(pb.sprite, pb.data->buildTotalSec, pb.data->buildRemainSec);
            }
            updateBuildTimerUI(pb.sprite, pb.data->buildTotalSec, pb.data->buildRemainSec);
            pb.sprite->setOpacity(160);
        }
    }

    if (anyFinished) {

        ResourceManager::setGold(ResourceManager::getGold());
        ResourceManager::setElixir(ResourceManager::getElixir());
        ResourceManager::setPopulation(ResourceManager::getPopulation());
    }
}

void MainScene::update(float dt)
{

    updateBuildSystems(dt);


    updateResearchSystems(dt);

    for (size_t i = 0; i < _buildings.size(); ++i) {
        auto& pb = _buildings[i];
        if (!pb.data) continue;


        if (!pb.data->isFunctional()) {

            if (auto ec = dynamic_cast<ElixirCollector*>(pb.data.get())) {
                if (ec->promptLabel) {
                    ec->promptLabel->removeFromParent();
                    ec->promptLabel = nullptr;
                }
            }
            if (auto gm = dynamic_cast<GoldMine*>(pb.data.get())) {
                if (gm->promptLabel) {
                    gm->promptLabel->removeFromParent();
                    gm->promptLabel = nullptr;
                }
            }
            continue;
        }

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



    if (_trainMask && _trainCampIndex >= 0 && _trainCampIndex < (int)_buildings.size()) {
        auto& pb = _buildings[_trainCampIndex];
        if (pb.data) {
            if (auto tc = dynamic_cast<TrainingCamp*>(pb.data.get())) {

                int sig = 0;
                const auto& ready = tc->getAllReadyCounts();
                for (const auto& kv : ready) {
                    sig = sig * 31 + kv.first;
                    sig = sig * 31 + kv.second;
                }


                if (sig != _trainLastSig) {
                    _trainLastSig = sig;
                    refreshTrainingCampUI();
                }

                if (_trainCapLabel)
                {
                    int totalCap = ResourceManager::getPopulationCap();
                    int usedHousing = tc->getUsedHousing();
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%d / %d", usedHousing, totalCap);
                    _trainCapLabel->setString(buf);
                }
            }
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
    if (!b.data) return;




    int curLv = b.data->level;
    bool isMax = (curLv >= ConfigManager::getMaxLevel());

    int nextLv = std::min(ConfigManager::getMaxLevel(), curLv + 1);
    auto cost = ConfigManager::getUpgradeCost(b.id, nextLv);
    int upgradeTime = ConfigManager::getBuildTimeSec(b.id, nextLv);

    std::string title = cocos2d::StringUtils::format("%s (Level %d)", ConfigManager::getBuildingName(b.id).c_str(), curLv);
    std::string resName = cost.useGold ? "Gold" : "Elixir";

    auto modal = CustomButton::createUpgradePanel(title, resName, cost.amount, false, isMax,
        [this, idx, nextLv, cost, upgradeTime]() -> bool {
            if (idx < 0 || idx >= (int)_buildings.size()) return true;
            auto& pb = _buildings[idx];
            if (!pb.data) return true;


            if (pb.data->level >= ConfigManager::getMaxLevel()) {
                showMainToast("Already Max Level");
                return false;
            }


            if (pb.data->buildState != Building::STATE_NORMAL && pb.data->buildRemainSec > 0.0f) {
                showMainToast("Not enough builders.");
                return false;
            }


            int th = getTownHallLevel();
            int maxBk2 = 0;
            for (const auto& it : _buildings) {
                if (it.id == 7 && it.data) {
                    maxBk2 = std::max(maxBk2, it.data->level);
                }
            }
            if (!ConfigManager::isUpgradeAllowed(pb.id, nextLv, th, maxBk2)) {
                showMainToast("Town Hall level too low.");
                return false;
            }


            if (upgradeTime > 0 && getActiveBuilderCount() >= 2) {
                showMainToast("Not enough builders.");
                return false;
            }


            if (cost.amount > 0) {
                if (cost.useGold) {
                    if (ResourceManager::getGold() < cost.amount) {
                        showMainToast("Not enough gold.");
                        return false;
                    }
                }
                else {
                    if (ResourceManager::getElixir() < cost.amount) {
                        showMainToast("Not enough elixir.");
                        return false;
                    }
                }
            }

            bool ok = false;
            if (cost.amount > 0) {
                ok = cost.useGold ? ResourceManager::spendGold(cost.amount)
                    : ResourceManager::spendElixir(cost.amount);
            }
            else {
                ok = true;
            }
            if (!ok) {

                showMainToast(cost.useGold ? "Not enough gold." : "Not enough elixir.");
                return false;
            }

            SoundManager::playSfx("music/building_construct_07.ogg", 1.0f);


            if (upgradeTime <= 0) {
                pb.data->level = nextLv;


                BuildingFactory::applyStats(pb.data.get(), pb.id, pb.data->level, true, false);



                if (pb.sprite) {
                    pb.sprite->removeFromParent();
                    pb.sprite = nullptr;
                }
                if (_world) {
                    auto s = pb.data->createSprite();
                    cocos2d::Vec2 center(_anchor.x + (pb.c - pb.r) * (_tileW * 0.5f),
                        _anchor.y - (pb.c + pb.r) * (_tileH * 0.5f));
                    int idx2 = std::max(1, std::min(11, pb.id));
                    cocos2d::Vec2 off = _buildingOffsetById[idx2];
                    s->setPosition(center + off);
                    s->setScale(_buildingScale * _buildingScaleById[idx2]);
                    _world->addChild(s, 3);
                    pb.sprite = s;
                }

                _saveDirty = true;
                return true;
            }


            pb.data->buildState = Building::STATE_UPGRADING;
            pb.data->buildTotalSec = (float)upgradeTime;
            pb.data->buildRemainSec = (float)upgradeTime;
            pb.data->upgradeTargetLevel = nextLv;
            if (pb.sprite) {
                pb.sprite->setOpacity(160);
                attachBuildTimerUI(pb.sprite, pb.data->buildTotalSec, pb.data->buildRemainSec);
            }
            _saveDirty = true;
            return true;
        },
        []() {}
    );



    if (b.id == 8) {
        auto inner = modal ? modal->getChildByName("__inner_upgrade_panel") : nullptr;
        auto panel = dynamic_cast<cocos2d::LayerColor*>(inner);

        auto trainLabel = Label::createWithSystemFont("Train", "Arial", 22);
        trainLabel->setColor(Color3B::BLACK);
        auto trainItem = MenuItemLabel::create(trainLabel, [this, idx, modal](Ref*) {
            if (modal) modal->removeFromParent();

            if (idx >= 0 && idx < (int)_buildings.size() && _buildings[idx].data) {
                auto bd = _buildings[idx].data;
                if (bd->buildState != Building::STATE_NORMAL && bd->buildRemainSec > 0.0f) {
                    showMainToast("Not enough builders.");
                    return;
                }
            }
            openTrainingCampPicker(idx);
            });

        float pw = panel ? panel->getContentSize().width : (modal ? modal->getContentSize().width : 0.0f);
        trainItem->setPosition(Vec2(pw * 0.5f, 80.f));
        auto trainMenu = Menu::create(trainItem, nullptr);
        trainMenu->setPosition(Vec2::ZERO);
        if (panel) panel->addChild(trainMenu, 4);
        else if (modal) modal->addChild(trainMenu, 4);
    }


    if (b.id == 11) {
        auto inner = modal ? modal->getChildByName("__inner_upgrade_panel") : nullptr;
        auto panel = dynamic_cast<cocos2d::LayerColor*>(inner);

        auto researchLabel = Label::createWithSystemFont("Research", "Arial", 22);
        researchLabel->setColor(Color3B::BLACK);
        auto researchItem = MenuItemLabel::create(researchLabel, [this, idx, modal](Ref*) {
            if (modal) modal->removeFromParent();


            if (idx >= 0 && idx < (int)_buildings.size() && _buildings[idx].data) {
                auto bd = _buildings[idx].data;
                if (bd->buildState != Building::STATE_NORMAL && bd->buildRemainSec > 0.0f) {
                    showMainToast("Laboratory is not ready.");
                    return;
                }
            }

            openLaboratoryResearchPicker(idx);
            });

        float pw = panel ? panel->getContentSize().width : (modal ? modal->getContentSize().width : 0.0f);
        researchItem->setPosition(Vec2(pw * 0.5f, 50.f));
        auto researchMenu = Menu::create(researchItem, nullptr);
        researchMenu->setPosition(Vec2::ZERO);
        if (panel) panel->addChild(researchMenu, 4);
        else if (modal) modal->addChild(researchMenu, 4);
    }



    this->addChild(modal, 1000);
}



void MainScene::setupInteraction()
{


    auto isUIBlocked = [this]() -> bool {
        if (_escMask && _escMask->isVisible()) return true;
        if (_settingsMask && _settingsMask->isVisible()) return true;
        if (_attackMask && _attackMask->isVisible()) return true;
        if (_trainMask && _trainMask->isVisible()) return true;
        if (_researchMask && _researchMask->isVisible()) return true;

        if (this->getChildByName("__modal_mask__")) return true;
        return false;
        };

    auto mouse = EventListenerMouse::create();
    mouse->onMouseScroll = [this, isUIBlocked](Event* e) {
        auto ev = static_cast<EventMouse*>(e);
        if (isUIBlocked()) return;

        if (_attackMask && _attackMask->isVisible()) return;
        float delta = ev->getScrollY();
        float k = 1.1f;
        if (delta > 0) setZoom(_zoom / k);
        else if (delta < 0) setZoom(_zoom * k);
        };
    mouse->onMouseDown = [this, isUIBlocked](Event* e) {
        auto ev = static_cast<EventMouse*>(e);
        if (isUIBlocked()) return;
        if (ev->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            Vec2 cur(ev->getCursorX(), ev->getCursorY());



            if (!_placing && !_moving && _world) {
                Vec2 local = _world->convertToNodeSpace(cur);
                for (size_t i = 0; i < _buildings.size(); ++i) {
                    auto& pb = _buildings[i];
                    if (!pb.data || !pb.sprite) continue;
                    if (!pb.data->isFunctional()) continue;

                    if (auto ec = dynamic_cast<ElixirCollector*>(pb.data.get())) {
                        if (ec->promptLabel) {
                            cocos2d::Rect r = ec->promptLabel->getBoundingBox();

                            r.origin -= Vec2(6.f, 4.f);
                            r.size.width += 12.f;
                            r.size.height += 8.f;
                            if (r.containsPoint(local) && ec->canCollect()) {
                                int deliver = ec->collect();
                                if (deliver > 0) {
                                    SoundManager::playSfx("music/elixir_pump_pickup_07.ogg", 1.0f);
                                    _saveDirty = true;
                                }
                                ec->manageCollectPrompt(_world, pb.sprite);
                                return;
                            }
                        }
                    }
                    else if (auto gm = dynamic_cast<GoldMine*>(pb.data.get())) {
                        if (gm->promptLabel) {
                            cocos2d::Rect r = gm->promptLabel->getBoundingBox();
                            r.origin -= Vec2(6.f, 4.f);
                            r.size.width += 12.f;
                            r.size.height += 8.f;
                            if (r.containsPoint(local) && gm->canCollect()) {
                                int deliver = gm->collect();
                                if (deliver > 0) {
                                    SoundManager::playSfx("music/goldmine_pickup4.ogg", 1.0f);
                                    _saveDirty = true;
                                }
                                gm->manageCollectPrompt(_world, pb.sprite);
                                return;
                            }
                        }
                    }
                }
            }

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
                            if (pb.data->isFunctional() && canCollect) {
                                int totalDeliver = 0;
                                for (auto& it : _buildings) {
                                    if (!it.data) continue;
                                    if (!it.data->isFunctional()) continue;
                                    if (auto ec2 = dynamic_cast<ElixirCollector*>(it.data.get())) {


                                        totalDeliver += ec2->collect(true);
                                    }
                                }
                                if (totalDeliver > 0) {
                                    SoundManager::playSfx("music/elixir_pump_pickup_07.ogg", 1.0f);
                                    _saveDirty = true;
                                }
                            }
                            else {
                                playPickupSfxForBuildingId(pb.id);
                                _moving = true;
                                _movingIndex = idx;
                                _hint->clear();
                            }
                        }
                        else if (auto gm = dynamic_cast<GoldMine*>(pb.data.get())) {
                            bool canCollect = gm->canCollect();
                            if (pb.data->isFunctional() && canCollect) {
                                int deliver = gm->collect();
                                if (deliver > 0) {
                                    SoundManager::playSfx("music/goldmine_pickup4.ogg", 1.0f);
                                    _saveDirty = true;
                                }
                            }
                            else {
                                playPickupSfxForBuildingId(pb.id);
                                _moving = true;
                                _movingIndex = idx;
                                _hint->clear();
                            }
                        }
                        else {
                            playPickupSfxForBuildingId(pb.id);
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
    mouse->onMouseUp = [this, isUIBlocked](Event* e) {
        auto ev = static_cast<EventMouse*>(e);
        if (isUIBlocked()) return;
        if (ev->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT) {
            _dragging = false;
        }
        };
    mouse->onMouseMove = [this, isUIBlocked](Event* e) {
        auto ev = static_cast<EventMouse*>(e);
        if (isUIBlocked()) return;
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
    if (id >= 1 && id <= 11 && countById(id) >= buildLimitForId(id)) {
        return;
    }


    {
        int th = getTownHallLevel();
        int maxBk = 0;
        for (const auto& it : _buildings) {
            if (it.id == 7 && it.data) maxBk = std::max(maxBk, it.data->level);
        }
        if (!ConfigManager::isUpgradeAllowed(id, 1, th, maxBk)) {
            showMainToast("Town Hall level too low.");
            return;
        }
    }


    int buildTime = ConfigManager::getBuildTimeSec(id, 1);
    if (buildTime > 0 && getActiveBuilderCount() >= 2) {
        showMainToast("Not enough builders.");
        return;
    }

    auto buildCost = ConfigManager::getBuildCost(id);

    if (buildCost.amount > 0) {
        if (buildCost.useGold) {
            if (ResourceManager::getGold() < buildCost.amount) {
                showMainToast("Not enough gold.");
                return;
            }
        }
        else {
            if (ResourceManager::getElixir() < buildCost.amount) {
                showMainToast("Not enough elixir.");
                return;
            }
        }
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


    bool applyCapsNow = (buildTime <= 0);
    auto b = BuildingFactory::create(id, 1, applyCapsNow, true);
    auto s = b->createSprite();
    Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
    int idx = std::max(1, std::min(11, id));
    Vec2 off = _buildingOffsetById[idx];
    s->setPosition(center + off);
    s->setScale(_buildingScale * _buildingScaleById[idx]);
    _world->addChild(s, 3);


    if (buildTime > 0) {
        b->buildState = Building::STATE_CONSTRUCTING;
        b->buildTotalSec = (float)buildTime;
        b->buildRemainSec = (float)buildTime;
        b->upgradeTargetLevel = 0;
        if (s) s->setOpacity(160);
        attachBuildTimerUI(s, b->buildTotalSec, b->buildRemainSec);
    }
    else {
        b->buildState = Building::STATE_NORMAL;
        b->buildTotalSec = 0.0f;
        b->buildRemainSec = 0.0f;
        b->upgradeTargetLevel = 0;
    }


    if (buildCost.amount > 0) {
        bool ok = buildCost.useGold ? ResourceManager::spendGold(buildCost.amount)
            : ResourceManager::spendElixir(buildCost.amount);
        if (!ok) {

            if (s) s->removeFromParent();
            return;
        }
    }

    std::shared_ptr<Building> ptr(b.release());
    _buildings.push_back({ id, r, c, s, ptr });


    playPlaceSfxForBuildingId(id);

    _saveDirty = true;
}

void MainScene::setBuildingScale(float s)
{
    _buildingScale = std::max(0.1f, std::min(3.0f, s));
}

void MainScene::setBuildingScaleForId(int id, float s)
{
    if (id == 0) id = 9;
    if (id < 1 || id >= (int)_buildingScaleById.size()) return;
    _buildingScaleById[id] = std::max(0.1f, std::min(3.0f, s));
}

void MainScene::setBuildingOffsetForId(int id, const Vec2& off)
{
    if (id == 0) id = 9;
    if (id < 1 || id >= (int)_buildingOffsetById.size()) return;
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
    int idx = std::max(1, std::min(11, b.id));
    Vec2 off = _buildingOffsetById[idx];
    if (b.sprite) b.sprite->setPosition(center + off);
    b.r = r; b.c = c;
    redrawOccupied();


    playPlaceSfxForBuildingId(b.id);

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




    {
        const int64_t now = static_cast<int64_t>(std::time(nullptr));
        const int64_t last = data.lastRealTime;
        if (last > 0 && now > last) {
            const double elapsed = static_cast<double>(now - last);


            for (auto& b : data.buildings) {
                if (b.buildState != Building::STATE_NORMAL && b.buildTotalSec > 0.0f && b.buildRemainSec > 0.0f) {
                    b.buildRemainSec = std::max(0.0f, b.buildRemainSec - (float)elapsed);
                    if (b.buildRemainSec <= 0.0f) {

                        if (b.buildState == Building::STATE_UPGRADING && b.upgradeTargetLevel > 0) {
                            b.level = b.upgradeTargetLevel;
                        }
                        b.buildState = Building::STATE_NORMAL;
                        b.buildTotalSec = 0.0f;
                        b.buildRemainSec = 0.0f;
                        b.upgradeTargetLevel = 0;
                    }
                }
            }


            for (auto& b : data.buildings) {
                if (b.buildState != Building::STATE_NORMAL) continue;
                if (b.id == 3) {
                    auto st = ConfigManager::getElixirCollectorStats(std::max(1, b.level));
                    float add = static_cast<float>(st.ratePerHour * (elapsed / 3600.0));
                    b.stored = std::min((float)st.capacity, b.stored + add);
                }
                else if (b.id == 5) {
                    auto st = ConfigManager::getGoldMineStats(std::max(1, b.level));
                    float add = static_cast<float>(st.ratePerHour * (elapsed / 3600.0));
                    b.stored = std::min((float)st.capacity, b.stored + add);
                }
            }


            if (data.researchUnitId > 0 && data.researchRemainSec > 0.0f && data.researchTotalSec > 0.0f) {
                data.researchRemainSec = std::max(0.0f, data.researchRemainSec - (float)elapsed);
                if (data.researchRemainSec <= 0.0f) {

                    int cur = 1;
                    auto it = data.troopLevels.find(data.researchUnitId);
                    if (it != data.troopLevels.end()) cur = it->second;
                    int newLv = std::max(cur, data.researchTargetLevel);
                    data.troopLevels[data.researchUnitId] = newLv;
                    data.researchUnitId = 0;
                    data.researchTargetLevel = 0;
                    data.researchTotalSec = 0.0f;
                    data.researchRemainSec = 0.0f;
                }
            }
        }

        data.lastRealTime = now;
        _saveDirty = true;
    }



    _timeScale = 1.0f;

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


    _troopLevels = data.troopLevels;
    for (int id = 1; id <= 4; ++id) {
        if (_troopLevels.find(id) == _troopLevels.end()) _troopLevels[id] = 1;
    }
    _activeResearchUnitId = data.researchUnitId;
    _activeResearchTargetLevel = data.researchTargetLevel;
    _activeResearchTotalSec = data.researchTotalSec;
    _activeResearchRemainSec = data.researchRemainSec;


    restoreStandTroopsFromSave(data);
    applyStandTroopsToTrainingCamps();

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

    data.timeScale = 1.0f;
    data.lastRealTime = static_cast<int64_t>(std::time(nullptr));


    data.troopLevels = _troopLevels;
    data.researchUnitId = _activeResearchUnitId;
    data.researchTargetLevel = _activeResearchTargetLevel;
    data.researchTotalSec = _activeResearchTotalSec;
    data.researchRemainSec = _activeResearchRemainSec;


    data.trainedTroops.clear();
    data.trainedTroops.reserve(_standTroops.size());
    for (const auto& t : _standTroops)
    {
        SaveTrainedTroop st;
        st.type = t.type;
        st.r = t.r;
        st.c = t.c;
        data.trainedTroops.push_back(st);
    }

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


            sb.buildState = b.data->buildState;
            sb.buildTotalSec = b.data->buildTotalSec;
            sb.buildRemainSec = b.data->buildRemainSec;
            sb.upgradeTargetLevel = b.data->upgradeTargetLevel;
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




    bool hasTimer = (info.buildState != 0 && info.buildTotalSec > 0.0f && info.buildRemainSec > 0.0f);
    bool applyCapsNow = true;
    if (hasTimer && info.buildState == Building::STATE_CONSTRUCTING) {
        applyCapsNow = false;
    }

    auto b = BuildingFactory::create(info.id, std::max(1, info.level), applyCapsNow, false);
    if (!b) return;
    b->level = std::max(1, info.level);
    if (info.hp > 0) b->hp = std::min(info.hp, b->hpMax);
    b->stored = info.stored;


    b->buildState = info.buildState;
    b->buildTotalSec = info.buildTotalSec;
    b->buildRemainSec = info.buildRemainSec;
    b->upgradeTargetLevel = info.upgradeTargetLevel;
    auto s = b->createSprite();
    Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
    int idx = std::max(1, std::min(11, info.id));
    Vec2 off = _buildingOffsetById[idx];
    s->setPosition(center + off);
    s->setScale(_buildingScale * _buildingScaleById[idx]);
    _world->addChild(s, 3);

    if (hasTimer && s) {
        s->setOpacity(160);
        attachBuildTimerUI(s, b->buildTotalSec, b->buildRemainSec);
    }
    std::shared_ptr<Building> ptr(b.release());
    _buildings.push_back({ info.id, r, c, s, ptr });
}

void MainScene::openAttackTargetPicker()
{
    if (_attackMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();


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


    auto closeLabel = Label::createWithSystemFont("X", "Arial", 44);
    closeLabel->setColor(Color3B::WHITE);
    auto closeItem = MenuItemLabel::create(closeLabel, [this](Ref*) {
        closeAttackTargetPicker();
        });
    closeItem->setPosition(Vec2(panelW - 30, panelH - 40));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu, 2);


    auto metasAll = SaveSystem::listAllSlots();
    std::vector<SaveMeta> targets;
    targets.reserve(metasAll.size());
    for (const auto& m : metasAll)
    {
        if (m.exists && m.slot != SaveSystem::getCurrentSlot())
            targets.push_back(m);
    }


    _attackScroll = ui::ScrollView::create();
    _attackScroll->setDirection(ui::ScrollView::Direction::VERTICAL);
    _attackScroll->setBounceEnabled(true);
    _attackScroll->setContentSize(Size(panelW - 40, panelH - 140));


    _attackScroll->setAnchorPoint(Vec2::ZERO);
    _attackScroll->setIgnoreAnchorPointForPosition(true);
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

                SaveSystem::setBattleReadyTroops(collectAllReadyTroops());
                SaveSystem::setBattleTroopLevels(_troopLevels);
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





void MainScene::openTrainingCampPicker(int buildingIndex)
{
    SoundManager::playSfxRandom("button_click", 1.0f);
    if (_trainMask) return;
    if (buildingIndex < 0 || buildingIndex >= (int)_buildings.size()) return;

    auto& pb = _buildings[buildingIndex];
    if (!pb.data) return;
    auto tc = dynamic_cast<TrainingCamp*>(pb.data.get());
    if (!tc) return;

    _trainCampIndex = buildingIndex;
    _trainLastSig = 0;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();


    _trainMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_trainMask, 260);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _trainMask);


    float panelW = visibleSize.width * 0.90f;
    float panelH = visibleSize.height * 0.85f;
    auto panel = LayerColor::create(Color4B(245, 245, 245, 255), panelW, panelH);
    panel->setPosition(origin + Vec2((visibleSize.width - panelW) * 0.5f, (visibleSize.height - panelH) * 0.5f));
    _trainMask->addChild(panel, 1);
    _trainPanel = panel;


    auto title = Label::createWithSystemFont("Training", "Arial", 26);
    title->setColor(Color3B::BLACK);
    title->setPosition(Vec2(panelW * 0.5f, panelH - 30.f));
    panel->addChild(title, 2);


    auto closeLbl = Label::createWithSystemFont("X", "Arial", 26);
    closeLbl->setColor(Color3B::BLACK);
    auto closeItem = MenuItemLabel::create(closeLbl, [this](Ref*) {
        SoundManager::playSfxRandom("button_click", 1.0f);
        closeTrainingCampPicker();
        });
    closeItem->setPosition(Vec2(panelW - 22.f, panelH - 28.f));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu, 3);


    _trainCapLabel = Label::createWithSystemFont("", "Arial", 18);
    _trainCapLabel->setColor(Color3B::BLACK);
    _trainCapLabel->setAnchorPoint(Vec2(1, 0.5));
    _trainCapLabel->setPosition(Vec2(panelW - 20.f, panelH - 70.f));
    panel->addChild(_trainCapLabel, 2);


    auto rTitle = Label::createWithSystemFont("Ready", "Arial", 18);
    rTitle->setColor(Color3B::BLACK);
    rTitle->setAnchorPoint(Vec2(0, 0.5));
    rTitle->setPosition(Vec2(20.f, panelH - 100.f));
    panel->addChild(rTitle, 2);


    float readyIconH = 80.0f;
    {
        auto probe = Sprite::create(TrainingCamp::getTroopIcon(TrainingCamp::TROOP_BARBARIAN));
        if (probe) readyIconH = probe->getContentSize().height;
    }

    _trainReadyRow = safeCreateNode();

    if (_trainReadyRow) {
        _trainReadyRow->setPosition(Vec2(rTitle->getPositionX() + rTitle->getContentSize().width + 15.0f, rTitle->getPositionY() - readyIconH));
        panel->addChild(_trainReadyRow, 2);
    }
    else {
        CCLOG("[TrainingUI] Failed to create _trainReadyRow");
    }



    _trainSelectRow = safeCreateNode();
    if (_trainSelectRow) {
        _trainSelectRow->setPosition(Vec2(20.f, 25.f));
        panel->addChild(_trainSelectRow, 2);
    }
    else {
        CCLOG("[TrainingUI] Failed to create _trainSelectRow");
    }


    refreshTrainingCampUI();
}

void MainScene::closeTrainingCampPicker()
{
    if (_trainMask) {
        _trainPanel = nullptr;
        _trainReadyRow = nullptr;
        _trainSelectRow = nullptr;
        _trainCapLabel = nullptr;
        _trainCampIndex = -1;
        _trainLastSig = 0;

        _trainMask->removeFromParent();
        _trainMask = nullptr;
    }
}

void MainScene::showTrainingToast(const std::string& msg)
{
    if (!_trainPanel) return;

    auto size = _trainPanel->getContentSize();

    auto label = Label::createWithSystemFont(msg, "Arial", 22);
    label->setColor(Color3B(200, 30, 30));
    label->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
    _trainPanel->addChild(label, 10);

    label->runAction(Sequence::create(
        DelayTime::create(2.0f),
        FadeOut::create(0.25f),
        RemoveSelf::create(),
        nullptr
    ));
}

void MainScene::refreshTrainingCampUI()
{
    if (!_trainMask || _trainCampIndex < 0 || _trainCampIndex >= (int)_buildings.size()) return;
    auto& pb = _buildings[_trainCampIndex];
    if (!pb.data) return;
    auto tc = dynamic_cast<TrainingCamp*>(pb.data.get());
    if (!tc) return;


    if (_trainReadyRow) _trainReadyRow->removeAllChildren();
    if (_trainSelectRow) _trainSelectRow->removeAllChildren();


    const float kTrainIconSide = 90.0f;
    const float kTrainIconGap = 10.0f;


    auto createReadyIcon = [this, kTrainIconSide](TrainingCamp::TroopType t, int count)->Node* {
        auto node = safeCreateNode();
        if (!node) return nullptr;

        auto sp = Sprite::create(TrainingCamp::getTroopIcon(t));
        if (!sp) {
            sp = Sprite::create();
        }
        if (!sp) return nullptr;
        sp->setAnchorPoint(Vec2(0, 0));
        sp->setPosition(Vec2(0, 0));
        float scale = 1.0f;
        {
            cocos2d::Size cs = sp->getContentSize();
            float denom = std::max(1.0f, std::max(cs.width, cs.height));
            scale = std::min(1.0f, kTrainIconSide / denom);
        }
        sp->setScale(scale);
        node->addChild(sp, 1);

        Size iconSize = sp->getContentSize() * scale;


        char buf[32];
        snprintf(buf, sizeof(buf), "x%d", count);
        auto cntLbl = Label::createWithSystemFont(buf, "Arial", 18);
        cntLbl->setColor(Color3B::BLACK);
        cntLbl->setAnchorPoint(Vec2(0, 1));
        cntLbl->setPosition(Vec2(4.f, iconSize.height - 4.f));
        node->addChild(cntLbl, 2);



        {
            int lv = getTroopLevel((int)t);
            auto lvLbl = Label::createWithSystemFont(StringUtils::format("LV%d", lv), "Arial", 28);
            lvLbl->setColor(Color3B::BLACK);

            lvLbl->enableOutline(Color4B::BLACK, 3);
            lvLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
            lvLbl->setPosition(Vec2(iconSize.width * 0.5f, iconSize.height * 0.5f));


            float maxW = iconSize.width * 0.95f;
            float maxH = iconSize.height * 0.60f;
            float bw = lvLbl->getContentSize().width;
            float bh = lvLbl->getContentSize().height;
            float s = 1.0f;
            if (bw > maxW) s = std::min(s, maxW / std::max(1.0f, bw));
            if (bh > maxH) s = std::min(s, maxH / std::max(1.0f, bh));
            lvLbl->setScale(s);

            node->addChild(lvLbl, 2);
        }

        auto minusLbl = Label::createWithSystemFont("-", "Arial", 22);
        minusLbl->setColor(Color3B(40, 40, 40));
        auto minusItem = MenuItemLabel::create(minusLbl, [this, t](Ref*) {
            SoundManager::playSfxRandom("button_click", 1.0f);
            if (_trainCampIndex < 0 || _trainCampIndex >= (int)_buildings.size()) return;
            auto& pb2 = _buildings[_trainCampIndex];
            if (!pb2.data) return;
            auto tc2 = dynamic_cast<TrainingCamp*>(pb2.data.get());
            if (!tc2) return;


            if (tc2->tryRemoveReadyTroop(t)) {
                _trainLastSig = 0;
                removeStandTroopOfType((int)t);
                _saveDirty = true;
                refreshTrainingCampUI();
            }
            });
        minusItem->setAnchorPoint(Vec2(1, 1));
        minusItem->setPosition(Vec2(iconSize.width - 4.f, iconSize.height - 4.f));
        auto menu = Menu::create(minusItem, nullptr);
        menu->setPosition(Vec2(0, 0));
        node->addChild(menu, 3);

        node->setContentSize(iconSize);
        return node;
        };


    float x = 0.f;
    float gap = 10.f;
    if (_trainReadyRow) {

        for (int key = 1; key <= 4; ++key) {
            auto t = (TrainingCamp::TroopType)key;
            int cnt = tc->getReadyCount(t);
            if (cnt <= 0) continue;

            auto icon = createReadyIcon(t, cnt);
            if (!icon) continue;
            if (icon) icon->setPosition(Vec2(x, 0));
            if (_trainReadyRow) _trainReadyRow->addChild(icon, 1);
            x += icon->getContentSize().width + gap;
        }
    }


    if (_trainSelectRow) {
        float bx = 0.f;
        for (int key = 1; key <= 4; ++key) {
            auto t = (TrainingCamp::TroopType)key;
            if (!tc->isUnlocked(t)) continue;

            auto btn = ui::Button::create(TrainingCamp::getTroopIcon(t));
            btn->setAnchorPoint(Vec2(0, 0));
            btn->setPosition(Vec2(bx, 0));
            {
                cocos2d::Size cs = btn->getContentSize();
                float denom = std::max(1.0f, std::max(cs.width, cs.height));
                float scale = std::min(1.0f, kTrainIconSide / denom);
                btn->setScale(scale);
            }



            {
                int lv = getTroopLevel((int)t);
                auto lvLbl = Label::createWithSystemFont(StringUtils::format("LV%d", lv), "Arial", 28);
                lvLbl->setColor(Color3B::BLACK);

                lvLbl->enableOutline(Color4B::BLACK, 3);
                lvLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
                lvLbl->setPosition(Vec2(btn->getContentSize().width * 0.5f, btn->getContentSize().height * 0.5f));


                float maxW = btn->getContentSize().width * 0.95f;
                float maxH = btn->getContentSize().height * 0.60f;
                float bw = lvLbl->getContentSize().width;
                float bh = lvLbl->getContentSize().height;
                float s = 1.0f;
                if (bw > maxW) s = std::min(s, maxW / std::max(1.0f, bw));
                if (bh > maxH) s = std::min(s, maxH / std::max(1.0f, bh));
                lvLbl->setScale(s);

                btn->addChild(lvLbl, 2);
            }
            btn->addClickEventListener([this, t](Ref*) {
                SoundManager::playSfxRandom("button_click", 1.0f);
                if (_trainCampIndex < 0 || _trainCampIndex >= (int)_buildings.size()) return;
                auto& pb2 = _buildings[_trainCampIndex];
                if (!pb2.data) return;
                auto tc2 = dynamic_cast<TrainingCamp*>(pb2.data.get());
                if (!tc2) return;


                if (tc2->tryAddReadyTroop(t)) {
                    _trainLastSig = 0;
                    spawnStandTroop((int)t);
                    _saveDirty = true;
                    refreshTrainingCampUI();
                }
                else {
                    showTrainingToast("Cannot add troop: capacity full or not unlocked.");
                }
                });
            _trainSelectRow->addChild(btn, 1);

            float w = btn->getContentSize().width * btn->getScale();
            bx += w + kTrainIconGap;
        }
    }


    if (_trainCapLabel) {
        int totalCap = ResourceManager::getPopulationCap();
        int usedHousing = tc->getUsedHousing();
        char buf[64];
        snprintf(buf, sizeof(buf), "%d / %d", usedHousing, totalCap);
        _trainCapLabel->setString(buf);
    }


    int sig = 0;
    const auto& ready = tc->getAllReadyCounts();
    for (const auto& kv : ready) {
        sig = sig * 31 + kv.first;
        sig = sig * 31 + kv.second;
    }
    _trainLastSig = sig;
}

std::unordered_map<int, int> MainScene::collectAllReadyTroops() const
{
    std::unordered_map<int, int> out;
    for (const auto& pb : _buildings)
    {
        if (!pb.data) continue;
        auto tc = dynamic_cast<TrainingCamp*>(pb.data.get());
        if (!tc) continue;

        const auto& ready = tc->getAllReadyCounts();
        for (const auto& kv : ready)
        {
            out[kv.first] += kv.second;
        }
    }
    return out;
}


void MainScene::removeStandTroopOfType(int troopType)
{
    for (int i = (int)_standTroops.size() - 1; i >= 0; --i)
    {
        if (_standTroops[i].type != troopType) continue;
        if (_standTroops[i].sprite) _standTroops[i].sprite->removeFromParent();
        _standTroops.erase(_standTroops.begin() + i);
        return;
    }
}

void MainScene::restoreStandTroopsFromSave(const SaveData& data)
{
    if (_standTroopLayer) _standTroopLayer->removeAllChildren();
    _standTroops.clear();

    for (const auto& t : data.trainedTroops)
    {

        int r = t.r;
        int c = t.c;
        bool occupied = false;
        for (const auto& ex : _standTroops)
        {
            if (ex.r == r && ex.c == c) { occupied = true; break; }
        }
        if (occupied || r < 0 || r >= _rows || c < 0 || c >= _cols)
        {
            spawnStandTroop(t.type);
            continue;
        }

        auto sp = createStandTroopSprite(t.type);
        if (!sp) continue;

        Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f),
            _anchor.y - (c + r) * (_tileH * 0.5f));
        float jx = randomRealSafe(-_tileW * 0.15f, _tileW * 0.15f);
        float jy = randomRealSafe(-_tileH * 0.10f, _tileH * 0.10f);
        Vec2 pos = center + Vec2(jx, _tileH * 0.10f + jy);

        float desiredW = _tileW * 0.60f;
        float s = desiredW / std::max(1.0f, sp->getContentSize().width);
        sp->setScale(s);
        sp->setPosition(pos);

        int z = 5 + r + c;
        _standTroopLayer->addChild(sp, z);

        StandTroopInfo info;
        info.type = t.type;
        info.r = r;
        info.c = c;
        info.sprite = sp;
        _standTroops.push_back(info);
    }
}

void MainScene::applyStandTroopsToTrainingCamps()
{
    std::unordered_map<int, int> counts;
    for (const auto& t : _standTroops) counts[t.type] += 1;

    bool applied = false;
    for (auto& pb : _buildings)
    {
        if (!pb.data) continue;
        auto tc = dynamic_cast<TrainingCamp*>(pb.data.get());
        if (!tc) continue;

        tc->clearAllReadyCounts();

        if (!applied)
        {
            for (const auto& kv : counts)
            {
                tc->setReadyCount((TrainingCamp::TroopType)kv.first, kv.second);
            }
            applied = true;
        }
    }
}

cocos2d::Sprite* MainScene::createStandTroopSprite(int troopType) const
{
    std::string path;
    if (troopType == (int)TrainingCamp::TROOP_BARBARIAN) path = "barbarian/barbarian_stand.png";
    else if (troopType == (int)TrainingCamp::TROOP_ARCHER) path = "archor/archor_stand.png";
    else if (troopType == (int)TrainingCamp::TROOP_GIANT) path = "giant/giant_stand.png";
    else if (troopType == (int)TrainingCamp::TROOP_WALLBREAKER) path = "wall_breaker/wall_breaker_stand.png";
    else path = "";

    if (!path.empty())
    {
        if (auto sp = Sprite::create(path))
        {
            sp->setAnchorPoint(Vec2(0.5f, 0.0f));
            return sp;
        }
    }


    auto iconPath = TrainingCamp::getTroopIcon((TrainingCamp::TroopType)troopType);
    if (auto sp = Sprite::create(iconPath))
    {
        sp->setAnchorPoint(Vec2(0.5f, 0.0f));
        return sp;
    }
    return nullptr;
}

void MainScene::spawnStandTroop(int troopType)
{
    if (!_world || !_standTroopLayer) return;


    std::vector<int> barracksIndices;
    for (int i = 0; i < (int)_buildings.size(); ++i)
    {
        if (_buildings[i].id == 7) barracksIndices.push_back(i);
    }

    if (barracksIndices.empty())
    {
        showTrainingToast("No Barracks found.");
        return;
    }


    int chosenR = 0;
    int chosenC = 0;
    bool found = false;

    for (int attempt = 0; attempt < 40 && !found; ++attempt)
    {
        int bi = barracksIndices[cocos2d::RandomHelper::random_int(0, (int)barracksIndices.size() - 1)];
        int rr = _buildings[bi].r;
        int cc = _buildings[bi].c;

        int r = rr + cocos2d::RandomHelper::random_int(-2, 2);
        int c = cc + cocos2d::RandomHelper::random_int(-2, 2);

        if (r < 0 || r >= _rows || c < 0 || c >= _cols) continue;

        bool occupied = false;
        for (const auto& t : _standTroops)
        {
            if (t.r == r && t.c == c) { occupied = true; break; }
        }
        if (occupied) continue;

        chosenR = r;
        chosenC = c;
        found = true;
    }

    if (!found)
    {

        int bi = barracksIndices.front();
        chosenR = _buildings[bi].r;
        chosenC = _buildings[bi].c;
    }

    auto sp = createStandTroopSprite(troopType);
    if (!sp) return;


    Vec2 center(_anchor.x + (chosenC - chosenR) * (_tileW * 0.5f),
        _anchor.y - (chosenC + chosenR) * (_tileH * 0.5f));


    float jx = randomRealSafe(-_tileW * 0.15f, _tileW * 0.15f);
    float jy = randomRealSafe(-_tileH * 0.10f, _tileH * 0.10f);
    Vec2 pos = center + Vec2(jx, _tileH * 0.10f + jy);


    float desiredW = _tileW * 0.60f;
    float s = desiredW / std::max(1.0f, sp->getContentSize().width);
    sp->setScale(s);
    sp->setPosition(pos);


    int z = 5 + chosenR + chosenC;
    _standTroopLayer->addChild(sp, z);

    StandTroopInfo info;
    info.type = troopType;
    info.r = chosenR;
    info.c = chosenC;
    info.sprite = sp;
    _standTroops.push_back(info);
}



int MainScene::getTroopLevel(int unitId) const
{
    auto it = _troopLevels.find(unitId);
    if (it == _troopLevels.end()) return 1;
    return std::max(1, it->second);
}

void MainScene::setTroopLevel(int unitId, int level)
{
    _troopLevels[unitId] = std::max(1, level);
}

int MainScene::getLaboratoryMaxTroopLevel(int labLevel, int unitId) const
{






    labLevel = std::max(1, std::min(5, labLevel));

    if (unitId == 1) {
        if (labLevel >= 4) return 3;
        if (labLevel >= 1) return 2;
        return 1;
    }
    if (unitId == 2) {
        if (labLevel >= 4) return 3;
        if (labLevel >= 2) return 2;
        return 1;
    }
    if (unitId == 3) {
        if (labLevel >= 5) return 3;
        if (labLevel >= 3) return 2;
        return 1;
    }
    if (unitId == 4) {
        if (labLevel >= 4) return 2;
        return 1;
    }
    return 1;
}

static std::string formatTimeMMSS(int sec)
{
    sec = std::max(0, sec);
    int mm = sec / 60;
    int ss = sec % 60;
    return cocos2d::StringUtils::format("%02d:%02d", mm, ss);
}

void MainScene::updateResearchSystems(float dt)
{
    if (_activeResearchUnitId <= 0 || _activeResearchRemainSec <= 0.0f || _activeResearchTotalSec <= 0.0f)
        return;


    float scaled = dt * _timeScale;
    _activeResearchRemainSec = std::max(0.0f, _activeResearchRemainSec - scaled);

    if (_activeResearchRemainSec <= 0.0f)
    {

        setTroopLevel(_activeResearchUnitId, _activeResearchTargetLevel);

        _activeResearchUnitId = 0;
        _activeResearchTargetLevel = 0;
        _activeResearchTotalSec = 0.0f;
        _activeResearchRemainSec = 0.0f;

        _saveDirty = true;


        if (_trainMask) _trainLastSig = 0;
        if (_researchMask) _researchLastSig = 0;
    }


    if (_researchMask)
    {

        int t = (int)std::round(_activeResearchRemainSec * 10.0f);
        int sig = _activeResearchUnitId * 100000 + _activeResearchTargetLevel * 1000 + t;
        if (sig != _researchLastSig)
        {
            _researchLastSig = sig;
            refreshResearchUI();
        }
    }
}

void MainScene::openLaboratoryResearchPicker(int buildingIndex)
{
    SoundManager::playSfxRandom("button_click", 1.0f);
    if (_researchMask) return;
    if (buildingIndex < 0 || buildingIndex >= (int)_buildings.size()) return;

    auto& pb = _buildings[buildingIndex];
    if (!pb.data) return;
    auto lab = dynamic_cast<Laboratory*>(pb.data.get());
    if (!lab) return;

    _researchLabIndex = buildingIndex;
    _researchLastSig = 0;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();


    _researchMask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(_researchMask, 260);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _researchMask);


    float panelW = visibleSize.width * 0.90f;
    float panelH = visibleSize.height * 0.85f;
    auto panel = LayerColor::create(Color4B(245, 245, 245, 255), panelW, panelH);
    panel->setPosition(origin + Vec2((visibleSize.width - panelW) * 0.5f, (visibleSize.height - panelH) * 0.5f));
    _researchMask->addChild(panel, 1);
    _researchPanel = panel;


    auto title = Label::createWithSystemFont("Research", "Arial", 26);
    title->setColor(Color3B::BLACK);
    title->setPosition(Vec2(panelW * 0.5f, panelH - 30.f));
    panel->addChild(title, 2);


    auto closeLbl = Label::createWithSystemFont("X", "Arial", 26);
    closeLbl->setColor(Color3B::BLACK);
    auto closeItem = MenuItemLabel::create(closeLbl, [this](Ref*) {
        SoundManager::playSfxRandom("button_click", 1.0f);
        closeLaboratoryResearchPicker();
        });
    closeItem->setPosition(Vec2(panelW - 22.f, panelH - 28.f));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu, 3);


    auto hint = Label::createWithSystemFont("Choose a troop to research.", "Arial", 18);
    hint->setColor(Color3B::BLACK);
    hint->setAnchorPoint(Vec2(0, 0.5f));
    hint->setPosition(Vec2(20.f, panelH - 80.f));
    panel->addChild(hint, 2);


    _researchSelectRow = safeCreateNode();
    if (_researchSelectRow) {
        _researchSelectRow->setPosition(Vec2(20.f, 25.f));
        panel->addChild(_researchSelectRow, 2);
    }

    refreshResearchUI();
}

void MainScene::closeLaboratoryResearchPicker()
{
    if (_researchMask)
    {
        if (_researchMouseListener)
        {
            _eventDispatcher->removeEventListener(_researchMouseListener);
            _researchMouseListener = nullptr;
        }

        _researchPanel = nullptr;
        _researchSelectRow = nullptr;
        _researchLabIndex = -1;
        _researchLastSig = 0;

        _researchMask->removeFromParent();
        _researchMask = nullptr;
    }
}

void MainScene::showResearchToast(const std::string& msg)
{
    if (!_researchPanel) return;

    auto size = _researchPanel->getContentSize();

    auto label = Label::createWithSystemFont(msg, "Arial", 22);
    label->setColor(Color3B(200, 30, 30));
    label->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
    _researchPanel->addChild(label, 10);

    label->runAction(Sequence::create(
        DelayTime::create(2.0f),
        FadeOut::create(0.25f),
        RemoveSelf::create(),
        nullptr
    ));
}

void MainScene::refreshResearchUI()
{
    if (!_researchMask || !_researchPanel || !_researchSelectRow) return;
    if (_researchLabIndex < 0 || _researchLabIndex >= (int)_buildings.size()) return;

    auto& pb = _buildings[_researchLabIndex];
    if (!pb.data) return;
    auto lab = dynamic_cast<Laboratory*>(pb.data.get());
    if (!lab) return;


    bool labFunctional = (lab->buildState == Building::STATE_NORMAL);

    int labLevel = std::max(1, lab->level);

    _researchSelectRow->removeAllChildren();

    float x = 0.0f;
    float y = 0.0f;
    float gap = 20.0f;


    float iconH = 80.0f;
    {
        auto probe = Sprite::create(TrainingCamp::getTroopIcon(TrainingCamp::TROOP_BARBARIAN));
        if (probe) iconH = probe->getContentSize().height;
    }

    for (int type = 1; type <= 4; ++type)
    {
        std::string iconPath = TrainingCamp::getTroopIcon((TrainingCamp::TroopType)type);
        auto btn = ui::Button::create(iconPath, iconPath, iconPath);
        if (!btn) continue;


        float iconScale = 1.0f;
        {
            auto spr = Sprite::create(iconPath);
            if (spr)
            {
                auto cs = spr->getContentSize();
                float denom = std::max(1.0f, std::max(cs.width, cs.height));
                iconScale = std::min(1.0f, 90.0f / denom);
            }
        }
        btn->setScale(iconScale);

        btn->setAnchorPoint(Vec2(0, 0));
        btn->setPosition(Vec2(x, y));
        _researchSelectRow->addChild(btn, 1);




        Size rawSz = btn->getContentSize();
        Size bsz = rawSz * iconScale;

        int curLv = getTroopLevel(type);
        int maxLv = getLaboratoryMaxTroopLevel(labLevel, type);


        if (curLv >= maxLv)
        {
            btn->setColor(Color3B(110, 110, 110));
        }



        {
            auto lvLbl = Label::createWithSystemFont(StringUtils::format("LV%d", curLv), "Arial", 28);
            lvLbl->setColor(Color3B::BLACK);

            lvLbl->enableOutline(Color4B::BLACK, 3);
            lvLbl->setAnchorPoint(Vec2(0.5f, 0.5f));
            lvLbl->setPosition(Vec2(rawSz.width * 0.5f, rawSz.height * 0.5f));


            float maxW = rawSz.width * 0.95f;
            float maxH = rawSz.height * 0.60f;
            float bw = lvLbl->getContentSize().width;
            float bh = lvLbl->getContentSize().height;
            float s = 1.0f;
            if (bw > maxW) s = std::min(s, maxW / std::max(1.0f, bw));
            if (bh > maxH) s = std::min(s, maxH / std::max(1.0f, bh));
            lvLbl->setScale(s);

            btn->addChild(lvLbl, 3);
        }



        {
            float barW = std::max(60.0f, rawSz.width);
            float barH = 16.0f;

            auto barBg = LayerColor::create(Color4B(20, 20, 20, 200), barW, barH);
            barBg->setAnchorPoint(Vec2(0.0f, 0.0f));
            barBg->setPosition(Vec2(0.0f, rawSz.height + 6.0f));
            btn->addChild(barBg, 4);

            auto barFill = LayerColor::create(Color4B(60, 200, 80, 220), barW, barH);
            barFill->setAnchorPoint(Vec2(0.0f, 0.0f));
            barFill->setPosition(Vec2(0.0f, 0.0f));
            barBg->addChild(barFill, 1);


            auto timeLbl = Label::createWithSystemFont("", "Arial", 32);
            timeLbl->setAnchorPoint(Vec2(0.5f, 0.0f));
            timeLbl->setColor(Color3B::WHITE);
            timeLbl->enableOutline(Color4B::BLACK, 3);
            timeLbl->setPosition(Vec2(barW * 0.5f, barH + 2.0f));
            barBg->addChild(timeLbl, 2);

            bool show = (_activeResearchUnitId > 0
                && _activeResearchRemainSec > 0.0f
                && _activeResearchTotalSec > 0.0f
                && type == _activeResearchUnitId);

            barBg->setVisible(show);
            if (show)
            {
                float prog = 1.0f - (_activeResearchRemainSec / std::max(0.001f, _activeResearchTotalSec));
                prog = std::max(0.0f, std::min(1.0f, prog));
                barFill->setContentSize(Size(barW * prog, barH));

                int sec = (int)std::ceil(_activeResearchRemainSec);
                timeLbl->setString(formatTimeMMSS(sec));
            }
        }


        btn->addClickEventListener([this, type, labFunctional, labLevel](Ref*) {
            SoundManager::playSfxRandom("button_click", 1.0f);
            if (!labFunctional) {
                showResearchToast("Laboratory is not ready.");
                return;
            }
            if (_activeResearchUnitId > 0 && _activeResearchRemainSec > 0.0f) {
                showResearchToast("Only one research at a time.");
                return;
            }

            int curLv = getTroopLevel(type);
            int maxLv = getLaboratoryMaxTroopLevel(labLevel, type);
            if (curLv >= maxLv) {
                showResearchToast("Max level for this Laboratory.");
                return;
            }

            int targetLv = std::min(maxLv, curLv + 1);
            int cost = ConfigManager::getTroopResearchCostElixir(type, targetLv);
            int sec = ConfigManager::getTroopResearchTimeSec(type, targetLv);

            if (ResourceManager::getElixir() < cost) {
                showResearchToast("Not enough elixir.");
                return;
            }

            ResourceManager::addElixir(-cost);

            _activeResearchUnitId = type;
            _activeResearchTargetLevel = targetLv;
            _activeResearchTotalSec = (float)sec;
            _activeResearchRemainSec = (float)sec;

            _saveDirty = true;
            _researchLastSig = 0;
            refreshResearchUI();
            });

        x += bsz.width + gap;
    }
}