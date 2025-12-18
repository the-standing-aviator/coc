#include "Scenes/LoginScene.h"
#include "Scenes/MainScene.h"
#include "Managers/SoundManager.h"
#include "ui/CocosGUI.h"

#ifdef _WIN32
#include <windows.h>
#endif

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

    // ===========================
    // Background (保留你的背景图逻辑)
    // ===========================
    auto bg = Sprite::create("background.jpg");
    if (!bg)
        bg = Sprite::create("backgrounds/village_map.jpg");

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

    // ===========================
    // Title
    // ===========================
    auto title = Label::createWithSystemFont("Clash Start", "Arial", 110);
    title->setPosition(origin + Vec2(visibleSize.width / 2, visibleSize.height - 150));
    this->addChild(title);

    // ===========================
    // Start Game -> friend's game scene (MainScene)
    // ===========================
    auto startLabel = Label::createWithSystemFont("Start Game", "Arial", 60);
    auto startItem = MenuItemLabel::create(startLabel, [&](Ref*) {
        Director::getInstance()->replaceScene(
            TransitionFade::create(0.35f, MainScene::createScene())
        );
        });
    startItem->setPosition(origin + Vec2(visibleSize.width / 2, visibleSize.height / 2 + 40));

    // ===========================
    // Settings
    // ===========================
    auto settingLabel = Label::createWithSystemFont("Settings", "Arial", 50);
    auto settingItem = MenuItemLabel::create(settingLabel, [&](Ref*) {
        this->openSettings();
        });
    settingItem->setPosition(origin + Vec2(visibleSize.width / 2, visibleSize.height / 2 - 40));

    // ===========================
    // Exit
    // ===========================
    auto exitLabel = Label::createWithSystemFont("Exit Game", "Arial", 50);
    auto exitItem = MenuItemLabel::create(exitLabel, [&](Ref*) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
        ExitProcess(0);
#else
        Director::getInstance()->end();
#endif
        });
    exitItem->setPosition(origin + Vec2(visibleSize.width / 2, visibleSize.height / 2 - 120));

    auto menu = Menu::create(startItem, settingItem, exitItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    this->addChild(menu);

    return true;
}

void LoginScene::openSettings()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // 1) Mask layer（吞掉触摸，防止点穿到 Start/Settings）
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(mask, 100);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(maskListener, mask);

    // 2) Panel
    const float panelW = 640.0f;
    const float panelH = 420.0f;

    auto panel = LayerColor::create(Color4B(70, 70, 70, 255), panelW, panelH);
    panel->setPosition(origin.x + visibleSize.width / 2 - panelW / 2,
        origin.y + visibleSize.height / 2 - panelH / 2);
    mask->addChild(panel);

    // 面板也吞触摸（更稳）
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

    // 4) Master volume slider (bind to SoundManager)
    auto volLabel = Label::createWithSystemFont("Master Volume", "Arial", 32);
    volLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    volLabel->setPosition(Vec2(60, 260));
    panel->addChild(volLabel);

    // 当前主音量（0~1），用它初始化滑条和mute状态
    float curVol = SoundManager::getVolume();

    auto volSlider = ui::Slider::create();
    volSlider->loadBarTexture("ui/sliderTrack.png");
    volSlider->loadSlidBallTextures("ui/sliderThumb.png", "ui/sliderThumb.png", "");
    volSlider->loadProgressBarTexture("ui/sliderProgress.png");
    volSlider->setPosition(Vec2(420, 260));
    volSlider->setPercent((int)(curVol * 100.0f + 0.5f));  // ✅ 不再固定80
    panel->addChild(volSlider);

    // 5) Mute checkbox
    auto mute = ui::CheckBox::create(
        "ui/checkbox_off.png",
        "ui/checkbox_on.png"
    );
    mute->setPosition(Vec2(110, 160));
    mute->setSelected(curVol <= 0.001f); // ✅ 根据当前音量初始化
    panel->addChild(mute);

    auto muteLabel = Label::createWithSystemFont("Mute", "Arial", 32);
    muteLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
    muteLabel->setPosition(Vec2(150, 160));
    panel->addChild(muteLabel);

    // 滑条事件：实时写入主音量，并同步 mute 状态
    volSlider->addEventListener([=](Ref* sender, ui::Slider::EventType type) {
        if (type == ui::Slider::EventType::ON_PERCENTAGE_CHANGED)
        {
            auto s = static_cast<ui::Slider*>(sender);
            float v = s->getPercent() / 100.0f;
            SoundManager::setVolume(v);
            mute->setSelected(v <= 0.001f);
        }
        });

    // mute事件：选中设0；取消则恢复为滑条位置
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
    auto closeItem = MenuItemLabel::create(closeLabel, [=](Ref*) {
        mask->removeFromParent();
        });
    closeItem->setPosition(Vec2(panelW - 40, panelH - 40));

    auto closeMenu = Menu::create(closeItem, nullptr);
    closeMenu->setPosition(Vec2::ZERO);
    panel->addChild(closeMenu);
}


