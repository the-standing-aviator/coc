#pragma once
#include "cocos2d.h"
#include "audio/include/AudioEngine.h"

class SoundManager {
public:
    static void play(const std::string& path, bool loop = true, float volume = 1.0f);
    static void stop();
    static void pause();
    static void resume();
    static void setVolume(float v);
private:
    static int _audioId;
    static float _volume;
};