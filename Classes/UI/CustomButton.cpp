#include "UI/CustomButton.h"

using namespace cocos2d;

static const char* INNER_PANEL_NAME = "__inner_upgrade_panel";
static const char* MODAL_MASK_NAME = "__modal_mask__";

LayerColor* CustomButton::createUpgradePanel(
    const std::string& title,
    const std::string& resName,
    int amount,
    bool disabled,
    bool isMaxLevel,
    const std::function<bool()>& onUpgrade,
    const std::function<void()>& onCancel)
{
    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    // Full-screen modal mask (fully transparent). It swallows touches so that
    // the scene below will NEVER receive input until the modal is closed.
    auto mask = LayerColor::create(Color4B(0, 0, 0, 0), vs.width, vs.height);
    mask->setName(MODAL_MASK_NAME);
    mask->setIgnoreAnchorPointForPosition(false);
    mask->setAnchorPoint(Vec2(0, 0));
    mask->setPosition(origin);

    auto maskListener = EventListenerTouchOneByOne::create();
    maskListener->setSwallowTouches(true);
    maskListener->onTouchBegan = [](Touch*, Event*) { return true; };
    mask->getEventDispatcher()->addEventListenerWithSceneGraphPriority(maskListener, mask);

    // On desktop, some interactions (e.g., dragging buildings) are driven by mouse events.
    // Touch swallowing does NOT block EventListenerMouse, so we must stop mouse propagation here.
    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseDown = [](Event* e) {
        if (auto* ev = dynamic_cast<EventMouse*>(e)) { ev->stopPropagation(); }
    };
    mouseListener->onMouseUp = [](Event* e) {
        if (auto* ev = dynamic_cast<EventMouse*>(e)) { ev->stopPropagation(); }
    };
    mouseListener->onMouseMove = [](Event* e) {
        if (auto* ev = dynamic_cast<EventMouse*>(e)) { ev->stopPropagation(); }
    };
    mouseListener->onMouseScroll = [](Event* e) {
        if (auto* ev = dynamic_cast<EventMouse*>(e)) { ev->stopPropagation(); }
    };
    mask->getEventDispatcher()->addEventListenerWithSceneGraphPriority(mouseListener, mask);

    // Inner white panel
    auto panel = LayerColor::create(Color4B::WHITE, vs.width * 0.4f, vs.height * 0.3f);
    panel->setName(INNER_PANEL_NAME);
    panel->setIgnoreAnchorPointForPosition(false);
    panel->setAnchorPoint(Vec2(0, 0));
    panel->setPosition(Vec2(origin.x + vs.width * 0.3f, origin.y + vs.height * 0.35f));
    mask->addChild(panel, 1);

    auto name = Label::createWithSystemFont(title, "Arial", 22);
    name->setColor(Color3B::BLACK);
    name->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 30.f));
    panel->addChild(name, 1);

    auto menu = Menu::create();
    menu->setPosition(Vec2::ZERO);
    panel->addChild(menu, 2);

    if (isMaxLevel)
    {
        auto info = Label::createWithSystemFont("Already Max Level", "Arial", 20);
        info->setColor(Color3B::BLACK);
        info->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height * 0.6f));
        panel->addChild(info, 1);

        auto btnCancelLabel = Label::createWithSystemFont("Cancel", "Arial", 20);
        btnCancelLabel->setColor(Color3B::BLACK);
        auto btnCancel = MenuItemLabel::create(btnCancelLabel, [onCancel, mask](Ref*) {
            if (onCancel) onCancel();
            mask->removeFromParent();
        });

        float w = panel->getContentSize().width;
        float y = 40.f;
        btnCancel->setPosition(Vec2(w * 0.5f, y));
        menu->addChild(btnCancel);

        return mask;
    }

    // Normal upgrade panel
    auto line1 = Label::createWithSystemFont(StringUtils::format("Required: %s", resName.c_str()), "Arial", 20);
    line1->setColor(Color3B::BLACK);
    line1->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 70.f));
    panel->addChild(line1, 1);

    auto line2 = Label::createWithSystemFont(StringUtils::format("Amount: %d", amount), "Arial", 20);
    line2->setColor(Color3B::BLACK);
    line2->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 100.f));
    panel->addChild(line2, 1);

    auto btnUpgradeLabel = Label::createWithSystemFont("Upgrade", "Arial", 20);
    auto btnCancelLabel = Label::createWithSystemFont("Cancel", "Arial", 20);
    btnUpgradeLabel->setColor(disabled ? Color3B(150, 150, 150) : Color3B::BLACK);
    btnCancelLabel->setColor(Color3B::BLACK);

    auto btnUpgrade = MenuItemLabel::create(btnUpgradeLabel, [onUpgrade, disabled, mask](Ref*) {
        if (disabled) return;
        bool shouldClose = true;
        if (onUpgrade) shouldClose = onUpgrade();
        if (shouldClose) mask->removeFromParent();
    });
    btnUpgrade->setEnabled(!disabled);

    auto btnCancel = MenuItemLabel::create(btnCancelLabel, [onCancel, mask](Ref*) {
        if (onCancel) onCancel();
        mask->removeFromParent();
    });

    float w = panel->getContentSize().width;
    float y = 40.f;
    btnUpgrade->setPosition(Vec2(w * 0.35f, y));
    btnCancel->setPosition(Vec2(w * 0.65f, y));
    menu->addChild(btnUpgrade);
    menu->addChild(btnCancel);

    return mask;
}
