// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <functional>
#include "piko/math.hpp"

namespace piko {
    
    struct Entity;
    
    class SceneManager;
    class Scene;

    class Component;
    class Drawable;
    class Collidable;
    class Script;

    class Renderer;

    class Component{
        public:
             enum class ANCHOR {
                TOP_LEFT, 
                CENTER,    
                BOTTOM_MID
            };

            enum class SIZING {
                MANUAL,      
                STRETCH_FILL
            };

            virtual void init(){}
            virtual void update(float dt){if(!doUpdate || !owner) {return;}}
            virtual void terminate(){discardedIDs.push_back(ID); owner = nullptr; }
            virtual ~Component() = default;

            virtual std::string serialize(); 
            virtual void deserialize(const std::string& rawJson);

            void setOffset(Vect2 offset);
            void setSize(Vect2 size);

            bool isMouseOver(const Vect2& rawMousePos);
            
            uint32_t getID() const {return ID;}
            std::string getAlias() const {return alias;}
            std::string getClassName()const{return className;}

            Entity& getOwner(){ return *owner; }

            uint32_t getOwnerID(){return ownerID;}

            const Vect2& getOffset()const{return offsetPos; }
            const Vect2& getSize()const{return size;}

            const Rect& getGlobalTransform();
            
            void invalidateTransform() { isTransformDirty = true; }

            void enableUpdate(bool enabled){this->doUpdate = enabled;}
            bool isUpdating()const{return doUpdate;}

            void setSizing(SIZING sizing);
            void setAnchor(ANCHOR anchor);
            
        protected:
            Component() {
                if(!discardedIDs.empty()){
                    ID = discardedIDs.at(discardedIDs.size()-1);
                    discardedIDs.pop_back();
                } else {
                    ID = nextID++; 
                }
            }

            Entity* owner =  nullptr;
            uint32_t ownerID = 0;

            std::string className = "Component";
            std::string alias = "unknown";

            Vect2 offsetPos = {0.0f, 0.0f};
            Vect2 size = {1.0f, 1.0f};

            Rect globalTransform = {0.0f, 0.0f, 0.0f, 0.0f};
            bool isTransformDirty = true;

            ANCHOR anchor = ANCHOR::TOP_LEFT;
            SIZING sizing = SIZING::MANUAL;

            bool doUpdate = true;
            friend class Scene;
            friend class SceneManager;

        private:
            static uint32_t nextID;
            uint32_t ID = 0;

            static std::vector<uint32_t> discardedIDs;
    };

    class PhysicsBody : public Component {
        public:
            bool wasCulled = false;
            Vect2 velocity = {0.0f, 0.0f};
            float gravityScale = 1.0f;
            bool isGrounded = false;

            void init() override {}
            void update(float dt) override {}
            void terminate() override;

            void setCulled(bool culled) { this->culled = culled; }
            bool isCulled() const { return culled;}

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            const std::vector<Collidable*>& getColliders(){return colliders;}
        
        protected:
            PhysicsBody() : Component() { className = "PhysicsBody"; }
            
            std::vector<Collidable*> colliders;
            void attachCollider(Collidable* coll) {if(coll){colliders.push_back(coll);}}
            void detachCollider(Collidable* coll) {
                if (coll) {
                    colliders.erase(std::remove(colliders.begin(), colliders.end(), coll), colliders.end());
                }
            }

            bool culled = false;
            
            friend class Scene;
            friend class SceneManager;
            friend class Collidable;
    };

    class Drawable : public Component {
        public:
            virtual void draw(Renderer& renderer){}

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setVisible(bool visible){this->visible = visible;}
            bool isVisible()const{return visible;}
            
            void setColor(Color4 c) { this->color = c; }
            Color4 getColor() const { return color; }

            void setOpacity(uint8_t opacity){color.a = opacity;}
            uint8_t getOpacity(){return color.a;}

            void setZIndex(int z) { this->zIndex = z; }
            int getZIndex() const { return zIndex; }

            void setScreenSpaceMode(bool enabled){useScreenSpace = enabled;}

            void setScissorClip(bool enabled, Rect region={0.0f,0.0f,0.0f,0.0f});
            inline Rect getScissorRegion(){return scissorBox;}

        protected:
            Drawable() : Component() {className = "Drawable";}

            Color4 color = {255, 255, 255, 255};
            int zIndex = 0;
            bool visible = true;
            bool useScreenSpace = false;
            bool useScissor = false;
            Rect scissorBox = {0.0f, 0.0f, 0.0f, 0.0f};

            friend class Scene;
            friend class SceneManager;
    };

    class Collidable : public Component {
        public:
        
            void terminate() override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setDynamic(bool dynamic) { this->dynamic = dynamic; }
            void setTrigger(bool trigger) { triggered = trigger; }
            void setCulled(bool culled) { this->culled = culled; }

            bool isTriggered() const { return triggered; }
            bool isDynamic() const { return dynamic; }
            bool isCulled() const { return culled;}

            void setBody(const std::string& bodyname);
            void setBody(uint32_t id);
            void removeBody();

            PhysicsBody* getPhysicsBody(){return parentBody;}

        protected:
            Collidable() : Component() { className = "Collidable"; }

            bool triggered = false;
            bool dynamic = false;
            bool culled = false;

            uint32_t pbodyID = 0;
            PhysicsBody* parentBody = nullptr;

            friend class Scene;
            friend class SceneManager;
    };

    class Script : public Component {
        public:
            virtual void onStart() {}
            virtual void onUpdate(float dt) {}
            virtual void onEarlyUpdate(float dt){}
            virtual void onLateUpdate(float dt){}
            
            void init() override { onStart(); }
            void update(float dt) override { onUpdate(dt); }
            void earlyUpdate (float dt) { onEarlyUpdate(dt); }
            void lateUpdate (float dt) { onLateUpdate(dt); }

        protected:
            Script() : Component() {className = "Script";}
            friend class Scene;
            friend class SceneManager;
    };
}