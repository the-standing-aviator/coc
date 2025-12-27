#pragma once
#include "cocos2d.h"
#include <vector>
#include <unordered_map>

#include "Systems/AISystem.h"

class BattleScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    virtual void update(float dt) override;
    CREATE_FUNC(BattleScene);

private:
    void renderTargetVillage();

    // ===== Grid helpers =====
    cocos2d::Vec2 gridToWorld(int r, int c) const;
    cocos2d::Vec2 worldToGridLocal(const cocos2d::Vec2& localPos) const;
    bool isPointInCellDiamondLocal(const cocos2d::Vec2& localPos, int r, int c) const;

    void setBuildingVisualParams();

    void buildEnemyOccupyGrid(bool outOcc[30][30]) const;
    bool isAdjacentToOccupied(const bool occ[30][30], int r, int c) const;
    bool findDeployCellFromClick(const cocos2d::Vec2& clickLocalPos, int& outR, int& outC) const;

    // ===== Battle countdown =====
    enum class Phase { Scout, Battle, End };
    void setupBattleHUD();
    void startPhase(Phase p, float durationSec);
    void updateBattleHUD();
    void showReturnButton();

    // ===== Deploy UI (Troop Bar) =====
    void setupTroopBar();
    void refreshTroopBar();

    bool handleTroopBarClick(const cocos2d::Vec2& glPos);
    bool isPosInTroopBar(const cocos2d::Vec2& glPos) const;
    void setSelectedTroop(int troopType);
    void deploySelectedTroop(const cocos2d::Vec2& glPos);
    void showBattleToast(const std::string& msg);

    void enableDeployInput(bool enabled);
    bool spawnUnit(int unitId, const cocos2d::Vec2& clickLocalPos);
    void checkBattleResult(bool timeUp);

private:
    Phase _phase = Phase::Scout;
    float _phaseRemaining = 0.0f;
    float _phaseTotal = 1.0f;

    cocos2d::Node* _hud = nullptr;
    cocos2d::Label* _phaseLabel = nullptr;
    cocos2d::Label* _timeLabel = nullptr;
    cocos2d::LayerColor* _barBg = nullptr;
    cocos2d::LayerColor* _barFill = nullptr;
    cocos2d::Menu* _returnMenu = nullptr;

    void openEscMenu();
    void closeEscMenu();
    void openSettings();
    cocos2d::LayerColor* _escMask = nullptr;
    cocos2d::LayerColor* _settingsMask = nullptr;

    cocos2d::Node* _world = nullptr;
    cocos2d::Sprite* _background = nullptr;

    int _rows = 30;
    int _cols = 30;
    float _tileW = 0.0f;
    float _tileH = 0.0f;
    cocos2d::Vec2 _anchor;

    float _buildingScale = 1.0f;
    std::vector<float> _buildingScaleById;
    std::vector<cocos2d::Vec2> _buildingOffsetById;

    AISystem _ai;
    float _cellSizePx = 32.0f;

    std::vector<EnemyBuildingRuntime> _enemyBuildings;
    std::vector<BattleUnitRuntime> _units;

    struct TroopSlot {
        int type = -1;
        cocos2d::Node* root = nullptr;
        cocos2d::Label* countLabel = nullptr;
        cocos2d::Label* selectedLabel = nullptr;
    };

    cocos2d::Node* _troopBar = nullptr;
    std::unordered_map<int, int> _troopCounts;
    std::vector<TroopSlot> _troopSlots;
    int _selectedTroopType = -1;

    long long _lastDeployMs = 0;

    bool _deployEnabled = false;
    bool _battleEnded = false;
};
