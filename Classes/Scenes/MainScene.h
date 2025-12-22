#pragma once
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <vector>
#include <memory>
#include <unordered_map>
struct SaveBuilding;
class MainScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init();
    CREATE_FUNC(MainScene);

    void setGridVisible(bool visible);
    cocos2d::Vec2 gridToWorld(int r, int c) const;
    cocos2d::Vec2 worldToGrid(const cocos2d::Vec2& pos) const;
    cocos2d::Vec2 imageUvToWorld(const cocos2d::Vec2& uv) const;
    void setZoom(float z);
    void setupInteraction();
    virtual void update(float dt) override;
    void setTimeScale(float s);
    virtual void onExit() override;
private:
    struct PlacedBuilding { int id; int r; int c; cocos2d::Sprite* sprite; std::shared_ptr<class Building> data; };
    std::vector<PlacedBuilding> _buildings;
    bool _moving = false;
    int _movingIndex = -1;
    bool canPlaceIgnoring(int r, int c, int ignoreIndex) const;
    int findBuildingCenter(int r, int c) const;
    void redrawOccupied();
    void commitMove(int r, int c);
    void cancelMove();
    void buildGrid();
    int _rows = 30;
    int _cols = 30;
    float _tileW = 0.f;
    float _tileH = 0.f;
    cocos2d::Vec2 _anchor;
    cocos2d::DrawNode* _grid = nullptr;
    cocos2d::Sprite* _background = nullptr;
    cocos2d::Node* _world = nullptr;
    float _zoom = 1.0f;
    float _minZoom = 0.6f;
    float _maxZoom = 2.0f;
    bool _dragging = false;
    cocos2d::Vec2 _lastMouse;
    void clampWorld();

    std::vector<std::vector<int>> _occupy;
    cocos2d::DrawNode* _hint = nullptr;
    cocos2d::DrawNode* _occupied = nullptr;
    bool _placing = false;
    int _placingId = 0;
    void startPlacement(int id);
    void cancelPlacement();
    bool canPlace(int r, int c) const;
    void drawCellFilled(int r, int c, const cocos2d::Color4F& color, cocos2d::DrawNode* layer);
    void placeBuilding(int r, int c, int id);

    void setBuildingScale(float s);
    float _buildingScale = 1.0f;
    void setBuildingScaleForId(int id, float s);
    void setBuildingOffsetForId(int id, const cocos2d::Vec2& off);
    std::vector<float> _buildingScaleById;
    std::vector<cocos2d::Vec2> _buildingOffsetById;

public:
    void setResourceUiScale(float s);
    void setBattleButtonScale(float s);
private:
    void openUpgradeWindowForIndex(int idx);
    int getTownHallLevel() const;
    float _timeScale = 1.0f;
    int countById(int id) const;
    int buildLimitForId(int id) const;

    cocos2d::MenuItemImage* _battleButton = nullptr;

private:
    // ESC menu (in-game pause/settings menu)
    void openEscMenu();
    void closeEscMenu();
    void openSettings();

    cocos2d::LayerColor* _escMask = nullptr;
    cocos2d::LayerColor* _settingsMask = nullptr;
    // Save/load helpers
    void loadFromCurrentSaveOrCreate();
    void saveToCurrentSlot(bool force);
    void placeBuildingLoaded(int r, int c, const SaveBuilding& info);

    bool _saveDirty = false;
    float _autosaveTimer = 0.0f;

    // Attack picker
    void openAttackTargetPicker();
    void closeAttackTargetPicker();
    cocos2d::LayerColor* _attackMask = nullptr;
    cocos2d::Node* _attackPanel = nullptr;
    cocos2d::ui::ScrollView* _attackScroll = nullptr;
    cocos2d::Node* _attackContent = nullptr;
    cocos2d::EventListenerMouse* _attackMouseListener = nullptr;
};
