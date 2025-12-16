#pragma once
#include "cocos2d.h"
#include "Managers/ResourceManager.h"
#include <functional>
class ResourcePanel : public cocos2d::Node {
public:
    CREATE_FUNC(ResourcePanel);
    virtual bool init() override;
    void setPanelScale(float s);
    std::function<void(float)> onSetTimeScale;
private:
    void layout();
    void updateTexts(const Resources& r);
    cocos2d::Label* _goldLabel = nullptr;
    cocos2d::Label* _elixirLabel = nullptr;
    cocos2d::Label* _popLabel = nullptr;
    cocos2d::LayerColor* _goldBg = nullptr;
    cocos2d::LayerColor* _elixirBg = nullptr;
    cocos2d::LayerColor* _popBg = nullptr;
    cocos2d::LayerColor* _fillGoldBg = nullptr;
    cocos2d::LayerColor* _fillElixirBg = nullptr;
    cocos2d::LayerColor* _timeBg = nullptr;
    cocos2d::Menu* _menu = nullptr;
    cocos2d::MenuItemLabel* _fillGoldItem = nullptr;
    cocos2d::MenuItemLabel* _fillElixirItem = nullptr;
    cocos2d::MenuItemLabel* _timeItem = nullptr;
    bool _timeActive = false;
    float _scale = 1.0f;
    float _padX = 10.f;
    float _padY = 6.f;
    float _gap = 6.f;
};
