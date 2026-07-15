// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once
#include <string>

// Forward declaring raylib's internal types 
typedef struct Sound Sound;
typedef struct Music Music;

namespace piko {
    /*
        AudioClip asset. Contains audio data for playback.
        Data can be a loaded memory-resident or a reference
        to be streamed.
    */
    class AudioClip {
        public:
            enum class AudioType {
                STATIC_SFX,         // Loaded fully into RAM (For sound effects and small audio files).
                STREAM_MUSIC        // Streamed directly from disk (For music and large audio files).
            };
            
            // Creates an AudioClip from a file via its path and assign a type.
            AudioClip(const std::string& filepath, AudioType type);
            ~AudioClip();

            // Prevent expensive/unsafe copies.
            AudioClip(const AudioClip&) = delete;
            AudioClip& operator=(const AudioClip&) = delete;

            // Allows moving for transferring ownersip.
            AudioClip(AudioClip&& other) noexcept;
            AudioClip& operator=(AudioClip&& other) noexcept;

            inline AudioType getType() const noexcept { return type; }
            inline const std::string& getFilePath() const noexcept { return path; }

            // Accessors for internal raylib data types.
            inline const Sound* getStaticData() const noexcept { return staticData; }
            inline Music* getStreamData() const noexcept { return streamData; }

        private:
            Sound* staticData = nullptr;
            Music* streamData = nullptr;

            std::string path = "";
            AudioType type;
    };

}