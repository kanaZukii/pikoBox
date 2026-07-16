// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <unordered_map>
#include <vector>

// Forward declare raylib's music for streaming and tracking
typedef struct Music Music; 

namespace piko {
    class Engine;
    class AudioClip;

    /*
        AudioManager, central manager for sound playback and channel mixing.
        Handles both memory-resident SFX and disk-streamed music.
    */
    class AudioManager {
    public:
        ~AudioManager() = default;

        // Enforce uniqueness, do not copy
        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;

        // Must be called every frame to refill streaming buffers and check playback.
        void update(); 

        // Sets volume (0.0f to 1.0f) for a specific playback channel.
        void setChannelVolume(int channelIdx, float volume);
        float getChannelVolume(int channelIdx) const;

        // Mutes/Unmutes a specific playback channel.
        void setChannelMute(int channelIdx, bool muted);

        // Plays an audio clip.
        void playClip(const AudioClip* clip, bool shouldLoop, int channel, float startAt=0.0f);
        
        // Immediately halts audio playback on the specified channel.
        void stopChannelStream(int channelIdx);

        // Returns true if the channel is currently playing audio.
        bool isChannelPlaying(int channelIdx) const;

    private:
        AudioManager(){}
        void init();
        void terminate();

        // Internal context for tracks requiring continuous streaming from disk.
        struct ActiveStream {
            Music* streamRef = nullptr;
            bool isActive = false;
            float loopStart = 0.0f;
        };

        // Indexed by channel ID.
        std::vector<float> channelVolumes;
        
        // Tracks active streaming instances.
        std::unordered_map<int, ActiveStream> activeStreams;

        // Helper to ensure the volume vector is large enough for the requested channel index.
        void ensureChannelExists(int channelIdx);

        friend class Engine;
    };

}