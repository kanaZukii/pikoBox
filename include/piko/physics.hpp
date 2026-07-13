// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once

#include "piko/time.hpp"
#include "piko/math.hpp"

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace piko {
    class Engine;
    class Scene;
    class Collidable;
    class PhysicsBody;
    struct Entity;
    
    struct CollisionManifold {
        Collidable* colA = nullptr;
        Collidable* colB = nullptr;
        Vect2 normal = {0.0f, 0.0f};
        float penetration = 0.0f;
    };

    struct GridBounds {
        int minX, maxX;
        int minY, maxY;
    };

    class PhysicsEngine {
        public:
            ~PhysicsEngine() = default;
            PhysicsEngine(const PhysicsEngine&) = delete;
            PhysicsEngine& operator=(const PhysicsEngine&) = delete;

            void update(float dt, Scene* scene, const Rect& viewBubble);

            void setGravity(float g) { gravity = g; }
            void setLinearDamping(float ld) { linearDamping = ld; }
            void setGroundFriction(float gf) { groundFriction = gf; }
            
            // Configuration for the simulation envelope thresholds
            void setSimPadding(float padding) { simPadding = padding; }
            void setCushionSize(float cushion) { cushionSize = cushion; }

        private:
            PhysicsEngine(){}
            void init();
            void terminate(){}

            friend class Engine;

            float gravity = 600.0f;
            float linearDamping = 1.0f;
            float groundFriction = 7.0f;
            
            // Customizable simulation window and boundary properties
            float simPadding = 512.0f;
            float cushionSize = 64.0f; // Set to CELL_SIZE by default
            static constexpr int CELL_SIZE = 64; 

            // Camera bounds padded out to act as an active simulation envelope
            Rect activeBounds;
            int gridWidth = 0;
            int gridHeight = 0;

            std::vector<std::vector<Collidable*>> flatGridMatrix;
            std::unordered_set<uint64_t> checkedPairs;

            int getGridIndex(int x, int y) const {
                return y * gridWidth + x;
            }

            uint64_t getPairKey(uint32_t colIdA, uint32_t colIdB) const;
            GridBounds getGridBounds(const Rect& rect) const;
            
            void checkCollisions(const std::vector<Collidable*>& collidables, Scene* scene);
            bool testAABB(const Rect& a, const Rect& b, CollisionManifold& outManifold);
            void resolveSolidCollision(const CollisionManifold& manifold);
    };
}