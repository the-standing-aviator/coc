#pragma once
#include "cocos2d.h"
#include <functional>
#include <string>

class BuildingButton : public cocos2d::Node {
public:
    CREATE_FUNC(BuildingButton);
    std::function<void()> onClicked;
    virtual bool init() override;
    void setButtonScale(float s);
    void setButtonOffset(const cocos2d::Vec2& o);
private:
    void layout();
    cocos2d::MenuItem* _item = nullptr;
    cocos2d::Menu* _menu = nullptr;
    float _scale = 1.0f;
    cocos2d::Vec2 _offset = cocos2d::Vec2(16.f, 16.f);
};