// SimpleAudioEngine for the Linux player.
//
// The player itself only uses experimental::AudioEngine (the OpenAL backend in
// audio/win32). SimpleAudioEngine is still exposed to scripts by the JS
// bindings, so this forwards it to AudioEngine instead of the stock FMOD-based
// Linux implementation.

#include "audio/include/SimpleAudioEngine.h"
#include "audio/include/AudioEngine.h"

#include <map>
#include <string>

using namespace CocosDenshion;
using cocos2d::experimental::AudioEngine;

namespace {

SimpleAudioEngine *s_engine = nullptr;
int s_musicId = AudioEngine::INVALID_AUDIO_ID;
std::string s_musicPath;
float s_musicVolume = 1.0f;
float s_effectsVolume = 1.0f;
// Keep each effect's gain so volume changes also update sounds already playing.
std::map<unsigned int, float> s_effects;

} // namespace

SimpleAudioEngine *SimpleAudioEngine::getInstance()
{
    if (!s_engine) {
        s_engine = new SimpleAudioEngine();
    }
    return s_engine;
}

void SimpleAudioEngine::end()
{
    delete s_engine;
    s_engine = nullptr;
}

SimpleAudioEngine::SimpleAudioEngine()
{
}

SimpleAudioEngine::~SimpleAudioEngine()
{
    // AudioEngine is shared with the player; only stop this wrapper's sounds.
    stopBackgroundMusic();
    stopAllEffects();
    s_musicPath.clear();
    s_musicVolume = 1.0f;
    s_effectsVolume = 1.0f;
}

void SimpleAudioEngine::preloadBackgroundMusic(const char *filePath)
{
    AudioEngine::preload(filePath);
}

void SimpleAudioEngine::playBackgroundMusic(const char *filePath, bool loop)
{
    stopBackgroundMusic();
    s_musicPath = filePath;
    s_musicId = AudioEngine::play2d(filePath, loop, s_musicVolume);
}

void SimpleAudioEngine::stopBackgroundMusic(bool releaseData)
{
    if (s_musicId != AudioEngine::INVALID_AUDIO_ID) {
        AudioEngine::stop(s_musicId);
        s_musicId = AudioEngine::INVALID_AUDIO_ID;
    }
    if (releaseData && !s_musicPath.empty()) {
        AudioEngine::uncache(s_musicPath);
    }
}

void SimpleAudioEngine::pauseBackgroundMusic()
{
    if (s_musicId != AudioEngine::INVALID_AUDIO_ID) {
        AudioEngine::pause(s_musicId);
    }
}

void SimpleAudioEngine::resumeBackgroundMusic()
{
    if (s_musicId != AudioEngine::INVALID_AUDIO_ID) {
        AudioEngine::resume(s_musicId);
    }
}

void SimpleAudioEngine::rewindBackgroundMusic()
{
    if (s_musicId != AudioEngine::INVALID_AUDIO_ID) {
        AudioEngine::setCurrentTime(s_musicId, 0.0f);
    }
}

bool SimpleAudioEngine::willPlayBackgroundMusic()
{
    return true;
}

bool SimpleAudioEngine::isBackgroundMusicPlaying()
{
    return s_musicId != AudioEngine::INVALID_AUDIO_ID
        && AudioEngine::getState(s_musicId) == AudioEngine::AudioState::PLAYING;
}

float SimpleAudioEngine::getBackgroundMusicVolume()
{
    return s_musicVolume;
}

void SimpleAudioEngine::setBackgroundMusicVolume(float volume)
{
    s_musicVolume = volume;
    if (s_musicId != AudioEngine::INVALID_AUDIO_ID) {
        AudioEngine::setVolume(s_musicId, volume);
    }
}

float SimpleAudioEngine::getEffectsVolume()
{
    return s_effectsVolume;
}

void SimpleAudioEngine::setEffectsVolume(float volume)
{
    s_effectsVolume = volume;
    for (const auto &effect : s_effects) {
        AudioEngine::setVolume(static_cast<int>(effect.first), effect.second * volume);
    }
}

unsigned int SimpleAudioEngine::playEffect(const char *filePath, bool loop, float pitch, float pan, float gain)
{
    (void)pitch;
    (void)pan;
    int id = AudioEngine::play2d(filePath, loop, gain * s_effectsVolume);
    if (id == AudioEngine::INVALID_AUDIO_ID) {
        return static_cast<unsigned int>(AudioEngine::INVALID_AUDIO_ID);
    }
    s_effects.emplace(static_cast<unsigned int>(id), gain);
    AudioEngine::setFinishCallback(id, [](int finishedId, const std::string &) {
        s_effects.erase(finishedId);
    });
    return static_cast<unsigned int>(id);
}

void SimpleAudioEngine::pauseEffect(unsigned int soundId)
{
    if (s_effects.count(soundId)) {
        AudioEngine::pause(static_cast<int>(soundId));
    }
}

void SimpleAudioEngine::pauseAllEffects()
{
    for (const auto &effect : s_effects) {
        AudioEngine::pause(static_cast<int>(effect.first));
    }
}

void SimpleAudioEngine::resumeEffect(unsigned int soundId)
{
    if (s_effects.count(soundId)) {
        AudioEngine::resume(static_cast<int>(soundId));
    }
}

void SimpleAudioEngine::resumeAllEffects()
{
    for (const auto &effect : s_effects) {
        AudioEngine::resume(static_cast<int>(effect.first));
    }
}

void SimpleAudioEngine::stopEffect(unsigned int soundId)
{
    if (s_effects.erase(soundId)) {
        AudioEngine::stop(static_cast<int>(soundId));
    }
}

void SimpleAudioEngine::stopAllEffects()
{
    // stop() updates the backend synchronously and may finish other effects.
    // Detach the collection so their callbacks cannot invalidate this iteration.
    std::map<unsigned int, float> effects;
    effects.swap(s_effects);
    for (const auto &effect : effects) {
        AudioEngine::stop(static_cast<int>(effect.first));
    }
}

void SimpleAudioEngine::preloadEffect(const char *filePath)
{
    AudioEngine::preload(filePath);
}

void SimpleAudioEngine::unloadEffect(const char *filePath)
{
    AudioEngine::uncache(filePath);
}
