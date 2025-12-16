#pragma once
#include "cocos2d.h"
#include <string>

class Building {
public:
    virtual ~Building() = default;
    int hpMax = 100;
    int hp = 100;
    int level = 1;
    bool isProducer = false;
    enum { NONE = 0, GOLD = 1, ELIXIR = 2 } produceType = NONE;
    int ratePerHour = 0;
    int capacity = 0;
    float stored = 0.f;
    int chunkMinutes = 0;
    std::string image;
    virtual cocos2d::Sprite* createSprite() const {
        auto s = cocos2d::Sprite::create(image);
        if (!s) {
            auto label = cocos2d::Label::createWithSystemFont("B", "Arial", 24);
            auto node = cocos2d::Sprite::create();
            node->addChild(label);
            return node;
        }
        return s;
    }
};
