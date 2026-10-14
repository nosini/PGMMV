#include "audio/include/SimpleAudioEngine.h"
#include "audio/include/AudioEngine.h"

#include <cstdio>
#include <cstdlib>
#include <map>

using CocosDenshion::SimpleAudioEngine;
using cocos2d::experimental::AudioEngine;
using cocos2d::experimental::AudioProfile;

struct Sound {
    float volume;
    AudioEngine::AudioState state;
    std::function<void(int, const std::string &)> finish;
};
static std::map<int, Sound> sounds;
static int nextId = 0;
// A stop can synchronously run the finish callbacks of other completed sounds
// during OpenAL's update(). Exercise that while stopping the wrapper's effects.
static int finishDuringStop = -1;

static void check(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

const int AudioEngine::INVALID_AUDIO_ID = -1;
int AudioEngine::play2d(const std::string &path, bool, float volume, float, float *, float, const AudioProfile *)
{
    if (path == "missing") return INVALID_AUDIO_ID;
    const int id = nextId++;
    sounds[id] = {volume, AudioState::PLAYING, {}};
    return id;
}
static void finish(int id)
{
    auto it = sounds.find(id);
    if (it == sounds.end()) return;
    const auto callback = it->second.finish;
    sounds.erase(it);
    if (callback) callback(id, "effect");
}
void AudioEngine::stop(int id)
{
    sounds.erase(id);
    if (finishDuringStop != INVALID_AUDIO_ID) {
        const int finished = finishDuringStop;
        finishDuringStop = INVALID_AUDIO_ID;
        finish(finished);
    }
}
void AudioEngine::pause(int id)
{
    if (sounds.count(id)) sounds.at(id).state = AudioState::PAUSED;
}
void AudioEngine::resume(int id)
{
    if (sounds.count(id)) sounds.at(id).state = AudioState::PLAYING;
}
bool AudioEngine::setCurrentTime(int, float) { return true; }
AudioEngine::AudioState AudioEngine::getState(int id)
{
    return sounds.count(id) ? sounds.at(id).state : AudioState::ERROR;
}
void AudioEngine::setVolume(int id, float volume)
{
    if (sounds.count(id)) sounds.at(id).volume = volume;
}
void AudioEngine::preload(const std::string &, std::function<void(bool)>) {}
void AudioEngine::uncache(const std::string &) {}
void AudioEngine::setFinishCallback(int id, const std::function<void(int, const std::string &)> &callback)
{
    sounds.at(id).finish = callback;
}

static void failedEffect()
{
    auto engine = SimpleAudioEngine::getInstance();
    engine->playBackgroundMusic("music", true); // The backend's first valid ID is zero.
    const auto failed = engine->playEffect("missing");
    check(failed != 0, "failed effect aliases background music ID zero");
    engine->pauseEffect(failed);
    check(engine->isBackgroundMusicPlaying(), "failed effect paused background music");
    engine->pauseBackgroundMusic();
    engine->resumeEffect(failed);
    check(!engine->isBackgroundMusicPlaying(), "failed effect resumed background music");
    engine->resumeBackgroundMusic();
    engine->stopEffect(failed);
    check(engine->isBackgroundMusicPlaying(), "failed effect stopped background music");

    // Controls must only operate on effects belonging to this wrapper.
    const int external = AudioEngine::play2d("main-player", true);
    engine->pauseEffect(external);
    check(AudioEngine::getState(external) == AudioEngine::AudioState::PLAYING,
        "effect control paused a sound owned by the main player");
    AudioEngine::pause(external);
    engine->resumeEffect(external);
    check(AudioEngine::getState(external) == AudioEngine::AudioState::PAUSED,
        "effect control resumed a sound owned by the main player");
    engine->stopEffect(external);
    check(sounds.count(external), "effect control stopped a sound owned by the main player");

    const auto effect = engine->playEffect("loop", true);
    engine->pauseEffect(effect);
    check(AudioEngine::getState(effect) == AudioEngine::AudioState::PAUSED, "effect did not pause");
    engine->resumeEffect(effect);
    check(AudioEngine::getState(effect) == AudioEngine::AudioState::PLAYING, "effect did not resume");
    engine->stopEffect(effect);
    check(!sounds.count(effect), "effect did not stop");
    SimpleAudioEngine::end();
}

static void cleanup()
{
    const int external = AudioEngine::play2d("main-player", true);
    auto engine = SimpleAudioEngine::getInstance();
    engine->setBackgroundMusicVolume(0.5f);
    engine->setEffectsVolume(0.25f);
    engine->playBackgroundMusic("music", true);
    engine->playEffect("loop1", true);
    engine->playEffect("loop2", true);
    SimpleAudioEngine::end();
    check(sounds.size() == 1 && sounds.count(external),
        "end did not stop all owned sounds while preserving main-player audio");
    SimpleAudioEngine::end(); // Repeated cleanup is safe.

    engine = SimpleAudioEngine::getInstance();
    check(!engine->isBackgroundMusicPlaying(), "new engine retained old music");
    check(engine->getBackgroundMusicVolume() == 1 && engine->getEffectsVolume() == 1,
        "new engine retained the old volume settings");
    engine->playEffect("loop3", true);
    const auto completed = engine->playEffect("one-shot");
    finishDuringStop = completed;
    engine->stopAllEffects();
    check(sounds.size() == 1 && sounds.count(external), "stopAllEffects left owned sounds active");
    SimpleAudioEngine::end();
}

int main(int argc, char **argv)
{
    check(argc == 2, "expected failure or cleanup");
    const std::string mode = argv[1];
    if (mode == "failure") failedEffect();
    else if (mode == "cleanup") cleanup();
    else check(false, "unknown test");
}
