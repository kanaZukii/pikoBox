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
    
    // Data package describing a collision. Used to resolve overlaps.
    struct CollisionManifold {
        Collidable* colA = nullptr;
        Collidable* colB = nullptr;
        Vect2 normal = {0.0f, 0.0f};    // Direction to push colA out of colB.
        float penetration = 0.0f;       // How deep are colA and colB overlapping with each other.
    };

    // Helper for defining the grid area a collider covers.
    struct GridBounds {
        int minX, maxX;
        int minY, maxY;
    };

    /*
        PhysicsEngine handles collision detection from a list of collidables 
        and minimize checks through a grid based spatial partitioning.
    */
    class PhysicsEngine {
        public:
            ~PhysicsEngine() = default;
            PhysicsEngine(const PhysicsEngine&) = delete;
            PhysicsEngine& operator=(const PhysicsEngine&) = delete;

            // Global simulation constants.
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
            
            /*
                Advances the simulation by 'dt'. Uses 'viewBubble' to perform 
                spatial partitioning only on entities near the active camera.
            */
            void update(float dt, Scene* scene, const Rect& viewBubble);

            friend class Engine;

            // Simulation constant variable
            float gravity = 600.0f;
            float linearDamping = 1.0f;
            float groundFriction = 7.0f;
            
            // Simulation window and boundary properties
            float simPadding = 512.0f;
            float cushionSize = 64.0f; // Set to CELL_SIZE by default
            static constexpr int CELL_SIZE = 64; 

            // Spatial Partitioning State
            Rect activeBounds;
            int gridWidth = 0;
            int gridHeight = 0;

            // Broad-phase structure spatial grid.
            std::vector<std::vector<Collidable*>> flatGridMatrix;

            // Cached checks for deduplication. 
            std::unordered_set<uint64_t> checkedPairs;

            // Helpers for spatial grid mapping.
            int getGridIndex(int x, int y) const { return y * gridWidth + x; }
            uint64_t getPairKey(uint32_t colIdA, uint32_t colIdB) const;
            GridBounds getGridBounds(const Rect& rect) const;
            
            // Physics pipeline
            void checkCollisions(const std::vector<Collidable*>& collidables, Scene* scene);
            bool testAABB(const Rect& a, const Rect& b, CollisionManifold& outManifold);
            void resolveSolidCollision(const CollisionManifold& manifold);
    };
}