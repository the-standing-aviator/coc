#include "Managers/SoundManager.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

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

int SoundManager::playSfxRandom(const std::string& key, float volume)
{
    initFromUserDefault();

    // NOTE:
    // This project stores sfx under "Resources/music" and uses many *variants*.
    // We map logical keys -> a list of candidates and pick one randomly.
    static const std::unordered_map<std::string, std::vector<std::string>> kSfxMap = {
        {"archer_death", {"music/archer_death_05.ogg", "music/archer_death_05v2.ogg"}},
        {"archer_tower_pick", {"music/archer_tower_pick_01.ogg"}},
        {"archer_tower_place", {"music/archer_tower_place_02.ogg"}},

        {"arrow_hit", {"music/arrow_hit_07.ogg", "music/arrow_hit_07v2.ogg", "music/arrow_hit_07v3.ogg", "music/arrow_hit_12.ogg"}},

        {"barbarian_death", {"music/barbarian_death_01.ogg", "music/barbarian_death_02.ogg"}},
        {"barbarian_hit_stuff", {"music/barbarian_hit_stuff.ogg", "music/barbarian_hit_stuff2.ogg", "music/barbarian_hit_stuff3.ogg", "music/barbarian_hit_stuff4.ogg"}},

        {"battle_lost", {"music/battle_lost_02.mp3"}},
        {"win", {"music/winwinwin.mp3"}},

        {"building_destroyed", {"music/building_destroyed_01.ogg"}},
        {"building_finished", {"music/building_finished_01.ogg"}},

        {"button_click", {"music/button_click.ogg"}},

        {"cannon_attack", {"music/cannon_08.ogg"}},
        {"cannon_drop", {"music/cannon_drop2.ogg"}},
        {"cannon_pick", {"music/cannon_pickup3.ogg"}},

        {"coin_steal", {"music/coin_steal_02.ogg"}},
        {"elixir_steal", {"music/elixir_steal_03.ogg"}},

        {"giant_attack", {"music/giant_attack_05.ogg", "music/giant_attack_06.ogg", "music/giant_attack_hit_01.ogg", "music/giant_attack_hit_02.ogg"}},
        {"giant_death", {"music/giant_death_02.ogg"}},

        {"wall_breaker_attack", {"music/wall_breaker_attack_01.ogg"}},
        {"wall_breaker_death", {"music/wall_breaker_die_01.ogg"}},

        // Generic building pickup/place (used when a building has no dedicated sound).
        {"building_pickup", {"music/build_pickup_05.ogg"}},
        {"building_place", {"music/builder_hut_place_02.ogg"}},

        // Resource buildings pickup/place
        {"goldmine_pick", {"music/goldmine_pickup4.ogg"}},
        {"goldmine_drop", {"music/goldmine_drop4.ogg"}},
        {"elixir_pump_pick", {"music/elixir_pump_pickup_07.ogg"}},
        {"elixir_pump_drop", {"music/elixir_pump_drop_03.ogg"}},
        {"elixir_storage_pick", {"music/elixir_storage_pickup_04.ogg"}},
        {"elixir_storage_drop", {"music/elixir_storage_drop_02.ogg"}},
        {"gold_storage_drop", {"music/golden_haus_drop_02.ogg"}},
    };

    auto it = kSfxMap.find(key);
    if (it == kSfxMap.end() || it->second.empty())
    {
        // Fallback: treat the key as a direct path.
        return playSfx(key, volume);
    }

    const auto& list = it->second;
    int idx = 0;
    if (list.size() > 1)
    {
        idx = RandomHelper::random_int(0, (int)list.size() - 1);
    }

    return playSfx(list[idx], volume);
}

