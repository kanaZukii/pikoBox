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

    using ComponentFactoryFunc = std::function<Component*()>;

    struct Entity{
        uint32_t id = 0;
        std::string alias = "";
        std::string sceneName = "";
        Scene* scene = nullptr;
        
        Rect transform = {0};
        std::unordered_map<std::string, uint32_t> components;
        void setDirtyTransform();
    };

    class Scene{
        public: 
            
            struct DeferredCommand {
                enum class TYPE { CREATE_ENTITY, ADD_COMPONENT, REMOVE_ENTITY, REMOVE_COMPONENT };
                TYPE type;
                std::function<void()> execute;
            };

            ~Scene() = default;

            const std::string& getName() const {return name;}
            void setInitCamPosition(Vect2 pos){initCamPos = pos;}
            void setInitCamZoom(float zoom){initCamZoom = zoom;}

            Entity* createEntity(std::string key);

            Entity* getEntity(uint32_t id);
            Entity* getEntity(std::string key);
           
            bool removeEntity(const uint32_t& id);
            bool removeEntity(const std::string& key);

            bool removeComponent(const uint32_t& id);
            bool removeComponent(const uint32_t& entity, const std::string& comp);

            void safeCreateEntity(const std::string& key, std::function<void(Entity*)> initCallback = nullptr);
 
            void safeRemoveEntity(uint32_t id);
            void safeRemoveEntity(const std::string& key);

            void safeRemoveComponent(uint32_t componentId);
            void safeRemoveComponent(uint32_t entityId, const std::string& compKey);

            void unsubscribeListeners();

            void setDirtyTransform(const uint32_t &entity);

            std::string serializeEntity(const uint32_t& entity); 
            void deserializeEntity(const uint32_t& entity, const std::string& rawJson);

            std::string serialize(); 
            void deserialize(const std::string& rawJson);

            void addPostLoadJob(std::function<void()> job) {postLoadJobs.push_back(job);}

            InputManager* getInput() const { return inputMAN; }
            AssetManager* getAssets() const { return assetMAN; }
            AudioManager* getAudio() const { return audioMAN; }
            Cam* getCamera() const { return sceneCam; }

            const std::vector<Collidable*>& getCollidables() const { return collidables; }
            const std::vector<PhysicsBody*>& getPhysicsBodies() const { return physicsBodies; }
            const std::vector<Drawable*>& getDrawables() const { return drawables; }
            const std::vector<Script*>& getScripts() const { return scripts; }

            const std::vector<std::string> getEntityNames() const;

        protected:
            Scene(){}

            void init();
            void update(float dt);
            void draw(Renderer& renderer);
            void terminate();
        
            void flushPostLoadJobs();
            void processDeferredCommands(); 
            
            bool registerComponent(std::unique_ptr<Component> baseComp, const uint32_t &entity, std::string key);
            bool registerComponent(std::unique_ptr<Component> baseComp, const std::string& entity, std::string key);

            void setEventBroker(EventBroker* broker) { eventBroker = broker; }
            void setInputManager(InputManager* input) { inputMAN = input; }
            void setAssetsManager(AssetManager* assets) { assetMAN = assets; }
            void setAudioManager(AudioManager* audio) { audioMAN = audio; }
            void setGameCamera(Cam* cam) { sceneCam = cam; }
            
            std::string name = "Scene";
            Vect2 initCamPos = {0.0f, 0.0f};
            float initCamZoom = 1.0f;
            
            friend class SceneManager;

        private:
            static uint32_t entityNextID;
            static std::vector<uint32_t> entityDiscardedIDs;

            std::vector<std::unique_ptr<Entity>> entities;
            std::unordered_map<uint32_t, int> entityByIDs;
            std::unordered_map<std::string, int> entityByAlias;

            std::vector<std::unique_ptr<Component>> components;
            std::unordered_map<uint32_t, int> componentByIDs;

            std::vector<Drawable*> drawables;
            std::unordered_set<uint32_t> drawableIDs;

            std::vector<Collidable*> collidables;
            std::unordered_set<uint32_t> collidableIDs;

            std::vector<Script*> scripts;
            std::unordered_set<uint32_t> scriptIDs;

            std::vector<PhysicsBody*> physicsBodies;
            std::unordered_set<uint32_t> physicsBodyIDs;
            std::unordered_map<std::string, ComponentFactoryFunc>* compRegistry = nullptr;

            std::vector<std::function<void()>> postLoadJobs;
            std::vector<DeferredCommand> deferredCommandBuffer;

            std::vector<uint32_t> subscriptionIDs;

            uint32_t removedComps = 0;
            uint32_t removedEnts = 0;
            
            void collectGarbageE();
            void collectGarbageC();

            EventBroker* eventBroker = nullptr;
            InputManager* inputMAN = nullptr;
            AssetManager* assetMAN = nullptr;
            AudioManager* audioMAN = nullptr;
            Cam* sceneCam = nullptr;
        
        public:

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

            inline bool entityExist(std::string key) const {
                return entityByAlias.count(key) > 0;
            }

            inline bool entityExist(uint32_t id) const {
                return entityByIDs.count(id) > 0;
            }

            inline bool componentExist(uint32_t id) const {
                return componentByIDs.count(id) > 0;
            }

            template <typename T>
            inline void listenToEvent(std::function<void(const T&)> callback) {
                uint32_t id = eventBroker->subscribe<T>(callback);
                subscriptionIDs.push_back(id);
            }

            template <typename T>
            inline void publishEvent(const T& event) {
                eventBroker->publish<T>(event);
            }
            
    };


    class SceneManager{
        public: 
            ~SceneManager() = default;
            SceneManager(const SceneManager&) = delete;
            SceneManager& operator=(const SceneManager&) = delete;

            Scene* createScene(std::string key);
            Scene* getScene(std::string key);
            
            Scene* setScene(std::string key);
            Scene* getCurrentScene(){return currentScene;}

            void initScene();
            void updateScene(DeltaTime dt);
            void drawScene(Renderer& renderer);
            void flushSceneDeferredCmds();

            bool saveSceneToFile(std::string path, std::string key=""); 
            bool loadSceneFromFile(std::string path);

            void initEventListeners(EventBroker& broker);
            void processEventRequests();

            template<typename T>
            void registerComponentType(const std::string& className) {
                registry[className] = []() -> Component* {
                        return new T(); 
                };
            }

            bool isPaused() const { return isGamePaused; }

        private:
            SceneManager(){}

            void init();
            void terminate();

            void initRegistry();

            void setEventBroker(EventBroker* broker);
            void setInputManager(InputManager* input) { inputMAN = input; }
            void setAssetsManager(AssetManager* assets) { assetMAN = assets; }
            void setAudioManager(AudioManager* assets) { audioMAN = assets; }
            void setGameCamera(Cam* cam) { sceneCam = cam; }

            std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
            std::unordered_map<std::string, ComponentFactoryFunc> registry;

            float clearGarbageT = 0.0f;
            
            Scene* currentScene = nullptr;

            std::string pendingSceneKey = "";
            std::string pendingLoadPath = "";
            bool shouldSwitchScene = false;
            bool shouldLoadScene = false;

            // Time Control state
            bool isGamePaused = false;

            EventBroker* eventBroker = nullptr;
            InputManager* inputMAN = nullptr;
            AssetManager* assetMAN = nullptr;
            AudioManager* audioMAN = nullptr;
            Cam* sceneCam = nullptr;

            friend class Engine;
    };
}
