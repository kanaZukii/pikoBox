#pragma once
#include <string>

// Forward declaring Raylib's internal types as raw structs 
typedef struct Sound Sound;
typedef struct Music Music;

namespace piko {
    class AudioClip {
        public:
            enum class AudioType {
                STATIC_SFX,
                STREAM_MUSIC
            };
            
            AudioClip(const std::string& filepath, AudioType type, int targetChannel);
            ~AudioClip();

            AudioClip(const AudioClip&) = delete;
            AudioClip& operator=(const AudioClip&) = delete;

            AudioClip(AudioClip&& other) noexcept;
            AudioClip& operator=(AudioClip&& other) noexcept;

            inline AudioType getType() const noexcept { return type; }
            inline int getDefaultChannel() const noexcept { return defaultChannel; }
            inline const std::string& getFilePath() const noexcept { return path; }

            inline const Sound* getStaticData() const noexcept { return staticData; }
            inline Music* getStreamData() const noexcept { return streamData; }

        private:
            Sound* staticData = nullptr;
            Music* streamData = nullptr;

            std::string path = "";
            AudioType type;
            int defaultChannel = 0;
    };

}