#pragma once
#include "cocos2d.h"

class LoginScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    CREATE_FUNC(LoginScene);

private:
    void openSettings();
};
