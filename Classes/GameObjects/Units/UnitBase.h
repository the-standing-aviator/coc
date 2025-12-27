#pragma once

#include "cocos2d.h"

#include <memory>
#include <string>

// UnitBase
// --------
// A lightweight unit definition (stats + a few helpers).
// We intentionally keep it similar to Building (plain class), so it can be
// serialized easily later if you want to store troops in JSON.
//
// When you implement battle logic, you can:
//  - create a Sprite by calling createSprite()
//  - update attack cooldown with tickAttack(dt)
//  - apply damage with takeDamage(...)
//
// NOTE: We do NOT start adding new files. Everything stays in existing files.
class UnitBase {
public:
    virtual ~UnitBase() = default;

    // Basic identity
    int unitId = 0;               // e.g. 1=Barbarian, 2=Archer
    int level = 1;
    std::string name;
    std::string image;            // sprite path in Resources (e.g. "Textures/Troops/barbarian.png")

    // Core stats
    int hpMax = 1;
    int hp = 1;
    int damage = 1;
    float attackInterval = 1.0f;  // seconds per hit
    float attackRange = 20.0f;    // pixels
    float moveSpeed = 60.0f;      // pixels per second

    // Training-related stats
    int housingSpace = 1;
    int costElixir = 0;
    int trainingTimeSec = 0;

    // Runtime helpers (not serialized by default)
    float attackCooldown = 0.0f;  // seconds

public:
    // Create a sprite for rendering this unit.
    // If image is missing, return a simple placeholder sprite with a label.
    virtual cocos2d::Sprite* createSprite() const;

    // Reset runtime status (e.g. when reusing from a pool)
    virtual void reset();

    // Apply damage. Returns current hp after applying.
    virtual int takeDamage(int amount);

    bool isDead() const { return hp <= 0; }

    // Convenience
    float getDPS() const { return attackInterval <= 0.0001f ? (float)damage : (float)damage / attackInterval; }

    // Attack cooldown helpers
    void tickAttack(float dt);     // reduce cooldown
    bool canAttack() const { return attackCooldown <= 0.0f; }
    void startAttackCooldown();    // set cooldown to attackInterval
};

// Optional factory helpers (kept in the same file for convenience)
namespace UnitFactory {
    std::unique_ptr<UnitBase> create(int unitId, int level);
}