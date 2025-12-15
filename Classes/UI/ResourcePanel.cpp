#include "UI/ResourcePanel.h"
using namespace cocos2d;
bool ResourcePanel::init() {
    _goldLabel = Label::createWithSystemFont("", "Arial", 20);
    _elixirLabel = Label::createWithSystemFont("", "Arial", 20);
    _popLabel = Label::createWithSystemFont("", "Arial", 20);
    _goldLabel->setColor(Color3B::BLACK);
    _elixirLabel->setColor(Color3B::BLACK);
    _popLabel->setColor(Color3B::BLACK);
    _goldBg = LayerColor::create(Color4B::WHITE, 10, 10);
    _elixirBg = LayerColor::create(Color4B::WHITE, 10, 10);
    _popBg = LayerColor::create(Color4B::WHITE, 10, 10);
    addChild(_goldBg);
    addChild(_elixirBg);
    addChild(_popBg);
    addChild(_goldLabel);
    addChild(_elixirLabel);
    addChild(_popLabel);
    updateTexts(ResourceManager::get());
    layout();
    ResourceManager::onChanged([this](const Resources& r) {
        updateTexts(r);
        layout();
    });
    return true;
}
void ResourcePanel::setPanelScale(float s) {
    _scale = std::max(0.5f, std::min(3.0f, s));
    layout();
}
void ResourcePanel::updateTexts(const Resources& r) {
    _goldLabel->setString(cocos2d::StringUtils::format("Gold:%d/%d", r.gold, r.goldCap));
    _elixirLabel->setString(cocos2d::StringUtils::format("Elixir:%d/%d", r.elixir, r.elixirCap));
    _popLabel->setString(cocos2d::StringUtils::format("Population:%d/%d", r.population, r.populationCap));
}
void ResourcePanel::layout() {
    auto vs = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    auto sizeOf = [this](Label* lb) {
        Size sz = lb->getContentSize();
        return Size(sz.width * _scale, sz.height * _scale);
    };
    _goldLabel->setScale(_scale);
    _elixirLabel->setScale(_scale);
    _popLabel->setScale(_scale);
    Size sg = sizeOf(_goldLabel);
    Size se = sizeOf(_elixirLabel);
    Size sp = sizeOf(_popLabel);
    float bw = std::max(std::max(sg.width, se.width), sp.width) + _padX * 2;
    float hg = sg.height + _padY * 2;
    float he = se.height + _padY * 2;
    float hp = sp.height + _padY * 2;
    float x = origin.x + vs.width - bw;
    float yTop = origin.y + vs.height;
    _goldBg->setContentSize(Size(bw, hg));
    _elixirBg->setContentSize(Size(bw, he));
    _popBg->setContentSize(Size(bw, hp));
    _goldBg->setPosition(Vec2(x, yTop - hg));
    _elixirBg->setPosition(Vec2(x, yTop - hg - _gap - he));
    _popBg->setPosition(Vec2(x, yTop - hg - _gap - he - _gap - hp));
    _goldLabel->setAnchorPoint(Vec2(0.f, 0.5f));
    _elixirLabel->setAnchorPoint(Vec2(0.f, 0.5f));
    _popLabel->setAnchorPoint(Vec2(0.f, 0.5f));
    _goldLabel->setPosition(Vec2(x + _padX, yTop - hg + hg * 0.5f));
    _elixirLabel->setPosition(Vec2(x + _padX, yTop - hg - _gap - he + he * 0.5f));
    _popLabel->setPosition(Vec2(x + _padX, yTop - hg - _gap - he - _gap - hp + hp * 0.5f));
}
