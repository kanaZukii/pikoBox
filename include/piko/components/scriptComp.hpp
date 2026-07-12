#pragma once
#include "piko/component.hpp"
#include "piko/math.hpp"

namespace piko {
    
    class SpriteRenderer;
    class TextBoxRenderer;

    class CollisionResScript : public Script {
        public:
            std::string targetEntity = "";

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            // Callback passes: 1. This script instance, 2. The foreign object hit
            using ResponseCallback = std::function<void(CollisionResScript* self, Collidable* other)>;

            void setCallback(ResponseCallback callback){ responseCallback = callback;}

            void onStart() override;

        protected:
            CollisionResScript() : Script() {className = "CollisionResScript";}
            CollisionResScript (std::string targetTag, ResponseCallback callback)
                : Script(), targetEntity(targetTag), responseCallback(callback) {
                className = "CollisionResScript";
            }

            friend class Scene;
            friend class SceneManager;

        private:
            ResponseCallback responseCallback;
    };

    class PlayerMoveScript : public Script {
        public:
            float speed = 200.0f;
            bool topDownMode = false;

            void setPlayerBody(uint32_t pBody);
            void setPlayerBody(PhysicsBody* pBody);
            void setPlayerBody(const std::string& bodyname);

            void onEarlyUpdate(float dt) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

        protected:
            PlayerMoveScript() : Script() {className = "PlayerMoveScript";}
            PlayerMoveScript(float movementSpeed, bool isometricOrTopDown = false) 
                : Script(), speed(movementSpeed), topDownMode(isometricOrTopDown) {
                className = "PlayerMoveScript";
            }

            uint32_t bodyId = 0;
            PhysicsBody* pBody = nullptr;

            friend class Scene;
            friend class SceneManager;
    };

    class CameraMoveScript : public Script {
        public:
            enum class MODE { MANUAL, FOLLOW };

            void setMode(MODE newMode) { mode = newMode; }
            
            void setTarget(const Entity* target);
            void setTarget(const uint32_t id);
            void setTarget(const std::string& targetname);

            void setFollowLerpSpeed(float speed) { lerpSpeed = speed; }
            void setManualPanSpeed(float speed) { panSpeed = speed; }
            void setCamPos(Vect2 position);
            void setCamOffset(Vect2 offset);

            void setTargetOffset(Vect2 offset){targetOffset = offset;}

            void onUpdate(float dt) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

        protected:
            CameraMoveScript() : Script() {className = "CameraMoveScript";}
            CameraMoveScript(Entity* targetEntity) 
                : Script(), targetEntity(targetEntity) {
                    className = "CameraMoveScript";
                    if(targetEntity){
                        mode = MODE::FOLLOW;
                    }
                }

            friend class Scene;
            friend class SceneManager;

        private:
            uint32_t targetId = 0;
            const Entity* targetEntity = nullptr;

            MODE mode = MODE::MANUAL;
            float lerpSpeed = 5.0f;
            float panSpeed = 300.0f;
            Vect2 targetOffset = {0.0f, 0.0f};
    };

    class ButtonScript : public Script {
        public:
            void onEarlyUpdate(float dt) override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setLabel(const std::string& newLabel);
            std::string getLabel() const { return label; }
            
            void enableHoverEffect(bool enabled) { hoverEffect = enabled; }
            void enableColorControl(bool enabled) { colorControl = enabled;}
            void setHoverScaleEffect(float scale) { hoverScale = scale; }

            void setBGColor(Color4 color) { bgColor = color; }
            void setLabelColor(Color4 color) { textColor = color; }

            void setBGHoverColor(Color4 color) { bgHover = color; }
            void setTextHoverColor(Color4 color) { textHover = color; }

            void attachSpriteRenderer(SpriteRenderer* sprrenderer);
            void attachTextBoxRenderer(TextBoxRenderer* txtrenderer);

            void attachSpriteRenderer(uint32_t sprrenderer);
            void attachTextBoxRenderer(uint32_t txtrenderer);

            void attachSpriteRenderer(const std::string& sprrenderer);
            void attachTextBoxRenderer(const std::string& txtrenderer);

        protected:
            ButtonScript() : Script() { className = "ButtonScript"; }
            ButtonScript(const std::string& label) : Script(), label(label) { className = "ButtonScript"; }

            friend class Scene;
            friend class SceneManager;

        private:
            std::string label = "";
            bool hoverEffect = true;
            bool colorControl = true;
            
            // Scale Interpolation Fields
            float hoverScale = 1.05f;      // Targets a 5% increase by default
            float currentScale = 1.0f;     // Current animated scale state
            float scaleSpeed = 10.0f;       // Tweak this to make the grow feel snappier or smoother

            Color4 bgHover = {255, 255, 255, 255};
            Color4 textHover = {50, 50, 50, 255};
            Color4 bgColor = {245, 245, 245, 255};
            Color4 textColor = {10, 10, 10, 255};

            uint32_t bgId = 0;
            uint32_t txtId = 0;

            SpriteRenderer* bgrenderer = nullptr;
            TextBoxRenderer* textrenderer = nullptr;
    };
}