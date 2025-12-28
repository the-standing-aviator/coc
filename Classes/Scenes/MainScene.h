// File: MainScene.h
// Brief: Declares the MainScene component.
#pragma once
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
// SaveBuilding encapsulates related behavior and state.
struct SaveBuilding;
// SaveData encapsulates related behavior and state.
struct SaveData;
// MainScene encapsulates related behavior and state.
class MainScene : public cocos2d::Scene
{
public:
    // Creates an instance.
    static cocos2d::Scene* createScene();
    // Initializes the object.
    virtual bool init();
    CREATE_FUNC(MainScene);

    // Sets the GridVisible.

    void setGridVisible(bool visible);
    // TODO: Add a brief description.
    cocos2d::Vec2 gridToWorld(int r, int c) const;
    // TODO: Add a brief description.
    cocos2d::Vec2 worldToGrid(const cocos2d::Vec2& pos) const;
    // TODO: Add a brief description.
    cocos2d::Vec2 imageUvToWorld(const cocos2d::Vec2& uv) const;
    // Sets the Zoom.
    void setZoom(float z);
    // Sets the upInteraction.
    void setupInteraction();
    // Updates the object state.
    virtual void update(float dt) override;
    // Sets the TimeScale.
    void setTimeScale(float s);
    // Handles an event callback.
    virtual void onExit() override;
private:

    // TODO: Add a brief description.

    void showMainToast(const std::string& msg);


    // Returns the ActiveBuilderCount.


    int getActiveBuilderCount() const;


    // TODO: Add a brief description.


    void attachBuildTimerUI(cocos2d::Sprite* sprite, float totalSec, float remainSec);
    // Updates the object state.
    void updateBuildTimerUI(cocos2d::Sprite* sprite, float totalSec, float remainSec);
    // Removes an item.
    void removeBuildTimerUI(cocos2d::Sprite* sprite);
    // Updates the object state.
    void updateBuildSystems(float dt);
    // StandTroopInfo encapsulates related behavior and state.
    struct StandTroopInfo
    {
        int type = 0;
        int r = 0;
        int c = 0;
        cocos2d::Sprite* sprite = nullptr;
    };

    // Removes an item.

    void removeStandTroopOfType(int troopType);
    // Saves data to storage.
    void restoreStandTroopsFromSave(const SaveData& data);
    // TODO: Add a brief description.
    void applyStandTroopsToTrainingCamps();

    // PlacedBuilding encapsulates related behavior and state.

    struct PlacedBuilding { int id; int r; int c; cocos2d::Sprite* sprite; std::shared_ptr<class Building> data; };
    std::vector<PlacedBuilding> _buildings;
    bool _moving = false;
    int _movingIndex = -1;
    // TODO: Add a brief description.
    bool canPlaceIgnoring(int r, int c, int ignoreIndex) const;
    // TODO: Add a brief description.
    int findBuildingCenter(int r, int c) const;
    // TODO: Add a brief description.
    void redrawOccupied();
    // TODO: Add a brief description.
    void commitMove(int r, int c);
    // TODO: Add a brief description.
    void cancelMove();
    // Builds and configures resources.
    void buildGrid();
    int _rows = 30;
    int _cols = 30;
    float _tileW = 0.f;
    float _tileH = 0.f;
    cocos2d::Vec2 _anchor;
    cocos2d::DrawNode* _grid = nullptr;
    cocos2d::Sprite* _background = nullptr;
    cocos2d::Node* _world = nullptr;

    std::vector<std::string> _backgroundOptions;
    size_t _backgroundIndex = 0;

    cocos2d::Node* _standTroopLayer = nullptr;
    std::vector<StandTroopInfo> _standTroops;

    float _zoom = 1.0f;
    float _minZoom = 0.6f;
    float _maxZoom = 2.0f;
    bool _dragging = false;
    cocos2d::Vec2 _lastMouse;
    // TODO: Add a brief description.
    void clampWorld();

    std::vector<std::vector<int>> _occupy;
    cocos2d::DrawNode* _hint = nullptr;
    cocos2d::DrawNode* _occupied = nullptr;
    bool _placing = false;
    int _placingId = 0;
    // TODO: Add a brief description.
    void startPlacement(int id);
    // TODO: Add a brief description.
    void cancelPlacement();
    // TODO: Add a brief description.
    bool canPlace(int r, int c) const;
    // TODO: Add a brief description.
    void drawCellFilled(int r, int c, const cocos2d::Color4F& color, cocos2d::DrawNode* layer);
    // TODO: Add a brief description.
    void placeBuilding(int r, int c, int id);

    // Sets the BuildingScale.

    void setBuildingScale(float s);
    float _buildingScale = 1.0f;
    // Sets the BuildingScaleForId.
    void setBuildingScaleForId(int id, float s);
    // Sets the BuildingOffsetForId.
    void setBuildingOffsetForId(int id, const cocos2d::Vec2& off);
    std::vector<float> _buildingScaleById;
    std::vector<cocos2d::Vec2> _buildingOffsetById;

    // Sets the ResourceUiScale.

public:
    void setResourceUiScale(float s);
    // Sets the BattleButtonScale.
    void setBattleButtonScale(float s);
private:
    // TODO: Add a brief description.
    void openUpgradeWindowForIndex(int idx);
    // Returns the TownHallLevel.
    int getTownHallLevel() const;
    float _timeScale = 1.0f;
    // TODO: Add a brief description.
    int countById(int id) const;
    // Builds and configures resources.
    int buildLimitForId(int id) const;

    cocos2d::MenuItemImage* _battleButton = nullptr;

    // TODO: Add a brief description.

private:

    void openEscMenu();
    // TODO: Add a brief description.
    void closeEscMenu();
    // TODO: Add a brief description.
    void openSettings();

    cocos2d::LayerColor* _escMask = nullptr;
    cocos2d::LayerColor* _settingsMask = nullptr;

    // Loads data from storage.

    void loadFromCurrentSaveOrCreate();
    // Saves data to storage.
    void saveToCurrentSlot(bool force);
    // Loads data from storage.
    void placeBuildingLoaded(int r, int c, const SaveBuilding& info);

    bool _saveDirty = false;
    float _autosaveTimer = 0.0f;


    // TODO: Add a brief description.


    void openAttackTargetPicker();
    // TODO: Add a brief description.
    void closeAttackTargetPicker();
    cocos2d::LayerColor* _attackMask = nullptr;
    cocos2d::Node* _attackPanel = nullptr;
    cocos2d::ui::ScrollView* _attackScroll = nullptr;
    cocos2d::Node* _attackContent = nullptr;
    cocos2d::EventListenerMouse* _attackMouseListener = nullptr;


    // TODO: Add a brief description.


    void openTrainingCampPicker(int buildingIndex);
    // TODO: Add a brief description.
    void closeTrainingCampPicker();
    // TODO: Add a brief description.
    void refreshTrainingCampUI();
    // TODO: Add a brief description.
    void showTrainingToast(const std::string& msg);


    std::unordered_map<int, int> collectAllReadyTroops() const;



    // TODO: Add a brief description.



    void spawnStandTroop(int troopType);
    // Creates an instance.
    cocos2d::Sprite* createStandTroopSprite(int troopType) const;

    cocos2d::LayerColor* _trainMask = nullptr;
    cocos2d::Node* _trainPanel = nullptr;
    cocos2d::Node* _trainReadyRow = nullptr;
    cocos2d::Node* _trainSelectRow = nullptr;
    cocos2d::Label* _trainCapLabel = nullptr;
    cocos2d::EventListenerMouse* _trainMouseListener = nullptr;
    int _trainCampIndex = -1;
    int _trainLastSig = 0;



    // Returns the TroopLevel.



    int getTroopLevel(int unitId) const;
    // Sets the TroopLevel.
    void setTroopLevel(int unitId, int level);
    // Returns the LaboratoryMaxTroopLevel.
    int getLaboratoryMaxTroopLevel(int labLevel, int unitId) const;
    // Updates the object state.
    void updateResearchSystems(float dt);


    // TODO: Add a brief description.


    void openLaboratoryResearchPicker(int buildingIndex);
    // TODO: Add a brief description.
    void closeLaboratoryResearchPicker();
    // TODO: Add a brief description.
    void refreshResearchUI();
    // TODO: Add a brief description.
    void showResearchToast(const std::string& msg);

    std::unordered_map<int, int> _troopLevels;
    int _activeResearchUnitId = 0;
    int _activeResearchTargetLevel = 0;
    float _activeResearchTotalSec = 0.0f;
    float _activeResearchRemainSec = 0.0f;

    cocos2d::LayerColor* _researchMask = nullptr;
    cocos2d::Node* _researchPanel = nullptr;
    cocos2d::Node* _researchSelectRow = nullptr;
    cocos2d::EventListenerMouse* _researchMouseListener = nullptr;
    int _researchLabIndex = -1;
    int _researchLastSig = 0;

};