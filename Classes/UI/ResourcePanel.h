#pragma once
#include "cocos2d.h"
#include "Managers/ResourceManager.h"
class ResourcePanel : public cocos2d::Node {
public:
    CREATE_FUNC(ResourcePanel);
    virtual bool init() override;
    void setPanelScale(float s);
private:
    void layout();
    void updateTexts(const Resources& r);
    cocos2d::Label* _goldLabel = nullptr;
    cocos2d::Label* _elixirLabel = nullptr;
    cocos2d::Label* _popLabel = nullptr;
    cocos2d::LayerColor* _goldBg = nullptr;
    cocos2d::LayerColor* _elixirBg = nullptr;
    cocos2d::LayerColor* _popBg = nullptr;
    float _scale = 1.0f;
    float _padX = 10.f;
    float _padY = 6.f;
    float _gap = 6.f;
};
