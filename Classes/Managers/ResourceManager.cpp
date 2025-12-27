#include "Managers/ResourceManager.h"
#include <algorithm>

Resources ResourceManager::_res;
std::function<void(const Resources&)> ResourceManager::_callback;
int ResourceManager::s_populationCap = 0;
int ResourceManager::s_population = 0;

void ResourceManager::addPopulationCap(int amount) {
    s_populationCap += amount;
    s_populationCap = std::max(0, s_populationCap);
}
void ResourceManager::setPopulation(int pop) {
    s_population = std::max(0, std::min(pop, s_populationCap));
}

const Resources& ResourceManager::get() { return _res; }
int ResourceManager::getElixir() { return _res.elixir; }
int ResourceManager::getGold() { return _res.gold; }
int ResourceManager::getPopulation() { return _res.population; }
int ResourceManager::getElixirCap() { return _res.elixirCap; }
int ResourceManager::getGoldCap() { return _res.goldCap; }
int ResourceManager::getPopulationCap() { return _res.populationCap; }

void ResourceManager::setElixirCap(int v) {
    _res.elixirCap = std::max(0, v);
    if (_res.elixirCap > 0) _res.elixir = std::min(_res.elixir, _res.elixirCap);
    notify();
}
void ResourceManager::setGoldCap(int v) {
    _res.goldCap = std::max(0, v);
    if (_res.goldCap > 0) _res.gold = std::min(_res.gold, _res.goldCap);
    notify();
}
void ResourceManager::setPopulationCap(int v) {
    _res.populationCap = std::max(0, v);
    if (_res.populationCap > 0) _res.population = std::min(_res.population, _res.populationCap);
    notify();
}

void ResourceManager::setElixir(int v) { _res.elixir = std::max(0, v); notify(); }
void ResourceManager::setGold(int v) { _res.gold = std::max(0, v); notify(); }

void ResourceManager::addElixir(int v) { setElixir(_res.elixir + v); }
void ResourceManager::addGold(int v) { setGold(_res.gold + v); }
void ResourceManager::addPopulation(int v) { setPopulation(_res.population + v); }

bool ResourceManager::spendElixir(int v) {
    if (v <= 0) return true;
    if (_res.elixir < v) return false;
    _res.elixir -= v;
    notify();
    return true;
}

bool ResourceManager::spendGold(int v) {
    if (v <= 0) return true;
    if (_res.gold < v) return false;
    _res.gold -= v;
    notify();
    return true;
}

bool ResourceManager::spendPopulation(int v) {
    if (v <= 0) return true;
    if (_res.population < v) return false;
    _res.population -= v;
    notify();
    return true;
}

void ResourceManager::reset() {
    _res = Resources();
    notify();
}

void ResourceManager::onChanged(const std::function<void(const Resources&)>& cb) {
    _callback = cb;
    notify();
}

void ResourceManager::notify() {
    if (_callback) _callback(_res);
}