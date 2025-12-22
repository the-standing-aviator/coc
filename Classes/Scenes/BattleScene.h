#pragma once
#include "cocos2d.h"
#include <vector>

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

    cocos2d::Vec2 gridToWorld(int r, int c) const;
    void setBuildingVisualParams();

    // ESC menu (settings / back / quit)
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
};
