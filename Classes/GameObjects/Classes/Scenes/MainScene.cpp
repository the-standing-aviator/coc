#include "Scenes/MainScene.h"
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
        int cols = 4, rows = 2;
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
                        startPlacement(idx);
                        panel->removeFromParent();
                        });
                    Size s = it->getContentSize();
                    float sx = std::min(cw / s.width, ch / s.height);
                    it->setScale(sx);
                    item = it;
                    int limit = buildLimitForId(idx);
                    if (countById(idx) >= limit) {
                        it->setEnabled(false);
                        if (auto normal = it->getNormalImage()) normal->setColor(Color3B(150,150,150));
                        if (auto selected = it->getSelectedImage()) selected->setColor(Color3B(150,150,150));
                    }
                }
                else {
                    auto label = Label::createWithSystemFont(StringUtils::format("%d", idx), "Arial", 20);
                    auto it = MenuItemLabel::create(label, [this, idx, panel](Ref*) {
                        startPlacement(idx);
                        panel->removeFromParent();
                        });
                    it->setContentSize(Size(cw, ch));
                    item = it;
                    int limit = buildLimitForId(idx);
                    if (countById(idx) >= limit) {
                        it->setEnabled(false);
                        label->setColor(Color3B(150,150,150));
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

    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode code, Event*) {
        if (code == EventKeyboard::KeyCode::KEY_G) {
            bool v = _grid && _grid->isVisible();
            setGridVisible(!v);
        }
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    //建筑大小接口
    setBuildingScale(0.33f);

    setBuildingScaleForId(1, 0.4f);
    setBuildingOffsetForId(1, Vec2(0, 20/3));

    setBuildingOffsetForId(2, Vec2(-2/3, 14/3));

    setBuildingOffsetForId(3, Vec2(-10/3, 14/3));

    setBuildingOffsetForId(4, Vec2(4/3, 4/3));

    setBuildingScaleForId(7, 0.7f);
    setBuildingOffsetForId(7, Vec2(-4/3, 0));

    setBuildingScaleForId(8, 1.3f);
    setBuildingOffsetForId(8, Vec2(4/3, 10/3));

    setBuildingScaleForId(9, 0.8f);
    setBuildingOffsetForId(9, Vec2(4/3, 10/3));
    int cr = _rows / 2;
    int cc = _cols / 2;
    if (canPlace(cr, cc)) placeBuilding(cr, cc, 9);

    return true;
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
    int th = getTownHallLevel();
    if (id == 3 || id == 5) {
        if (th <= 1) return 1;
        if (th == 2) return 2;
        return 3;
    }
    if (id == 4 || id == 6) {
        if (th <= 2) return 1;
        return 2;
    }
    return 1;
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
void MainScene::setResourceUiScale(float s)
{
    auto n = this->getChildByName("ResourcePanel");
    if (n) {
        auto panel = dynamic_cast<ResourcePanel*>(n);
        if (panel) panel->setPanelScale(s);
    }
}

void MainScene::update(float dt)
{
    for (size_t i = 0; i < _buildings.size(); ++i) {
        auto& pb = _buildings[i];
        if (!pb.data) continue;
        if (auto ec = dynamic_cast<ElixirCollector*>(pb.data.get())) {
            ec->updateProduction(dt, _timeScale);
            ec->manageCollectPrompt(_world, pb.sprite);
        } else if (auto gm = dynamic_cast<GoldMine*>(pb.data.get())) {
            gm->updateProduction(dt, _timeScale);
            gm->manageCollectPrompt(_world, pb.sprite);
        }
    }
}
#include "Managers/ConfigManager.h"

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
    bool disabled = isMax || (b.id != 9 && nextLv > thLevel);
    std::string title = cocos2d::StringUtils::format("%s (Level %d)", ConfigManager::getBuildingName(b.id).c_str(), curLv);
    std::string resName = cost.useGold ? "Gold" : "Elixir";
    auto modal = CustomButton::createUpgradePanel(title, resName, cost.amount, disabled, isMax,
        [this, idx, nextLv, cost]() {
            auto& pb = _buildings[idx];
            if (!pb.data) return;
            int curLv2 = pb.data->level;
            if (curLv2 >= ConfigManager::getMaxLevel()) { return; }
            if (pb.id != 9) {
                int th = getTownHallLevel();
                if (nextLv > th) return;
            }
            bool ok = false;
            if (cost.useGold) ok = ResourceManager::spendGold(cost.amount);
            else ok = ResourceManager::spendElixir(cost.amount);
            if (ok) {
                pb.data->level = curLv2 + 1;
                if (pb.id == 3) {
                    auto ec = dynamic_cast<ElixirCollector*>(pb.data.get());
                    if (ec) {
                        ec->setupStats(pb.data->level);
                    } else {
                        pb.data->hpMax = std::max(pb.data->hpMax, pb.data->hpMax); // no-op safeguard
                    }
                } else if (pb.id == 5) {
                    auto gm = dynamic_cast<GoldMine*>(pb.data.get());
                    if (gm) {
                        gm->setupStats(pb.data->level);
                    } else {
                        pb.data->hpMax = std::max(pb.data->hpMax, pb.data->hpMax);
                    }
                } else if (pb.id == 4) {
                    auto es = dynamic_cast<ElixirStorage*>(pb.data.get());
                    if (es) {
                        es->setupStats(pb.data->level);
                        es->applyCap();
                    } else {
                        pb.data->hpMax = std::max(pb.data->hpMax, pb.data->hpMax);
                    }
                } else if (pb.id == 6) {
                    auto gs = dynamic_cast<GoldStorage*>(pb.data.get());
                    if (gs) {
                        gs->setupStats(pb.data->level);
                        gs->applyCap();
                    } else {
                        pb.data->hpMax = std::max(pb.data->hpMax, pb.data->hpMax);
                    }
                } else if (pb.id == 9) {
                    auto th = dynamic_cast<TownHall*>(pb.data.get());
                    if (th) {
                        th->setupStats(pb.data->level);
                        th->applyCap();
                    } else {
                        pb.data->hpMax = std::max(pb.data->hpMax, pb.data->hpMax);
                    }
                }
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
                if (canPlace(r, c)) placeBuilding(r, c, _placingId);
                cancelPlacement();
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
                    } else {
                        _dragging = true;
                        _lastMouse = Vec2(ev->getCursorX(), ev->getCursorY());
                    }
                } else {
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
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    int rr = r + dr, cc = c + dc;
                    if (rr >= 0 && rr < _rows && cc >= 0 && cc < _cols)
                        drawCellFilled(rr, cc, Color4F(0.3f, 0.9f, 0.3f, 0.35f), _hint);
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
    if (r < 1 || r > _rows - 2 || c < 1 || c > _cols - 2) return false;
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
            if (_occupy[r + dr][c + dc] != 0) return false;
    return true;
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
    if (id >= 1 && id <= 9 && countById(id) >= buildLimitForId(id)) {
        return;
    }
    auto buildCost = ConfigManager::getBuildCost(id);
    if (buildCost.amount > 0) {
        bool ok = buildCost.useGold ? ResourceManager::spendGold(buildCost.amount)
                                    : ResourceManager::spendElixir(buildCost.amount);
        if (!ok) return;
    }
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
            _occupy[r + dr][c + dc] = id;
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
            drawCellFilled(r + dr, c + dc, Color4F(0.2f, 0.8f, 0.2f, 0.6f), _occupied);

    std::unique_ptr<Building> b;
    switch (id) {
        case 1: b.reset(new ArrowTower()); break;
        case 2: b.reset(new Cannon()); break;
        case 3: b.reset(new ElixirCollector()); break;
        case 4: b.reset(new ElixirStorage()); break;
        case 5: b.reset(new GoldMine()); break;
        case 6: b.reset(new GoldStorage()); break;
        case 7: b.reset(new Barracks()); break;
        case 8: b.reset(new TrainingCamp()); break;
        case 9: b.reset(new TownHall()); break;
        default: b.reset(new Building()); break;
    }
    b->hpMax = 100;
    b->hp = 100;
    b->level = 1;
    if (id == 3) {
        auto ec = dynamic_cast<ElixirCollector*>(b.get());
        if (ec) {
            ec->setupStats(b->level);
            ec->stored = 0.f;
        }
    }
    if (id == 4) {
        auto es = dynamic_cast<ElixirStorage*>(b.get());
        if (es) {
            es->setupStats(b->level);
            es->applyCap();
        }
    }
    if (id == 5) {
        auto gm = dynamic_cast<GoldMine*>(b.get());
        if (gm) {
            gm->setupStats(b->level);
            gm->stored = 0.f;
        }
    }
    if (id == 6) {
        auto gs = dynamic_cast<GoldStorage*>(b.get());
        if (gs) {
            gs->setupStats(b->level);
            gs->applyCap();
        }
    }
    if (id == 9) {
        auto th = dynamic_cast<TownHall*>(b.get());
        if (th) {
            th->setupStats(b->level);
            th->applyCap();
        }
    }
    auto s = b->createSprite();
    Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
    int idx = std::max(1, std::min(9, id));
    Vec2 off = _buildingOffsetById[idx];
    s->setPosition(center + off);
    s->setScale(_buildingScale * _buildingScaleById[idx]);
    _world->addChild(s, 3);
    std::shared_ptr<Building> ptr(b.release());
    _buildings.push_back({ id, r, c, s, ptr });
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

bool MainScene::canPlaceIgnoring(int r, int c, int ignoreIndex) const
{
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
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc)
                drawCellFilled(b.r + dr, b.c + dc, Color4F(0.2f, 0.8f, 0.2f, 0.6f), _occupied);
    }
}

void MainScene::commitMove(int r, int c)
{
    auto& b = _buildings[_movingIndex];
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
            _occupy[b.r + dr][b.c + dc] = 0;
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
            _occupy[r + dr][c + dc] = b.id;
    Vec2 center(_anchor.x + (c - r) * (_tileW * 0.5f), _anchor.y - (c + r) * (_tileH * 0.5f));
    int idx = std::max(1, std::min(9, b.id));
    Vec2 off = _buildingOffsetById[idx];
    if (b.sprite) b.sprite->setPosition(center + off);
    b.r = r; b.c = c;
    redrawOccupied();
}

void MainScene::cancelMove()
{
    _moving = false;
    _movingIndex = -1;
    if (_hint) _hint->clear();
}
