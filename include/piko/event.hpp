// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <string>
#include <cstdint>
#include <algorithm>

#include "piko/math.hpp"

namespace piko {
    struct Entity;
    class Component;
    class Collidable;
    class ButtonScript;

    struct Event {
        virtual ~Event() = default;
    };

    struct SceneChangeEvent : public Event {
        std::string targetSceneKey;
        SceneChangeEvent(std::string key) : targetSceneKey(key) {}
    };

    struct SceneSaveEvent : public Event {
        std::string filePath;
        SceneSaveEvent(std::string path) : filePath(path) {}
    };

    struct SceneLoadEvent : public Event {
        std::string filePath;
        SceneLoadEvent(std::string path) : filePath(path) {}
    };

    struct EnginePauseEvent : public Event {
        bool pauseState;
        EnginePauseEvent(bool state) : pauseState(state) {}
    };

    struct CollisionEvent : public Event {
        Collidable* colA;
        Collidable* colB;
        Vect2 normal;
        float penetration;

        CollisionEvent(Collidable* a, Collidable* b, Vect2 norm, float pen)
            : colA(a), colB(b), normal(norm), penetration(pen) {}
    };

    struct ButtonEvent : public Event {
        enum class STATE {CLICK, HOLD, HOVER};

        ButtonScript* btn;
        STATE state;

        ButtonEvent(ButtonScript* btn, STATE state)
            : btn(btn), state(state) {}
    };

    struct GameplayEvent : public Event {
        enum class TYPE {
            READY,
            BEGIN,
            END,
            PAUSE,
            RESUME,
            WIN,
            LOSE,
            SPECIAL
        };

        TYPE type;
        int payload; // Can store score, level index, or error code
        Entity* entity; // Optional pointer

        GameplayEvent(TYPE type, int p = 0, Entity* e = nullptr) 
            : type(type), payload(p), entity(e) {}
    };

    /*
        EventBroker provides a decoupled messaging system.
        Allows components and subsystem to broadcast signals and listeners to react without 
        knowing about each other's existence.
    */
    class EventBroker {
        private:
            struct Subscription {
                uint32_t id;
                std::function<void(const Event&)> handler;
            };

        public:
            EventBroker() = default;
            ~EventBroker() = default;

            EventBroker(const EventBroker&) = delete;
            EventBroker& operator=(const EventBroker&) = delete;

            // Registers a callback for a specific event type. Returns a unique ID for unsubscription.
            template <typename T>
            uint32_t subscribe(std::function<void(const T&)> callback) {
                uint32_t subID = nextSubscriptionID++;
                
                subscribers[typeid(T)].push_back({
                    subID,
                    [callback](const Event& e) {
                        callback(static_cast<const T&>(e));
                    }
                });

                return subID;
            }

            // Removes a subscription using the ID returned by subscribe().
            void unsubscribe(uint32_t subID) {
                if (subID == 0) return;

                for (auto& [type, list] : subscribers) {
                    auto it = std::remove_if(list.begin(), list.end(), 
                        [subID](const Subscription& sub) {
                            return sub.id == subID;
                        });
                    
                    if (it != list.end()) {
                        list.erase(it, list.end());
                        return;
                    }
                }
            }

            /* 
                Use this before subscribing a callback. 
                Not yet implemented in Scene.
            */ 
            bool isSubscribed(uint32_t subID) {
                // Iterate through each event type list
                for (const auto& [type, list] : subscribers) {
                    // Quick check, does this list contain the subID?
                    for (const auto& sub : list) {
                        if (sub.id == subID) return true;
                    }
                }
                return false;
            }

            // Broadcasts an event to all registered listeners of type T.
            template <typename T>
            void publish(const T& event) {
                auto it = subscribers.find(typeid(T));
                if (it != subscribers.end()) {
                    auto handlersCopy = it->second;
                    for (auto& sub : handlersCopy) {
                        sub.handler(event);
                    }
                }
            }

        private:
            uint32_t nextSubscriptionID = 1;
            std::unordered_map<std::type_index, std::vector<Subscription>> subscribers;
    };
}