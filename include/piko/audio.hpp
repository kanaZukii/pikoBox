// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <unordered_map>
#include <vector>

typedef struct Music Music; 

namespace piko {
    class Engine;
    class AudioClip;

    class AudioManager {
    public:
        ~AudioManager() = default;

        // Enforce uniqueness for the audio hardware context controller
        AudioManager(const AudioManager&) = delete;
        AudioManager& operator=(const AudioManager&) = delete;

        void update(); // Must tick every frame to refill streaming buffers

        // The Abstract mixing board API
        void setChannelVolume(int channelIdx, float volume);
        float getChannelVolume(int channelIdx) const;

        void setChannelMute(int channelIdx, bool muted);

        // Playback API
        void playClip(const AudioClip* clip, bool shouldLoop, int channel, float startAt=0.0f);
        void stopChannelStream(int channelIdx);

        bool isChannelPlaying(int channelIdx) const;

    private:
        AudioManager(){}
        void init();
        void terminate();

        // Holds tracking info for a channel currently streaming data from disk
        struct ActiveStream {
            Music* streamRef = nullptr;
            bool isActive = false;
            float loopStart = 0.0f;
        };

        // Storage for abstract volumes. Resizes dynamically based on incoming channel IDs.
        std::vector<float> channelVolumes;
        
        // Maps an abstract channel index to its actively executing disk stream context
        std::unordered_map<int, ActiveStream> activeStreams;

        // Internal helper to ensure our volume array scales out safely on the fly
        void ensureChannelExists(int channelIdx);

        friend class Engine;
    };

}