#include "UI/ResourcePanel.h"
using namespace cocos2d;
bool ResourcePanel::init() {
    // Smaller numbers on the left + a progress bar on the right.
    _goldLabel = Label::createWithSystemFont("", "Arial", 14);
    _elixirLabel = Label::createWithSystemFont("", "Arial", 14);
    _popLabel = Label::createWithSystemFont("", "Arial", 16);
    _goldLabel->setColor(Color3B::BLACK);
    _elixirLabel->setColor(Color3B::BLACK);
    _popLabel->setColor(Color3B::BLACK);
    _goldBg = LayerColor::create(Color4B::WHITE, 10, 10);
    _elixirBg = LayerColor::create(Color4B::WHITE, 10, 10);
    _popBg = LayerColor::create(Color4B::WHITE, 10, 10);

    // Bar backgrounds / fills
    _goldBarBg = LayerColor::create(Color4B(60, 60, 60, 220), 10, 10);
    _goldBarFill = LayerColor::create(Color4B(230, 200, 60, 255), 10, 10);
    _elixirBarBg = LayerColor::create(Color4B(60, 60, 60, 220), 10, 10);
    _elixirBarFill = LayerColor::create(Color4B(170, 90, 230, 255), 10, 10);
    _fillGoldBg = LayerColor::create(Color4B::WHITE, 10, 10);
    _fillElixirBg = LayerColor::create(Color4B::WHITE, 10, 10);
    _timeBg = LayerColor::create(Color4B::WHITE, 10, 10);
    addChild(_goldBg);
    addChild(_elixirBg);
    addChild(_popBg);
    addChild(_goldBarBg);
    addChild(_goldBarFill);
    addChild(_elixirBarBg);
    addChild(_elixirBarFill);
    addChild(_fillGoldBg);
    addChild(_fillElixirBg);
    addChild(_timeBg);
    addChild(_goldLabel);
    addChild(_elixirLabel);
    addChild(_popLabel);
    _menu = Menu::create();
    _menu->setPosition(Vec2::ZERO);
    addChild(_menu, 2);
    auto goldText = Label::createWithSystemFont("Fill", "Arial", 18);
    goldText->setColor(Color3B::BLACK);
    _fillGoldItem = MenuItemLabel::create(goldText, [](cocos2d::Ref*) {
        ResourceManager::setGold(ResourceManager::getGoldCap());
        });
    auto elixirText = Label::createWithSystemFont("Fill", "Arial", 18);
    elixirText->setColor(Color3B::BLACK);
    _fillElixirItem = MenuItemLabel::create(elixirText, [](cocos2d::Ref*) {
        ResourceManager::setElixir(ResourceManager::getElixirCap());
        });
    auto timeText = Label::createWithSystemFont("x100", "Arial", 18);
    timeText->setColor(Color3B::BLACK);
    _timeItem = MenuItemLabel::create(timeText, [this, timeText](cocos2d::Ref*) {
        _timeActive = !_timeActive;
        if (_timeActive) {
            timeText->setString("Cancel");
            if (onSetTimeScale) onSetTimeScale(100.f);
        }
        else {
            timeText->setString("x100");
            if (onSetTimeScale) onSetTimeScale(1.f);
        }
        });
    _menu->addChild(_fillGoldItem);
    _menu->addChild(_fillElixirItem);
    _menu->addChild(_timeItem);
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
    _gold = r.gold;
    _goldCap = std::max(1, r.goldCap);
    _elixir = r.elixir;
    _elixirCap = std::max(1, r.elixirCap);

    // Show only numbers so they don't overlap with the bars.
    _goldLabel->setString(cocos2d::StringUtils::format("%d/%d", r.gold, r.goldCap));
    _elixirLabel->setString(cocos2d::StringUtils::format("%d/%d", r.elixir, r.elixirCap));
    _popLabel->setString(cocos2d::StringUtils::format("Population: %d/%d", r.population, r.populationCap));
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

    // Fixed widths keep everything aligned and prevent overlaps.
    float textW = 95.0f * _scale;
    float barW = 170.0f * _scale;
    float barH = 14.0f * _scale;
    float rowH = std::max(barH, 20.0f * _scale) + _padY * 2;
    float popH = std::max(sizeOf(_popLabel).height, 20.0f * _scale) + _padY * 2;
    float bw = _padX * 2 + textW + 6.0f * _scale + barW;

    float x = origin.x + vs.width - bw;
    float yTop = origin.y + vs.height;

    _goldBg->setContentSize(Size(bw, rowH));
    _elixirBg->setContentSize(Size(bw, rowH));
    _popBg->setContentSize(Size(bw, popH));

    _goldBg->setPosition(Vec2(x, yTop - rowH));
    _elixirBg->setPosition(Vec2(x, yTop - rowH - _gap - rowH));
    _popBg->setPosition(Vec2(x, yTop - rowH - _gap - rowH - _gap - popH));

    _goldLabel->setAnchorPoint(Vec2(0.f, 0.5f));
    _elixirLabel->setAnchorPoint(Vec2(0.f, 0.5f));
    _popLabel->setAnchorPoint(Vec2(0.f, 0.5f));

    _goldLabel->setPosition(Vec2(x + _padX, yTop - rowH + rowH * 0.5f));
    _elixirLabel->setPosition(Vec2(x + _padX, yTop - rowH - _gap - rowH + rowH * 0.5f));
    _popLabel->setPosition(Vec2(x + _padX, yTop - rowH - _gap - rowH - _gap - popH + popH * 0.5f));

    // Bars: right side of each row
    float barX = x + _padX + textW + 6.0f * _scale;
    float goldCenterY = yTop - rowH + rowH * 0.5f;
    float elixirCenterY = yTop - rowH - _gap - rowH + rowH * 0.5f;

    _goldBarBg->setContentSize(Size(barW, barH));
    _goldBarBg->setAnchorPoint(Vec2(0.f, 0.5f));
    _goldBarBg->setPosition(Vec2(barX, goldCenterY));

    _goldBarFill->setContentSize(Size(barW, barH));
    _goldBarFill->setAnchorPoint(Vec2(0.f, 0.5f));
    _goldBarFill->setPosition(Vec2(barX, goldCenterY));
    _goldBarFill->setScaleX(std::max(0.0f, std::min(1.0f, (float)_gold / (float)_goldCap)));

    _elixirBarBg->setContentSize(Size(barW, barH));
    _elixirBarBg->setAnchorPoint(Vec2(0.f, 0.5f));
    _elixirBarBg->setPosition(Vec2(barX, elixirCenterY));

    _elixirBarFill->setContentSize(Size(barW, barH));
    _elixirBarFill->setAnchorPoint(Vec2(0.f, 0.5f));
    _elixirBarFill->setPosition(Vec2(barX, elixirCenterY));
    _elixirBarFill->setScaleX(std::max(0.0f, std::min(1.0f, (float)_elixir / (float)_elixirCap)));
    float bxw = 60.f * _scale;
    float bxX = x - bxw - 10.f;
    _fillGoldBg->setContentSize(Size(bxw, rowH));
    _fillElixirBg->setContentSize(Size(bxw, rowH));
    _timeBg->setContentSize(Size(bxw, popH));
    _fillGoldBg->setPosition(Vec2(bxX, yTop - rowH));
    _fillElixirBg->setPosition(Vec2(bxX, yTop - rowH - _gap - rowH));
    _timeBg->setPosition(Vec2(bxX, yTop - rowH - _gap - rowH - _gap - popH));
    if (_fillGoldItem) _fillGoldItem->setPosition(Vec2(bxX + bxw * 0.5f, yTop - rowH + rowH * 0.5f));
    if (_fillElixirItem) _fillElixirItem->setPosition(Vec2(bxX + bxw * 0.5f, yTop - rowH - _gap - rowH + rowH * 0.5f));
    if (_timeItem) _timeItem->setPosition(Vec2(bxX + bxw * 0.5f, yTop - rowH - _gap - rowH - _gap - popH + popH * 0.5f));
}