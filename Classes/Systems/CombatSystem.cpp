#include "Systems/CombatSystem.h"

#include "Patterns/AttackVisitor.h"
#include "GameObjects/Buildings/DefenseBuilding.h"
#include "GameObjects/Units/wall_breaker.h"

#include <algorithm>
#include <cmath>

using namespace cocos2d;

static const char* HPBAR_NAME = "__hpbar";
static const char* HPFILL_NAME = "__hpfill";
static const char* HPBG_NAME = "__hpbg";

bool CombatSystem::isInRange(const Vec2& a, const Vec2& b, float range)
{
    return a.distance(b) <= range;
}

static void punchScale(Sprite* s, int tag)
{
    if (!s) return;
    s->stopActionByTag(tag);
    float sx = s->getScaleX();
    float sy = s->getScaleY();
    auto a = Sequence::create(
        ScaleTo::create(0.05f, sx * 1.03f, sy * 1.03f),
        ScaleTo::create(0.06f, sx, sy),
        nullptr
    );
    a->setTag(tag);
    s->runAction(a);
}

static void showDamage(Node* parent, const Vec2& pos, int dmg)
{
    if (!parent) return;
    auto lab = Label::createWithSystemFont(StringUtils::format("-%d", dmg), "Arial", 20);
    if (!lab) return;
    lab->setPosition(pos + Vec2(0, 16));
    lab->setOpacity(230);
    parent->addChild(lab, 999);

    auto act = Sequence::create(
        Spawn::create(
            MoveBy::create(0.45f, Vec2(0, 28)),
            FadeOut::create(0.45f),
            nullptr
        ),
        RemoveSelf::create(),
        nullptr
    );
    lab->runAction(act);
}

void CombatSystem::ensureHpBar(Sprite* sprite, int hp, int hpMax, bool isUnit)
{
    if (!sprite) return;
    if (hpMax <= 0) hpMax = 1;
    if (hp < 0) hp = 0;
    if (hp > hpMax) hp = hpMax;

    Node* bar = sprite->getChildByName(HPBAR_NAME);
    LayerColor* bg = nullptr;
    LayerColor* fill = nullptr;

    // ¸üÐ¡Ò»µã
    const float w = isUnit ? 40.0f : 48.0f;
    const float h = 5.0f;
    const float pad = 1.0f;

    // ¡ï¹Ø¼ü£ºÓÃÃªµã×÷Îª»ù×¼£¬±£Ö¤¡°¾«×¼ÔÚµ¥Î»½ÅÏÂ¡±
    Vec2 ap = sprite->getAnchorPointInPoints();          // ÃªµãÔÚ sprite ±¾µØ×ø±êÖÐµÄÎ»ÖÃ
    float gapUnder = 7.0f;                               // Äã¿ÉÒÔÎ¢µ÷£º6~12 ¶¼ÐÐ
    float yUnit = ap.y - (h * 0.5f + gapUnder);          // µ¥Î»ÑªÌõ£ºÃªµãÕýÏÂ·½
    float yBuilding = sprite->getContentSize().height + 12.0f; // ½¨Öþ£ºÈÔÔÚÉÏ·½

    if (!bar)
    {
        bar = Node::create();
        bar->setName(HPBAR_NAME);

        // ÑªÌõ´óÐ¡²»Ëæ sprite Ëõ·Å±ä»¯
        float sx = std::max(0.001f, sprite->getScaleX());
        float sy = std::max(0.001f, sprite->getScaleY());
        bar->setScaleX(1.0f / sx);
        bar->setScaleY(1.0f / sy);

        // ¡ï¶¨Î»£ºµ¥Î»½ÅÏÂ / ½¨ÖþÉÏ·½
        bar->setPosition(Vec2(std::round(ap.x), std::round(isUnit ? yUnit : yBuilding)));
        sprite->addChild(bar, 999);

        bg = LayerColor::create(Color4B(0, 0, 0, 160), w, h);
        bg->setName(HPBG_NAME);
        bg->setIgnoreAnchorPointForPosition(false);
        bg->setAnchorPoint(Vec2(0.5f, 0.5f));
        bg->setPosition(Vec2(0, 0));
        bar->addChild(bg);

        float fw = std::max(1.0f, w - pad * 2.0f);
        float fh = std::max(1.0f, h - pad * 2.0f);
        fill = LayerColor::create(Color4B(60, 220, 90, 220), fw, fh);
        fill->setName(HPFILL_NAME);
        fill->setIgnoreAnchorPointForPosition(false);
        fill->setAnchorPoint(Vec2(0.0f, 0.5f));
        fill->setPosition(Vec2(-w * 0.5f + pad, 0));
        bar->addChild(fill);
    }
    else
    {
        // ¡ïÈç¹û sprite Ãªµã/³ß´ç²»Í¬£¬ÔËÐÐÊ±Ò²¸ú×ÅÖØÐÂ¶¨Î»Ò»´Î£¨±£³Ö¾«×¼£©
        bar->setPosition(Vec2(std::round(ap.x), std::round(isUnit ? yUnit : yBuilding)));

        bg = dynamic_cast<LayerColor*>(bar->getChildByName(HPBG_NAME));
        fill = dynamic_cast<LayerColor*>(bar->getChildByName(HPFILL_NAME));
    }

    if (!fill) return;

    float pct = (float)hp / (float)hpMax;
    pct = std::max(0.0f, std::min(1.0f, pct));
    fill->setScaleX(pct);

    // ÈÔ±£Áô£º½¨ÖþÂúÑªÒþ²Ø£¨ÄãÒ²¿ÉÒÔ¸Ä³ÉÒ»Ö±ÏÔÊ¾£©
    if (!isUnit) bar->setVisible(pct < 0.999f);
    else bar->setVisible(true);
}


bool CombatSystem::tryUnitAttackBuilding(UnitBase& attacker,
    Sprite* attackerSprite,
    Building& target,
    Sprite* targetSprite)
{
    if (!attackerSprite || !targetSprite) return false;
    if (attacker.isDead() || target.hp <= 0) return false;

    if (!attacker.canAttack()) return false;

    Vec2 ap = attackerSprite->getPosition();
    Vec2 tp = targetSprite->getPosition();

    if (!isInRange(ap, tp, attacker.attackRange))
        return false;

    int dmg = AttackVisitor::computeDamage(attacker, target);
    target.hp -= dmg;
    if (target.hp < 0) target.hp = 0;

    attacker.startAttackCooldown();

    // UI feedback
    punchScale(targetSprite, 12345);
    ensureHpBar(targetSprite, target.hp, target.hpMax, false);
    showDamage(targetSprite->getParent(), tp, dmg);

    return true;
}

bool CombatSystem::unitHitBuildingNoRange(UnitBase& attacker,
    Sprite* attackerSprite,
    Building& target,
    Sprite* targetSprite)
{
    if (!attackerSprite || !targetSprite) return false;
    if (attacker.isDead() || target.hp <= 0) return false;
    if (!attacker.canAttack()) return false;

    Vec2 tp = targetSprite->getPosition();

    int dmg = AttackVisitor::computeDamage(attacker, target);
    target.hp -= dmg;
    if (target.hp < 0) target.hp = 0;

    attacker.startAttackCooldown();

    punchScale(targetSprite, 12345);
    ensureHpBar(targetSprite, target.hp, target.hpMax, false);
    showDamage(targetSprite->getParent(), tp, dmg);
    return true;
}

bool CombatSystem::tryBomberExplode(UnitBase& bomber,
    Sprite* bomberSprite,
    EnemyBuildingRuntime& targetWall,
    std::vector<EnemyBuildingRuntime>& enemyBuildings)
{
    if (!bomberSprite || !targetWall.sprite || !targetWall.building) return false;
    if (bomber.isDead()) return false;
    if (targetWall.building->hp <= 0) return false;
    if (targetWall.id != 10) return false; // must be wall

    if (!bomber.canAttack()) return false;

    Vec2 bp = bomberSprite->getPosition();
    Vec2 wp = targetWall.sprite->getPosition();
    if (!isInRange(bp, wp, bomber.attackRange))
        return false;

    // Explode: deal high damage to nearby walls, then self-destruct.
    const WallBreaker* wb = dynamic_cast<const WallBreaker*>(&bomber);
    int multiplier = wb ? wb->wallDamageMultiplier : 40;
    float radiusTiles = wb ? wb->damageRadiusTiles : 2.0f;
    int wallDmg = std::max(1, bomber.damage * multiplier);

    int tr = targetWall.r;
    int tc = targetWall.c;

    for (auto& e : enemyBuildings)
    {
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;
        if (e.id != 10) continue;

        float dr = (float)(e.r - tr);
        float dc = (float)(e.c - tc);
        float dist = std::sqrt(dr * dr + dc * dc);
        if (dist > radiusTiles + 0.001f) continue;

        e.building->hp -= wallDmg;
        if (e.building->hp < 0) e.building->hp = 0;

        punchScale(e.sprite, 22345);
        ensureHpBar(e.sprite, e.building->hp, e.building->hpMax, false);
        showDamage(e.sprite->getParent(), e.sprite->getPosition(), wallDmg);
    }

    // self-destruct
    bomber.hp = 0;

    // small visual cue (placeholder)
    showDamage(bomberSprite->getParent(), bp, 999);
    bomber.startAttackCooldown();
    return true;
}

bool CombatSystem::bomberExplodeNoRange(UnitBase& bomber,
    Sprite* bomberSprite,
    EnemyBuildingRuntime& targetWall,
    std::vector<EnemyBuildingRuntime>& enemyBuildings)
{
    if (!bomberSprite || !targetWall.sprite || !targetWall.building) return false;
    if (bomber.isDead()) return false;
    if (targetWall.building->hp <= 0) return false;
    if (targetWall.id != 10) return false;

    if (!bomber.canAttack()) return false;

    // Reuse the same explode logic as tryBomberExplode, but WITHOUT any range check.
    const WallBreaker* wb = dynamic_cast<const WallBreaker*>(&bomber);
    int multiplier = wb ? wb->wallDamageMultiplier : 40;
    float radiusTiles = wb ? wb->damageRadiusTiles : 2.0f;
    int wallDmg = std::max(1, bomber.damage * multiplier);

    int tr = targetWall.r;
    int tc = targetWall.c;

    for (auto& e : enemyBuildings)
    {
        if (!e.building || e.building->hp <= 0 || !e.sprite) continue;
        if (e.id != 10) continue;

        float dr = (float)(e.r - tr);
        float dc = (float)(e.c - tc);
        float dist = std::sqrt(dr * dr + dc * dc);
        if (dist > radiusTiles + 0.001f) continue;

        e.building->hp -= wallDmg;
        if (e.building->hp < 0) e.building->hp = 0;

        punchScale(e.sprite, 22345);
        ensureHpBar(e.sprite, e.building->hp, e.building->hpMax, false);
        showDamage(e.sprite->getParent(), e.sprite->getPosition(), wallDmg);
    }

    // self-destruct
    bomber.hp = 0;

    Vec2 bp = bomberSprite->getPosition();
    showDamage(bomberSprite->getParent(), bp, 999);
    bomber.startAttackCooldown();
    return true;
}

bool CombatSystem::tryDefenseShoot(float dt,
    Building& defense,
    Sprite* defenseSprite,
    std::vector<BattleUnitRuntime>& units,
    float& cooldown,
    float cellSizePx)
{
    if (!defenseSprite) return false;
    if (defense.hp <= 0) return false;

    float dmgPerHit = 0.0f;
    float atkPerSec = 0.0f;
    int rangeCells = 0;

    if (auto t = dynamic_cast<ArrowTower*>(&defense))
    {
        dmgPerHit = t->damagePerHit;
        atkPerSec = t->attacksPerSecond;
        rangeCells = t->rangeCells;
    }
    else if (auto c = dynamic_cast<Cannon*>(&defense))
    {
        dmgPerHit = c->damagePerHit;
        atkPerSec = c->attacksPerSecond;
        rangeCells = c->rangeCells;
    }
    else
    {
        return false;
    }

    if (atkPerSec <= 0.0001f) return false;

    float atkInterval = 1.0f / atkPerSec;
    float rangePx = std::max(20.0f, (float)rangeCells * std::max(8.0f, cellSizePx));

    cooldown -= dt;
    if (cooldown > 0.0f) return false;

    // nearest unit in range
    int best = -1;
    float bestDist = 1e30f;
    Vec2 ep = defenseSprite->getPosition();

    for (int i = 0; i < (int)units.size(); ++i)
    {
        auto& u = units[i];
        if (!u.unit || !u.sprite) continue;
        if (u.unit->isDead()) continue;

        float d = u.sprite->getPosition().distance(ep);
        if (d <= rangePx && d < bestDist)
        {
            bestDist = d;
            best = i;
        }
    }

    if (best < 0) return false;

    int dmg = (int)std::ceil(std::max(1.0f, dmgPerHit));
    auto& victim = units[best];

    victim.unit->takeDamage(dmg);

    // UI feedback
    ensureHpBar(victim.sprite, victim.unit->hp, victim.unit->hpMax, true);
    showDamage(victim.sprite ? victim.sprite->getParent() : nullptr,
        victim.sprite ? victim.sprite->getPosition() : ep, dmg);

    cooldown = atkInterval;
    return true;
}
