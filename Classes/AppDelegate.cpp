#include "AppDelegate.h"
#include "Scenes/MenuScene.h"
#include "Scenes/MainScene.h"
#include "Scenes/LoginScene.h"
#include "Managers/SoundManager.h"
#include"Scenes/MenuScene.h"
#ifdef _WIN32
#include <windows.h>
#endif
using namespace cocos2d;

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate() {}

void AppDelegate::initGLContextAttrs() {
    GLContextAttrs attrs = {8, 8, 8, 8, 24, 8};
    GLView::setGLContextAttrs(attrs);
}

bool AppDelegate::applicationDidFinishLaunching() {
    auto director = Director::getInstance();
    auto glview = director->getOpenGLView();
    if (!glview) {
#ifdef _WIN32
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        float scale = 0.95f;
        glview = GLViewImpl::createWithRect("Clash of Clans", Rect(0, 0, sw * scale, sh * scale));
#else
        glview = GLViewImpl::create("Clash of Clans");
#endif
        director->setOpenGLView(glview);
    }
    director->setAnimationInterval(1.0f / 60);
    glview->setDesignResolutionSize(1136, 640, ResolutionPolicy::NO_BORDER);
    auto scene = MenuScene::createScene();
    director->runWithScene(scene);
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
    SoundManager::pause();
}

void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
    SoundManager::resume();
}
