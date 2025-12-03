// HelloWorldScene.cpp
#include "MainMenuScene.h"
#include "audio/include/SimpleAudioEngine.h"   // 背景音乐要用到

USING_NS_CC;

Scene* MainMenu::createScene()
{
    return MainMenu::create();
}

bool MainMenu::init()
{
    if (!Scene::init()) return false;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 高清村庄背景
    auto bgSprite = Sprite::create("village_bg.jpg");
    if (bgSprite) {
        bgSprite->setPosition(Vec2(visibleSize.width / 2 + origin.x,
            visibleSize.height / 2 + origin.y));
        bgSprite->setAnchorPoint(Vec2::ANCHOR_MIDDLE);  // 中心对齐

        // 自动计算缩放比例（保持宽高比，完美填充）
        float scaleX = visibleSize.width / bgSprite->getContentSize().width;
        float scaleY = visibleSize.height / bgSprite->getContentSize().height;
        bgSprite->setScale(std::min(scaleX, scaleY));  // 用最小比例，避免变形

        this->addChild(bgSprite, -10);  // z-order -10，放到底层
    }

    // 开始游戏按钮
    auto startItem = MenuItemFont::create("START GAME", [](Ref* sender) {
        Director::getInstance()->replaceScene(MainMenu::createScene()); // 先留着，后面换成村庄
        });
    startItem->setColor(Color3B::WHITE);
    startItem->setScale(2.0f);

    auto menu = Menu::create(startItem, nullptr);
    menu->setPosition(Vec2(visibleSize.width / 2 + origin.x,
        visibleSize.height * 0.4 + origin.y));
    this->addChild(menu);

    // 播放背景音乐
    CocosDenshion::SimpleAudioEngine::getInstance()->playBackgroundMusic("bgm.mp3", true);

    return true;
}