#pragma once
#include "cocos2d.h"
#include "ui/CocosGUI.h"

// LoginScene is used as "Save Selection Scene"
// - show existing saves (Enter / Delete)
// - top-right Back to MenuScene
class LoginScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    virtual bool init() override;
    CREATE_FUNC(LoginScene);

private:
    void openSaveSelector();
    void closeSaveSelector();
    void buildSaveUI();
    void refreshSaveList();
    void backToMenu();

private:
    cocos2d::LayerColor* _saveMask = nullptr;
    cocos2d::ui::ScrollView* _saveScroll = nullptr;
    cocos2d::Node* _saveContent = nullptr;
};