// License: MIT
// Copyright (c) 2026 kanaZukii (GelBanana)

#pragma once
#include "piko/component.hpp"
#include "piko/math.hpp"

namespace piko {
    
    class BoxCollider : public Collidable {
        public:
            void init() override {}
            void update(float dt) override {}
        
        protected:
            BoxCollider() : Collidable() {className = "BoxCollider";}
            BoxCollider(Vect2 size, Vect2 offset = {0.0f, 0.0f}) : Collidable() {
                className = "BoxCollider";
                this->setSize(size);
                this->setOffset(offset);
            }
            friend class Scene;
            friend class SceneManager;
    };

}