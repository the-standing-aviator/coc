#include "GameObjects/Buildings/Building.h"
#include "GameObjects/Buildings/DefenseBuilding.h"
#include "GameObjects/Buildings/ResourceBuilding.h"
#include "GameObjects/Buildings/TroopBuilding.h"
#include "GameObjects/Buildings/TownHall.h"
#include "Managers/ConfigManager.h"
#include "Managers/ResourceManager.h"
#include <memory>

using namespace std;

namespace BuildingFactory {
    std::unique_ptr<Building> create(int id, int level) {
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
        default: b.reset(new Building()); break;
        }
        b->hpMax = 100;
        b->hp = 100;
        b->level = level;
        applyStats(b.get(), id, level);
        return b;
    }
    void applyStats(Building* b, int id, int level) {
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
            if (ec) { ec->setupStats(level); ec->stored = 0.f; }
        }
        else if (id == 4) {
            auto es = dynamic_cast<ElixirStorage*>(b);
            if (es) { es->setupStats(level); es->applyCap(); }
        }
        else if (id == 5) {
            auto gm = dynamic_cast<GoldMine*>(b);
            if (gm) { gm->setupStats(level); gm->stored = 0.f; }
        }
        else if (id == 6) {
            auto gs = dynamic_cast<GoldStorage*>(b);
            if (gs) { gs->setupStats(level); gs->applyCap(); }
        }
        else if (id == 9) {
            auto th = dynamic_cast<TownHall*>(b);
            if (th) { th->setupStats(level); th->applyCap(); }
        }
        else if (id == 10) {
            auto wl = dynamic_cast<Wall*>(b);
            if (wl) wl->setupStats();
        }
        else if (id == 7) {
            auto bk = dynamic_cast<Barracks*>(b);
            if (bk) {
                bk->setupStats(level);
                bk->applyCap(); // Á¢¼´Ó¦ÓÃ±øÓªÈÝÁ¿
            }
        }
        else if (id == 8) {
            auto tc = dynamic_cast<TrainingCamp*>(b);
            if (tc) tc->setupStats(level);
        }
    }
}