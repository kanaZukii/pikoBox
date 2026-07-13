// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once
#include "piko/component.hpp"
#include "piko/math.hpp"

namespace piko {
   
    struct Sprite;
    class AnimationClip;
    class TransformClip;

    class AudioClip;
    class SpriteRenderer;

    class AnimationPlayer : public Component {
        public:
            void update(float dt) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setRenderer(uint32_t id);
            void setRenderer(const std::string& sprrenderer);
            void setRenderer(SpriteRenderer* renderer);

            void setLoop(bool loop){onLoop = loop;}
            void setTransformLerp(bool enabled){lerpTransform = enabled;}
            void setColorLerp(bool enabled){lerpColor = enabled;}

            void play(const std::string& name, bool loop=false);
            void play(const AnimationClip* animation, bool loop=false);
            void stop();

            void resume();
            void pause();

            std::string getCurrentClipName() const;
            bool isPlaying() const {return playing;}
            bool isLooping() const {return onLoop; }

         protected:
            AnimationPlayer() : Component() {className = "AnimationPlayer";}

            friend class Scene;
            friend class SceneManager;

        private:
            const AnimationClip* currentClip = nullptr;
            
            float time = 0.0f;

            bool playing = false;
            bool onLoop = false;
            bool lerpTransform = false;
            bool lerpColor = false;
            
            uint32_t rendererID = 0;
            SpriteRenderer* rendererPtr = nullptr;
    };

    class AudioPlayer : public Component {
        public:
            // Mandatory Lifecycle overrides matching your engine's design
            void init() override {}
            void update(float dt) override {} 
            void terminate() override { stop(); Component::terminate(); }

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;
            
            // Execution API
            void play(const std::string& audio, bool loop = false, int channel=0, float startAt=0.0f);
            void play(const AudioClip* audio, bool loop = false, int channel=0, float startAt=0.0f);
            void stop();
            void setVolumeModifier(float volume);

            inline const AudioClip* getCurrentClip() const noexcept { return currentClip; }
            inline float getVolumeModifier() const noexcept { return volumeMod; }
            bool isPlaying() const;

        protected:
            // Protected constructor enforces Scene allocation rules
            AudioPlayer() : Component() { className = "AudioPlayer"; }

            friend class Scene;
            friend class SceneManager;

        private:
            const AudioClip* currentClip = nullptr;
            int currentChannel = 0;
            float volumeMod = 1.0f;
    };
}