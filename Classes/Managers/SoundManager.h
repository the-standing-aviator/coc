#pragma once
#include "cocos2d.h"
#include "audio/include/AudioEngine.h"

class SoundManager {
public:
    // Play background music (single instance).
    // `volume` is the track/base volume (0~1). Real output volume = base * master.
    static void play(const std::string& path, bool loop = true, float volume = 1.0f);
    static void stop();
    static void pause();
    static void resume();

    // Master volume (0~1). Applies to both current and future audio.
    static void setVolume(float v);
    static float getVolume();

    // Optional: base volume for current BGM (0~1).
    static void setBgmBaseVolume(float v);
    static float getBgmBaseVolume();
private:
    static void initFromUserDefault();
    static int _audioId;
    static float _masterVolume;
    static float _bgmBaseVolume;
    static bool _inited;
};
