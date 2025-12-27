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

    // 禁放区：建筑占用 + 周围 1 格缓冲（含对角）
    void buildEnemyForbiddenGrid(bool outForbid[30][30]) const;

    // 从点击点得到所在格子（只用于判定合法性），不做“吸附/找最近”
    bool getCellFromClick(const cocos2d::Vec2& clickLocalPos, int& outR, int& outC) const;

    // ===== Battle countdown =====
    enum class Phase { Scout, Battle, End };
    void setupBattleHUD();
    void startPhase(Phase p, float durationSec);
    void updateBattleHUD();
    void showReturnButton();

    // ===== Deploy UI (Troop Bar) =====
    void setupTroopBar();
    void refreshTroopBar();

    bool handleTroopBarClick(const cocos2d::Vec2& glPos); // click on bar consumes
    bool isPosInTroopBar(const cocos2d::Vec2& glPos) const;
    void setSelectedTroop(int troopType);                 // 1..4, -1 clear
    void deploySelectedTroop(const cocos2d::Vec2& glPos); // deploy at exact click pos
    void showBattleToast(const std::string& msg);

    void enableDeployInput(bool enabled);
    bool spawnUnit(int unitId, const cocos2d::Vec2& clickLocalPos);
    void checkBattleResult(bool timeUp);

    // ESC menu
    void openEscMenu();
    void closeEscMenu();
    void openSettings();

private:
    Phase _phase = Phase::Scout;
    float _phaseRemaining = 0.0f;
    float _phaseTotal = 1.0f;

    // HUD
    cocos2d::Node* _hud = nullptr;
    cocos2d::Label* _phaseLabel = nullptr;
    cocos2d::Label* _timeLabel = nullptr;
    cocos2d::LayerColor* _barBg = nullptr;
    cocos2d::LayerColor* _barFill = nullptr;
    cocos2d::Menu* _returnMenu = nullptr;

    cocos2d::LayerColor* _escMask = nullptr;
    cocos2d::LayerColor* _settingsMask = nullptr;

    // World & background
    cocos2d::Node* _world = nullptr;
    cocos2d::Sprite* _background = nullptr;

    // Map grid
    int _rows = 30;
    int _cols = 30;
    float _tileW = 0.0f;
    float _tileH = 0.0f;
    cocos2d::Vec2 _anchor;

    // Building visuals
    float _buildingScale = 1.0f;
    std::vector<float> _buildingScaleById;
    std::vector<cocos2d::Vec2> _buildingOffsetById;

    // Battle runtime
    AISystem _ai;
    float _cellSizePx = 32.0f;

    std::vector<EnemyBuildingRuntime> _enemyBuildings;
    std::vector<BattleUnitRuntime> _units;

    // Troop bar UI state
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
