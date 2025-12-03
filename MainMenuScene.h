#pragma once

#include "cocos2d.h"

class MainMenu : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();        // 创建场景的函数
    virtual bool init() override;                // 初始化函数

    CREATE_FUNC(MainMenu);
};
