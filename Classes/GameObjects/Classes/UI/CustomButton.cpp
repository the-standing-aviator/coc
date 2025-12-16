#include "UI/CustomButton.h"
using namespace cocos2d;
LayerColor* CustomButton::createUpgradePanel(const std::string& title, const std::string& resName, int amount, bool disabled, bool isMaxLevel, const std::function<void()>& onUpgrade, const std::function<void()>& onCancel) {
    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    auto panel = LayerColor::create(Color4B::WHITE, vs.width * 0.4f, vs.height * 0.3f);
    panel->setPosition(Vec2(origin.x + vs.width * 0.3f, origin.y + vs.height * 0.35f));
    auto name = Label::createWithSystemFont(title, "Arial", 22);
    name->setColor(Color3B::BLACK);
    name->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 30.f));
    panel->addChild(name, 1);
    if (isMaxLevel) {
        auto info = Label::createWithSystemFont("Already Max Level", "Arial", 20);
        info->setColor(Color3B::BLACK);
        info->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height * 0.6f));
        panel->addChild(info, 1);
        auto menu = Menu::create();
        menu->setPosition(Vec2::ZERO);
        panel->addChild(menu, 2);
        auto btnCancelLabel = Label::createWithSystemFont("Cancel", "Arial", 20);
        btnCancelLabel->setColor(Color3B::BLACK);
        auto btnCancel = MenuItemLabel::create(btnCancelLabel, [onCancel, panel](Ref*) {
            if (onCancel) onCancel();
            panel->removeFromParent();
        });
        float w = panel->getContentSize().width;
        float y = 40.f;
        btnCancel->setPosition(Vec2(w * 0.5f, y));
        menu->addChild(btnCancel);
        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [](Touch*, Event*) { return true; };
        panel->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, panel);
    } else {
        auto line1 = Label::createWithSystemFont(StringUtils::format("Required: %s", resName.c_str()), "Arial", 20);
        line1->setColor(Color3B::BLACK);
        line1->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 70.f));
        panel->addChild(line1, 1);
        auto line2 = Label::createWithSystemFont(StringUtils::format("Amount: %d", amount), "Arial", 20);
        line2->setColor(Color3B::BLACK);
        line2->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 100.f));
        panel->addChild(line2, 1);
        auto menu = Menu::create();
        menu->setPosition(Vec2::ZERO);
        panel->addChild(menu, 2);
        auto btnUpgradeLabel = Label::createWithSystemFont("Upgrade", "Arial", 20);
        auto btnCancelLabel = Label::createWithSystemFont("Cancel", "Arial", 20);
        btnUpgradeLabel->setColor(disabled ? Color3B(150,150,150) : Color3B::BLACK);
        btnCancelLabel->setColor(Color3B::BLACK);
        auto btnUpgrade = MenuItemLabel::create(btnUpgradeLabel, [onUpgrade, disabled, panel](Ref*) {
            if (disabled) return;
            if (onUpgrade) onUpgrade();
            panel->removeFromParent();
        });
        btnUpgrade->setEnabled(!disabled);
        auto btnCancel = MenuItemLabel::create(btnCancelLabel, [onCancel, panel](Ref*) {
            if (onCancel) onCancel();
            panel->removeFromParent();
        });
        float w = panel->getContentSize().width;
        float y = 40.f;
        btnUpgrade->setPosition(Vec2(w * 0.35f, y));
        btnCancel->setPosition(Vec2(w * 0.65f, y));
        menu->addChild(btnUpgrade);
        menu->addChild(btnCancel);
        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [](Touch*, Event*) { return true; };
        panel->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, panel);
    }
    return panel;
}
