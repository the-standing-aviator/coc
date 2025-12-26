#include "GameObjects/Units/UnitBase.h"

#include "GameObjects/Units/Barbarian.h"
#include "GameObjects/Units/Archer.h"
#include "GameObjects/Units/Giant.h"
#include "GameObjects/Units/wall_breaker.h"

using namespace cocos2d;

Sprite* UnitBase::createSprite() const
{
    auto s = Sprite::create(image);
    if (!s)
    {
        // Placeholder: an empty sprite with a label.
        auto node = Sprite::create();
        node->setTextureRect(Rect(0, 0, 64, 64));
        node->setColor(Color3B(200, 200, 200));

        auto label = Label::createWithSystemFont(name.empty() ? "Unit" : name, "Arial", 16);
        label->setPosition(Vec2(32, 32));
        label->setColor(Color3B::BLACK);
        node->addChild(label);
        return node;
    }
    return s;
}

void UnitBase::reset()
{
    hp = hpMax;
    attackCooldown = 0.0f;
}

int UnitBase::takeDamage(int amount)
{
    if (amount <= 0) return hp;
    hp -= amount;
    if (hp < 0) hp = 0;
    return hp;
}

void UnitBase::tickAttack(float dt)
{
    if (attackCooldown <= 0.0f) return;
    attackCooldown -= dt;
    if (attackCooldown < 0.0f) attackCooldown = 0.0f;
}

void UnitBase::startAttackCooldown()
{
    attackCooldown = attackInterval;
}

namespace UnitFactory {

std::unique_ptr<UnitBase> create(int unitId, int level)
{
    // NOTE: level scaling is kept minimal for now.
    // You can later read JSON config and apply multipliers by level.
    switch (unitId)
    {
    case 1: {
        auto u = std::make_unique<Barbarian>();
        u->level = level;
        return u;
    }
    case 2: {
        auto u = std::make_unique<Archer>();
        u->level = level;
        return u;
    }
    default:
        break;
    }
    return nullptr;
}

} // namespace UnitFactory
