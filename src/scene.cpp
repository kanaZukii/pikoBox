#include "piko/scene.hpp"
#include "piko/renderer.hpp"
#include "piko/cam.hpp"
#include "piko/logger.hpp"

#include "raylib.h"
#include "json.hpp"

#include <fstream>

using json = nlohmann::json;
using namespace piko;

uint32_t Scene::entityNextID = 1;
std::vector<uint32_t> Scene::entityDiscardedIDs = {};

void Scene::init(){
    sceneCam->setPosition(initCamPos);
    sceneCam->setZoom(initCamZoom);

    for (std::unique_ptr<Component> &c : components) {
        if (c) { c->init(); }
    }
    PBOX_INFO("SCENE [%s]: Initialized all scene entities and components.", name.c_str());
}

void Scene::terminate(){
    unsubscribeListeners();

    deferredCommandBuffer.clear();
    postLoadJobs.clear();

    for (auto& comp : components) {
        if (comp) {
            comp->terminate();
        }
    }
    
    PBOX_INFO("SCENE [%s]: Structural cleanup complete.", name.c_str());
}

void Scene::update(float dt) {
    for(Script* s: scripts){
        if(s) { 
            if(!s->isUpdating()){continue;}
            s->earlyUpdate(dt);
        }
    }

    for (std::unique_ptr<Component> &c : components) {
        if (c) { 
            if(!c->isUpdating()){continue;}
            c->update(dt); 
        }
    }
}

void Scene::draw(Renderer &renderer) {
    for (Drawable *d : drawables) {
        if (d) { 
            if(!d->isVisible()){continue;}
            d->draw(renderer); 
        }
    }
}

void Scene::flushPostLoadJobs() {
    for (auto& job : postLoadJobs) {
        if (job) job();
    }
    postLoadJobs.clear(); 
}

void Scene::processDeferredCommands() {
    if (deferredCommandBuffer.empty()) return;

    for (auto& cmd : deferredCommandBuffer) {
        if (cmd.execute) {
            cmd.execute();
        }
    }

    deferredCommandBuffer.clear();
}

 void Scene::unsubscribeListeners(){
    for (uint32_t id : subscriptionIDs) {
        eventBroker->unsubscribe(id);
    }
    subscriptionIDs.clear();
}

bool Scene::registerComponent(std::unique_ptr<Component> baseComp, const std::string& entity, std::string key) {
    Entity* e = getEntity(entity);
    if(e == nullptr){
        return false;
    }
    return registerComponent(std::move(baseComp), e->id, key);
}

bool Scene::registerComponent(std::unique_ptr<Component> baseComp, const uint32_t &entity, std::string key) {

    bool isDrawable    = (dynamic_cast<Drawable*>(baseComp.get()) != nullptr);
    bool isCollidable  = (dynamic_cast<Collidable*>(baseComp.get()) != nullptr);
    bool isPhysicsBody = (dynamic_cast<PhysicsBody*>(baseComp.get()) != nullptr);
    bool isScript      = (dynamic_cast<Script*>(baseComp.get()) != nullptr);

    Entity* e = getEntity(entity);
    if(e == nullptr){
        return false;
    }
  
    std::unordered_map<std::string, uint32_t>& e_clist = e->components;
    auto it = e_clist.find(key);
    if (it != e_clist.end()) {
        return false;
    }

    baseComp->owner = e;
    baseComp->ownerID = entity;
    baseComp->alias = key;

    uint32_t c_id = baseComp->getID();
    int c_index = components.size();

    e_clist[key] = c_id;
    componentByIDs[c_id] = c_index;

    if (isDrawable) {
        Drawable *drawPtr = static_cast<Drawable *>(baseComp.get());

        Drawable *lastValidDrawable = nullptr;
        for (int i = (int)drawables.size() - 1; i >= 0; --i) {
            if (drawables[i] != nullptr) {
                lastValidDrawable = drawables[i];
                break;
            }
        }

        if (lastValidDrawable == nullptr || entity >= lastValidDrawable->ownerID) {
            drawables.push_back(drawPtr);
        } else {
            auto insertPos =
                std::lower_bound(drawables.begin(), drawables.end(), entity,
                                [](const Drawable *d, uint32_t id) {
                                    if (d == nullptr)
                                    return false;
                                    return d->ownerID < id;
                                });
            drawables.insert(insertPos, drawPtr);
        }

        drawableIDs.insert(c_id);
    }
    
    if (isCollidable){
        Collidable* collPtr = static_cast<Collidable*>(baseComp.get());
        collidables.push_back(collPtr); 
        collidableIDs.insert(c_id);
    }
    
    if (isPhysicsBody){
        PhysicsBody* pbodyPtr = static_cast<PhysicsBody*>(baseComp.get());
        physicsBodies.push_back(pbodyPtr);
        physicsBodyIDs.insert(c_id);
    }

    if(isScript){
        Script* scptPtr = static_cast<Script*>(baseComp.get());
        scripts.push_back(scptPtr);
        scriptIDs.insert(c_id);
    }
    
    components.push_back(std::move(baseComp));

    return true;
}

void Entity::setDirtyTransform(){
    if(scene){
        scene->setDirtyTransform(id);
    }
}

Entity* Scene::createEntity(std::string key) {
    if (entityByAlias.find(key) != entityByAlias.end()) {
        PBOX_ERROR("SCENE [%s]: Cannot create Entity. Key '%s' is already taken.", name.c_str(), key.c_str());
        return nullptr;
    }

    auto ent = std::make_unique<Entity>();

    Entity *ptr = ent.get();
    ptr->alias = key;
    ptr->sceneName = name;
    ptr->scene = this;

    if (!entityDiscardedIDs.empty()) {
        ptr->id = entityDiscardedIDs.at(entityDiscardedIDs.size() - 1);
        entityDiscardedIDs.pop_back();
    } else {
        ptr->id = entityNextID++;
    }

    entityByIDs[ptr->id] = entities.size();
    entityByAlias[key] = entities.size();

    entities.push_back(std::move(ent));

    return ptr;
}

Entity *Scene::getEntity(std::string key) {
    auto it = entityByAlias.find(key);
    if (it != entityByAlias.end()) { return entities[it->second].get(); }
    PBOX_ERROR("SCENE [%s]: Cannot find Entity with key '%s'.", name.c_str(), key.c_str());
    return nullptr;
}

Entity *Scene::getEntity(uint32_t id) {
    auto it = entityByIDs.find(id);
    if (it != entityByIDs.end()) { return entities[it->second].get(); }
    PBOX_ERROR("SCENE [%s]: Cannot find Entity with ID '%d'.", name.c_str(), id);
    return nullptr;
}

bool Scene::removeComponent(const uint32_t &id) {
    auto c_it = componentByIDs.find(id);
    if (c_it == componentByIDs.end()) {
        return false;
    }

    int index = c_it->second;
    Component *ptr = components.at(index).get();
    if (!ptr) return false;

    std::unordered_map<std::string, uint32_t>& e_clist = getEntity(ptr->getOwnerID())->components;

    for (const auto &pair : e_clist) {
        if (pair.second == id) {
            e_clist.erase(pair.first);
            break;
        }
    }

    if (drawableIDs.count(ptr->getID())) {
        for (int i = 0; i < drawables.size(); ++i) {
            if (drawables.at(i) == ptr) {
                drawables[i] = nullptr;
                break;
            }
        }
        drawableIDs.erase(ptr->getID());
    }

    if(collidableIDs.count(ptr->getID())){
        for(size_t i = 0; i < collidables.size(); ++i){
            if(collidables.at(i) == ptr){
                collidables[i] = nullptr;
                break;
            }
        }
        collidableIDs.erase(ptr->getID());
    }

    if(physicsBodyIDs.count(ptr->getID())){
        for(size_t i = 0; i < physicsBodies.size(); ++i){
            if(physicsBodies.at(i) == ptr){
                physicsBodies[i] = nullptr;
                break;
            }
        }
        physicsBodyIDs.erase(ptr->getID());
    }

    if(scriptIDs.count(ptr->getID())){
        for(size_t i = 0; i < scripts.size(); ++i){
            if(scripts.at(i) == ptr){
                scripts[i] = nullptr;
                break;
            }
        }
        scriptIDs.erase(ptr->getID());
    }

    ptr->terminate();
    componentByIDs.erase(c_it);

    components[index] = nullptr;
    removedComps++;
    return true;
}

bool Scene::removeComponent(const uint32_t &entity, const std::string &comp) {
    Entity *e = getEntity(entity);
    if (e == nullptr) {
        return false;
    }

    std::unordered_map<std::string, uint32_t>& e_clist = e->components;
    auto c_it = e_clist.find(comp);
    if (c_it == e_clist.end()) {
        return false;
    }

    return removeComponent(c_it->second);
}

bool Scene::removeEntity(const uint32_t &id) {
    auto id_it = entityByIDs.find(id);
    if (id_it == entityByIDs.end()) {
        return false;
    }

    int index = id_it->second;
    Entity *e = entities[index].get();
    if (e == nullptr) {
        return false;
    }

    e->scene = nullptr;
    e->sceneName = "";

    std::unordered_map<std::string, uint32_t> e_clist = e->components;
    for(auto& [key, val] : e_clist){
        removeComponent(val);
    }

    entityByAlias.erase(e->alias);
    entityByIDs.erase(id_it);
    entityDiscardedIDs.push_back(id);

    entities[index] = nullptr;
    removedEnts++;

    return true;
}

bool Scene::removeEntity(const std::string &key) {
    auto id_it = entityByAlias.find(key);
    if (id_it == entityByAlias.end()) {
        return false;
    }

    int index = id_it->second;
    Entity* e = entities[index].get();
    if (e == nullptr) {
        return false;
    }

    return removeEntity(e->id);
}

void Scene::safeCreateEntity(const std::string& key, std::function<void(Entity*)> initCallback) {
    deferredCommandBuffer.push_back({
        DeferredCommand::TYPE::CREATE_ENTITY,
        [this, key, initCallback]() { 
            Entity* ptr = this->createEntity(key); 
            if(ptr && initCallback){
                initCallback(ptr);
            }
        }
    });
}

void Scene::safeRemoveEntity(uint32_t id) {
    deferredCommandBuffer.push_back({
        DeferredCommand::TYPE::REMOVE_ENTITY,
        [this, id]() { this->removeEntity(id); }
    });
}

void Scene::safeRemoveEntity(const std::string& key) {
    deferredCommandBuffer.push_back({
        DeferredCommand::TYPE::REMOVE_ENTITY,
        [this, key]() { this->removeEntity(key); }
    });
}

void Scene::safeRemoveComponent(uint32_t componentId) {
    deferredCommandBuffer.push_back({
        DeferredCommand::TYPE::REMOVE_COMPONENT,
        [this, componentId]() { this->removeComponent(componentId); }
    });
}

void Scene::safeRemoveComponent(uint32_t entityId, const std::string& compKey) {
    deferredCommandBuffer.push_back({
        DeferredCommand::TYPE::REMOVE_COMPONENT,
        [this, entityId, compKey]() { this->removeComponent(entityId, compKey); }
    });
}

void Scene::setDirtyTransform(const uint32_t &entity) {
    Entity *e = getEntity(entity);
    if (e == nullptr) {
        return;
    }

    std::unordered_map<std::string, uint32_t> &e_clist = e->components;
    if(!e_clist.empty()){
        for (const auto &[key, val] : e_clist) {
            auto index = componentByIDs.find(val);

            if(index != componentByIDs.end()) {
                components[index->second].get()->invalidateTransform();
            }
        }
    }
}

const std::vector<std::string> Scene::getEntityNames() const{
    std::vector<Entity*> ents;
   
    for (const auto& entityPtr : entities) {
        if (entityPtr) {
            ents.push_back(entityPtr.get());
        }
    }
    std::stable_sort(ents.begin(), ents.end(), [](const Entity* a, const Entity* b) {
        return a->id < b->id;
    });

    std::vector<std::string> names;
    names.reserve(ents.size());

     for (const auto& e : ents) {
        if (e) {
            names.push_back(e->alias);
        }
    }
    
    return names;
}

std::string Scene::serializeEntity(const uint32_t& entity){
    Entity* e = getEntity(entity);
    if (e == nullptr) {
        return "";
    }
    
    std::unordered_map<std::string, uint32_t> &e_clist = e->components;
    json serializedComps = json::object(); // Force empty dictionary format if no components exist
    
    for (const auto& [key, val] : e_clist) {
        auto index = componentByIDs.find(val);

        if(index != componentByIDs.end()) {
            // Guard: Ensure the index is inside the boundaries of our vector array
            if (index->second < components.size()) {
                // Pull out the raw pointer safely from the unique_ptr array slot
                Component* ptr = components[index->second].get();
                
                if (ptr != nullptr) {
                    // PBOX_INFO("SCENE [%s]: Serializing Entity '%s': '%s' at %p", 
                    //     name.c_str(), 
                    //     e->alias.c_str(), 
                    //     key.c_str(), 
                    //     (void*)ptr
                    // );

                    serializedComps[key] = json::parse(ptr->serialize());
                }
            }
        }
    }

    json data = {
        {"id", e->id},
        {"transform", {
            {"x", e->transform.x},
            {"y", e->transform.y},
            {"w", e->transform.w},
            {"h", e->transform.h}
        }},
        {"components", serializedComps}
    };
    return data.dump();
}

void Scene::deserializeEntity(const uint32_t& entityId, const std::string& rawJson) {
    json entJson = json::parse(rawJson);
    
    Entity* e = getEntity(entityId);
    if (e == nullptr) return;

    // 1. Hydrate structural scene property states
    if (entJson.contains("transform") && entJson["transform"].is_object()) {
        auto& tJson = entJson["transform"];
        e->transform.x = tJson.value("x", 0.0f);
        e->transform.y = tJson.value("y", 0.0f);
        e->transform.w = tJson.value("w", 0.0f);
        e->transform.h = tJson.value("h", 0.0f);
    }

    // 2. Extract and Sort components by their embedded ID tokens
    if (entJson.contains("components") && entJson["components"].is_object()) {
        
        // Struct to hold component data alongside its mapping configuration
        struct ComponentLoadTask {
            std::string key;
            uint32_t originalId;
            json data;
        };
        
        std::vector<ComponentLoadTask> loadQueue;
        
        for (const auto& [compKey, compData] : entJson["components"].items()) {
            uint32_t cId = compData.value("id", 1);
            loadQueue.push_back({ compKey, cId, compData });
        }

        // FORCE LOAD ORDER MATCHING: Sort by the component's original runtime ID!
        std::sort(loadQueue.begin(), loadQueue.end(), [](const ComponentLoadTask& a, const ComponentLoadTask& b) {
            return a.originalId < b.originalId;
        });

        // 3. Sequential Factory Allocation and Registration Pass
        for (const auto& task : loadQueue) {
            std::string typeTag = task.data.value("className", "Component");

            if (this->compRegistry == nullptr) {
                PBOX_ERROR("SCENE [%s]: Deserialization failed. CompRegistry reference is missing.", name.c_str());
                continue;
            }

            auto it = this->compRegistry->find(typeTag);
            if (it == this->compRegistry->end()) {
                PBOX_ERROR("SCENE [%s]: Deserialization of type '%s' faild. Type not registered.", name.c_str(), typeTag.c_str());
                continue;
            }

            // Execute the creation lambda factory hook from your localized pointer registry
            Component* rawComp = it->second();
            if (rawComp) {
                std::unique_ptr<Component> compPtr(rawComp);
                Component* targetComponent = compPtr.get();

                if (registerComponent(std::move(compPtr), entityId, task.key)) {
                    targetComponent->deserialize(task.data.dump());
                } else {
                    PBOX_WARN("SCENE [%s]: Deserialization invalid assignment for component '%s'. Component is discarded.", name.c_str(), task.key.c_str());
                }
            }
        }
    }
}

std::string Scene::serialize(){
    json data = {
        {"name", name},
        {"camera", {
            {"x", initCamPos.x},
            {"y", initCamPos.y},
            {"zoom", initCamZoom}
        }}
    };
    json serializedEntities = json::object(); // Explicitly force this to be a key-value dictionary mapping
    
    for (const auto &[key, val] : entityByIDs) {
        Entity* e = getEntity(key);
        if (e == nullptr) continue;

        std::string entityRawStr = serializeEntity(key);
        if (entityRawStr.empty()) continue;

        // Use a safe backup fallback string if the alias property is blank
        std::string entityKey = e->alias.empty() ? "Entity_" + std::to_string(e->id) : e->alias;
        
        // Parse the entity data and map it under its human-readable string key
        serializedEntities[entityKey] = json::parse(entityRawStr);
    }
    
    data["entities"] = serializedEntities;
    return data.dump(4);
}

void Scene::deserialize(const std::string& rawJson) {
    json sceneData = json::parse(rawJson);

    if (sceneData.contains("camera") && sceneData["camera"].is_object()){
        auto camJson = sceneData["camera"];
        initCamPos.x = camJson.value("x", 0.0f);
        initCamPos.y = camJson.value("y", 0.0f);
        initCamZoom = camJson.value("zoom", 1.0f);
    }

    if (!sceneData.contains("entities") || !sceneData["entities"].is_object()) return;

    auto& entitiesJson = sceneData["entities"];

    // LAYER PRE-SORT BUFFER
    struct SortedEntityNode {
        std::string alias;
        uint32_t originalId;
        json data;
    };
    std::vector<SortedEntityNode> sortBuffer;

    for (const auto& [entityAlias, entityData] : entitiesJson.items()) {
        uint32_t originalId = entityData.value("id", 1);
        sortBuffer.push_back({entityAlias, originalId, entityData});
    }

    std::sort(sortBuffer.begin(), sortBuffer.end(), [](const SortedEntityNode& a, const SortedEntityNode& b) {
        return a.originalId < b.originalId;
    });

    for (const auto& node : sortBuffer) {
        Entity* newEntity = createEntity(node.alias);
        
        if (newEntity != nullptr) {
            deserializeEntity(newEntity->id, node.data.dump());
        }
    }

    flushPostLoadJobs(); 
}

void Scene::collectGarbageE() {
    if (removedEnts == 0) return;

    for (size_t i = 0; i < entities.size();) {
        if (entities[i] == nullptr) {
            if (entities.back() == nullptr) {
                entities.pop_back();
                removedEnts--;
                continue;
            }

            if (i != entities.size() - 1) {
                entities[i] = std::move(entities.back());
                entityByIDs[entities[i]->id] = i;
                entityByAlias[entities[i]->alias] = i;
            }
            entities.pop_back();
            removedEnts--;
        } else {
            i++;
        }
    }
}

void Scene::collectGarbageC() {
    if (removedComps == 0) return;

    for (size_t i = 0; i < components.size();) {
        if (components[i] == nullptr) {
            if (components.back() == nullptr) {
                components.pop_back();
                removedComps--;
                continue; 
            }

            if (i != components.size() - 1) {
                components[i] = std::move(components.back());
                componentByIDs[components[i]->getID()] = i;
            }
            components.pop_back();
            removedComps--;
        } else {
            i++;
        }
    }

    size_t writeDrawIdx = 0;
    for (size_t readIndex = 0; readIndex < drawables.size(); ++readIndex) {
        if (drawables[readIndex] != nullptr) {
            drawables[writeDrawIdx++] = drawables[readIndex];
        }
    }
    drawables.resize(writeDrawIdx);

    size_t writeCollIdx = 0;
    for (size_t readIndex = 0; readIndex < collidables.size(); ++readIndex) {
        if (collidables[readIndex] != nullptr) {
            collidables[writeCollIdx++] = collidables[readIndex];
        }
    }
    collidables.resize(writeCollIdx);

    size_t writePbodIdx = 0;
    for(size_t readIndex = 0; readIndex < physicsBodies.size(); ++readIndex){
        if (physicsBodies[readIndex] != nullptr) {
            physicsBodies[writePbodIdx++] = physicsBodies[readIndex];
        }
    }
    physicsBodies.resize(writePbodIdx);

    size_t writeScptIdx = 0;
    for(size_t readIndex = 0; readIndex < scripts.size(); ++readIndex){
        if (scripts[readIndex] != nullptr) {
            scripts[writeScptIdx++] = scripts[readIndex];
        }
    }
    scripts.resize(writeScptIdx);
}

void SceneManager::terminate(){
    for (auto& [key, scenePtr] : scenes) {
        scenePtr->terminate();
    }

    scenes.clear();
    registry.clear();
}

void SceneManager::init(){
    initRegistry();
    PBOX_INFO("SCENE_MAN: Scene Manager Initialized.");
    PBOX_INFO("SCENE_MAN: Registered all built-in component types.");
}

void SceneManager::initRegistry() {
    registerComponentType<Component>("Component");
    registerComponentType<Drawable>("Drawable");
    registerComponentType<Collidable>("Collidable");
    registerComponentType<Script>("Script");
    registerComponentType<PhysicsBody>("PhysicsBody");
    registerComponentType<AnimationPlayer>("AnimationPlayer");
    registerComponentType<AudioPlayer>("AudioPlayer");
    registerComponentType<SpriteRenderer>("SpriteRenderer");
    registerComponentType<TextRenderer>("TextRenderer");
    registerComponentType<TextBoxRenderer>("TextBoxRenderer");
    registerComponentType<UIPanel>("UIPanel");
    registerComponentType<BoxCollider>("BoxCollider");
    registerComponentType<CollisionResScript>("CollisionResScript");
    registerComponentType<PlayerMoveScript>("PlayerMoveScript");
    registerComponentType<CameraMoveScript>("CameraMoveScript");
    registerComponentType<ButtonScript>("ButtonScript");
}

Scene *SceneManager::createScene(std::string key) {
    auto it = scenes.find(key);
    if (it != scenes.end()) {
        PBOX_WARN("SCENE_MAN: Cannot create new scene '%s'. It already exist.", key.c_str());
        return it->second.get();
    }

    auto scene = std::unique_ptr<Scene>(new Scene());
    scene->setInputManager(inputMAN);
    scene->setAssetsManager(assetMAN);
    scene->setAudioManager(audioMAN);
    scene->setCamera(sceneCam);
    scene->setEventBroker(eventBroker);

    Scene *ptr = scene.get();
    ptr->name = key;
    ptr->compRegistry = &registry;

    scenes[key] = std::move(scene);

    if (!currentScene) {
        currentScene = ptr;
    }

    return ptr;
}

 void SceneManager::setEventBroker(EventBroker* broker) { 
    eventBroker = broker; 
    if(eventBroker){
        initEventListeners(*eventBroker);
    }
}

Scene* SceneManager::getScene(std::string key) {
    if (key.empty() || scenes.find(key) == scenes.end()) {
        PBOX_ERROR("SCENE_MAN: Cannot find the scene '%s'.", key.c_str());
        return nullptr;
    } else {
        return scenes[key].get();
    }
}

Scene* SceneManager::setScene(std::string key) {
  if (key.empty() || scenes.find(key) == scenes.end()) {
    PBOX_ERROR("SCENE_MAN: Cannot find the scene '%s'.", key.c_str());
    return nullptr;
  }

  if(currentScene){
     if(currentScene->name == key) return currentScene;
  }

  currentScene = scenes[key].get();
  return currentScene;
}

void SceneManager::initScene() {
    if (currentScene) currentScene->init();
}

void SceneManager::updateScene(DeltaTime dt) {
    if (currentScene) {
        clearGarbageT += dt.raw;

        if (clearGarbageT >= 1.0f) {
            clearGarbageT = 0.0f;
            currentScene->collectGarbageC();
            currentScene->collectGarbageE();
        }

        currentScene->update(dt.physics);
    }
    
}

void SceneManager::drawScene(Renderer &renderer) {
    if (currentScene) currentScene->draw(renderer);
}

void SceneManager::flushSceneDeferredCmds(){
    if (currentScene) currentScene->processDeferredCommands();
}

void SceneManager::initEventListeners(EventBroker& broker) {
    // 1. Listen for standard registered scene hops
    broker.subscribe<SceneChangeEvent>([this](const SceneChangeEvent& e) {
        this->pendingSceneKey = e.targetSceneKey;
        this->shouldSwitchScene = true;
    });

    // 2. Listen for external file loading requests
    broker.subscribe<SceneLoadEvent>([this](const SceneLoadEvent& e) {
        this->pendingLoadPath = e.filePath;
        this->shouldLoadScene = true;
    });

    // 3. Listen for immediate disk saving requests (Safe to run mid-frame as it only reads data)
    broker.subscribe<SceneSaveEvent>([this](const SceneSaveEvent& e) {
        this->saveSceneToFile(e.filePath);
    });

    // 4. Listen for pause state triggers
    broker.subscribe<EnginePauseEvent>([this](const EnginePauseEvent& e) {
        this->isGamePaused = e.pauseState;
    });
}

void SceneManager::processEventRequests() {
    if (shouldLoadScene) {
        shouldLoadScene = false;
        this->loadSceneFromFile(pendingLoadPath);
        pendingLoadPath = "";
    }

    if (shouldSwitchScene) {
        shouldSwitchScene = false;
        this->setScene(pendingSceneKey);
        this->initScene();
        pendingSceneKey = "";
    }
}

bool SceneManager::saveSceneToFile(std::string path, std::string key){
    if(path.empty()) return false;
    if(!currentScene && key.empty()) return false;
    try {
        // 1. Parse the raw string into a structured nlohmann json object
        json sceneData;
        if(key.empty()){
            sceneData = json::parse(currentScene->serialize());
        }else {
            auto sc = scenes.find(key);
            if(sc == scenes.end()){
                PBOX_ERROR("SCENE_MAN: Serialization failed. Scene '%s' does not exist.", key.c_str());
                return false;
            }

            sceneData = json::parse(sc->second.get()->serialize());
        }

        // 2. Open an output file stream to your destination path
        std::ofstream output_file(path);
        
        // Check if the file opened successfully (e.g., directory exists, permissions are good)
        if (!output_file.is_open()) {
            PBOX_ERROR("SCENE_MAN: Serialization failed. Could not open or create file at '%s'", path.c_str());
            return false;
        }

        // 3. Dump the structured JSON directly into the file stream
        // Passing '4' as an argument turns on beautiful auto-indentation spacing!
        output_file << sceneData.dump(4);

        // 4. Close the stream (or let the destructor handle it when it goes out of scope)
        output_file.close();
        PBOX_INFO("SCENE_MAN: Successfully saved scene state to '%s'", path.c_str());
        return true;

    } catch (const json::parse_error& e) {
        // Catch any syntax errors in the incoming string (like a missing comma or brace)
        PBOX_ERROR("SCENE_MAN: Serialization failed. JSON Parsing Failed '%s'", e.what());
        return false;
    }
}

bool SceneManager::loadSceneFromFile(std::string path) {
    std::ifstream input_file(path);
    
    // Check if the file exists and is accessible
    if (!input_file.is_open()) {
        PBOX_ERROR("SCENE_MAN: Deserialization failed. Could not open file at '%s'", path.c_str());
        return false;
    }

    try {
        // 1. Slurp the file stream straight into a structured JSON block
        json sceneData;
        input_file >> sceneData;
        input_file.close();

        // 2. Safely extract the scene identifier string
        std::string sceneKey = sceneData.value("name", "");
        if (sceneKey.empty()) {
            PBOX_ERROR("SCENE_MAN: Deserialization failed. Missing valid 'name' attribute inside scene JSON.");
            return false;
        }

        std::string targetKeyToDelete = sceneKey;

        // 2. Safely check for active scene collisions
        auto it = scenes.find(targetKeyToDelete);
        if (it != scenes.end()) {
            PBOX_WARN("SCENE_MAN: Scene '%s' already exist and loaded. Overwriting scene...", targetKeyToDelete.c_str());
            
            // Clear the active engine pointer first if it matches the target to break dependencies
            if (currentScene && currentScene->name == targetKeyToDelete) {
                currentScene = nullptr;
            }
            
            // Explicitly erase using the isolated stack copy
            scenes.erase(it); 
        }

        // 4. ROUTE THROUGH YOUR OFFICIAL BUILDER PIPELINE
        // This links the localized component registry, inputs, assets, and audio managers natively!
        Scene* newScene = createScene(sceneKey);
        if (newScene == nullptr) {
            PBOX_ERROR("SCENE_MAN: Deserialization failed. Failed to create scene '%s'.", sceneKey.c_str());
            return false;
        }

        // 5. Fire off the complete structural Two-Pass Deserialization sequence
        newScene->deserialize(sceneData.dump());

        PBOX_INFO("SCENE_MAN: Successfully loaded scene '%s' from file: '%s'", sceneKey.c_str(), path.c_str());
        return true;

    } catch (const json::parse_error& e) {
        PBOX_ERROR("SCENE_MAN: Deserialization failed. Load Parse Error in file '%s': %s", path.c_str(), e.what());
        return false;
    } catch (const std::exception& e) {
        PBOX_ERROR("SCENE_MAN: Deserialization failed. Unexpected structural execution drop during load phase: %s", e.what());
        return false;
    }
}