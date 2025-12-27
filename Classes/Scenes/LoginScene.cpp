#include "Scenes/LoginScene.h"

#include "Data/SaveSystem.h"
#include "Scenes/MainScene.h"
#include "Scenes/MenuScene.h"
#include "Managers/SoundManager.h"

#include "ui/CocosGUI.h"

USING_NS_CC;

Scene* LoginScene::createScene()
{
    return LoginScene::create();
}

bool LoginScene::init()
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

    // Clear battle target when entering selector
    SaveSystem::setBattleTargetSlot(-1);

    // Build and show selector
    buildSaveUI();
    openSaveSelector();

    // UI jingle on entering save selector.
    SoundManager::playSfx("music/loading_screen_jingle.ogg", 1.0f);

    return true;
}

void LoginScene::backToMenu()
{
    closeSaveSelector();
    Director::getInstance()->replaceScene(
        TransitionFade::create(0.25f, MenuScene::createScene())
    );
}

void LoginScene::openSaveSelector()
{
    if (!_saveMask) buildSaveUI();
    _saveMask->setVisible(true);
    refreshSaveList();
}

void LoginScene::closeSaveSelector()
{
    if (_saveMask) _saveMask->setVisible(false);
}

void LoginScene::buildSaveUI()
{
    if (_saveMask) return;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // mask
    _saveMask = LayerColor::create(Color4B(0, 0, 0, 120));
    this->addChild(_saveMask, 10);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, _saveMask);

    const float panelW = 860.0f;
    const float panelH = 540.0f;

    auto panel = LayerColor::create(Color4B(240, 240, 240, 240), panelW, panelH);
    panel->setPosition(origin.x + visibleSize.width / 2 - panelW / 2,
        origin.y + visibleSize.height / 2 - panelH / 2);
    _saveMask->addChild(panel);

    auto panelListener = EventListenerTouchOneByOne::create();
    panelListener->setSwallowTouches(true);
    panelListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(panelListener, panel);

    // Title
    auto header = Label::createWithSystemFont("Select Save", "Arial", 56);
    header->setColor(Color3B::BLACK);
    header->setPosition(Vec2(panelW / 2, panelH - 50));
    panel->addChild(header);

    // Top-right back button to MenuScene
    auto backLabel = Label::createWithSystemFont("Back", "Arial", 36);
    backLabel->setColor(Color3B::BLACK);
    auto backItem = MenuItemLabel::create(backLabel, [this](Ref*) {
        this->backToMenu();
        });
    backItem->setPosition(Vec2(panelW - 70, panelH - 55));
    auto backMenu = Menu::create(backItem, nullptr);
    backMenu->setPosition(Vec2::ZERO);
    panel->addChild(backMenu, 2);

    // Scroll view
    _saveScroll = ui::ScrollView::create();
    _saveScroll->setDirection(ui::ScrollView::Direction::VERTICAL);
    _saveScroll->setBounceEnabled(true);
    _saveScroll->setContentSize(Size(panelW - 40, panelH - 120));
    _saveScroll->setAnchorPoint(Vec2(0.5f, 0.5f));
    _saveScroll->setPosition(Vec2(panelW / 2, panelH / 2 - 30));
    _saveScroll->setScrollBarEnabled(true);
    panel->addChild(_saveScroll);

    _saveContent = Node::create();
    _saveScroll->addChild(_saveContent);
}

void LoginScene::refreshSaveList()
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
        _saveContent->setPosition(Vec2(_saveScroll->getContentSize().width * 0.5f,
            _saveScroll->getContentSize().height));

        auto msg = Label::createWithSystemFont("No saves.\nGo back and create a new save.", "Arial", 30);
        msg->setColor(Color3B::BLACK);
        msg->setPosition(Vec2(_saveScroll->getContentSize().width * 0.5f,
            _saveScroll->getContentSize().height * 0.5f));
        _saveContent->addChild(msg);
        return;
    }

    const float rowH = 64.0f;
    float innerH = std::max(_saveScroll->getContentSize().height, rowH * metas.size());
    _saveScroll->setInnerContainerSize(Size(_saveScroll->getContentSize().width, innerH));
    _saveContent->setPosition(Vec2(_saveScroll->getContentSize().width * 0.5f, innerH));

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

        enterItem->setPosition(Vec2(rowBg->getContentSize().width - 170, rowH * 0.5f));
        delItem->setPosition(Vec2(rowBg->getContentSize().width - 70, rowH * 0.5f));

        auto menu = Menu::create(enterItem, delItem, nullptr);
        menu->setPosition(Vec2::ZERO);
        rowBg->addChild(menu);
    }
}