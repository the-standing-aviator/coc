#include "AppDelegate.h"
#include "Scenes/MenuScene.h"
#include "Scenes/MainScene.h"
#include "Scenes/LoginScene.h"
#include "Managers/SoundManager.h"
#include"Scenes/MenuScene.h"
#include <vector>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace cocos2d;

// Add robust resource search paths for Win32 and classroom setups.
// In some VS configurations, new assets may stay in the repo root Resources/
// and not be copied to $(OutDir)/Resources. We proactively add a few common
// locations so Sprite::create("ui/..."), Sprite::create("buildings/...") can
// find newly added files (e.g. build_button11.png, building11L1.png).
static void AddFallbackResourceSearchPaths() {
    auto fu = FileUtils::getInstance();

    std::vector<std::string> candidates;
    // Typical runtime working directory is $(OutDir).
    candidates.push_back("Resources");
    candidates.push_back("../Resources");
    candidates.push_back("../../Resources");
    candidates.push_back("../../../Resources");
    candidates.push_back("../../../../Resources");

    // Some repos use lowercase.
    candidates.push_back("resources");
    candidates.push_back("../resources");
    candidates.push_back("../../resources");

#ifdef _WIN32
    // Also add absolute candidates based on exe folder.
    char modulePath[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, modulePath, MAX_PATH) > 0) {
        std::string exePath = modulePath;
        size_t slash = exePath.find_last_of("\\/");
        std::string exeDir = (slash == std::string::npos) ? std::string(".") : exePath.substr(0, slash);
        auto join = [&](const std::string& rel) {
            std::string p = exeDir;
            p += "/";
            p += rel;
            return p;
        };
        candidates.push_back(join("Resources"));
        candidates.push_back(join("../Resources"));
        candidates.push_back(join("../../Resources"));
        candidates.push_back(join("../../../Resources"));
        candidates.push_back(join("resources"));
        candidates.push_back(join("../resources"));
    }
#endif

    for (const auto& p : candidates) {
        fu->addSearchPath(p, true);
    }

    // Helpful debug logs (Win32 console/output).
    CCLOG("[SearchPath] ui/build_button11.png -> %s", fu->fullPathForFilename("ui/build_button11.png").c_str());
    CCLOG("[SearchPath] buildings/building11L1.png -> %s", fu->fullPathForFilename("buildings/building11L1.png").c_str());
}

AppDelegate::AppDelegate() {}

AppDelegate::~AppDelegate() {}

void AppDelegate::initGLContextAttrs() {
    GLContextAttrs attrs = {8, 8, 8, 8, 24, 8};
    GLView::setGLContextAttrs(attrs);
}

bool AppDelegate::applicationDidFinishLaunching() {
    // Ensure newly added assets can be found regardless of VS working dir/copy steps.
    AddFallbackResourceSearchPaths();

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
