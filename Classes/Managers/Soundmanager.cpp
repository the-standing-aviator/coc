#include "Managers/SoundManager.h"

#include <algorithm>

using namespace cocos2d;
using namespace cocos2d::experimental;

int   SoundManager::_audioId = -1;
float SoundManager::_masterVolume = 1.0f;
float SoundManager::_bgmBaseVolume = 1.0f;
bool  SoundManager::_inited = false;

static float clamp01(float v)
{
    return std::max(0.0f, std::min(1.0f, v));
}

void SoundManager::initFromUserDefault()
{
    if (_inited) return;
    _inited = true;
    // Persisted master volume (so setting it in LoginScene affects MainScene later).
    _masterVolume = clamp01(UserDefault::getInstance()->getFloatForKey("master_volume", 1.0f));
}

void SoundManager::play(const std::string& path, bool loop, float volume)
{
    initFromUserDefault();
    _bgmBaseVolume = clamp01(volume);

    if (_audioId != -1) {
        AudioEngine::stop(_audioId);
        _audioId = -1;
    }

    const float outVol = clamp01(_bgmBaseVolume * _masterVolume);
    _audioId = AudioEngine::play2d(path, loop, outVol);
}

void SoundManager::stop()
{
    if (_audioId != -1) {
        AudioEngine::stop(_audioId);
        _audioId = -1;
    }
}

void SoundManager::pause()
{
    if (_audioId != -1) AudioEngine::pause(_audioId);
}

void SoundManager::resume()
{
    if (_audioId != -1) AudioEngine::resume(_audioId);
}

void SoundManager::setBgmBaseVolume(float v)
{
    initFromUserDefault();
    _bgmBaseVolume = clamp01(v);
    if (_audioId != -1) {
        AudioEngine::setVolume(_audioId, clamp01(_bgmBaseVolume * _masterVolume));
    }
}

float SoundManager::getBgmBaseVolume()
{
    initFromUserDefault();
    return _bgmBaseVolume;
}

void SoundManager::setVolume(float v)
{
    initFromUserDefault();
    _masterVolume = clamp01(v);
    UserDefault::getInstance()->setFloatForKey("master_volume", _masterVolume);
    UserDefault::getInstance()->flush();

    if (_audioId != -1) {
        AudioEngine::setVolume(_audioId, clamp01(_bgmBaseVolume * _masterVolume));
    }
}

float SoundManager::getVolume()
{
    initFromUserDefault();
    return _masterVolume;
}
int SoundManager::playSfx(const std::string& path, float volume)
{
    initFromUserDefault();
    const float outVol = clamp01(volume) * _masterVolume;
    return AudioEngine::play2d(path, false, outVol);
}