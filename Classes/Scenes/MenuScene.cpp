#include "Scenes/MenuScene.h"

#include "Scenes/MainScene.h"
#include "Data/SaveSystem.h"

#include "ui/CocosGUI.h"

#ifdef _WIN32
#include <windows.h>
#endif

USING_NS_CC;

Scene* MenuScene::createScene()
{
    return MenuScene::create();
}

bool MenuScene::init()
{
    if (!Scene::init())
        return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // Background
    auto bg = Sprite::create("background.jpg");
    if (!bg) bg = Sprite::create("backgrounds/village_map.jpg");
    if (bg)
    {
        bg->setPosition(origin + visibleSize / 2);
        bg->setScale(
            visibleSize.width / bg->getContentSize().width,
            visibleSize.height / bg->getContentSize().height
        );
        this->addChild(bg, -1);
    }
    else
    {
        this->addChild(LayerColor::create(Color4B(60, 80, 120, 255)), -1);
    }

    // Title
    auto title = Label::createWithSystemFont("Clash Of Clans Demo", "Arial", 76);
    title->setPosition(origin + Vec2(visibleSize.width / 2, visibleSize.height - 140));
    this->addChild(title, 1);

    // Reset battle target every time we return to menu
    SaveSystem::setBattleTargetSlot(-1);

    // Buttons
    auto newLabel = Label::createWithSystemFont("Create New Save", "Arial", 54);
    auto newItem = MenuItemLabel::create(newLabel, [this](Ref*) {
        this->createNewSaveAndEnter();
        });

    auto loadLabel = Label::createWithSystemFont("Load Existing Save", "Arial", 54);
    auto loadItem = MenuItemLabel::create(loadLabel, [this](Ref*) {
        this->openSaveSelector();
        });

    auto exitLabel = Label::createWithSystemFont("Exit Game", "Arial", 46);
    auto exitItem = MenuItemLabel::create(exitLabel, [](Ref*) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
        ExitProcess(0);
#else
        Director::getInstance()->end();
#endif
        });

    newItem->setPosition(origin + Vec2(visibleSize.width / 2, visibleSize.height * 0.52f));
    loadItem->setPosition(origin + Vec2(visibleSize.width / 2, visibleSize.height * 0.40f));
    exitItem->setPosition(origin + Vec2(visibleSize.width / 2, 90));

    auto menu = Menu::create(newItem, loadItem, exitItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu, 2);
    // Save selector overlay will be created on demand
    return true;
}

void MenuScene::createNewSaveAndEnter()
{
    int chosenSlot = -1;
    for (int i = 0; i < 20; ++i)
    {
        if (!SaveSystem::exists(i)) { chosenSlot = i; break; }
    }

    if (chosenSlot < 0)
    {
        auto visibleSize = Director::getInstance()->getVisibleSize();
        auto origin = Director::getInstance()->getVisibleOrigin();
        auto tip = Label::createWithSystemFont("No empty slot (max 20)", "Arial", 32);
        tip->setPosition(origin + Vec2(visibleSize.width / 2, visibleSize.height * 0.28f));
        tip->setColor(Color3B::YELLOW);
        this->addChild(tip, 5);
        tip->runAction(Sequence::create(DelayTime::create(2.0f), FadeOut::create(0.35f), RemoveSelf::create(), nullptr));
        return;
    }

    std::string defName = StringUtils::format("Save %02d", chosenSlot + 1);
    SaveData data = SaveSystem::makeDefault(chosenSlot, defName);
    SaveSystem::save(data);
    SaveSystem::setCurrentSlot(chosenSlot);
    SaveSystem::setBattleTargetSlot(-1);

    Director::getInstance()->replaceScene(
        TransitionFade::create(0.25f, MainScene::createScene())
    );
}

void MenuScene::openSaveSelector()
{
    // Create overlay on demand (so no hidden listeners can block touches)
    buildSaveUI();
    _saveMask->setVisible(true);
    refreshSaveList();
}

void MenuScene::closeSaveSelector()
{
    // Destroy overlay completely to avoid hidden nodes/listeners blocking touches
    if (_saveMask)
    {
        // Remove mouse listener to avoid blocking input after closing
        if (_saveMouseListener)
        {
            _eventDispatcher->removeEventListener(_saveMouseListener);
            _saveMouseListener = nullptr;
        }
        _savePanel = nullptr;
        _saveMask->removeFromParent();
        _saveMask = nullptr;
        _saveScroll = nullptr;
        _saveContent = nullptr;
    }
}

void MenuScene::buildSaveUI()
{
    if (_saveMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // Semi-transparent mask
    _saveMask = LayerColor::create(Color4B(0, 0, 0, 120));
    this->addChild(_saveMask, 10);

    const float panelW = 860.0f;
    const float panelH = 540.0f;

    auto panel = LayerColor::create(Color4B(240, 240, 240, 240), panelW, panelH);
    panel->setPosition(origin.x + visibleSize.width / 2 - panelW / 2,
        origin.y + visibleSize.height / 2 - panelH / 2);
    _saveMask->addChild(panel);


    _savePanel = panel;
    // Touch listener: only swallow touches OUTSIDE panel, so buttons inside panel still work.
    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [panel](Touch* t, Event*) {
        Vec2 world = t->getLocation();
        Vec2 local = panel->convertToNodeSpace(world);
        Rect pr(0, 0, panel->getContentSize().width, panel->getContentSize().height);
        // If touch inside panel -> do NOT swallow (return false), allow panel buttons to receive touches.
        return !pr.containsPoint(local);
        };
    maskListener->onTouchEnded = [this](Touch*, Event*) {
        // Touch ended outside panel -> close overlay
        this->closeSaveSelector();
        };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _saveMask);

    // Title
    auto header = Label::createWithSystemFont("Select Save", "Arial", 56);
    header->setColor(Color3B::BLACK);
    header->setPosition(Vec2(panelW / 2, panelH - 50));
    panel->addChild(header);

    // Close button (top-right of panel)
    auto closeLabel = Label::createWithSystemFont("X", "Arial", 44);
    closeLabel->setColor(Color3B::BLACK);
    auto closeItem = MenuItemLabel::create(closeLabel, [this](Ref*) {
        this->closeSaveSelector();
        });
    closeItem->setPosition(Vec2(panelW - 40, panelH - 45));
    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu, 2);

    // Scroll view
    _saveScroll = ui::ScrollView::create();
    _saveScroll->setDirection(ui::ScrollView::Direction::VERTICAL);
    _saveScroll->setBounceEnabled(true);
    _saveScroll->setContentSize(Size(panelW * 2 - 100, panelH - 120));

    _saveScroll->setAnchorPoint(Vec2::ZERO);
    _saveScroll->ignoreAnchorPointForPosition(false);

    _saveScroll->setPosition(Vec2(-800, 0));
    _saveScroll->setScrollBarEnabled(true);
    panel->addChild(_saveScroll);

    _saveContent = Node::create();
    _saveScroll->addChild(_saveContent);

    // Mouse wheel support (Win32): scroll the list when cursor is over the panel
    if (!_saveMouseListener)
    {
        _saveMouseListener = EventListenerMouse::create();
        _saveMouseListener->onMouseScroll = [this](Event* e)
            {
                auto m = static_cast<EventMouse*>(e);
                if (!_saveMask || !_saveMask->isVisible() || !_saveScroll || !_savePanel) return;

                // Only scroll when cursor is inside the panel
                Vec2 glPos = Director::getInstance()->convertToGL(Vec2(m->getCursorX(), m->getCursorY()));
                Vec2 local = _savePanel->convertToNodeSpace(glPos);
                Rect r(0, 0, _savePanel->getContentSize().width, _savePanel->getContentSize().height);
                if (!r.containsPoint(local)) return;

                float dy = m->getScrollY() * 60.0f; // wheel speed
                Vec2 pos = _saveScroll->getInnerContainerPosition();
                pos.y += dy;

                float minY = _saveScroll->getContentSize().height - _saveScroll->getInnerContainerSize().height;
                if (minY > 0) minY = 0;

                if (pos.y > 0) pos.y = 0;
                if (pos.y < minY) pos.y = minY;

                _saveScroll->setInnerContainerPosition(pos);
                e->stopPropagation();
            };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(_saveMouseListener, _saveMask);
    }
}

void MenuScene::refreshSaveList()
{
    if (!_saveScroll || !_saveContent) return;

    auto metasAll = SaveSystem::listAllSlots();
    std::vector<SaveMeta> metas;
    metas.reserve(metasAll.size());
    for (const auto& m : metasAll)
    {
        if (m.exists) metas.push_back(m);
    }

    _saveContent->removeAllChildren();

    if (metas.empty())
    {
        _saveScroll->setInnerContainerSize(_saveScroll->getContentSize());
        _saveContent->setPosition(Vec2::ZERO);

        auto msg = Label::createWithSystemFont(
            "No saves.\nPlease create a new save in menu.",
            "Arial", 30
        );
        msg->setColor(Color3B::BLACK);
        msg->setPosition(Vec2(_saveScroll->getContentSize().width * 0.5f + 400,
            _saveScroll->getContentSize().height * 0.5f + 100));
        _saveContent->addChild(msg);
        return;
    }

    const float rowH = 64.0f;
    float innerH = std::max(_saveScroll->getContentSize().height, rowH * metas.size());
    _saveScroll->setInnerContainerSize(Size(_saveScroll->getContentSize().width, innerH));
    _saveContent->setPosition(Vec2::ZERO);

    for (size_t i = 0; i < metas.size(); ++i)
    {
        const auto& meta = metas[i];
        float y = innerH - rowH * (i + 0.5f);

        auto rowBg = LayerColor::create(Color4B(220, 220, 220, 255),
            _saveScroll->getContentSize().width,
            rowH - 6);
        rowBg->setAnchorPoint(Vec2(0.5f, 0.5f));
        rowBg->setPosition(Vec2(_saveScroll->getContentSize().width * 0.5f, y));
        _saveContent->addChild(rowBg);

        std::string labelText = StringUtils::format("[%02d] %s", meta.slot + 1, meta.name.c_str());
        auto label = Label::createWithSystemFont(labelText, "Arial", 30);
        label->setAnchorPoint(Vec2(0.0f, 0.5f));
        label->setPosition(Vec2(18, rowH * 0.5f));
        label->setColor(Color3B::BLACK);
        rowBg->addChild(label);

        // Enter
        auto enterLabel = Label::createWithSystemFont("Enter", "Arial", 28);
        enterLabel->setColor(Color3B::BLACK);
        auto enterItem = MenuItemLabel::create(enterLabel, [meta](Ref*) {
            SaveSystem::setCurrentSlot(meta.slot);
            SaveSystem::setBattleTargetSlot(-1);
            Director::getInstance()->replaceScene(
                TransitionFade::create(0.25f, MainScene::createScene())
            );
            });

        // Delete
        auto delLabel = Label::createWithSystemFont("Delete", "Arial", 28);
        delLabel->setColor(Color3B::BLACK);
        auto delItem = MenuItemLabel::create(delLabel, [this, meta](Ref*) {
            SaveSystem::remove(meta.slot);
            this->refreshSaveList();
            });

        enterItem->setPosition(Vec2(rowBg->getContentSize().width - 970, rowH * 0.5f));
        delItem->setPosition(Vec2(rowBg->getContentSize().width - 870, rowH * 0.5f));

        auto menu = Menu::create(enterItem, delItem, nullptr);
        menu->setPosition(Vec2::ZERO);
        rowBg->addChild(menu);
    }
}