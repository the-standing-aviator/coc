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
            auto label = cocos2d::Label::createWithSystemFont("B", "Arial", 14);
            auto node = cocos2d::Sprite::create();
            node->addChild(label);
            return node;
        }

        // Show building level label in the center surface of the building sprite.
        // This should work both in main village and in battle scene, because both call createSprite().
        static const int kLevelLabelTag = 20251225;
        auto old = s->getChildByTag(kLevelLabelTag);
        if (old) old->removeFromParent();

        std::string txt = "level" + std::to_string(level);
        auto lvl = cocos2d::Label::createWithSystemFont(txt, "Arial", 16);
        lvl->setTag(kLevelLabelTag);
        lvl->setColor(cocos2d::Color3B::BLACK);
        lvl->enableOutline(cocos2d::Color4B::WHITE, 2);
        lvl->setPosition(cocos2d::Vec2(s->getContentSize().width * 0.5f,
            s->getContentSize().height * 0.55f));
        s->addChild(lvl, 99);

        return s;
    }
};

namespace BuildingFactory {
    std::unique_ptr<Building> create(int id, int level);
    void applyStats(Building* b, int id, int level);
}