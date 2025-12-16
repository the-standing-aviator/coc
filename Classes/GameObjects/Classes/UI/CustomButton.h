#pragma once
#include "cocos2d.h"
#include <functional>
class CustomButton {
public:
    static cocos2d::LayerColor* createUpgradePanel(const std::string& title, const std::string& resName, int amount, bool disabled, bool isMaxLevel, const std::function<void()>& onUpgrade, const std::function<void()>& onCancel);
};
