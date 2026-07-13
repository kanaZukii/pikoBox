// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include "piko/time.hpp"
#include "piko/event.hpp"
#include "piko/component.hpp"
#include "piko/components/drawableComp.hpp"
#include "piko/components/collidableComp.hpp"
#include "piko/components/scriptComp.hpp"
#include "piko/components/controllerComp.hpp"
#include "piko/logger.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <functional>

namespace piko {
    class AssetManager;
    class InputManager;
    class AudioManager;
    class SceneManager;
    class Cam;

    class Renderer;
    class Scene;

    /*
        A function pointer type used by the scene's component registry to
        allow it to be created from a serialized data.
    */
    using ComponentFactoryFunc = std::function<Component*()>;

    /*
        Entity, a container for components and main transformation data.
        Must be created and owned by a Scene. 
    */
    struct Entity{
        uint32_t id = 0;            // Instance ID
        std::string alias = "";     // Custom defined name.
        std::string sceneName = "";
        Scene* scene = nullptr;     // Reference to the parent scene.
        
        Rect transform = {0};

        // Maps the entity's component identifiers to their specific instance IDs.
        std::unordered_map<std::string, uint32_t> components;

        // Set all components under this entity to dirty.
        void setDirtyTransform();
    };

    /*
        Scene, it manages the entities' lifecycles and component to entity registration.
    */
    class Scene{
        public: 
            /* 
                Mutation commands that must be deferred until the end of the frame.
                Use this to prevent invalidating collections when mutating inside
                main game loop runtime.
            */
            struct DeferredCommand {
                enum class TYPE { CREATE_ENTITY, ADD_COMPONENT, REMOVE_ENTITY, REMOVE_COMPONENT };
                TYPE type;
                std::function<void()> execute;
            };

            ~Scene() = default;

            const std::string& getName() const {return name;}

            // Sets the camera's initial state for when this scene is loaded.
            void setInitCamPosition(Vect2 pos){initCamPos = pos;}
            void setInitCamZoom(float zoom){initCamZoom = zoom;}

            // Creates a new entity to this scene and returns a pointer to it.
            Entity* createEntity(std::string key);

            // Getter methods for an entity via its instance ID or string alias.
            Entity* getEntity(uint32_t id);
            Entity* getEntity(std::string key);
           
            // Removes an entity IMMEDIATELY. Use this only outside the game loop.
            bool removeEntity(const uint32_t& id);
            bool removeEntity(const std::string& key);

            // Removes a component IMMEDIATELY. Use this only outside the game loop.
            bool removeComponent(const uint32_t& id);
            bool removeComponent(const uint32_t& entity, const std::string& comp);

            /* 
                Queues an entity's creation to be performed at the 
                end of the current frame to ensure loop stability.
            */
            void safeCreateEntity(const std::string& key, std::function<void(Entity*)> initCallback = nullptr);
 
            /* 
                Queues an entity's deletion to be performed at the 
                end of the current frame to ensure loop stability.
            */
            void safeRemoveEntity(uint32_t id);
            void safeRemoveEntity(const std::string& key);

             /* 
                Queues a component's deletion to be performed at the 
                end of the current frame to ensure loop stability.
            */
            void safeRemoveComponent(uint32_t componentId);
            void safeRemoveComponent(uint32_t entityId, const std::string& compKey);

            // Clears all event listeners associated with this scene.
            void unsubscribeListeners();

            // Flags an entity's transform and its components dirty.
            void setDirtyTransform(const uint32_t &entity);

            /*
                Entity serialization/deserialization for saving and loading the entity's state.
                Also triggers all child component's serialization/deserialization.
            */
            std::string serializeEntity(const uint32_t& entity); 
            void deserializeEntity(const uint32_t& entity, const std::string& rawJson);

            // Scene serialization/deserialization for saving and loading scene state.
            std::string serialize(); 
            void deserialize(const std::string& rawJson);

            // Registers a callback to be executed after the scene has finished loading via deserialization.
            void addPostLoadJob(std::function<void()> job) {postLoadJobs.push_back(job);}

            /*
                Accessors for core engine subsystem.
                Allowing components to access via a scene reference.
            */
            InputManager* inputs() const { return inputMAN; }
            AssetManager* assets() const { return assetMAN; }
            AudioManager* audio() const { return audioMAN; }
            Cam* camera() const { return sceneCam; }

            // Returns raw access to collections of components for iteration.
            const std::vector<Collidable*>& getCollidables() const { return collidables; }
            const std::vector<PhysicsBody*>& getPhysicsBodies() const { return physicsBodies; }
            const std::vector<Drawable*>& getDrawables() const { return drawables; }
            const std::vector<Script*>& getScripts() const { return scripts; }

            // Returns a list of all entity aliases/keys currently that exist in the scene.
            const std::vector<std::string> getEntityNames() const;

        protected:
            // Protected constructor. Must be created by SceneManager.
            Scene(){}

            // Scene lifecycle methods. Must be handled by the SceneManager/Engine class.
            void init();
            void update(float dt);
            void draw(Renderer& renderer);
            void terminate();
        
            // Executes deferred setup tasks queued during scene loading via deserialization.
            void flushPostLoadJobs();

            // Processes the deferred command queue (creation/deletion).
            void processDeferredCommands(); 
            
            /*
                Registers a component to an entity. 
                Returns false if registration fails.
            */
            bool registerComponent(std::unique_ptr<Component> baseComp, const uint32_t &entity, std::string key);
            bool registerComponent(std::unique_ptr<Component> baseComp, const std::string& entity, std::string key);

            // Dependency injection methods for core engine subsystems.
            void setEventBroker(EventBroker* broker) { eventBroker = broker; }
            void setInputManager(InputManager* input) { inputMAN = input; }
            void setAssetsManager(AssetManager* assets) { assetMAN = assets; }
            void setAudioManager(AudioManager* audio) { audioMAN = audio; }
            void setCamera(Cam* cam) { sceneCam = cam; }
            
            std::string name = "Scene";
            Vect2 initCamPos = {0.0f, 0.0f};
            float initCamZoom = 1.0f;
            
            friend class SceneManager;

        private:
            // For entity instance ID generation
            static uint32_t entityNextID;                       // Incremental counter for new Entities. Start at 1.
            static std::vector<uint32_t> entityDiscardedIDs;    // Pool for reusing IDs from deleted entities.

            // For entity storage
            std::vector<std::unique_ptr<Entity>> entities;      // List of entities in the scene.
            std::unordered_map<uint32_t, int> entityByIDs;      // ID to index lookup for Entities.
            std::unordered_map<std::string, int> entityByAlias; // Alias to index lookup for Entities.

            // For component storage
            std::vector<std::unique_ptr<Component>> components; // List of components in the scene.
            std::unordered_map<uint32_t, int> componentByIDs;   // ID to index lookup for Components.

            // Cached pointers for Drawable components for speedy iteration.
            std::vector<Drawable*> drawables;
            std::unordered_set<uint32_t> drawableIDs;

            // Cached pointers for Collidable components for speedy iteration.
            std::vector<Collidable*> collidables;
            std::unordered_set<uint32_t> collidableIDs;

            // Cached pointers for Script components for speedy iteration.
            std::vector<Script*> scripts;
            std::unordered_set<uint32_t> scriptIDs;

            // Cached pointers for PhysicsBody components for speedy iteration.
            std::vector<PhysicsBody*> physicsBodies;
            std::unordered_set<uint32_t> physicsBodyIDs;

            // Component factory registry passed by the SceneManager.
            std::unordered_map<std::string, ComponentFactoryFunc>* compRegistry = nullptr;

            // Tasks for post-deserialization.
            std::vector<std::function<void()>> postLoadJobs;

            // Queued scene mutation commands.
            std::vector<DeferredCommand> deferredCommandBuffer;

            std::vector<uint32_t> subscriptionIDs;  // Track active event listeners for cleanup.

            uint32_t removedComps = 0;              // Counters to track component removal.
            uint32_t removedEnts = 0;               // Counters to track entity removal.
            
            // Garbage collection methods for cleaning up nulled entities/components.
            void collectGarbageE();
            void collectGarbageC();

            EventBroker* eventBroker = nullptr;
            InputManager* inputMAN = nullptr;
            AssetManager* assetMAN = nullptr;
            AudioManager* audioMAN = nullptr;
            Cam* sceneCam = nullptr;
        
        public:

            /*
                Adds a new component instance to an entity with a component type of T.
                Returns a pointer to the added component on success, or nullptr if registration fails.
                T must be a valid Component type.
            */
            template <typename T, typename... Args>
            inline T* addComponent(const uint32_t& entity, std::string key, Args&&... args) {

                auto comp = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
                T* ptr = comp.get();
                std::unique_ptr<Component> baseComp = std::move(comp);

                if(registerComponent(std::move(baseComp), entity, key)){
                    return ptr;
                }
                
                return nullptr;
            }

            /*
                Adds a new component instance to an entity with a component type of T.
                Returns a pointer to the added component on success, or nullptr if registration fails.
                T must be a valid Component type.
            */
            template <typename T, typename... Args>
            inline T* addComponent(const std::string& entity, std::string key, Args&&... args) {

                auto comp = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
                T* ptr = comp.get();
                std::unique_ptr<Component> baseComp = std::move(comp);

                if(registerComponent(std::move(baseComp), entity, key)){
                    return ptr;
                }
                
                return nullptr;
            }

            /*
                Retrieves a component of type T by its unique ID.
                Requires T to be a valid descendant of Component.
            */
            template<typename T>
            inline T* getComponent(const uint32_t& id){
                auto it = componentByIDs.find(id);

                if(it != componentByIDs.end()) {
                    Component* base = components[it->second].get();
                    return dynamic_cast<T*>(base);
                }
                PBOX_ERROR("SCENE [%s]: Component '%d' not found.", name.c_str(), id);
                return nullptr;
            }

            /*
                Retrieves a component of type T by entity identifier and component alias.
                Requires T to be a valid descendant of Component.
            */
            template<typename T>
            inline T* getComponent(std::string entity, const std::string& comp){
                std::unordered_map<std::string, uint32_t>& e_clist = getEntity(entity)->components; 
                auto it = e_clist.find(comp);

                if(it != e_clist.end()) {
                    auto index = componentByIDs.find(it->second);

                    if(index != componentByIDs.end()) {
                        Component* base = components[index->second].get();
                        return dynamic_cast<T*>(base);
                    }else {
                        return nullptr;
                    }
                }
                PBOX_ERROR("SCENE [%s]: Component '%s' not found.", name.c_str(), comp.c_str());
                return nullptr;
            }

            /*
                Retrieves a component of type T by entity identifier and component alias.
                Requires T to be a valid descendant of Component.
            */
            template<typename T>
            inline T* getComponent(const uint32_t& entity, const std::string& comp){
                std::unordered_map<std::string, uint32_t>& e_clist = getEntity(entity)->components; 
                auto it = e_clist.find(comp);

                if(it != e_clist.end()) {
                    auto index = componentByIDs.find(it->second);

                    if(index != componentByIDs.end()) {
                        Component* base = components[index->second].get();
                        return dynamic_cast<T*>(base);
                    }else {
                        return nullptr;
                    }
                }
                PBOX_ERROR("SCENE [%s]: Component '%s' not found.", name.c_str(), comp.c_str());
                return nullptr;
            }

            /*
                Queues a component addition to an entity to be performed at the end of the current frame.
                The optional 'initCallback' allows for immediate configuration of the component.
            */
            template <typename T>
            inline void safeAddComponent(uint32_t entityId, const std::string& compKey, 
                                std::function<void(T*)> initCallback = nullptr) {

                deferredCommandBuffer.push_back({
                    DeferredCommand::TYPE::ADD_COMPONENT,
                    [this, entityId, compKey, initCallback]() {
                        // 1. Instantiated cleanly using the required default constructor
                        T* ptr = this->addComponent<T>(entityId, compKey);
                        
                        // 2. Configure properties immediately on the spot
                        if (ptr && initCallback) {
                            initCallback(ptr);
                        }
                    }
                });
            }

            /*
                Queues a component addition to an entity to be performed at the end of the current frame.
                The optional 'initCallback' allows for immediate configuration of the component.
            */
            template <typename T>
            inline void safeAddComponent(const std::string& entityKey, const std::string& compKey, 
                                std::function<void(T*)> initCallback = nullptr) {
                
                deferredCommandBuffer.push_back({
                    DeferredCommand::TYPE::ADD_COMPONENT,
                    [this, entityKey, compKey, initCallback]() {
                        T* ptr = this->addComponent<T>(entityKey, compKey);
                        if (ptr && initCallback) {
                            initCallback(ptr);
                        }
                    }
                });
            }

            // Checks if the entity still exist
            inline bool entityExist(std::string key) const { return entityByAlias.count(key) > 0;}
            inline bool entityExist(uint32_t id) const { return entityByIDs.count(id) > 0; }

            // Checks if the component still exist
            inline bool componentExist(uint32_t id) const { return componentByIDs.count(id) > 0;}

            // Registers a listener's callback for events of type T.
            template <typename T>
            inline void listenToEvent(std::function<void(const T&)> callback) {
                uint32_t id = eventBroker->subscribe<T>(callback);
                subscriptionIDs.push_back(id);
            }

            // Publishes an event of type T to the global event broker.
            template <typename T>
            inline void publishEvent(const T& event) {
                eventBroker->publish<T>(event);
            }
            
    };

    /*
        SceneManager, it acts as the central owner of every scenes.
        Handles scene creation and its entire lifecycle.
    */
    class SceneManager{
        public: 
            ~SceneManager() = default;

            // Managers should be unique owners. Prevent copying.
            SceneManager(const SceneManager&) = delete;
            SceneManager& operator=(const SceneManager&) = delete;

            // Creates a scene.
            Scene* createScene(std::string key);

            /* 
                Returns a scene pointer with the specified key/name.
                Returns nullptr if not found.
            */
            Scene* getScene(std::string key);

            // Sets the active scene.
            Scene* setScene(std::string key);

            // Returns the current scene.
            Scene* getCurrentScene(){return currentScene;}

            // Frame lifecycle
            void initScene();
            void updateScene(DeltaTime dt);
            void drawScene(Renderer& renderer);
            void flushSceneDeferredCmds();

            // Serialization in JSON format to file
            bool saveSceneToFile(std::string path, std::string key=""); 

            // Deserialization from a file
            bool loadSceneFromFile(std::string path);

            // For SceneManager's events
            void initEventListeners(EventBroker& broker);
            void processEventRequests();

            bool isPaused() const { return isGamePaused; }

        protected:
            // Protected constructor. Must be created by Engine class.
            SceneManager(){}

            void init();        // Initializes SceneManager
            void terminate();   // SceneManager cleanup

            // Dependency Injection. Setup engine systems for scenes to consume.
            void setEventBroker(EventBroker* broker);
            void setInputManager(InputManager* input) { inputMAN = input; }
            void setAssetsManager(AssetManager* assets) { assetMAN = assets; }
            void setAudioManager(AudioManager* assets) { audioMAN = assets; }
            void setGameCamera(Cam* cam) { sceneCam = cam; }

            friend class Engine;
        private:
            std::unordered_map<std::string, std::unique_ptr<Scene>> scenes; // List of all created scenes.
            std::unordered_map<std::string, ComponentFactoryFunc> registry; // Registry for component deserialization

            float clearGarbageT = 0.0f;         // Timer for periodic memory cleanup.
            
            Scene* currentScene = nullptr;

            // Scene deffered transition state via events
            std::string pendingSceneKey = "";
            std::string pendingLoadPath = "";
            bool shouldSwitchScene = false;
            bool shouldLoadScene = false;

            bool isGamePaused = false;

            // Engine System References
            EventBroker* eventBroker = nullptr;
            InputManager* inputMAN = nullptr;
            AssetManager* assetMAN = nullptr;
            AudioManager* audioMAN = nullptr;
            Cam* sceneCam = nullptr;

            // Registers all built-in component types in pikoBox
            void initRegistry();
        
        public:
            // Registers a custom component type. Curcial for deserialization
            template<typename T>
            void registerComponentType(const std::string& className) {
                registry[className] = []() -> Component* {
                        return new T(); 
                };
            }


    };
}
