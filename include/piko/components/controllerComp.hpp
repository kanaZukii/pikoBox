#pragma once
#include "piko/component.hpp"
#include "piko/math.hpp"

namespace piko {
   
    class AudioClip;
    class SpriteRenderer;
    struct Sprite;

    class AnimationPlayer : public Component {
        public:
            struct Frame {
                const Sprite* sprite = nullptr;
                float duration = 0.2f;
                Vect2 offset = {0.0f, 0.0f};
            };

            struct Clip {
                std::string name;
                std::vector<AnimationPlayer::Frame> frames;
                bool loop = true;
            };

            void update(float dt) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setRenderer(uint32_t id);
            void setRenderer(const std::string& sprrenderer);
            void setRenderer(SpriteRenderer* renderer);

            void addClip(const Clip& clip);
            void play(const std::string& name);
            void stop();
            void pause();
            void resume();

            inline std::string getCurrentClipName() const {
                if(currentClip) {return currentClip->name;}
                return "";
            }
            bool getIsPlaying() const {return isPlaying;}

         protected:
            AnimationPlayer() : Component() {className = "AnimationPlayer";}

            std::string serializeClip(const Clip& clip);
            Clip deserializeClip(const std::string& clipName, const std::string& rawJson);

            std::unordered_map<std::string, Clip> clips;
            const Clip* currentClip = nullptr;
            
            int currentFrameIndex = 0;
            float frameTimer = 0.0f;
            bool isPlaying = false;
            
            uint32_t rendererID = 0;
            SpriteRenderer* rendererPtr = nullptr;

            friend class Scene;
            friend class SceneManager;
    };

    class AudioPlayer : public Component {
        public:
            // Mandatory Lifecycle overrides matching your engine's design
            void init() override {}
            void update(float dt) override {} // Stays silent; backend AudioManager does the heavy ticking
            void terminate() override { Component::terminate(); stop(); }

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            // Configuration API
            void addTrack(const std::string& trackName, const AudioClip* clip);
            
            // Execution API
            void play(const std::string& trackName, bool loop = false);
            void stop();
            void setVolumeModifier(float volume);

            inline const std::string& getCurrentTrackName() const noexcept { return currentTrackName; }
            inline float getVolumeModifier() const noexcept { return volumeMod; }

        protected:
            // Protected constructor enforces Scene allocation rules
            AudioPlayer() : Component() { className = "AudioPlayer"; }

            friend class Scene;
            friend class SceneManager;

        private:
            std::unordered_map<std::string, const AudioClip*> tracks;
            const AudioClip* activeClip = nullptr;
            std::string currentTrackName = "";
            float volumeMod = 1.0f;
    };

    class CompTransformAnimator : public Component{
        public: 
            struct TransformFrame{
                Rect target = {0.0f, 0.0f, 0.0f, 0.0f};
                float duration = 0.2f;
            };

            struct TransformClip{
                std::string name = "";
                std::vector<TransformFrame> keyframes;
                bool loop = false;
            };

            void init() override {}

            void update(float dt) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setTargetComponent(uint32_t c);
            void setTargetComponent(Component* c);

            void addClip(const TransformClip& clip);
            void play(const std::string& name);
            void stop();
            void pause();
            void resume();

            inline std::string getCurrentClipName() const {
                if(currentClip) {return currentClip->name;}
                return "";
            }
            bool getIsPlaying() const {return isPlaying;}

        protected:
            CompTransformAnimator() : Component() {className = "CompTransformAnimator";}   

            friend class Scene;
            friend class SceneManager;
        private:
            std::unordered_map<std::string, TransformClip> clips;

            const TransformClip* currentClip = nullptr;
            int currentFrameIndex = 0;
            float frameTimer = 0.0f;
            bool isPlaying = false;

            uint32_t targetID = 0;
            Component* targetC = nullptr;

            std::string serializeClip(const TransformClip& clip);
            TransformClip deserializeClip(const std::string& clipName, const std::string& rawJson);
    };

    class DrawColorAnimator : public Component{
        public: 
            struct ColorFrame{
                Color4 target = {0, 0, 0, 0};
                float duration = 0.2f;
            };

            struct ColorClip{
                std::string name = "";
                std::vector<ColorFrame> keyframes;
                bool loop = false;
            };

            void init() override {}

            void update(float dt) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setTargetDrawable(uint32_t d);
            void setTargetDrawable(Drawable* d);

            void addClip(const ColorClip& clip);
            void play(const std::string& name);
            void stop();
            void pause();
            void resume();

            inline std::string getCurrentClipName() const {
                if(currentClip) {return currentClip->name;}
                return "";
            }
            bool getIsPlaying() const {return isPlaying;}

        protected:
            DrawColorAnimator() : Component() {className = "DrawColorAnimator";}   

            friend class Scene;
            friend class SceneManager;
        private:
            std::unordered_map<std::string, ColorClip> clips;

            const ColorClip* currentClip = nullptr;
            int currentFrameIndex = 0;
            float frameTimer = 0.0f;
            bool isPlaying = false;

            uint32_t targetID = 0;
            Drawable* targetD = nullptr;

            std::string serializeClip(const ColorClip& clip);
            ColorClip deserializeClip(const std::string& clipName, const std::string& rawJson);
    };
}