#include "piko/physics.hpp"
#include "piko/component.hpp"
#include "piko/scene.hpp"
#include "piko/event.hpp"
#include "piko/logger.hpp"

#include <algorithm>
#include <cmath>

using namespace piko;

void PhysicsEngine::init() {
    flatGridMatrix.resize(2048);
    checkedPairs.reserve(2048); 
    PBOX_INFO("PHYSICS: Physics Engine Initialized.");
}

void PhysicsEngine::update(float dt, Scene* scene, const Rect& viewBubble) {
    if (!scene) return;

    const auto& collidables = scene->getCollidables();
    if (collidables.empty()) return;

    // 1. Pad out the simulation window bounds using configurable padding
    activeBounds.x = viewBubble.x - simPadding;
    activeBounds.y = viewBubble.y - simPadding;
    activeBounds.w = viewBubble.w + (simPadding * 2.0f);
    activeBounds.h = viewBubble.h + (simPadding * 2.0f);

    gridWidth = static_cast<int>(std::ceil(activeBounds.w / CELL_SIZE)) + 1;
    gridHeight = static_cast<int>(std::ceil(activeBounds.h / CELL_SIZE)) + 1;
    
    size_t totalCellsRequired = static_cast<size_t>(gridWidth * gridHeight);
    if (flatGridMatrix.size() < totalCellsRequired) {
        flatGridMatrix.resize(totalCellsRequired);
    }

    for (size_t i = 0; i < totalCellsRequired; ++i) {
        flatGridMatrix[i].clear();
    }

    static std::unordered_set<PhysicsBody*> processedBodies;
    processedBodies.clear();

    // 2. PASS A: Kinematics Integration & Velocity Lockdown Bypass
    for (Collidable* col : collidables) {
        if (!col) continue;

        Rect bounds = col->getGlobalTransform();

        bool isColOutside = (bounds.x + bounds.w < activeBounds.x || bounds.x > activeBounds.x + activeBounds.w ||
                            bounds.y + bounds.h < activeBounds.y || bounds.y > activeBounds.y + activeBounds.h);
        
        col->setCulled(isColOutside);

        PhysicsBody* pbody = col->getPhysicsBody();
        if (!pbody) continue;

        if (processedBodies.insert(pbody).second) {
            Entity& e = pbody->getOwner();

            // Unified out-of-bounds check accounting for the cushion size
            bool isEntOutside = (e.transform.x + e.transform.w < activeBounds.x || 
                                 e.transform.x > activeBounds.x + activeBounds.w ||
                                 e.transform.y + e.transform.h < activeBounds.y || 
                                 e.transform.y > activeBounds.y + activeBounds.h);

            pbody->setCulled(isEntOutside);
            
            if (isEntOutside) {
                pbody->velocity = {0.0f, 0.0f};
                continue; 
            }

            bool inCushion = (e.transform.x < activeBounds.x + cushionSize || 
                          e.transform.x + e.transform.w > activeBounds.x + activeBounds.w - cushionSize ||
                          e.transform.y < activeBounds.y + cushionSize || 
                          e.transform.y + e.transform.h > activeBounds.y + activeBounds.h - cushionSize);

            pbody->isGrounded = false;
            pbody->velocity.y += gravity * pbody->gravityScale * dt;

            float dampFactor = inCushion ? 6.0f : 1.0f;
            pbody->velocity.x -= pbody->velocity.x * (linearDamping * pbody->dragScale * dampFactor) * dt;

            e.transform.x += pbody->velocity.x * dt;
            e.transform.y += pbody->velocity.y * dt;
            e.setDirtyTransform();
        }
    }

    // 3. PASS B: Spatial Grid Injection
    for (Collidable* col : collidables) {
        if (!col) continue;

        Rect bounds = col->getGlobalTransform();

        // Sync grid rejection criteria with our cushion boundaries
        if (bounds.x + bounds.w < activeBounds.x + cushionSize || bounds.x > activeBounds.x + activeBounds.w - cushionSize ||
            bounds.y + bounds.h < activeBounds.y + cushionSize || bounds.y > activeBounds.y + activeBounds.h - cushionSize) {
            continue;
        }

        GridBounds gBounds = getGridBounds(bounds);
        for (int x = gBounds.minX; x <= gBounds.maxX; ++x) {
            for (int y = gBounds.minY; y <= gBounds.maxY; ++y) {
                int cellIndex = getGridIndex(x, y);
                flatGridMatrix[cellIndex].push_back(col);
            }
        }
    }

    // 4. COLLISION PASS: This runs full spatial resolution and resolves overlapping entities
    checkCollisions(collidables, scene);

    // // 5. PASS C: POST-COLLISION FREEZING LAYER
    // for (PhysicsBody* pbody : processedBodies) {
    //     if (!pbody || pbody->isCulled()) continue;

    //     Entity& e = pbody->getOwner();
        
    //     bool isInCushionZone = 
    //         (e.transform.x < activeBounds.x + cushionSize || e.transform.x + e.transform.w > activeBounds.x + activeBounds.w - cushionSize ||
    //          e.transform.y < activeBounds.y + cushionSize || e.transform.y + e.transform.h > activeBounds.y + activeBounds.h - cushionSize);

    //     if (isInCushionZone) {
    //         pbody->velocity.x = 0.0f;
    //         pbody->velocity.y = 0.0f;
    //     }
    // }
}

void PhysicsEngine::checkCollisions(const std::vector<Collidable*>& collidables, Scene* scene) {
    checkedPairs.clear();

    for (Collidable* colA : collidables) {
        if (!colA || colA->isCulled()) continue;

        bool canInitiateCheck = colA->isDynamic() || colA->isTrigger();
        if (!canInitiateCheck) continue;

        Rect boundsA = colA->getGlobalTransform();

        if (boundsA.x + boundsA.w < activeBounds.x || boundsA.x > activeBounds.x + activeBounds.w ||
            boundsA.y + boundsA.h < activeBounds.y || boundsA.y > activeBounds.y + activeBounds.h) {
            continue;
        }

        GridBounds gBounds = getGridBounds(boundsA);

        for (int x = gBounds.minX; x <= gBounds.maxX; ++x) {
            for (int y = gBounds.minY; y <= gBounds.maxY; ++y) {
                int cellIndex = getGridIndex(x, y);
                const auto& cellObjects = flatGridMatrix[cellIndex];

                if (cellObjects.size() < 2) continue;

                for (Collidable* colB : cellObjects) {
                    if (!colB || colA == colB || colB->isCulled()) continue;
                    if (colA->getOwnerID() == colB->getOwnerID()) continue; 
                    if (!colB->isDynamic() && !colA->isDynamic()) continue; 

                    uint64_t pairKey = getPairKey(colA->getID(), colB->getID());
                    if (checkedPairs.count(pairKey)) continue;
                    checkedPairs.insert(pairKey);

                    Rect liveBoundsA = colA->getGlobalTransform();
                    Rect liveBoundsB = colB->getGlobalTransform();

                    CollisionManifold manifold;
                    if (testAABB(liveBoundsA, liveBoundsB, manifold)) {
                        manifold.colA = colA;
                        manifold.colB = colB;
                        
                        if (colA->isTrigger() || colB->isTrigger()) {
                            scene->publishEvent<CollisionEvent>(
                                CollisionEvent(colA, colB, manifold.normal, manifold.penetration)
                            ); 
                        } 
                        else {
                            resolveSolidCollision(manifold);
                        }
                    }
                }
            }
        }
    }
}

bool PhysicsEngine::testAABB(const Rect& a, const Rect& b, CollisionManifold& outManifold) {
    float halfW_A = a.w * 0.5f;
    float halfH_A = a.h * 0.5f;
    float halfW_B = b.w * 0.5f;
    float halfH_B = b.h * 0.5f;

    float nX = (b.x + halfW_B) - (a.x + halfW_A);
    float overlapX = halfW_A + halfW_B - std::abs(nX);
    if (overlapX <= P_EPSILON) return false;

    float nY = (b.y + halfH_B) - (a.y + halfH_A);
    float overlapY = halfH_A + halfH_B - std::abs(nY);
    if (overlapY <= P_EPSILON) return false;

    if (overlapX < overlapY) {
        outManifold.penetration = overlapX;
        outManifold.normal = { nX > 0.0f ? 1.0f : -1.0f, 0.0f };
    } else {
        outManifold.penetration = overlapY;
        outManifold.normal = { 0.0f, nY > 0.0f ? 1.0f : -1.0f };
    }

    return true;
}

void PhysicsEngine::resolveSolidCollision(const CollisionManifold& manifold) {
    Collidable& colA = *manifold.colA;
    Collidable& colB = *manifold.colB;
    
    PhysicsBody* physA = colA.getPhysicsBody();
    PhysicsBody* physB = colB.getPhysicsBody();

    const float SLEEP_THRESHOLD = 0.01f;

    // 1. Stability Check: Only treat as an unmovable anchor if velocity is negligible.
    bool aStable = physA && (physA->velocity.lengthSquared() < SLEEP_THRESHOLD);
    bool bStable = physB && (physB->velocity.lengthSquared() < SLEEP_THRESHOLD);

    // 2. Dynamic mass calculation: Stable objects act as infinite mass (0.0f dynamic mass)
    float massA = colA.isDynamic() ? 1.0f : 0.0f;
    float massB = colB.isDynamic() ? 1.0f : 0.0f;
        
    float totalMass = massA + massB;
    if (totalMass <= 0.0f) return;

    // 3. Setup Correction Vectors
    Vect2 cleanNormal = manifold.normal;
    Entity& entA = colA.getOwner();
    Entity& entB = colB.getOwner();

    // float dirX = (entA.transform.x + entA.transform.w * 0.5f) - (entB.transform.x + entB.transform.w * 0.5f);
    // float dirY = (entA.transform.y + entA.transform.h * 0.5f) - (entB.transform.y + entB.transform.h * 0.5f);

    Vect2 relativePos = {
        (entA.transform.x + entA.transform.w * 0.5f) - (entB.transform.x + entB.transform.w * 0.5f),
        (entA.transform.y + entA.transform.h * 0.5f) - (entB.transform.y + entB.transform.h * 0.5f)
    };

    if ((cleanNormal.x * relativePos.x + cleanNormal.y * relativePos.y) < 0.0f) {
        cleanNormal.x = -cleanNormal.x;
        cleanNormal.y = -cleanNormal.y;
    }

    // 4. Calculate Correction
    float positionalCorrectionPercent = (manifold.penetration > 5.0f) ? 1.0f : 0.8f;
    float correctionMagnitude = std::max(0.0f, manifold.penetration - 0.05f) * positionalCorrectionPercent;
    
    if (correctionMagnitude <= 0.0f) return;

    // 5. Apply Positional Correction
    float weightA = (massB > 0.0f) ? (massA / totalMass) : 1.0f;
    float weightB = (massA > 0.0f) ? (massB / totalMass) : 1.0f;

    // If B is stable, A takes all the correction. If both are dynamic, they split it.
    if (colA.isDynamic() && !aStable) {
        entA.transform.x += cleanNormal.x * correctionMagnitude * weightA;
        entA.transform.y += cleanNormal.y * correctionMagnitude * weightA;
        colA.invalidateTransform();
    }
    
    if (colB.isDynamic() && !bStable) {
        entB.transform.x -= cleanNormal.x * correctionMagnitude * weightB;
        entB.transform.y -= cleanNormal.y * correctionMagnitude * weightB;
        colB.invalidateTransform();
    }

    // 6. Resolve Velocities (Friction/Bounce)
    if (physA || physB) {
        // A. Handle Grounded State
        if (physA && cleanNormal.y < -0.7f) physA->isGrounded = true;
        if (physB && cleanNormal.y > 0.7f) physB->isGrounded = true;

        // B. Calculate Relative Velocity
        Vect2 relVel = {
            (physA ? physA->velocity.x : 0.0f) - (physB ? physB->velocity.x : 0.0f),
            (physA ? physA->velocity.y : 0.0f) - (physB ? physB->velocity.y : 0.0f)
        };

        // C. Normal Impulse (Bounce)
        float velAlongNormal = relVel.x * cleanNormal.x + relVel.y * cleanNormal.y;
        if (velAlongNormal < 0.0f) {
            float e = 0.0f; // Inelastic collision
            float j = -(1.0f + e) * velAlongNormal;
            j /= totalMass;

            Vect2 impulse = { j * cleanNormal.x, j * cleanNormal.y };
            if (physA && massA > 0.0f) { physA->velocity.x += impulse.x; physA->velocity.y += impulse.y; }
            if (physB && massB > 0.0f) { physB->velocity.x -= impulse.x; physB->velocity.y -= impulse.y; }
        }

        // D. Friction Impulse (Tangential)
        // Refresh relVel after normal impulse
        relVel = {
            (physA ? physA->velocity.x : 0.0f) - (physB ? physB->velocity.x : 0.0f),
            (physA ? physA->velocity.y : 0.0f) - (physB ? physB->velocity.y : 0.0f)
        };

        Vect2 tangent = { -cleanNormal.y, cleanNormal.x }; // Tangent is perp to normal
        float velAlongTangent = relVel.x * tangent.x + relVel.y * tangent.y;

        // Apply friction only if there is tangential motion
        if (std::abs(velAlongTangent) > 0.001f) {
            // Friction force: F = -vel * friction_coeff
            // We scale by dt internally here or keep it constant to mimic surface resistance
            float jt = -velAlongTangent * std::min(groundFriction * 0.1f, 1.0f);
            jt /= totalMass;

            Vect2 frictionImpulse = { jt * tangent.x, jt * tangent.y };
            if (physA && massA > 0.0f) { physA->velocity.x += frictionImpulse.x; physA->velocity.y += frictionImpulse.y; }
            if (physB && massB > 0.0f) { physB->velocity.x -= frictionImpulse.x; physB->velocity.y -= frictionImpulse.y; }
        }
    }
}

uint64_t PhysicsEngine::getPairKey(uint32_t colIdA, uint32_t colIdB) const {
    if (colIdA > colIdB) std::swap(colIdA, colIdB);
    return (static_cast<uint64_t>(colIdA) << 32) | static_cast<uint64_t>(colIdB);
}

GridBounds PhysicsEngine::getGridBounds(const Rect& rect) const {
    int minX = static_cast<int>((rect.x - activeBounds.x) / CELL_SIZE);
    int maxX = static_cast<int>((rect.x + rect.w - activeBounds.x) / CELL_SIZE);
    int minY = static_cast<int>((rect.y - activeBounds.y) / CELL_SIZE);
    int maxY = static_cast<int>((rect.y + rect.h - activeBounds.y) / CELL_SIZE);

    return GridBounds{
        std::max(0, std::min(minX, gridWidth - 1)),
        std::max(0, std::min(maxX, gridWidth - 1)),
        std::max(0, std::min(minY, gridHeight - 1)),
        std::max(0, std::min(maxY, gridHeight - 1))
    };
}