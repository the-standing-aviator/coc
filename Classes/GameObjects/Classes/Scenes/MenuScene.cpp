#include "Scenes/MenuScene.h"
#include "Scenes/MainScene.h"
using namespace cocos2d;

Scene* MenuScene::createScene()
{
    return MainScene::createScene();
}