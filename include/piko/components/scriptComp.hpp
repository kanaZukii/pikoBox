// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once
#include "piko/component.hpp"
#include "piko/math.hpp"

namespace piko {
    
    class SpriteRenderer;
    class TextBoxRenderer;

    /*
        CollisionRes Script for handling collision response logic.
        Uses a functional callback to define behavior upon entity overlap.
        Experimental, does not serialize well.
     */
    class CollisionResScript : public Script {
        public:
            // Tag of the entity this script reacts to
            std::string targetEntity = "";

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            // Callback triggered on collision
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

    /*
        PlayerMove Script for standard 2D movement parent Entity's PhysicsBody.
        Supports both platformer and top-down navigation.
        
        Required Input Action Bindings:
         - "player_move_left"  | "player_move_right"
         - "player_move_up"    | "player_move_down"
     */
    class PlayerMoveScript : public Script {
        public:
            float speed = 200.0f;
            bool topDownMode = false;

            // Binds the Entity's PhysicsBody to response to inputs.
            void setPlayerBody(uint32_t pBody);
            void setPlayerBody(PhysicsBody* pBody);

            // Binds the Parent Entity's PhysicsBody to response to inputs.
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

    /*
        CameraMove Script for controlling camera behavior.
        Supports static manual controls or dynamic target tracking (follow).
        
         Required Input Action Bindings for manual control:
         - "camera_move_left"  | "camera_move_right"
         - "camera_move_up"    | "camera_move_down"
     */
    class CameraMoveScript : public Script {
        public:
            enum class MODE { MANUAL, FOLLOW };

            void setMode(MODE newMode) { mode = newMode; }
            
            // Define the entity for the camera to track. Automatically sets mode to FOLLOW
            void setTarget(const Entity* target);
            // Define the entity for the camera to track. Automatically sets mode to FOLLOW
            void setTarget(const uint32_t id);
            // Define the entity for the camera to track. Automatically sets mode to FOLLOW
            void setTarget(const std::string& targetname);

            void setFollowLerpSpeed(float speed) { lerpSpeed = speed; }
            void setManualPanSpeed(float speed) { panSpeed = speed; }

            // Direct active camera position override
            void setCamPos(Vect2 position);
            // Direct active camera offset override
            void setCamOffset(Vect2 offset);

            // Offset relative to the target for adjusting and centering.
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

    /*
        Button UI Script for interactive button behavior.
        Manages hover states, scale interpolation, and visual feedback.
        Requires the parent Entity's SpriteRenderer and TextBoxRenderer for visuals.
        Publishes a Button Event when interacted with.
     */
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

            // Background Renderer linking
            void attachSpriteRenderer(SpriteRenderer* sprrenderer);
            // TextBox Renderer linking
            void attachTextBoxRenderer(TextBoxRenderer* txtrenderer);

            // Background Renderer linking
            void attachSpriteRenderer(uint32_t sprrenderer);
            // TextBox Renderer linking
            void attachTextBoxRenderer(uint32_t txtrenderer);

            // Background Renderer linking
            void attachSpriteRenderer(const std::string& sprrenderer);
            // TextBox Renderer linking
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

            // Visual properties
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