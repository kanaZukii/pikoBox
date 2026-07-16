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

    /*
        AnimationPlayer Component, it orchestrates playback of 
        AnimationClips on the parent Entity's SpriteRenderer.
        Supports configurable keyframe interpolation and looping states.
     */
    class AnimationPlayer : public Component {
        public:
            // Renderer assignment by ID
            void setRenderer(uint32_t id);
            // Renderer assignment by alias
            void setRenderer(const std::string& sprrenderer);
            // Renderer assignment by reference
            void setRenderer(SpriteRenderer* renderer);

            // Playback configurations

            void setLoop(bool loop){onLoop = loop;}
            void setTransformLerp(bool enabled){lerpTransform = enabled;}
            void setColorLerp(bool enabled){lerpColor = enabled;}

            // Playback configurations

            void play(const std::string& name, bool loop=false);
            void play(const AnimationClip* animation, bool loop=false);
            void stop();
            void resume();
            void pause();

            std::string getCurrentClipName() const;
            bool isPlaying() const {return playing;}
            bool isLooping() const {return onLoop; }

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

         protected:
            AnimationPlayer() : Component() {className = "AnimationPlayer";}

            void update(float dt) override;

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

     /*
        AudioPlayer Component, for managing sound effects and music playback.
        Consumes a loaded AudioClip asset.
     */
    class AudioPlayer : public Component {
        public:
            // Execution API

            void play(const std::string& audio, bool loop = false, int channel=0, float startAt=0.0f);
            void play(const AudioClip* audio, bool loop = false, int channel=0, float startAt=0.0f);
            void stop();

            // Sets the volume of the audio (0.0f - 1.0f)
            void setVolumeModifier(float volume);

            inline const AudioClip* getCurrentClip() const noexcept { return currentClip; }
            inline float getVolumeModifier() const noexcept { return volumeMod; }
            bool isPlaying() const;
 
            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;
            
        protected:
            AudioPlayer() : Component() { className = "AudioPlayer"; }
  
            void init() override {}
            void update(float dt) override {} 
            void terminate() override { stop(); Component::terminate(); }

            friend class Scene;
            friend class SceneManager;

        private:
            const AudioClip* currentClip = nullptr;
            int currentChannel = 0;
            float volumeMod = 1.0f;
    };
}