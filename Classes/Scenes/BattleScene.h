#pragma once
#include "cocos2d.h"
#include <vector>
#include <unordered_map>

class BattleScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    virtual void update(float dt) override;
    CREATE_FUNC(BattleScene);

private:
    void renderTargetVillage();

    // ===== Battle countdown (NEW) =====
    // 45s scout phase -> 180s battle phase -> show return button.
    enum class Phase { Scout, Battle, End };
    void setupBattleHUD();
    void setupLootHUD();
    void updateLootHUD();

    void setupTroopBar();
    void refreshTroopBar();

    // Troop selection & deployment
    bool handleTroopBarClick(const cocos2d::Vec2& glPos); // return true if click was consumed by troop bar
    bool isPosInTroopBar(const cocos2d::Vec2& glPos) const; // UI hit test without changing selection
    bool isPosInDeployArea(const cocos2d::Vec2& worldLocal) const;
    void setSelectedTroop(int troopType);                 // troopType uses TrainingCamp::TroopType values (1..4), -1 to clear
    void deploySelectedTroop(const cocos2d::Vec2& glPos); // deploy 1 troop at world position
    void showBattleToast(const std::string& msg);
    void setZoom(float z);
    void clampWorld();
    void startPhase(Phase p, float durationSec);
    void updateBattleHUD();
    void showReturnButton();

    Phase _phase = Phase::Scout;
    float _phaseRemaining = 0.0f;
    float _phaseTotal = 1.0f;

    cocos2d::Node* _hud = nullptr;
    cocos2d::Label* _phaseLabel = nullptr;
    cocos2d::Label* _timeLabel = nullptr;
    cocos2d::LayerColor* _barBg = nullptr;
    cocos2d::LayerColor* _barFill = nullptr;
    cocos2d::Menu* _returnMenu = nullptr;

    // Loot UI (top-left): lootable resources and looted resources.
    cocos2d::Node* _lootHud = nullptr;
    cocos2d::Label* _lootableTitle = nullptr;
    cocos2d::Label* _lootedTitle = nullptr;
    cocos2d::LayerColor* _lootableGoldBg = nullptr;
    cocos2d::LayerColor* _lootableGoldFill = nullptr;
    cocos2d::Label* _lootableGoldText = nullptr;
    cocos2d::LayerColor* _lootableElixirBg = nullptr;
    cocos2d::LayerColor* _lootableElixirFill = nullptr;
    cocos2d::Label* _lootableElixirText = nullptr;
    cocos2d::LayerColor* _lootedGoldBg = nullptr;
    cocos2d::LayerColor* _lootedGoldFill = nullptr;
    cocos2d::Label* _lootedGoldText = nullptr;
    cocos2d::LayerColor* _lootedElixirBg = nullptr;
    cocos2d::LayerColor* _lootedElixirFill = nullptr;
    cocos2d::Label* _lootedElixirText = nullptr;

    int _lootGoldTotal = 0;
    int _lootElixirTotal = 0;
    int _lootedGold = 0;
    int _lootedElixir = 0;

    cocos2d::Vec2 gridToWorld(int r, int c) const;
    void setBuildingVisualParams();

    // ESC menu (settings / back / quit)
    void openEscMenu();
    void closeEscMenu();
    void openSettings();
    cocos2d::LayerColor* _escMask = nullptr;
    cocos2d::LayerColor* _settingsMask = nullptr;

    cocos2d::Node* _world = nullptr;
    // Zoom & pan for enemy village viewing
    float _zoom = 1.0f;
    float _minZoom = 0.5f;
    float _maxZoom = 2.5f;
    bool _dragging = false;
    cocos2d::Vec2 _dragLast = cocos2d::Vec2::ZERO;

    struct TroopSlot {
        int type = -1;
        cocos2d::Node* root = nullptr;
        cocos2d::Label* countLabel = nullptr;
        cocos2d::Label* selectedLabel = nullptr;
    };

    std::unordered_map<int, int> _troopCounts; // local battle session counts
    std::vector<TroopSlot> _troopSlots;
    int _selectedTroopType = -1;
    
    long long _lastDeployMs = 0;
bool _hasDeployedAnyTroop = false;

    // Mouse click vs drag handling
    bool _mouseDown = false;
    bool _mouseConsumed = false;
    bool _mouseMoved = false;
    cocos2d::Vec2 _mouseDownPos = cocos2d::Vec2::ZERO;

    // Touch input (for laptops / touch screens). This coexists with mouse input.
    bool _touchDown = false;
    bool _touchConsumed = false;
    bool _touchMoved = false;
    cocos2d::Vec2 _touchDownPos = cocos2d::Vec2::ZERO;

        // Bottom troop bar (READY troops from MainScene)
    cocos2d::Node* _troopBar = nullptr;
    cocos2d::Sprite* _background = nullptr;
    int _rows = 30;
    int _cols = 30;
    float _tileW = 0.0f;
    float _tileH = 0.0f;
    cocos2d::Vec2 _anchor;

    // Deployable diamond area in _world node space (cached from background UV points).
    bool _deployAreaReady = false;
    cocos2d::Vec2 _deployTop = cocos2d::Vec2::ZERO;
    cocos2d::Vec2 _deployRight = cocos2d::Vec2::ZERO;
    cocos2d::Vec2 _deployBottom = cocos2d::Vec2::ZERO;
    cocos2d::Vec2 _deployLeft = cocos2d::Vec2::ZERO;
    float _buildingScale = 1.0f;
    std::vector<float> _buildingScaleById;
    std::vector<cocos2d::Vec2> _buildingOffsetById;
};