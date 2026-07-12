#include "piko/audio.hpp"
#include "piko/audioClip.hpp"
#include "piko/logger.hpp"

#include "raylib.h"

#include <iostream>
#include <utility>
#include <algorithm>

using namespace piko;

AudioClip::AudioClip(const std::string& filepath, AudioType type)
    : path(filepath), type(type) {

    if (!FileExists(filepath.c_str())) {
        throw std::runtime_error("AUDIOCLIP: File does not exist: " + filepath);
    }
    
    if (type == AudioClip::AudioType::STATIC_SFX) {
        Sound tempSound = LoadSound(path.c_str());

        if (tempSound.frameCount == 0) {
            throw std::runtime_error("AUDIOCLIP: this audio file failed to load: " + filepath);
        }

        staticData = new Sound(tempSound);

    } else if (type == AudioClip::AudioType::STREAM_MUSIC) {
        Music tempMusic = LoadMusicStream(path.c_str());
        
        if (tempMusic.frameCount == 0) {
            throw std::runtime_error("AUDIOCLIP: this audio file failed to load: " + filepath);
        }

        streamData = new Music(tempMusic);
    }
}

AudioClip::~AudioClip() {
    if (staticData) {
        if (staticData->frameCount > 0) UnloadSound(*staticData);
        delete staticData;
    }
    if (streamData) {
        if (streamData->frameCount > 0) UnloadMusicStream(*streamData);
        delete streamData;
    }
}

// Move Constructor
AudioClip::AudioClip(AudioClip&& other) noexcept 
    : staticData(other.staticData),
        streamData(other.streamData),
        path(std::move(other.path)),
        type(other.type) {
    
    // Nullify the temporary source object so its destructor doesn't clear the hardware buffer
    other.staticData = nullptr;
    other.streamData = nullptr;
}

// Move Assignment Operator
AudioClip& AudioClip::operator=(AudioClip&& other) noexcept {
    if (this != &other) {
        // Clean up own existing allocations first
        if (staticData) {
            if (staticData->frameCount > 0) UnloadSound(*staticData);
            delete staticData;
        }
        if (streamData) {
            if (streamData->frameCount > 0) UnloadMusicStream(*streamData);
            delete streamData;
        }

        // Steal the references
        staticData = other.staticData;
        streamData = other.streamData;
        path = std::move(other.path);
        type = other.type;

        other.staticData = nullptr;
        other.streamData = nullptr;
    }
    return *this;
}

void AudioManager::init() {
    InitAudioDevice();
    ensureChannelExists(0);
    channelVolumes[0] = 0.5f;
    SetMasterVolume(channelVolumes[0]);
    PBOX_INFO("AUIDO_MAN: Audio device backend initialized.");
}

void AudioManager::terminate() {
    std::vector<int> activeKeys;
    for (auto& [channel, stream] : activeStreams) {
        activeKeys.push_back(channel);
    }
    for (int key : activeKeys) {
        stopChannelStream(key);
    }
    CloseAudioDevice();
    PBOX_INFO("AUDIO_MAN: Audio device backend closed.");
}

void AudioManager::ensureChannelExists(int channelIdx) {
    if (channelIdx >= static_cast<int>(channelVolumes.size())) {
        channelVolumes.resize(channelIdx + 1, 0.8f); 
    }
}

void AudioManager::setChannelVolume(int channelIdx, float volume) {
    if (channelIdx < 0) return;
    ensureChannelExists(channelIdx);

    channelVolumes[channelIdx] = std::clamp(volume, 0.0f, 1.0f);

    if (channelIdx == 0) {
        SetMasterVolume(channelVolumes[0]);
        return;
    }

    if (activeStreams.count(channelIdx) && activeStreams[channelIdx].isActive) {
        SetMusicVolume(*activeStreams[channelIdx].streamRef, channelVolumes[channelIdx]);
    }
}

float AudioManager::getChannelVolume(int channelIdx) const {
    if (channelIdx < 0 || channelIdx >= static_cast<int>(channelVolumes.size())) return 0.0f;
    return channelVolumes[channelIdx];
}

// Allow passing an optional channel, defaulting to -1 (no channel tracking)
void AudioManager::playClip(const AudioClip* clip, bool shouldLoop, int channel, float startAt) {
    if (!clip) return;

    // Use override if provided (!= -1), otherwise fallback to 0 (default)
    channel = (channel != -1) ? channel : 0;
    ensureChannelExists(channel);

    if (clip->getType() == AudioClip::AudioType::STATIC_SFX) {
        const Sound* sfx = clip->getStaticData();
        if (sfx) {
            SetSoundVolume(*sfx, channelVolumes[channel]);
            PlaySound(*sfx);
        }
    } else if (clip->getType() == AudioClip::AudioType::STREAM_MUSIC) {
        stopChannelStream(channel);
        Music* stream = clip->getStreamData();
        if (stream) {
            stream->looping = shouldLoop;
            PlayMusicStream(*stream);
            if (startAt > 0.0f) SeekMusicStream(*stream, startAt);
            SetMusicVolume(*stream, channelVolumes[channel]);
            activeStreams[channel] = ActiveStream{ stream, true, startAt };
        }
    }
}

// AudioManager.cpp
void AudioManager::setChannelMute(int channelIdx, bool muted) {
    static std::unordered_map<int, float> cachedVolumes;

    if (muted) {
        // Store current and set to 0
        cachedVolumes[channelIdx] = channelVolumes[channelIdx];
        setChannelVolume(channelIdx, 0.0f);
    } else {
        // Restore from cache if it exists, otherwise default to 1.0f
        float vol = cachedVolumes.count(channelIdx) ? cachedVolumes[channelIdx] : 1.0f;
        setChannelVolume(channelIdx, vol);
    }
}

void AudioManager::stopChannelStream(int channelIdx) {
    if (activeStreams.count(channelIdx) && activeStreams[channelIdx].isActive) {
        StopMusicStream(*activeStreams[channelIdx].streamRef);
        activeStreams.erase(channelIdx);
    }
}

bool AudioManager::isChannelPlaying(int channelIdx) const {
    auto it = activeStreams.find(channelIdx);
    if (it != activeStreams.end() && it->second.isActive) {
        return IsMusicStreamPlaying(*it->second.streamRef);
    }
    return false;
}

void AudioManager::update() {
    for (auto& [channel, stream] : activeStreams) {
        if (stream.isActive) {
            UpdateMusicStream(*stream.streamRef);

            if (stream.streamRef->looping && IsMusicStreamPlaying(*stream.streamRef) == false) {
                PlayMusicStream(*stream.streamRef);

                if (stream.loopStart > 0.0f) {
                    SeekMusicStream(*stream.streamRef, stream.loopStart);
                }
            }
            #ifdef __EMSCRIPTEN__
                // On web, if a frame lag spike occurs, don't immediately force-restart the audio
                // stream if Raylib's Web Audio layer is just waiting for the next buffer cycle.
                if (stream.streamRef->looping && !IsMusicStreamPlaying(*stream.streamRef)) {
                    // Only restart if it has genuinely stopped entirely beyond a thread stall
                    PlayMusicStream(*stream.streamRef);
                }
            #else 
                // For desktop platforms
                if (stream.streamRef->looping && IsMusicStreamPlaying(*stream.streamRef) == false) {
                    PlayMusicStream(*stream.streamRef);
                    if (stream.loopStart > 0.0f) {
                        SeekMusicStream(*stream.streamRef, stream.loopStart);
                    }
                }
            #endif
        }
    }
}