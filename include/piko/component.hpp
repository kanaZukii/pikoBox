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

    /*
        Component. Base class for all entity behaviors and data.
    */
    class Component{
        public:
            // Anchor a component's offset position relative to its parent Entity.
             enum class ANCHOR {
                TOP_LEFT, 
                CENTER,    
                BOTTOM_MID
            };

            // Sizing mode for components relative to its parent Entity.
            enum class SIZING {
                MANUAL,         // Follows the width and height set via setSize
                STRETCH_FILL    // Expand and set the size to the parent Entity's width and height.
            };

            virtual void init(){}
            virtual void update(float dt){if(!doUpdate || !owner) {return;}}
            virtual void terminate(){discardedIDs.push_back(ID); owner = nullptr; }
            virtual ~Component() = default;

            // Serialization, to save Component data to a string in JSON format.
            virtual std::string serialize(); 

            // Deserialization, to load Component data from a string in JSON format.
            virtual void deserialize(const std::string& rawJson);

            // Set an offset position relative to its parent Entity's world position.
            void setOffset(Vect2 offset);

            void setSize(Vect2 size);

            /*
                Returns true if the mouse position is within the bounds of the Component.
                Partialy implemented, only works if bounds matches screen coords.
            */
            bool isMouseOver(const Vect2& rawMousePos);
            
            // Returns the instance ID of the Component.
            uint32_t getID() const {return ID;}

            // Returns the name/alias assigned to the Component.
            std::string getAlias() const {return alias;}

            // Returns the class name. Used for type checking.
            std::string getClassName()const{return className;}

            // Returns a pointer to its parent Entity.
            Entity& getOwner(){ return *owner; }

            // Returns the instance ID of the parent Entity.
            uint32_t getOwnerID(){return ownerID;}


            const Vect2& getOffset()const{return offsetPos; }
            const Vect2& getSize()const{return size;}

            /*
                Returns the transform of the Component in world space.
                Lazy getter, calculated when dirtied based on its size,
                offset position, anchor point and sizing mode.
            */            
            const Rect& getGlobalTransform();
            
            // Sets transform to dirty. Transform will be recalculated when retrieved.
            void invalidateTransform() { isTransformDirty = true; }

            void enableUpdate(bool enabled){this->doUpdate = enabled;}
            bool isUpdating()const{return doUpdate;}

            // Sets a sizing method either MANUAL or STRETCH_FILL
            void setSizing(SIZING sizing);

            // Sets an anchor point for offsets
            void setAnchor(ANCHOR anchor);
            
        protected:
            /*
                Base component constructor. Auto assign its runtime instance ID.
                Derived classes must have no parameters in their constructor.
                Must be called in the constructor of a derived class.
            */ 
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

            /*
                Setting a className must be done upon object construction.
                Crucial for runtime type check and serialization, especially for customs.
            */
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

    /*
        PhysicsBody Component. Defines physical properties and 
        gives a collider to have a controllable body.
        Simulated by the PhysicsEngine.
    */
    class PhysicsBody : public Component {
        public:
            // Will leave these as public members for easy access
            // Flag used ONLY by the PhysicsEngine for accurate culling
            bool wasCulled = false;
            // How fast it is moving and where is it going?
            Vect2 velocity = {0.0f, 0.0f};
            // How strong will it be affected by the PhysicsEngine's gravity?
            float gravityScale = 1.0f;
            // Is the attached collider on top of another collider?
            bool isGrounded = false;

            void init() override {}
            void update(float dt) override {}
            void terminate() override;

            // Only used by the PhysicsEngine, set to true when outside Simulation Bounds
            void setCulled(bool culled) { this->culled = culled; }

            // Returns true if culled and ignored by the PhysicsEngine this frame.
            bool isCulled() const { return culled;}

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            // Getter for all attached Collidables.
            const std::vector<Collidable*>& getColliders(){return colliders;}
        
        protected:
            PhysicsBody() : Component() { className = "PhysicsBody"; }
            
            std::vector<Collidable*> colliders;

            // Called by a Collidable during setBody.
            void attachCollider(Collidable* coll) {if(coll){colliders.push_back(coll);}}

            // Must be called when a Collidable Component is removed/deleted.
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

    /*
        Drawable Component. Contains data and methods for rendering.
    */
    class Drawable : public Component {
        public:
            /* 
                Submits a draw request to the renderer. 
                Implemention differs per derived class.
            */
            virtual void draw(Renderer& renderer){}

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            void setVisible(bool visible){this->visible = visible;}
            bool isVisible()const{return visible;}
            
            void setColor(Color4 c) { this->color = c; }
            Color4 getColor() const { return color; }

            // Manipulates the Color4 alpha value for transparency.
            void setOpacity(uint8_t opacity){color.a = opacity;}
            uint8_t getOpacity(){return color.a;}

            /* 
                Set a Z depth that defines its draw order. 
                Lower values are rendered first.
            */
            void setZIndex(int z) { this->zIndex = z; }
            int getZIndex() const { return zIndex; }

            /*
                Toggles screen space coordinates.
                If true, it is fixed to the screen.
            */
            void setScreenSpaceMode(bool enabled){useScreenSpace = enabled;}
            
            /* 
               Enables/disables a scissor clipping region.
               Pixels outside this Rect will be discarded when drawn.
            */
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

    /*
        Collidable Component. Defines collision geometry. 
        Links to a parent PhysicsBody for physical responses.
    */
    class Collidable : public Component {
        public:
        
            void terminate() override;

            std::string serialize() override; 
            void deserialize(const std::string& rawJson) override;

            // If true, it will react to forces applied via PhysicsBody.
            void setDynamic(bool dynamic) { this->dynamic = dynamic; }

            /*
                If true, overlap will be detected but collisions will not be resolved.
                When overlapped with another trigger, a Collision Event will be published.
            */
            void setTrigger(bool trigger) { this->trigger = trigger; }

            // Used by the PhysicsEngine.
            void setCulled(bool culled) { this->culled = culled; }
            bool isTrigger() const { return trigger; }
            bool isDynamic() const { return dynamic; }

            // Returns true if culled and ignored by the PhysicsEngine this frame.
            bool isCulled() const { return culled;}

            // Attach a PhysicsBody to the Collidable, immediately set dynamic to true.
            void setBody(const std::string& bodyname);
            // Attach a PhysicsBody to the Collidable, immediately set dynamic to true.
            void setBody(uint32_t id);

            void removeBody();

            PhysicsBody* getPhysicsBody(){return parentBody;}

        protected:
            Collidable() : Component() { className = "Collidable"; }

            bool trigger = false;
            bool dynamic = false;
            bool culled = false;

            uint32_t pbodyID = 0;
            PhysicsBody* parentBody = nullptr;

            friend class Scene;
            friend class SceneManager;
    };

    /*
        Script Component. For user-definable logic container. 
        Provides hook methods for different stages of the game loop.
    */
    class Script : public Component {
        public:
            // Called at init()
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