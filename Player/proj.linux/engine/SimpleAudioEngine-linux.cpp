// SimpleAudioEngine for the Linux player.
//
// The player itself only uses experimental::AudioEngine (the OpenAL backend in
// audio/win32). SimpleAudioEngine is still exposed to scripts by the JS
// bindings, so this forwards it to AudioEngine instead of the stock FMOD-based
// Linux implementation.

#include "audio/include/SimpleAudioEngine.h"
#include "audio/include/AudioEngine.h"

#include <set>
#include <string>

using namespace CocosDenshion;
using cocos2d::experimental::AudioEngine;

namespace {

SimpleAudioEngine *s_engine = nullptr;
int s_musicId = AudioEngine::INVALID_AUDIO_ID;
std::string s_musicPath;
float s_musicVolume = 1.0f;
float s_effectsVolume = 1.0f;
std::set<int> s_effects;

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
}

unsigned int SimpleAudioEngine::playEffect(const char *filePath, bool loop, float pitch, float pan, float gain)
{
    (void)pitch;
    (void)pan;
    int id = AudioEngine::play2d(filePath, loop, gain * s_effectsVolume);
    if (id == AudioEngine::INVALID_AUDIO_ID) {
        return 0;
    }
    s_effects.insert(id);
    AudioEngine::setFinishCallback(id, [](int finishedId, const std::string &) {
        s_effects.erase(finishedId);
    });
    return static_cast<unsigned int>(id);
}

void SimpleAudioEngine::pauseEffect(unsigned int soundId)
{
    AudioEngine::pause(static_cast<int>(soundId));
}

void SimpleAudioEngine::pauseAllEffects()
{
    for (int id : s_effects) {
        AudioEngine::pause(id);
    }
}

void SimpleAudioEngine::resumeEffect(unsigned int soundId)
{
    AudioEngine::resume(static_cast<int>(soundId));
}

void SimpleAudioEngine::resumeAllEffects()
{
    for (int id : s_effects) {
        AudioEngine::resume(id);
    }
}

void SimpleAudioEngine::stopEffect(unsigned int soundId)
{
    AudioEngine::stop(static_cast<int>(soundId));
    s_effects.erase(static_cast<int>(soundId));
}

void SimpleAudioEngine::stopAllEffects()
{
    for (int id : s_effects) {
        AudioEngine::stop(id);
    }
    s_effects.clear();
}

void SimpleAudioEngine::preloadEffect(const char *filePath)
{
    AudioEngine::preload(filePath);
}

void SimpleAudioEngine::unloadEffect(const char *filePath)
{
    AudioEngine::uncache(filePath);
}
