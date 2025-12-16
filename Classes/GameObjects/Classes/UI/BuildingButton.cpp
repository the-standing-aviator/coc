#include "UI/BuildingButton.h"
using namespace cocos2d;

bool BuildingButton::init()
{
    auto vs = Director::getInstance()->getVisibleSize();
    std::string img = "ui/build_button.png";
    if (FileUtils::getInstance()->isFileExist(img)) {
        _item = MenuItemImage::create(img, img, [this](Ref*) { if (onClicked) onClicked(); });
    }
    else {
        auto label = Label::createWithSystemFont("Build", "Arial", 28);
        _item = MenuItemLabel::create(label, [this](Ref*) { if (onClicked) onClicked(); });
        _item->setContentSize(Size(120, 50));
    }
    _item->setScale(_scale);
    _menu = Menu::create(_item, nullptr);
    _menu->setPosition(Vec2::ZERO);
    addChild(_menu);
    setContentSize(vs);
    layout();
    return true;
}

void BuildingButton::layout()
{
    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    if (!_item) return;
    auto sz = _item->getContentSize();
    float sx = _item->getScaleX();
    float sy = _item->getScaleY();
    float halfW = sz.width * sx * 0.5f;
    float halfH = sz.height * sy * 0.5f;
    float x = origin.x + vs.width - _offset.x - halfW;
    float y = origin.y + _offset.y + halfH;
    _item->setPosition(Vec2(x, y));
}

void BuildingButton::setButtonScale(float s)
{
    _scale = s;
    if (_item) _item->setScale(_scale);
    layout();
}

void BuildingButton::setButtonOffset(const Vec2& o)
{
    _offset = o;
    layout();
}