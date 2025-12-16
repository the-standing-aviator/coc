#include "Managers/SoundManager.h"
using namespace cocos2d::experimental;

int SoundManager::_audioId = -1;
float SoundManager::_volume = 1.0f;

void SoundManager::play(const std::string& path, bool loop, float volume)
{
    if (_audioId != -1) {
        AudioEngine::stop(_audioId);
        _audioId = -1;
    }
    _audioId = AudioEngine::play2d(path, loop, volume);
    _volume = volume;
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

void SoundManager::setVolume(float v)
{
    _volume = std::max(0.0f, std::min(1.0f, v));
    if (_audioId != -1) AudioEngine::setVolume(_audioId, _volume);
}