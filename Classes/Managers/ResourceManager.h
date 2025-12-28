#pragma once
#include <functional>

struct Resources {
    int elixir = 5000;
    int gold = 5000;
    int population = 0;
    int elixirCap = 5000;
    int goldCap = 5000;
    int populationCap = 0;
};

class ResourceManager {
public:
    static const Resources& get();
    static int getElixir();
    static int getGold();
    static int getPopulation();
    static int getElixirCap();
    static int getGoldCap();
    static int getPopulationCap();
    static void setElixir(int v);
    static void setGold(int v);
    static void setPopulation(int v);
    static void setElixirCap(int v);
    static void setGoldCap(int v);
    static void setPopulationCap(int v);
    static void addElixir(int v);
    static void addGold(int v);
    static void addPopulation(int v);
    static bool spendElixir(int v);
    static bool spendGold(int v);
    static bool spendPopulation(int v);
    static void reset();
    static void onChanged(const std::function<void(const Resources&)>& cb);
    // 人口容量管理
    static void addPopulationCap(int amount);
   
private:
    static void notify();
    static Resources _res;
    static std::function<void(const Resources&)> _callback;
    static int s_populationCap;
    static int s_population;
};