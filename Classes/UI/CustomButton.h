#pragma once
#include "cocos2d.h"
#include <functional>
class CustomButton {
public:
    // onUpgrade returns true if the upgrade is successfully started/applied.
    // The panel will only auto-close when onUpgrade returns true.
    static cocos2d::LayerColor* createUpgradePanel(const std::string& title, const std::string& resName, int amount, bool disabled, bool isMaxLevel, const std::function<bool()>& onUpgrade, const std::function<void()>& onCancel);
};