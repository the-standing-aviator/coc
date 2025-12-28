#pragma once
#include "cocos2d.h"
#include "ui/CocosGUI.h"

// Main entry menu
// - Create New Save: auto create first empty slot, then enter MainScene
// - Load Existing Save: open a save selector overlay (Enter/Delete), and can close back to menu
class MenuScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    CREATE_FUNC(MenuScene);

private:
    void createNewSaveAndEnter();

    // Save selector overlay
    void openSaveSelector();
    void closeSaveSelector();
    void buildSaveUI();
    void refreshSaveList();

private:
    cocos2d::LayerColor* _saveMask = nullptr;
    cocos2d::ui::ScrollView* _saveScroll = nullptr;
    cocos2d::Node* _saveContent = nullptr;
    cocos2d::Node* _savePanel = nullptr;
    cocos2d::EventListenerMouse* _saveMouseListener = nullptr;
};
