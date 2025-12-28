#include "GameObjects/Buildings/Building.h"
#include "GameObjects/Buildings/DefenseBuilding.h"
#include "GameObjects/Buildings/ResourceBuilding.h"
#include "GameObjects/Buildings/TroopBuilding.h"
#include "GameObjects/Buildings/TownHall.h"
#include "Managers/ConfigManager.h"
#include "Managers/ResourceManager.h"
#include <algorithm>
#include <memory>

using namespace std;

// Resolve resource in a way that respects cocos2d-x search paths.
// IMPORTANT: We prefer keeping canonical relative paths ("ui/...", "buildings/..."),
// because many projects already add "Resources" as a search path.
static bool CanResolveResource(const std::string& relPath)
{
    auto fu = cocos2d::FileUtils::getInstance();
    const std::string full = fu->fullPathForFilename(relPath);
    return !full.empty();
}

static std::string FallbackResourcePath(const std::string& relPath)
{
    // Keep canonical relative paths whenever possible.
    if (CanResolveResource(relPath)) return relPath;

    const std::string tries[] = {
        std::string("Resources/") + relPath,
        std::string("resources/") + relPath,
        std::string("../Resources/") + relPath,
        std::string("../../Resources/") + relPath,
    };
    for (const auto& p : tries) {
        if (CanResolveResource(p)) return p;
    }
    return relPath;
}


// Select sprite image path by building type and level.
// NOTE:
// - buildings1 (ArrowTower), buildings2 (Cannon), buildings5 (GoldMine) stay as old images.
// - Others should switch to the leveled images: buildings/building{index}L{level}.png
//   where index mapping follows the resource folder convention.
static void updateBuildingImageByLevel(Building* b, int id, int level)
{
    if (!b) return;

    // Clamp to existing resource set (project uses 1..5 levels).
    level = std::max(1, std::min(5, level));

    int imageIndex = -1;
    std::string fallback;

    switch (id)
    {
    case 9:  // Town Hall uses building0L*
        imageIndex = 0;
        fallback = "buildings/buildings0.png";
        break;
    case 3:  // Elixir Collector uses building3L*
        imageIndex = 3;
        fallback = "buildings/buildings3.png";
        break;
    case 4:  // Elixir Storage uses building4L*
        imageIndex = 4;
        fallback = "buildings/buildings4.png";
        break;
    case 6:  // Gold Storage uses building6L*
        imageIndex = 6;
        fallback = "buildings/buildings6.png";
        break;
    case 7:  // Barracks (Army Camp) uses building7L*
        imageIndex = 7;
        fallback = "buildings/buildings7.png";
        break;
    case 8:  // Training Camp uses building8L*
        imageIndex = 8;
        fallback = "buildings/buildings8.png";
        break;
    case 10: // Wall uses building9L*
        imageIndex = 9;
        fallback = "buildings/buildings9.png";
        break;
    case 11: // Laboratory uses building11L*
        imageIndex = 11;
        fallback = "buildings/building11L1.png";
        break;
    default:
        // Keep the original image for buildings1/2/5 and any other types.
        return;
    }

    // Keep canonical relative paths whenever possible so Sprite::create(...) works with existing
    // cocos2d-x search paths (most projects already add "Resources" as a search path).
    std::string leveled = "buildings/building" + std::to_string(imageIndex) + "L" + std::to_string(level) + ".png";
    leveled = FallbackResourcePath(leveled);
    if (CanResolveResource(leveled)) {
        b->image = leveled;
        return;
    }

    // Fall back to old non-leveled assets.
    fallback = FallbackResourcePath(fallback);
    if (CanResolveResource(fallback)) {
        b->image = fallback;
        return;
    }

    // Absolute last-resort placeholder to keep the building visible.
    b->image = "buildings/buildings0.png";
}

namespace BuildingFactory {
    std::unique_ptr<Building> create(int id, int level, bool applyCaps, bool resetStored) {
        std::unique_ptr<Building> b;
        switch (id) {
            case 1: b.reset(new ArrowTower()); break;
            case 2: b.reset(new Cannon()); break;
            case 3: b.reset(new ElixirCollector()); break;
            case 4: b.reset(new ElixirStorage()); break;
            case 5: b.reset(new GoldMine()); break;
            case 6: b.reset(new GoldStorage()); break;
            case 7: b.reset(new Barracks()); break;
            case 8: b.reset(new TrainingCamp()); break;
            case 9: b.reset(new TownHall()); break;
            case 10: b.reset(new Wall()); break;
            case 11: b.reset(new Laboratory()); break;
            default: b.reset(new Building()); break;
        }
        b->hpMax = 100;
        b->hp = 100;
        b->level = level;
        applyStats(b.get(), id, level, applyCaps, resetStored);
        return b;
    }
    void applyStats(Building* b, int id, int level, bool applyCaps, bool resetStored) {
        if (!b) return;
        if (id == 1) {
            auto at = dynamic_cast<ArrowTower*>(b);
            if (at) at->setupStats(level);
        }
        else if (id == 2) {
            auto cn = dynamic_cast<Cannon*>(b);
            if (cn) cn->setupStats(level);
        }
        else if (id == 3) {
            auto ec = dynamic_cast<ElixirCollector*>(b);
            if (ec) {
                ec->setupStats(level);
                if (resetStored) ec->stored = 0.f;
            }
        }
        else if (id == 4) {
            auto es = dynamic_cast<ElixirStorage*>(b);
            if (es) {
                es->setupStats(level);
                if (applyCaps) es->applyCap();
            }
        }
        else if (id == 5) {
            auto gm = dynamic_cast<GoldMine*>(b);
            if (gm) {
                gm->setupStats(level);
                if (resetStored) gm->stored = 0.f;
            }
        }
        else if (id == 6) {
            auto gs = dynamic_cast<GoldStorage*>(b);
            if (gs) {
                gs->setupStats(level);
                if (applyCaps) gs->applyCap();
            }
        }
        else if (id == 9) {
            auto th = dynamic_cast<TownHall*>(b);
            if (th) {
                th->setupStats(level);
                if (applyCaps) th->applyCap();
            }
        }
        else if (id == 10) {
            auto wl = dynamic_cast<Wall*>(b);
            if (wl) wl->setupStats(level);
        }
        else if (id == 7) {
            auto bk = dynamic_cast<Barracks*>(b);
            if (bk) {
                bk->setupStats(level);
                if (applyCaps) bk->applyCap(); // Apply population cap immediately if allowed.
            }
        }
        else if (id == 8) {
            auto tc = dynamic_cast<TrainingCamp*>(b);
            if (tc) tc->setupStats(level);
        }
        else if (id == 11) {
            auto lab = dynamic_cast<Laboratory*>(b);
            if (lab) lab->setupStats(level);
        }

        // Update sprite image after stats (some buildings switch image by level).
        updateBuildingImageByLevel(b, id, level);
    }
}